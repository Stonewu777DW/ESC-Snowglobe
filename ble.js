/* A BLE notification is a hint to read a full snapshot, not an audio command.
 * This also recovers a globe already sitting on the reader when you connect. */
(function (root) {
  'use strict';
  const SERVICE = '5b8c0001-8f28-4a8d-a1f8-667363736e6f';
  const STATE = '5b8c0002-8f28-4a8d-a1f8-667363736e6f';
  function decodeSnapshot(value) {
    const bytes = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
    const text = Array.from(bytes, (b) => String.fromCharCode(b)).join('');
    const m = /^1\|([A-F0-9]{4})\|(ok|bad)\|(-|[a-z0-9-]{1,24})$/.exec(text);
    if (!m) throw new Error('Unsupported pedestal snapshot. Update its firmware.');
    return { reader: m[1], healthy: m[2] === 'ok', song: m[3] === '-' ? null : m[3], present: m[3] !== '-' };
  }
  function createConnection(opts = {}) {
    const report = opts.onState || (() => {});
    const deliver = opts.onSnapshot || (() => {});
    const every = opts.setInterval || root.setInterval.bind(root);
    const cancel = opts.clearInterval || root.clearInterval.bind(root);
    let generation = 0, device = null, characteristic = null, timer = null;
    let notify = null, disconnected = null, pending = null, reading = false;
    function cleanup() {
      generation++;
      if (timer !== null) cancel(timer);
      timer = null;
      if (characteristic && notify) characteristic.removeEventListener('characteristicvaluechanged', notify);
      if (device && disconnected) device.removeEventListener('gattserverdisconnected', disconnected);
      const old = device;
      device = characteristic = notify = disconnected = null;
      reading = false;
      if (old && old.gatt.connected) old.gatt.disconnect();
    }
    function disconnect() { cleanup(); report('disconnected'); }
    async function connect() {
      if (pending) return pending;
      cleanup();
      const mine = generation;
      report('connecting');
      pending = (async () => {
        try {
          const bluetooth = opts.bluetooth || (root.navigator && root.navigator.bluetooth);
          if (!bluetooth) throw new Error('Use desktop Chrome or Edge with Bluetooth enabled.');
          const picked = await bluetooth.requestDevice({ filters: [{ services: [SERVICE] }] });
          if (mine !== generation) return;
          device = picked;
          disconnected = () => { if (mine === generation) disconnect(); };
          device.addEventListener('gattserverdisconnected', disconnected);
          const server = await device.gatt.connect();
          if (mine !== generation) { if (picked.gatt.connected) picked.gatt.disconnect(); return; }
          const service = await server.getPrimaryService(SERVICE);
          if (mine !== generation) return;
          const char = await service.getCharacteristic(STATE);
          if (mine !== generation) return;
          characteristic = char;
          async function refresh() {
            if (mine !== generation) return;
            // readValue itself can emit characteristicvaluechanged. Do not read recursively.
            if (reading) return;
            reading = true;
            try {
              const value = await char.readValue();
              if (mine !== generation) return;
              deliver(decodeSnapshot(value));
            } catch (error) {
              if (mine === generation) { cleanup(); report('error', error.message); }
            } finally { if (mine === generation) reading = false; }
          }
          notify = () => { void refresh(); };
          char.addEventListener('characteristicvaluechanged', notify);
          await char.startNotifications();
          if (mine !== generation) return;
          await refresh();
          if (mine !== generation) return;
          report('connected', picked.name || 'ESC Pedestal');
          // Recover a missed notification and keep the reader's liveness current.
          timer = every(() => { void refresh(); }, 1000);
        } catch (error) {
          if (mine === generation) {
            cleanup();
            report(error.name === 'NotFoundError' ? 'cancelled' : 'error', error.message);
          }
        }
      })();
      try { await pending; } finally { pending = null; }
    }
    return { connect, disconnect };
  }
  root.EscBle = { SERVICE, STATE, createConnection, decodeSnapshot };
})(typeof window !== 'undefined' ? window : globalThis);
