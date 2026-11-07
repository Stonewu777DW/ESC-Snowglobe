// Pure page arithmetic for MIFARE Ultralight / NTAG memory.
//
// These chips address memory in 4-byte pages, and this particular family of clones
// only stores the FIRST page of a 16-byte block write (measured: a 16-byte write to
// page 4 landed 4 bytes and zeroed the rest). So the payload has to be pushed one
// page at a time, which needs this splitting done exactly right.
//
// No Arduino headers here on purpose: tests/pagewrite_test.cpp builds it on a laptop.
#pragma once
#include <stdint.h>
#include <string.h>

#define ESC_PAGE_SIZE 4

// How many 4-byte pages a buffer of `len` bytes needs (last page zero-padded).
inline uint16_t escPageCount(uint16_t len) {
  return (uint16_t)((len + ESC_PAGE_SIZE - 1) / ESC_PAGE_SIZE);
}

// Copy one 4-byte page out of the buffer, zero-padding past the end.
inline void escPageSlice(const uint8_t *data, uint16_t len, uint16_t pageIndex, uint8_t out[4]) {
  memset(out, 0, ESC_PAGE_SIZE);
  uint16_t off = (uint16_t)(pageIndex * ESC_PAGE_SIZE);
  for (uint8_t i = 0; i < ESC_PAGE_SIZE; i++) {
    if ((uint16_t)(off + i) < len) out[i] = data[off + i];
  }
}
