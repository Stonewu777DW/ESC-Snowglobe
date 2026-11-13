import test from 'node:test';
import assert from 'node:assert/strict';
await import('../ble.js');
const { createConnection, decodeSnapshot, SERVICE, STATE } = globalThis.EscBle;
const data = (s) => new DataView(new TextEncoder().encode(s).buffer);
function rig(options = {}) {
  const received = [], states = [], handlers = new Map();
  let snapshot = '1|E068|ok|-';
  const char = new EventTarget();
  char.startNotifications = async () => char;
  char.readValue = options.read || (async () => data(snapshot));
  const device = new EventTarget();
  device.name = 'ESC Pedestal E068';
  device.gatt = {
    connected: false,
    connect: async () => { device.gatt.connected = true; return {
      getPrimaryService: async (id) => { assert.equal(id, SERVICE); return {
        getCharacteristic: async (id) => { assert.equal(id, STATE); return char; },
      }; },
    }; },
    disconnect: () => { device.gatt.connected = false; device.dispatchEvent(new Event('gattserverdisconnected')); },
  };
  let requests = 0;
  const bluetooth = { requestDevice: async (opts) => {
    requests++; assert.deepEqual(opts.filters, [{ services: [SERVICE] }]); return device;
  } };
  const connection = createConnection({ bluetooth, onSnapshot: (s) => received.push(s),
    onState: (s, detail) => states.push([s, detail]),
    setInterval: (fn) => { handlers.set(1, fn); return 1; }, clearInterval: (id) => handlers.delete(id) });
  return { connection, char, device, received, states, handlers, requests: () => requests,
    set: (s) => { snapshot = s; }, notify: () => char.dispatchEvent(new Event('characteristicvaluechanged')) };
}
const flush = async () => { for (let i = 0; i < 15; i++) await Promise.resolve(); };

test('validates versioned snapshots, reader health and the full 24-character slug', () => {
  assert.deepEqual(decodeSnapshot(data('1|E068|ok|white-christmas')), {
    reader: 'E068', healthy: true, song: 'white-christmas', present: true,
  });
  assert.equal(decodeSnapshot(data('1|E068|bad|-')).healthy, false);
  assert.equal(decodeSnapshot(data('1|E068|ok|-')).present, false);
  assert.equal(decodeSnapshot(data('1|AB12|ok|' + 'a'.repeat(24))).song.length, 24);
  for (const s of ['2|E068|ok|-', '1|????|ok|-', '1|AB12|ok|<script>', '1|AB12|yes|-',
    '1|AB12|ok|' + 'a'.repeat(25), '1|AB12|ok|', '1|AB12|ok|-|extra']) {
    assert.throws(() => decodeSnapshot(data(s)), /snapshot/i);
  }
});

test('connection reads an already-present globe and refreshes from notifications', async () => {
  const r = rig(); r.set('1|E068|ok|frosty');
  await r.connection.connect();
  assert.equal(r.received[0].song, 'frosty');
  assert.equal(r.states.at(-1)[0], 'connected');
  r.set('1|E068|ok|-'); r.notify(); await flush();
  assert.equal(r.received.at(-1).present, false);
  assert.equal(r.handlers.size, 1);
});

test('notifications are hints: read the complete characteristic, not a truncated packet', async () => {
  const r = rig(); await r.connection.connect();
  r.set('1|E068|ok|white-christmas');
  const e = new Event('characteristicvaluechanged'); e.targetValue = data('1|E068|ok|white');
  r.char.dispatchEvent(e); await flush();
  assert.equal(r.received.at(-1).song, 'white-christmas');
});

test('disconnect stops polling and discards an in-flight old read', async () => {
  let resolve, count = 0;
  const r = rig({ read: () => ++count === 1 ? Promise.resolve(data('1|E068|ok|-'))
    : new Promise((r) => { resolve = r; }) });
  await r.connection.connect(); r.notify(); await flush();
  r.device.gatt.disconnect();
  resolve(data('1|E068|ok|frosty')); await flush();
  assert.equal(r.received.length, 1);
  assert.equal(r.handlers.size, 0);
  assert.equal(r.states.at(-1)[0], 'disconnected');
});

test('a characteristic change emitted by readValue does not retrigger reads', async () => {
  const r = rig(); let reads = 0;
  r.char.readValue = async () => {
    if (++reads > 3) throw new Error('recursive read');
    r.notify();
    return data('1|E068|ok|-');
  };
  await r.connection.connect();
  assert.equal(r.states.at(-1)[0], 'connected');
  assert.equal(reads, 1);
});

test('two connect clicks do not open two device pickers', async () => {
  const r = rig(); await Promise.all([r.connection.connect(), r.connection.connect()]);
  assert.equal(r.requests(), 1);
});

test('read failure tears down the connection instead of showing a false connected badge', async () => {
  const r = rig(); await r.connection.connect();
  r.char.readValue = async () => { throw new Error('radio lost'); };
  r.notify(); await flush();
  assert.equal(r.device.gatt.connected, false);
  assert.equal(r.states.at(-1)[0], 'error');
});

test('manual disconnect while chooser is open cancels the late result', async () => {
  const r = rig();
  let resolve;
  const c = createConnection({ bluetooth: { requestDevice: () => new Promise((r) => { resolve = r; }) },
    onSnapshot: () => assert.fail('cancelled connection delivered a snapshot') });
  const pending = c.connect(); c.disconnect(); resolve(r.device); await pending;
  assert.equal(r.device.gatt.connected, false);
});
