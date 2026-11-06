// End-to-end test of the REAL index.html in a DOM, with the message broker
// stubbed. Proves the page the GM looks at reacts to pedestal messages.
//   node tests/page.test.mjs   (run from the repo root; needs `npm i jsdom` once)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { JSDOM } from 'jsdom';

const html = readFileSync(new URL('../index.html', import.meta.url), 'utf8');
// jsdom does not fetch the page's <script src> files; feed player.js in by hand so
// the page runs exactly as it does in a browser (mqtt.js is stubbed anyway).
const playerSrc = readFileSync(new URL('../player.js', import.meta.url), 'utf8');

function boot(t) {
  const published = [];
  const listeners = {};
  const client = {
    on: (evt, fn) => { (listeners[evt] ||= []).push(fn); return client; },
    subscribe: (topic) => published.push(['subscribe', topic]),
    end: () => {},
    emit: (evt, ...args) => (listeners[evt] || []).forEach((fn) => fn(...args)),
  };
  const played = [];

  const dom = new JSDOM(html, {
    url: 'http://localhost/index.html?room=esc-snowglobe-test',
    runScripts: 'dangerously',
    pretendToBeVisual: true,
    beforeParse(window) {
      window.mqtt = { connect: () => client };
      window.eval(playerSrc);
      // jsdom has no media stack; record what the page asked to play.
      window.Audio = class {
        constructor() { this.volume = 1; this.currentTime = 0; }
        play() { played.push(this.src); return Promise.resolve(); }
        pause() {}
      };
    },
  });

  const { window } = dom;
  // The page runs timers and rAF; close the window so node can exit.
  if (t) t.after(() => window.close());
  return { window, client, published, played, dom };
}

const text = (window, id) => window.document.getElementById(id).textContent;
const value = (window, id) => window.document.getElementById(id).value;

test('page loads and subscribes to its room', (t) => {
  const { client, published, window } = boot(t);
  client.emit('connect');
  const topics = published.filter((p) => p[0] === 'subscribe').map((p) => p[1]);
  assert.deepEqual(topics, ['esc-snowglobe/esc-snowglobe-test/tag', 'esc-snowglobe/esc-snowglobe-test/reader']);
  assert.equal(value(window, 'room'), 'esc-snowglobe-test');
  assert.equal(text(window, 'readerState'), 'Pedestal offline');
});

test('a globe on the pedestal shows its song title and starts that file', (t) => {
  const { client, window, played } = boot(t);
  client.emit('connect');
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/reader', Buffer.from('{"reader":"AB12","state":"online"}'));
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag', Buffer.from('{"reader":"AB12","song":"frosty","present":true}'));

  assert.equal(text(window, 'nowPlaying'), 'Frosty the Snowman');
  assert.equal(text(window, 'readerState'), 'Pedestal connected');
  assert.ok(played.at(-1).includes('Frosty'));
  assert.equal(window.document.getElementById('dot').className, 'dot on');
});

test('lifting the globe returns the page to idle', (t) => {
  const { client, window } = boot(t);
  client.emit('connect');
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag', Buffer.from('{"reader":"AB12","song":"rudolph","present":true}'));
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag', Buffer.from('{"reader":"AB12","song":null,"present":false}'));
  assert.equal(text(window, 'nowPlaying'), 'Waiting for a globe');
  assert.match(window.document.getElementById('nowPlaying').className, /idle/);
});

test('an unlisted slug is reported, not silently ignored', (t) => {
  const { client, window } = boot(t);
  client.emit('connect');
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag', Buffer.from('{"reader":"AB12","song":"jingle","present":true}'));
  assert.match(text(window, 'status'), /not in SONGS/);
  assert.equal(text(window, 'nowPlaying'), 'Waiting for a globe');
});

test('keyboard 1-4 tests the speakers with no pedestal attached', (t) => {
  const { window, played } = boot(t);
  window.dispatchEvent(new window.KeyboardEvent('keydown', { key: '3' }));
  assert.equal(text(window, 'nowPlaying'), 'Rudolph the Red-Nosed Reindeer');
  assert.ok(played.at(-1).includes('Rudolph'));
  window.dispatchEvent(new window.KeyboardEvent('keydown', { key: 'Escape' }));
  assert.equal(text(window, 'nowPlaying'), 'Waiting for a globe');
});

test('bad room code shows what to do instead of failing silently', (t) => {
  const dom = new JSDOM(html, {
    url: 'http://localhost/index.html',
    runScripts: 'dangerously',
    pretendToBeVisual: true,
    beforeParse(window) {
      window.mqtt = { connect: () => ({ on() {}, subscribe() {}, end() {} }) };
      window.eval(playerSrc);
      window.Audio = class { play() { return Promise.resolve(); } pause() {} };
    },
  });
  const { window } = dom;
  t.after(() => window.close());
  assert.equal(text(window, 'nowPlaying'), 'Set a room code');
  assert.match(text(window, 'status'), /config\.h/);
});