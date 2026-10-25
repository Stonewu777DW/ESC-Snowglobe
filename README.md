# ESC Snowglobe

Show an AprilTag to a phone's front camera and the game computer plays that tag's song.
The song starts from the beginning when a tag appears and stops when the tag is gone for half a second.

**Live page:** https://stonewu777dw.github.io/ESC-Snowglobe/

| Tag (AprilTag 36h11) | Song |
|---|---|
| [tag0](tags/tag0.svg) | Jingle Bells |
| [tag1](tags/tag1.svg) | Frosty the Snowman |
| [tag2](tags/tag2.svg) | Rudolph the Red-Nosed Reindeer |
| [tag3](tags/tag3.svg) | White Christmas |

## For GMs
1. On the game computer open **https://stonewu777dw.github.io/ESC-Snowglobe/** (bookmark it) and click the orange **Click to turn on sound** button.
2. Scan the QR code on the computer screen with the phone's camera app, open the link, and allow the camera.
3. Hold a tag in front of the phone's front camera.

Each computer keeps the same QR code, so you can print it and leave it in the room.
If the phone is closed or loses connection, the computer stops the music after about 3 seconds.

## Requirements
- The computer and the phone both need internet. They connect directly to each other and use the free PeerJS service only to find each other.
- Print the tags from `tags/` at least 5 cm wide, keeping the white margin around each tag.

## Changing songs
Put the mp3 files in `songs/` and list them in the `SONGS` list in `index.html`. The first line is tag 0, the second tag 1, and so on.

## Files
- `index.html`: the game computer page (QR code and music)
- `phone.html`: the phone page (camera and tag detection)
- `songs/`, `tags/`: songs and printable tags
- `lib/`: [js-aruco2](https://github.com/damianofalcioni/js-aruco2), [PeerJS](https://peerjs.com) and [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator), all MIT licensed
