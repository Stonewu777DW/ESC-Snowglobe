// Tests for player.js — the computer page's logic, no browser needed.
//   node --test tests/
import test from 'node:test';
import assert from 'node:assert/strict';
import '../player.js';

const { createPlayer } = globalThis.EscPlayer;

const SONGS = [
  { slug: 'jingle-bells', file: 'songs/a.mp3' },
  { slug: 'frosty', file: 'songs/b.mp3' },
  { slug: 'rudolph', file: 'songs/c.mp3' },
  { slug: 'white-christmas', file: 'songs/d.mp3' },
];

function rig() {
  let t = 0;
  const events = [];
  const player = createPlayer({
    songs: SONGS,
    timeoutMs: 3000,
    now: () => t,
    hooks: {
      onPlay: (song) => events.push(['play', song.slug]),
      onStop: (slug, reason) => events.push(['stop', slug, reason]),
      onUnknown: (slug) => events.push(['unknown', slug]),
      onReader: (id, online) => events.push(['reader', id, online]),
    },
  });
  return { player, events, advance: (ms) => { t += ms; }, at: () => t };
}

test('a tag message starts its song', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  assert.deepEqual(events, [['play', 'frosty']]);
  assert.equal(player.currentSong(), 'frosty');
});

test('the same tag repeated does not restart the song', () => {
  const { player, events, advance } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  advance(1000);
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  advance(1000);
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  assert.deepEqual(events, [['play', 'frosty']]);
});

test('"no tag" from the owning reader stops the music', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'rudolph', present: true });
  player.onTagMessage({ reader: 'AB12', song: null, present: false });
  assert.deepEqual(events.at(-1), ['stop', 'rudolph', 'tag removed']);
  assert.equal(player.currentSong(), null);
});

test('a second reader reporting nothing does not stop the first reader\'s song', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'rudolph', present: true });
  player.onTagMessage({ reader: 'FFFF', song: null, present: false });
  assert.equal(player.currentSong(), 'rudolph');
  assert.deepEqual(events, [['play', 'rudolph']]);
});

test('moving to a different globe swaps the song', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'rudolph', present: true });
  player.onTagMessage({ reader: 'AB12', song: 'white-christmas', present: true });
  assert.deepEqual(events, [['play', 'rudolph'], ['play', 'white-christmas']]);
});

test('if the pedestal goes quiet the music stops (unplugged / WiFi dropped)', () => {
  const { player, events, advance } = rig();
  player.onReaderStatus({ reader: 'AB12', state: 'online' });
  player.onTagMessage({ reader: 'AB12', song: 'jingle-bells', present: true });
  advance(2000);
  player.tick();
  assert.equal(player.currentSong(), 'jingle-bells', 'still playing inside the timeout');
  advance(1500);
  player.tick();
  assert.equal(player.currentSong(), null);
  assert.ok(events.some((e) => e[0] === 'stop' && e[1] === 'jingle-bells'), 'music was stopped');
  assert.deepEqual(events.at(-1), ['reader', 'AB12', false]);
});

test('an unknown slug is ignored instead of breaking playback', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'song-that-does-not-exist', present: true });
  assert.deepEqual(events, [['unknown', 'song-that-does-not-exist']]);
  assert.equal(player.currentSong(), null);
});

test('re-placing the same globe after removal starts it from the top', () => {
  const { player, events } = rig();
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  player.onTagMessage({ reader: 'AB12', song: null, present: false });
  player.onTagMessage({ reader: 'AB12', song: 'frosty', present: true });
  assert.deepEqual(events, [
    ['play', 'frosty'],
    ['stop', 'frosty', 'tag removed'],
    ['play', 'frosty'],
  ]);
});

test('reader status flips to offline when the pedestal disappears', () => {
  const { player, advance } = rig();
  player.onReaderStatus({ reader: 'AB12', state: 'online' });
  assert.equal(player.readerOnline(), true);
  advance(4000);
  assert.equal(player.readerOnline(), false);
});

test('manual trigger works with no hardware at all (speaker test)', () => {
  const { player, events } = rig();
  player.playManually('jingle-bells');
  assert.deepEqual(events, [['play', 'jingle-bells']]);
});
