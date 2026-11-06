# Tags

## Which stickers

Any **13.56 MHz NFC sticker in the MIFARE Ultralight family**:

| Chip | User memory | Fine here? |
|---|---|---|
| NTAG213 | 144 bytes | yes, lots of room |
| NTAG215 | 504 bytes | yes |
| NTAG216 | 888 bytes | yes |
| NTAG203 | 144 bytes | yes |
| MIFARE Classic 1k | — | not written by this tool (it uses a different memory layout) |

We only use the first ~26 bytes, so the smallest sticker is plenty. 25 mm round stickers
are the usual cheap option and sit nicely on a globe base.

Physical placement: the tag goes on the **bottom face** of the globe's plastic base,
centred. It does not matter which way round it is — the reader does not care about
orientation, only about being within ~2 mm and roughly concentric.

## What gets written

An ordinary NDEF text record, so a phone can still read the sticker:

```
esc:rudolph
```

That is it. The whole "song table" lives in `index.html`; the tag just names a song.
Because it is real NDEF, any phone with an NFC reader can see the text, which is the
fastest way to check a sticker in the field.

## Programming them — the tool you already have

Flash `firmware/ESC_Tag_Programmer`, open the serial monitor at **115200** with line
ending "Newline", and:

```
scan                 # what chip is this, what UID, what payload does it hold now
write rudolph        # write the song payload, then read it back to prove it stuck
write 2              # same thing, by position in the programmer's SONGS list
erase                # remove the payload, keep the UID
```

The sketch refuses anything that is not a MIFARE Ultralight/NTAG, because writing raw
pages to a Classic card would wreck it. It also verifies every write by reading it back
before it says OK.

**Label every sticker as you write it.** Blank NTAG stickers are physically identical;
four unlabelled stickers on four globes is an hour of guessing.

## One tag per globe

| Globe | Slug written to the tag | Song on the page |
|---|---|---|
| Jingle Bells | `jingle-bells` | Jingle Bells |
| Frosty | `frosty` | Frosty the Snowman |
| Rudolph | `rudolph` | Rudolph the Red-Nosed Reindeer |
| White Christmas | `white-christmas` | White Christmas |

The slug must exist in `SONGS` in `index.html`, or the page shows
`Tag says "x", which is not in SONGS`. It will not break anything — it just tells you.

## Locked or wiped tags

If a sticker will not accept a write (some come pre-locked, and some phone apps set a
read-only bit), the UID fallback still works:

1. Hold it on the pedestal reader and read the serial log:
   `[tag] 04A1B2C3D4E580 has no song payload, and is not in UID_SONGS`
2. Copy that UID into `firmware/ESC_NFC_Pedestal/songs.h`:

```c
static const UidSong UID_SONGS[] = {
  {"04A1B2C3D4E580", "jingle-bells"},
};
```

3. Reflash the pedestal.

The NDEF payload is checked first every time, so a properly programmed tag always wins.

## Verifying a tag with a phone

iPhone: Control Centre → NFC Tag Reader (iOS 14+), tap the sticker to the top of the
phone. Android: any "NFC Tools" app → Read. You should see `esc:<slug>`.
