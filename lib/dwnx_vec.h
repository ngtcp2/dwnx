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
#ifndef DWNX_VEC_H
#define DWNX_VEC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

/*
 * dwnx_vec_init initializes |vec| with the given parameters.  It
 * returns |vec|.
 */
dwnx_vec *dwnx_vec_init(dwnx_vec *vec, const uint8_t *base, size_t len);

/*
 * dwnx_vec_len returns the sum of length in |vec| of |n| elements.
 */
uint64_t dwnx_vec_len(const dwnx_vec *vec, size_t n);

static inline dwnx_vec dwnx_vec_sub(const dwnx_vec *vec, size_t len) {
  return (dwnx_vec){
    .base = vec->base + len,
    .len = vec->len - len,
  };
}

/*
 * dwnx_vec_len_varint is similar to dwnx_vec_len, but it returns -1
 * if the sum of the length exceeds DWNX_MAX_VARINT.
 */
int64_t dwnx_vec_len_varint(const dwnx_vec *vec, size_t n);

/*
 * dwnx_vec_copy_at_most copies |src| of length |srccnt| to |dst| of
 * length |dstcnt|.  The total number of bytes which the copied
 * dwnx_vec refers to is at most |left|.  The empty elements in |src|
 * are ignored.  This function returns the number of elements copied.
 */
size_t dwnx_vec_copy_at_most(dwnx_vec *dst, size_t dstcnt, const dwnx_vec *src,
                             size_t srccnt, size_t left);

#endif /* !defined(DWNX_VEC_H) */
