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
#include "dwnx_fmt.h"

#include <string.h>

#include "dwnx_str.h"

char *dwnx_fmt_write_int64(char *dest, int64_t n) {
  if (n < 0) {
    *dest++ = '-';
    return dwnx_fmt_write_uint64(dest, 0 - (uint64_t)n);
  }

  return dwnx_fmt_write_uint64(dest, (uint64_t)n);
}

char *dwnx_fmt_write_uint64(char *dest, uint64_t n) {
  return (char *)dwnx_encode_uint((uint8_t *)dest, n);
}

char *dwnx_fmt_write_char(char *dest, char c) {
  *dest++ = c;

  return dest;
}

char *dwnx_fmt_write_str(char *dest, const char *s) {
  return dwnx_cpymem(dest, s, strlen(s));
}

static char *fill_zero(char *dest, size_t len, size_t width) {
  if (len < width) {
    return (char *)dwnx_setmem((uint8_t *)dest, '0', width - len);
  }

  return dest;
}

char *dwnx_fmt_write_uint64w(char *dest, dwnx_fmt_uint64w f) {
  size_t len = dwnx_encode_uintlen(f.n);

  return (char *)dwnx_encode_uint((uint8_t *)fill_zero(dest, len, f.width),
                                  f.n);
}

char *dwnx_fmt_write_hex(char *dest, dwnx_fmt_hex f) {
  return (char *)dwnx_encode_uint_hex((uint8_t *)dest, f.n);
}

char *dwnx_fmt_write_hexw(char *dest, dwnx_fmt_hexw f) {
  size_t len = dwnx_encode_uint_hexlen(f.n);

  return (char *)dwnx_encode_uint_hex((uint8_t *)fill_zero(dest, len, f.width),
                                      f.n);
}

char *dwnx_fmt_write_bhex(char *dest, dwnx_fmt_bhex f) {
  return (char *)dwnx_encode_hex((uint8_t *)dest, f.data, f.len);
}

char *dwnx_fmt_write_printable_ascii(char *dest,
                                     const dwnx_fmt_printable_ascii f) {
  return (char *)dwnx_encode_printable_ascii((uint8_t *)dest, f.data, f.len);
}
