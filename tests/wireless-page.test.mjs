import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { JSDOM } from 'jsdom';
const root = new URL('../', import.meta.url);
const html = readFileSync(new URL('wireless.html', root), 'utf8');
const source = readFileSync(new URL('player.js', root), 'utf8');
function boot(t, supported = true) {
  let opts, connections = 0, disconnections = 0;
  const audio = [];
  const dom = new JSDOM(html, { url: 'https://snowglobe.example/', runScripts: 'dangerously',
    beforeParse(w) {
      Object.defineProperty(w, 'isSecureContext', { value: true });
      if (supported) Object.defineProperty(w.navigator, 'bluetooth', { value: {} });
      w.eval(source);
      w.EscBle = { createConnection: (o) => { opts = o; return {
        connect: async () => { connections++; o.onState('connecting'); o.onState('connected', 'ESC Pedestal E068'); },
        disconnect: () => { disconnections++; o.onState('disconnected'); },
      }; } };
      w.Audio = class {
        constructor() { this.paused = true; audio.push(this); }
        play() { this.paused = false; return Promise.resolve(); }
        pause() { this.paused = true; }
      };
    } });
  t.after(() => dom.window.close());
  return { window: dom.window, audio: audio[0], snapshot: (s) => opts.onSnapshot(s),
    state: (...a) => opts.onState(...a), connections: () => connections, disconnections: () => disconnections };
}
test('speaker test has a stop control even while connected', (t) => {
  const r = boot(t); const d = r.window.document;
  r.state('connected', 'ESC Pedestal E068');
  d.querySelector('#test').click();
  assert.equal(d.querySelector('#stop').hidden, false);
  assert.match(d.querySelector('#status').textContent, /Speaker test/);
  d.querySelector('#stop').click();
  assert.equal(r.audio.paused, true);
  assert.equal(d.querySelector('#stop').hidden, true);
});

test('a song repeats while its globe stays on the pedestal', (t) => {
  const r = boot(t);
  r.state('connected', 'ESC Pedestal E068');
  r.snapshot(healthy('frosty'));
  assert.equal(r.audio.loop, true);
  assert.equal(r.audio.paused, false);
});

test('live statistics follow playback, reader health, volume and disconnect', (t) => {
  const r = boot(t); const d = r.window.document;
  r.state('connected', 'ESC Pedestal E068');
  r.snapshot(healthy('rudolph'));
  assert.equal(d.querySelector('#transport').textContent, 'Bluetooth LE');
  assert.equal(d.querySelector('#readerState').textContent, 'E068 · Ready');
  assert.equal(d.querySelector('#playbackState').textContent, 'Playing');
  assert.equal(d.querySelector('#connect').hidden, true);
  assert.equal(d.querySelector('#disconnect').hidden, false);
  d.querySelector('#volume').value = '0.35';
  d.querySelector('#volume').dispatchEvent(new r.window.Event('input'));
  assert.equal(d.querySelector('#volumeValue').textContent, '35%');
  assert.equal(r.audio.volume, 0.35);
  r.snapshot(healthy(null));
  assert.equal(d.querySelector('#playbackState').textContent, 'Idle');
  r.state('disconnected');
  assert.equal(d.querySelector('#transport').textContent, 'Offline');
  assert.equal(d.querySelector('#readerState').textContent, '—');
});
test('instructions live on a separate page with source and setup links', () => {
  const guide = new JSDOM(readFileSync(new URL('instructions.html', root), 'utf8')).window.document;
  assert.ok(guide.querySelector('a[href="wireless.html"]'));
  assert.ok(guide.querySelector('a[href="https://github.com/sidatpho/ESC-Snowglobe"]'));
  assert.match(guide.body.textContent, /9 V/);
  assert.match(guide.body.textContent, /Chrome or Edge/);
  assert.match(guide.body.textContent, /computer.*speaker/s);
});
const healthy = (song) => ({ reader: 'E068', healthy: true, song, present: !!song });

test('default wireless page asks for no WiFi password, room code or local installation', (t) => {
  const r = boot(t);
  assert.equal(r.window.document.querySelector('#room'), null);
  assert.equal(r.window.document.querySelector('#connect').textContent.trim(), 'Connect');
  assert.equal(r.window.document.querySelector('.steps'), null);
  assert.equal(r.window.document.querySelector('a[href="instructions.html"]').textContent.trim(), 'How it works');
  assert.ok(r.window.document.querySelector('#elapsed'));
  assert.ok(r.window.document.querySelector('#playbackState'));
  r.window.document.querySelector('#connect').click();
  assert.equal(r.connections(), 1);
});
test('wireless snapshots play the trimmed song, keep playing on repeats and stop on removal', (t) => {
  const r = boot(t);
  r.snapshot(healthy('jingle-bells'));
  assert.match(r.audio.src, /Intro Trimmed/);
  assert.equal(r.audio.paused, false);
  r.audio.currentTime = 5;
  r.snapshot(healthy('jingle-bells'));
  assert.equal(r.audio.currentTime, 5);
  r.snapshot(healthy(null));
  assert.equal(r.audio.paused, true);
});
test('battery loss/disconnection stops music immediately and offers reconnect', (t) => {
  const r = boot(t); r.snapshot(healthy('frosty')); r.state('disconnected');
  assert.equal(r.audio.paused, true);
  assert.match(r.window.document.querySelector('#status').textContent, /connect/i);
  assert.equal(r.window.document.querySelector('#connect').disabled, false);
});
test('broken NFC reader is not presented as a ready pedestal', (t) => {
  const r = boot(t); r.snapshot(healthy('frosty'));
  r.snapshot({ ...healthy(null), healthy: false });
  assert.equal(r.audio.paused, true);
  assert.match(r.window.document.querySelector('#status').textContent, /reader.*wiring/i);
});
test('unsupported browsers get a specific desktop-browser instruction', (t) => {
  const r = boot(t, false);
  assert.equal(r.window.document.querySelector('#connect').disabled, true);
  assert.match(r.window.document.querySelector('#status').textContent, /Chrome or Edge/);
});
test('new site and existing player agree on the four song paths', (t) => {
  const r = boot(t);
  const legacy = readFileSync(new URL('index.html', root), 'utf8');
  for (const path of [...legacy.matchAll(/file: '([^']+)'/g)].map((m) => m[1])) {
    assert.ok(html.includes(path), path);
  }
});
