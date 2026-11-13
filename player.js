/*
 * The game computer's logic, with the browser taken out of it so it can be tested
 * (tests/player.test.mjs runs this file in node).
 *
 * The pedestal owns the timing: it only says "this song is on the pedestal" after
 * the tag has sat there for half a second, and "nothing" after it has been gone
 * half a second. This file just follows along, and adds one safety net of its own:
 * if the pedestal stops talking (unplugged, WiFi dropped), the music stops.
 */
(function (root) {
  'use strict';

  function createPlayer(opts) {
    const songs = opts.songs || [];
    const timeoutMs = opts.timeoutMs || 3000;
    const now = opts.now || (() => Date.now());
    const hooks = opts.hooks || {};

    let manual = false;
    let current = null;        // slug of the song playing, or null
    let owner = null;          // reader that started it
    let lastHeard = 0;         // last tag message from that reader
    let lastReaderSeen = 0;    // last status message from any reader
    let readerId = null;
    let readerFlag = null;    // last state reported to the UI, so we only fire on change

    const bySlug = new Map(songs.map((s) => [s.slug, s]));

    function songFor(slug) {
      return bySlug.get(slug) || null;
    }

    function play(slug) {
      const song = songFor(slug);
      if (!song) {
        if (hooks.onUnknown) hooks.onUnknown(slug);
        return false;
      }
      if (current === slug) return false;
      current = slug;
      if (hooks.onPlay) hooks.onPlay(song);
      return true;
    }

    function stop(reason) {
      if (current === null) return false;
      const was = current;
      current = null;
      manual = false;
      if (hooks.onStop) hooks.onStop(was, reason || 'none');
      return true;
    }

    // {"reader":"AB12","fw":"1.0.0","state":"online"} — the pedestal's heartbeat.
    function setReaderOnline(online) {
      if (readerFlag === online) return;
      readerFlag = online;
      if (hooks.onReader) hooks.onReader(readerId, online);
    }

    function onReaderStatus(msg) {
      if (!msg || msg.state !== 'online') return;
      readerId = msg.reader || readerId;
      lastReaderSeen = now();
      setReaderOnline(true);
    }

    // {"reader":"AB12","song":"jingle-bells","present":true}
    function onTagMessage(msg) {
      if (!msg) return;
      lastReaderSeen = now(); // any traffic proves the reader is alive
      if (msg.present === false) {
        // A reader with nothing on it must not stop a song another reader started.
        if (owner !== null && msg.reader === owner) {
          owner = null;
          stop('tag removed');
        }
        return;
      }
      if (!msg.song) return;
      manual = false;
      owner = msg.reader || owner;
      lastHeard = now();
      play(msg.song);
    }

    // Call on a timer; returns nothing, fires hooks when the reader goes quiet.
    function tick() {
      const t = now();
      if (current !== null && !manual && t - lastHeard > timeoutMs) {
        owner = null;
        stop('lost the reader');
      }
      if (readerId !== null) setReaderOnline(t - lastReaderSeen <= timeoutMs);
    }

    return {
      onReaderStatus,
      onTagMessage,
      tick,
      currentSong: () => current,
      readerOnline: () => readerId !== null && now() - lastReaderSeen <= timeoutMs,
      readerId: () => readerId,
      // Manual trigger, for testing the speakers before the hardware is on the table.
      playManually: (slug) => {
        if (songFor(slug)) { manual = true; owner = null; }
        return play(slug);
      },
      stopManually: () => stop('manual'),
    };
  }

  root.EscPlayer = { createPlayer };
})(typeof window !== 'undefined' ? window : globalThis);
