# ESC Snowglobe

Show an AprilTag to a phone's front camera and the game computer plays that tag's song.
The song starts from the beginning when a tag appears and stops when the tag is gone for half a second.

| Tag (36h11) | Song |
|---|---|
| [tag0](tags/tag0.svg) | Jingle Bells |
| [tag1](tags/tag1.svg) | Frosty the Snowman |
| [tag2](tags/tag2.svg) | Rudolph the Red-Nosed Reindeer |
| [tag3](tags/tag3.svg) | White Christmas |

To change which tag plays which song, edit the `SONGS` list in `index.html`.

## Setup (once)
1.## For GMs
1. On the game computer open **https://stonewu777dw.github.io/ESC-Snowglobe/** (bookmark it) and click anywhere on the page to turn on sound.
2. Scan the QR code with the phone and allow the camera.
3. Hold a tag in front of the phone's camera.

The computer and phone both need internet. They connect directly to each other, using the free PeerJS service only to find each other.
Each computer keeps the same QR code, so you can print it and leave it in the room.

## Setup (once)
- Print the tags from `tags/`, at least 5 cm wide. Keep the white margin around each tag.
- Turn on GitHub Pages: Settings → Pages → Deploy from a branch → `main`, `/ (root)`.

Tag detection uses [js-aruco2](https://github.com/damianofalcioni/js-aruco2) (MIT), with [PeerJS](https://peerjs.com) (MIT) and [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator) (MIT), all vendored in `lib/`.
