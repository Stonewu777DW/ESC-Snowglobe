// NDEF text-record helpers shared by the reader and the tag programmer.
//
// Deliberately dependency-free (no Arduino headers) so the exact same code that
// runs on the UNO R4 can be compiled and tested on a laptop. See tests/.
//
// Tag layout we write (an ordinary NDEF message, so phones can still read it):
//   page 4: TLV 0x03 <len>   NDEF record  D1 01 <len> 54 02 'e' 'n'  "esc:<slug>"
//   ...     0xFE terminator
//
// Record bytes:  D1 = MB|ME|SR|TNF_WELL_KNOWN
//                01 = type length
//                payload length
//                54 = 'T' (text record)
//                02 = UTF-8, language code length 2, then "en", then the text.

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char NDEF_PREFIX[] = "esc:";
#define NDEF_SLUG_MAX 24
// Page 4 onward, 4 pages (16 bytes) per read: 4 reads cover 64 bytes, far more than one record.
#define NDEF_READ_PAGES 4

// Encode "esc:<slug>" as an NDEF TLV blob. Returns bytes written, 0 on error.
// A slug is lowercase letters, digits and hyphens, 1..NDEF_SLUG_MAX chars.
// Both the writer and the reader enforce it, so a half-programmed or foreign tag
// can never turn into a song name.
inline bool ndefValidSlug(const char *slug) {
  size_t n = slug ? strlen(slug) : 0;
  if (n == 0 || n > NDEF_SLUG_MAX) return false;
  for (size_t i = 0; i < n; i++) {
    char c = slug[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
  }
  return true;
}

inline size_t ndefBuildSlug(const char *slug, uint8_t *out, size_t cap) {
  if (!ndefValidSlug(slug)) return 0;

  char text[NDEF_SLUG_MAX + 5];
  size_t textLen = (size_t)snprintf(text, sizeof(text), "%s%s", NDEF_PREFIX, slug);
  if (textLen >= sizeof(text)) return 0;

  size_t payloadLen = 3 + textLen;      // status byte + "en" + text
  size_t recordLen = 4 + payloadLen;    // header, type len, payload len, type 'T'
  if (recordLen > 254) return 0;        // short-record only

  size_t total = 2 + recordLen + 1;     // TLV header + record + terminator
  if (total > cap) return 0;

  size_t i = 0;
  out[i++] = 0x03;                      // NDEF message TLV
  out[i++] = (uint8_t)recordLen;
  out[i++] = 0xD1;                      // MB|ME|SR, TNF=well-known
  out[i++] = 0x01;                      // type length
  out[i++] = (uint8_t)payloadLen;
  out[i++] = 'T';                       // text record
  out[i++] = 0x02;                      // UTF-8, lang length 2
  out[i++] = 'e';
  out[i++] = 'n';
  memcpy(out + i, text, textLen);
  i += textLen;
  out[i++] = 0xFE;                      // terminator TLV
  return i;
}

// Pull the text out of the first text record found in `data`.
// Returns true and fills `out` on success.
inline bool ndefParseText(const uint8_t *data, size_t len, char *out, size_t cap) {
  size_t i = 0;
  while (i + 2 <= len) {
    uint8_t tag = data[i++];
    if (tag == 0x00) continue;          // null TLV, skip
    if (tag == 0xFE) return false;      // terminator, nothing found
    size_t tlvLen = data[i++];
    if (tlvLen == 0xFF) {               // long form, 2-byte length
      if (i + 2 > len) return false;
      tlvLen = ((size_t)data[i] << 8) | data[i + 1];
      i += 2;
    }
    if (i + tlvLen > len) tlvLen = len - i;
    if (tag != 0x03) { i += tlvLen; continue; }

    size_t end = i + tlvLen;
    size_t p = i;
    while (p + 3 <= end) {
      uint8_t header = data[p++];
      uint8_t typeLen = data[p++];
      bool shortRecord = (header & 0x10) != 0;
      bool hasId = (header & 0x08) != 0;
      size_t payloadLen;
      if (shortRecord) {
        payloadLen = data[p++];
      } else {
        if (p + 4 > end) return false;
        payloadLen = ((size_t)data[p] << 24) | ((size_t)data[p + 1] << 16) |
                     ((size_t)data[p + 2] << 8) | data[p + 3];
        p += 4;
      }
      size_t typeStart = p;
      p += typeLen;
      size_t idLen = 0;
      if (hasId) { idLen = data[p]; p += 1; }
      p += idLen;
      size_t payloadStart = p;
      p += payloadLen;
      if (payloadStart + payloadLen > end) return false;

      uint8_t tnf = header & 0x07;
      if (tnf == 0x01 && typeLen == 1 && data[typeStart] == 'T' && payloadLen >= 1) {
        uint8_t status = data[payloadStart];
        size_t langLen = status & 0x3F;
        if (1 + langLen > payloadLen) return false;
        size_t textLen = payloadLen - 1 - langLen;
        if (textLen >= cap) textLen = cap - 1;
        memcpy(out, data + payloadStart + 1 + langLen, textLen);
        out[textLen] = '\0';
        return textLen > 0;
      }
      if (header & 0x40) break;         // ME set: last record
    }
    return false;
  }
  return false;
}

// True if `text` is our payload and copies the slug into `slug`.
inline bool ndefTextToSlug(const char *text, char *slug, size_t cap) {
  size_t prefixLen = strlen(NDEF_PREFIX);
  if (strncmp(text, NDEF_PREFIX, prefixLen) != 0) return false;
  const char *s = text + prefixLen;
  size_t n = strlen(s);
  if (n >= cap || !ndefValidSlug(s)) return false;
  memcpy(slug, s, n);
  slug[n] = '\0';
  return true;
}
