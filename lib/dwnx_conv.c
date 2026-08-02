/*
 * dwnx
 *
 * Copyright (c) 2026 dwnx contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "dwnx_conv.h"

#include <string.h>
#include <assert.h>

#include "dwnx_str.h"
#include "dwnx_net.h"
#include "dwnx_unreachable.h"

const uint8_t *dwnx_get_uint64be(uint64_t *dest, const uint8_t *p) {
  memcpy(dest, p, sizeof(*dest));
  *dest = dwnx_ntohl64(*dest);
  return p + sizeof(*dest);
}

const uint8_t *dwnx_get_uint32be(uint32_t *dest, const uint8_t *p) {
  memcpy(dest, p, sizeof(*dest));
  *dest = dwnx_ntohl(*dest);
  return p + sizeof(*dest);
}

const uint8_t *dwnx_get_uint24be(uint32_t *dest, const uint8_t *p) {
  *dest = 0;
  memcpy(((uint8_t *)dest) + 1, p, 3);
  *dest = dwnx_ntohl(*dest);
  return p + 3;
}

const uint8_t *dwnx_get_uint16be(uint16_t *dest, const uint8_t *p) {
  memcpy(dest, p, sizeof(*dest));
  *dest = dwnx_ntohs(*dest);
  return p + sizeof(*dest);
}

const uint8_t *dwnx_get_uint16(uint16_t *dest, const uint8_t *p) {
  memcpy(dest, p, sizeof(*dest));
  return p + sizeof(*dest);
}

const uint8_t *dwnx_get_uvarint(uint64_t *dest, const uint8_t *p) {
  uint16_t n16;
  uint32_t n32;
  uint64_t n64;

  switch (*p >> 6) {
  case 0:
    *dest = *p++;
    return p;
  case 1:
    memcpy(&n16, p, 2);
    n16 = dwnx_ntohs(n16);
    n16 &= 0x3FFFU;
    *dest = n16;

    return p + 2;
  case 2:
    memcpy(&n32, p, 4);
    n32 = dwnx_ntohl(n32);
    n32 &= 0x3FFFFFFFU;
    *dest = n32;

    return p + 4;
  case 3:
    memcpy(&n64, p, 8);
    n64 = dwnx_ntohl64(n64);
    n64 &= 0x3FFFFFFFFFFFFFFFU;
    *dest = n64;

    return p + 8;
  default:
    dwnx_unreachable();
  }
}

uint8_t *dwnx_put_uint64be(uint8_t *p, uint64_t n) {
  n = dwnx_htonl64(n);
  return dwnx_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *dwnx_put_uint32be(uint8_t *p, uint32_t n) {
  n = dwnx_htonl(n);
  return dwnx_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *dwnx_put_uint24be(uint8_t *p, uint32_t n) {
  n = dwnx_htonl(n);
  return dwnx_cpymem(p, ((const uint8_t *)&n) + 1, 3);
}

uint8_t *dwnx_put_uint16be(uint8_t *p, uint16_t n) {
  n = dwnx_htons(n);
  return dwnx_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *dwnx_put_uint16(uint8_t *p, uint16_t n) {
  return dwnx_cpymem(p, (const uint8_t *)&n, sizeof(n));
}

uint8_t *dwnx_put_uvarint(uint8_t *p, uint64_t n) {
  uint8_t *rv;
  if (n < 64) {
    *p++ = (uint8_t)n;
    return p;
  }
  if (n < 16384) {
    rv = dwnx_put_uint16be(p, (uint16_t)n);
    *p |= 0x40U;
    return rv;
  }
  if (n < 1073741824) {
    rv = dwnx_put_uint32be(p, (uint32_t)n);
    *p |= 0x80U;
    return rv;
  }
  assert(n < 4611686018427387904ULL);
  rv = dwnx_put_uint64be(p, n);
  *p |= 0xC0U;
  return rv;
}

uint8_t *dwnx_put_uvarintw(uint8_t *p, uint64_t n, size_t width) {
  uint8_t *rv;

  switch (width) {
  case 1:
    assert(n < 64);
    *p++ = (uint8_t)n;
    return p;
  case 2:
    assert(n < 16384);
    rv = dwnx_put_uint16be(p, (uint16_t)n);
    *p |= 0x40U;
    return rv;
  case 4:
    assert(n < 1073741824);
    rv = dwnx_put_uint32be(p, (uint32_t)n);
    *p |= 0x80U;
    return rv;
  case 8:
    assert(n < 4611686018427387904ULL);
    rv = dwnx_put_uint64be(p, n);
    *p |= 0xC0U;
    return rv;
  default:
    dwnx_unreachable();
  }
}

size_t dwnx_get_uvarintlen(const uint8_t *p) {
  return (size_t)(1U << (*p >> 6));
}

size_t dwnx_put_uvarintlen(uint64_t n) {
  if (n < 64) {
    return 1;
  }
  if (n < 16384) {
    return 2;
  }
  if (n < 1073741824) {
    return 4;
  }
  assert(n < 4611686018427387904ULL);
  return 8;
}

uint64_t dwnx_ord_stream_id(int64_t stream_id) {
  return (uint64_t)(stream_id >> 2) + 1;
}
