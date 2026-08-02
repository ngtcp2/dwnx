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
#ifndef DWNX_BUF_H
#define DWNX_BUF_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

typedef struct dwnx_buf {
  /* begin points to the beginning of the buffer. */
  uint8_t *begin;
  /* end points to the one beyond of the last byte of the buffer */
  uint8_t *end;
  /* pos points to the start of data.  Typically, this points to the
     point that next data should be read.  Initially, it points to
     |begin|. */
  uint8_t *pos;
  /* last points to the one beyond of the last data of the buffer.
     Typically, new data is written at this point.  Initially, it
     points to |begin|. */
  uint8_t *last;
} dwnx_buf;

/*
 * dwnx_buf_init initializes |buf| with the given buffer.
 */
void dwnx_buf_init(dwnx_buf *buf, uint8_t *begin, size_t len);

/*
 * dwnx_buf_reset resets pos and last fields to match begin field to
 * make dwnx_buf_len(buf) return 0.
 */
void dwnx_buf_reset(dwnx_buf *buf);

/*
 * dwnx_buf_left returns the number of additional bytes which can be
 * written to the underlying buffer.  In other words, it returns
 * buf->end - buf->last.
 */
static inline size_t dwnx_buf_left(const dwnx_buf *buf) {
  return (size_t)(buf->end - buf->last);
}

/*
 * dwnx_buf_len returns the number of bytes left to read.  In other
 * words, it returns buf->last - buf->pos.
 */
#define dwnx_buf_len(BUF) (size_t)((BUF)->last - (BUF)->pos)

/*
 * dwnx_buf_cap returns the capacity of the buffer.  In other words,
 * it returns buf->end - buf->begin.
 */
size_t dwnx_buf_cap(const dwnx_buf *buf);

/*
 * dwnx_buf_trunc truncates the number of bytes to read to at most
 * |len|.  In other words, it sets buf->last = buf->pos + len if
 * dwnx_buf_len(buf) > len.
 */
void dwnx_buf_trunc(dwnx_buf *buf, size_t len);

#endif /* !defined(DWNX_BUF_H) */
