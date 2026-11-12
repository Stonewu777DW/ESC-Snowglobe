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

function boot(t, options = {}) {
  const published = [];
  const listeners = {};
  const client = {
    on: (evt, fn) => { (listeners[evt] ||= []).push(fn); return client; },
    subscribe: (topic) => published.push(['subscribe', topic]),
    end: () => {},
    emit: (evt, ...args) => (listeners[evt] || []).forEach((fn) => fn(...args)),
  };
  const played = [];
  const frames = [];
  const audios = [];

  const dom = new JSDOM(html, {
    url: 'http://localhost/index.html?room=esc-snowglobe-test',
    runScripts: 'dangerously',
    pretendToBeVisual: true,
    beforeParse(window) {
      window.mqtt = { connect: () => client };
      window.eval(playerSrc);
      // jsdom has no media stack; record what the page asked to play.
      window.requestAnimationFrame = (fn) => { frames.push(fn); return frames.length; };
      window.Audio = class {
        constructor() { this.volume = 1; this.currentTime = 0; this.paused = true; audios.push(this); }
        play() {
          this.paused = false;
          played.push(this.src);
          return options.play ? options.play(this) : Promise.resolve();
        }
        pause() { this.paused = true; }
      };
    },
  });

  const { window } = dom;
  // The page runs timers and rAF; close the window so node can exit.
  if (t) t.after(() => window.close());
  return { window, client, published, played, dom, audio: audios[0], frames };
}

test('a completed old stop fade cannot pause a newly scanned globe', (t) => {
  const { client, window, audio, frames } = boot(t);
  const topic = 'esc-snowglobe/esc-snowglobe-test/tag';
  const send = (msg) => client.emit('message', topic, Buffer.from(JSON.stringify({ reader: 'AB12', ...msg })));
  send({ song: 'rudolph', present: true });
  send({ song: null, present: false });
  send({ song: 'frosty', present: true });
  while (frames.length) frames.shift()(window.performance.now() + 1000);
  assert.equal(audio.paused, false, 'old removal callback must not pause the new song');
  assert.equal(audio.volume, 1);
  assert.equal(text(window, 'nowPlaying'), 'Frosty the Snowman');
});

test('a fade clamps an animation timestamp older than its start time', (t) => {
  const { client, window, audio, frames } = boot(t);
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag',
    Buffer.from('{"reader":"AB12","song":"frosty","present":true}'));
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag',
    Buffer.from('{"reader":"AB12","song":null,"present":false}'));
  frames.shift()(0);
  assert.ok(audio.volume >= 0 && audio.volume <= 1, 'volume must stay inside the browser media range');
  while (frames.length) frames.shift()(window.performance.now() + 1000);
  assert.equal(audio.paused, true);
});

test('sound-unlock completion cannot pause a scan that arrived while audio was loading', async (t) => {
  let resolveUnlock;
  let calls = 0;
  const { client, window, audio } = boot(t, { play: () => ++calls === 1
    ? new Promise((resolve) => { resolveUnlock = resolve; }) : Promise.resolve() });
  window.document.getElementById('sound').click();
  client.emit('message', 'esc-snowglobe/esc-snowglobe-test/tag',
    Buffer.from('{"reader":"AB12","song":"frosty","present":true}'));
  resolveUnlock();
  await Promise.resolve();
  assert.equal(audio.paused, false, 'unlock callback must not pause a real song');
  assert.match(audio.src, /Frosty/);
});

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