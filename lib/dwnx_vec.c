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
#include "dwnx_vec.h"

dwnx_vec *dwnx_vec_init(dwnx_vec *vec, const uint8_t *base, size_t len) {
  *vec = (dwnx_vec){
    .base = (uint8_t *)base,
    .len = len,
  };

  return vec;
}

uint64_t dwnx_vec_len(const dwnx_vec *vec, size_t n) {
  uint64_t sum = 0;

  for (; n; ++vec, --n) {
    sum += vec->len;
  }

  return sum;
}

int64_t dwnx_vec_len_varint(const dwnx_vec *vec, size_t n) {
  uint64_t res = 0;
  size_t len;
  size_t i;

  for (i = 0; i < n; ++i) {
    len = vec[i].len;
    if (len > DWNX_MAX_VARINT - res) {
      return -1;
    }

    res += len;
  }

  return (int64_t)res;
}

size_t dwnx_vec_copy_at_most(dwnx_vec *dst, size_t dstcnt, const dwnx_vec *src,
                             size_t srccnt, size_t left) {
  size_t i, j;

  for (i = 0, j = 0; left > 0 && i < srccnt && j < dstcnt;) {
    if (src[i].len == 0) {
      ++i;
      continue;
    }

    if (src[i].len > left) {
      dst[j] = (dwnx_vec){
        .base = src[i].base,
        .len = left,
      };

      return j + 1;
    }

    dst[j] = src[i];
    left -= dst[j].len;
    ++i;
    ++j;
  }

  return j;
}
