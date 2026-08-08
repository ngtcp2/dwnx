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
#include "dwnx_str.h"

#include <string.h>

#include "dwnx_unreachable.h"

void *dwnx_cpymem(void *dest, const void *src, size_t n) {
  memcpy(dest, src, n);
  return (uint8_t *)dest + n;
}

uint8_t *dwnx_setmem(uint8_t *dest, uint8_t b, size_t n) {
  memset(dest, b, n);
  return dest + n;
}

#define LOWER_XDIGITS "0123456789abcdef"

uint8_t *dwnx_encode_hex(uint8_t *dest, const uint8_t *data, size_t len) {
  size_t i;

  for (i = 0; i < len; ++i) {
    *dest++ = (uint8_t)LOWER_XDIGITS[data[i] >> 4];
    *dest++ = (uint8_t)LOWER_XDIGITS[data[i] & 0xFU];
  }

  return dest;
}

size_t dwnx_encode_uint_hexlen(uint64_t n) {
  size_t i;
  uint8_t d;

  if (n == 0) {
    return 1;
  }

  for (i = 0; i < sizeof(n); ++i) {
    d = (uint8_t)(n >> (sizeof(n) - 1 - i) * 8);
    if (!d) {
      continue;
    }

    if (d >> 4) {
      return (sizeof(n) - i) * 2;
    }

    return (sizeof(n) - i) * 2 - 1;
  }

  dwnx_unreachable();
}

uint8_t *dwnx_encode_uint_hex(uint8_t *dest, uint64_t n) {
  size_t i;
  uint8_t d;

  if (n == 0) {
    *dest++ = '0';

    return dest;
  }

  for (i = 0; i < sizeof(n); ++i) {
    d = (uint8_t)(n >> (sizeof(n) - 1 - i) * 8);
    if (d) {
      if (d >> 4) {
        *dest++ = (uint8_t)LOWER_XDIGITS[d >> 4];
      }

      *dest++ = (uint8_t)LOWER_XDIGITS[d & 0xFU];
      ++i;

      break;
    }
  }

  for (; i < sizeof(n); ++i) {
    d = (uint8_t)(n >> (sizeof(n) - 1 - i) * 8);

    *dest++ = (uint8_t)LOWER_XDIGITS[d >> 4];
    *dest++ = (uint8_t)LOWER_XDIGITS[d & 0xFU];
  }

  return dest;
}

/* countl_zero counts the number of leading zeros in |x|.  It is
   undefined if |x| is 0. */
static int countl_zero(uint64_t x) {
#ifdef __GNUC__
  return __builtin_clzll(x);
#else  /* !defined(__GNUC__) */
  /* This is the same implementation of Go's LeadingZeros64 in
     math/bits package. */
  static const uint8_t len8tab[] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  };
  int n = 0;

  if (x >= 1ULL << 32) {
    x >>= 32;
    n += 32;
  }

  if (x >= 1 << 16) {
    x >>= 16;
    n += 16;
  }

  if (x >= 1 << 8) {
    x >>= 8;
    n += 8;
  }

  return 64 - (n + len8tab[x]);
#endif /* !defined(__GNUC__) */
}

/*
 * count_digit returns the minimum number of digits to represent |x|
 * in base 10.
 *
 * credit:
 * https://lemire.me/blog/2025/01/07/counting-the-digits-of-64-bit-integers/
 */
static size_t count_digit(uint64_t x) {
  static const uint64_t count_digit_tbl[] = {
    9ULL,
    99ULL,
    999ULL,
    9999ULL,
    99999ULL,
    999999ULL,
    9999999ULL,
    99999999ULL,
    999999999ULL,
    9999999999ULL,
    99999999999ULL,
    999999999999ULL,
    9999999999999ULL,
    99999999999999ULL,
    999999999999999ULL,
    9999999999999999ULL,
    99999999999999999ULL,
    999999999999999999ULL,
    9999999999999999999ULL,
  };
  size_t y = (size_t)(19 * (63 - countl_zero(x | 1)) >> 6);

  y += x > count_digit_tbl[y];

  return y + 1;
}

size_t dwnx_encode_uintlen(uint64_t n) { return count_digit(n); }

uint8_t *dwnx_encode_uint(uint8_t *dest, uint64_t n) {
  static const uint8_t uint_digits[] =
    "00010203040506070809101112131415161718192021222324252627282930313233343536"
    "37383940414243444546474849505152535455565758596061626364656667686970717273"
    "7475767778798081828384858687888990919293949596979899";
  uint8_t *p;
  const uint8_t *tp;

  if (n < 10) {
    *dest++ = (uint8_t)('0' + n);
    return dest;
  }

  if (n < 100) {
    tp = &uint_digits[n * 2];
    *dest++ = *tp++;
    *dest++ = *tp;
    return dest;
  }

  dest += count_digit(n);
  p = dest;

  for (; n >= 100; n /= 100) {
    p -= 2;
    tp = &uint_digits[(n % 100) * 2];
    p[0] = *tp++;
    p[1] = *tp;
  }

  if (n < 10) {
    *--p = (uint8_t)('0' + n);
    return dest;
  }

  p -= 2;
  tp = &uint_digits[n * 2];
  p[0] = *tp++;
  p[1] = *tp;

  return dest;
}

uint8_t *dwnx_encode_printable_ascii(uint8_t *dest, const uint8_t *data,
                                     size_t len) {
  size_t i;
  uint8_t c;

  for (i = 0; i < len; ++i) {
    c = data[i];
    if (0x20 <= c && c <= 0x7E) {
      *dest++ = c;
    } else {
      *dest++ = '.';
    }
  }

  return dest;
}
