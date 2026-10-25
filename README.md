# ESC Snowglobe

Show an AprilTag to a phone's front camera and a computer on the same WiFi plays that tag's song.
The song starts from the beginning when a tag appears and stops when the tag is gone for half a second.

| Tag (36h11) | Song |
|---|---|
| [tag0](tags/tag0.svg) | `songs/song0.mp3` |
| [tag1](tags/tag1.svg) | `songs/song1.mp3` |
| [tag2](tags/tag2.svg) | `songs/song2.mp3` |
| [tag3](tags/tag3.svg) | `songs/song3.mp3` |

## Setup (once)
1. Install [Node.js](https://nodejs.org) on the computer that plays the music.
2. Put the four songs in `songs/` with the names above.
3. Print the tags from `tags/`. Keep the white margin around each tag.
4. In this folder run `npm install`.

## Run
1. On the computer run `npm start`. It prints two links.
2. On the computer open the **Player** link, accept the security warning, and click once (browsers need a click before they play sound). Leave this tab open.
3. On a phone on the same WiFi open the **Phones** link, accept the security warning, and tap once to start the camera.
4. Hold a tag in front of the phone's camera.

The security warning appears because the computer uses its own certificate. Phones only allow the camera on HTTPS pages, so this is needed. Each browser asks once.

Tag detection uses [js-aruco2](https://github.com/damianofalcioni/js-aruco2) (MIT), vendored in `public/lib/`.
