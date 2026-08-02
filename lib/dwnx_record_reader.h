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
#ifndef DWNX_RECORD_READ_STATE_H
#define DWNX_RECORD_READ_STATE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_frame.h"
#include "dwnx_buf.h"

typedef struct dwnx_mem dwnx_mem;

typedef struct dwnx_varint_reader {
  uint64_t acc;
  size_t left;
} dwnx_varint_reader;

void dwnx_varint_reader_reset(dwnx_varint_reader *vird);

dwnx_ssize dwnx_varint_reader_read(dwnx_varint_reader *vird,
                                   const uint8_t *begin, const uint8_t *end,
                                   int fin);

static inline int dwnx_varint_reader_done(dwnx_varint_reader *vird) {
  return vird->left == 0;
}

typedef enum dwnx_record_read_state {
  DWNX_RECORD_READ_STATE_RECORD_SIZE,
  DWNX_RECORD_READ_STATE_FRAME_TYPE,
  DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_LEN,
  DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_PARAMS,
} dwnx_record_read_state;

typedef struct dwnx_record_reader {
  dwnx_record_read_state state;
  dwnx_frame fr;
  size_t record_left;
  dwnx_buf buf;
} dwnx_record_reader;

void dwnx_record_reader_reset(dwnx_record_reader *rcrd, const dwnx_mem *mem);

void dwnx_record_reader_next_frame(dwnx_record_reader *rcrd,
                                   const dwnx_mem *mem);

size_t dwnx_record_reader_buf_avail(dwnx_record_reader *rcrd, size_t len);

int dwnx_record_reader_fin(dwnx_record_reader *rcrd, size_t len);

#endif /* !defined(DWNX_RECORD_READ_STATE_H) */
