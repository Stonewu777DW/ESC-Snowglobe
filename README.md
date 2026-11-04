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
3. Hold a tag up to the phone's front camera. Anywhere in the picture works, and bigger printed tags work from farther away.

Each computer keeps the same QR code, so you can print it and leave it in the room.
If the phone is closed or loses connection, the computer stops the music after about 3 seconds.

## How it works
```
 phone.html (front camera)                          index.html (game computer)
 finds tag 0-3 in each frame  --- "tag": 2 --->  plays song 2 from the start
 sends the tag every second       via a free     stops when the phone says "no tag"
 and whenever it changes          public MQTT    or goes quiet for 3 seconds
                                  server
```
- The computer page makes a random room code, saves it in the browser, and puts it in the QR code (`phone.html#<room code>`). Phone and computer only hear messages for that room.
- A song starts once its tag has been in view for 0.5 seconds and stops once it has been gone for 0.5 seconds. These and other settings are at the top of each page's script.
- Everything runs in the browser. There is no server of our own, so the site is just files on GitHub Pages.

## Requirements
- The computer and the phone both need internet. They pass messages through the free public EMQX message server (broker.emqx.io), so they work on any network, including mobile data.
- Print the tags from `tags/` at least 5 cm wide, keeping the white margin around each tag.

## Changing songs
Put the mp3 files in `songs/` and list them in the `SONGS` list in `index.html`. The first line is tag 0, the second tag 1, and so on.

## Files
- `index.html`: the game computer page (QR code and music)
- `phone.html`: the phone page (camera and tag detection)
- `songs/`, `tags/`: songs and printable tags
- `lib/`: [js-aruco2](https://github.com/damianofalcioni/js-aruco2) (tag detection, with one marked change in `aruco.js` for easier detection), [MQTT.js](https://github.com/mqttjs/MQTT.js) (messages) and [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator) (QR code), all MIT licensed
