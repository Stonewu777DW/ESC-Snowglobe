# ESC Snowglobe

A web page that plays a song when its AprilTag is shown to the phone's front camera.
The song starts from the beginning when a tag appears and stops when the tag is gone for half a second.

| Tag (36h11) | Song |
|---|---|
| [tag0](tags/tag0.svg) | `songs/song0.mp3` |
| [tag1](tags/tag1.svg) | `songs/song1.mp3` |
| [tag2](tags/tag2.svg) | `songs/song2.mp3` |
| [tag3](tags/tag3.svg) | `songs/song3.mp3` |

## Use
1. Open the page on a phone and tap anywhere (browsers need one tap before they allow sound and the camera).
2. Hold a printed tag in front of the camera.

## Setup
- Add the four songs to `songs/` with the names above.
- Print the tags from `tags/`. Keep the white margin around each tag.
- Host with GitHub Pages (Settings → Pages → deploy from `main`, root). The camera only works over HTTPS, which Pages provides.

Tag detection uses [js-aruco2](https://github.com/damianofalcioni/js-aruco2) (MIT), vendored in `lib/`.
