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
#ifndef DWNX_CONV_H
#define DWNX_CONV_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

/*
 * dwnx_get_uint64be reads 8 bytes from |p| as 64 bits unsigned
 * integer encoded as network byte order, and stores it in the buffer
 * pointed by |dest| in host byte order.  It returns |p| + 8.
 */
const uint8_t *dwnx_get_uint64be(uint64_t *dest, const uint8_t *p);

/*
 * dwnx_get_uint32be reads 4 bytes from |p| as 32 bits unsigned
 * integer encoded as network byte order, and stores it in the buffer
 * pointed by |dest| in host byte order.  It returns |p| + 4.
 */
const uint8_t *dwnx_get_uint32be(uint32_t *dest, const uint8_t *p);

/*
 * dwnx_get_uint24be reads 3 bytes from |p| as 24 bits unsigned
 * integer encoded as network byte order, and stores it in the buffer
 * pointed by |dest| in host byte order.  It returns |p| + 3.
 */
const uint8_t *dwnx_get_uint24be(uint32_t *dest, const uint8_t *p);

/*
 * dwnx_get_uint16be reads 2 bytes from |p| as 16 bits unsigned
 * integer encoded as network byte order, and stores it in the buffer
 * pointed by |dest| in host byte order.  It returns |p| + 2.
 */
const uint8_t *dwnx_get_uint16be(uint16_t *dest, const uint8_t *p);

/*
 * dwnx_get_uint16 reads 2 bytes from |p| as 16 bits unsigned integer
 * encoded as network byte order, and stores it in the buffer pointed
 * by |dest| as is.  It returns |p| + 2.
 */
const uint8_t *dwnx_get_uint16(uint16_t *dest, const uint8_t *p);

/*
 * dwnx_get_uvarint reads variable-length unsigned integer from |p|,
 * and stores it in the buffer pointed by |dest| in host byte order.
 * It returns |p| plus the number of bytes read from |p|.
 */
const uint8_t *dwnx_get_uvarint(uint64_t *dest, const uint8_t *p);

/*
 * dwnx_get_varint reads variable-length unsigned integer from |p|,
 * and casts it to the signed integer, and stores it in the buffer
 * pointed by |dest| in host byte order.  No information should be
 * lost in this cast, because the variable-length integer is 62
 * bits. It returns |p| plus the number of bytes read from |p|.
 */
static inline const uint8_t *dwnx_get_varint(int64_t *dest, const uint8_t *p) {
  uint64_t n;

  p = dwnx_get_uvarint(&n, p);
  *dest = (int64_t)n;

  return p;
}

/*
 * dwnx_put_uint64be writes |n| in host byte order in |p| in network
 * byte order.  It returns the one beyond of the last written
 * position.
 */
uint8_t *dwnx_put_uint64be(uint8_t *p, uint64_t n);

/*
 * dwnx_put_uint32be writes |n| in host byte order in |p| in network
 * byte order.  It returns the one beyond of the last written
 * position.
 */
uint8_t *dwnx_put_uint32be(uint8_t *p, uint32_t n);

/*
 * dwnx_put_uint24be writes |n| in host byte order in |p| in network
 * byte order.  It writes only least significant 24 bits.  It returns
 * the one beyond of the last written position.
 */
uint8_t *dwnx_put_uint24be(uint8_t *p, uint32_t n);

/*
 * dwnx_put_uint16be writes |n| in host byte order in |p| in network
 * byte order.  It returns the one beyond of the last written
 * position.
 */
uint8_t *dwnx_put_uint16be(uint8_t *p, uint16_t n);

/*
 * dwnx_put_uint16 writes |n| as is in |p|.  It returns the one beyond
 * of the last written position.
 */
uint8_t *dwnx_put_uint16(uint8_t *p, uint16_t n);

/*
 * dwnx_put_uvarint writes |n| in |p| using variable-length integer
 * encoding.  It returns the one beyond of the last written position.
 */
uint8_t *dwnx_put_uvarint(uint8_t *p, uint64_t n);

/*
 * dwnx_get_uvarintlen returns the required number of bytes to read
 * variable-length integer starting at |p|.
 */
size_t dwnx_get_uvarintlen(const uint8_t *p);

/*
 * dwnx_put_uvarintlen returns the required number of bytes to encode
 * |n|.
 */
size_t dwnx_put_uvarintlen(uint64_t n);

uint8_t *dwnx_put_uvarintw(uint8_t *p, uint64_t n, size_t width);

/*
 * dwnx_ord_stream_id returns the ordinal number of |stream_id|.
 */
uint64_t dwnx_ord_stream_id(int64_t stream_id);

#endif /* !defined(DWNX_CONV_H) */
