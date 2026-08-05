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
                                   const uint8_t *data, size_t datalen,
                                   int fin);

static inline int dwnx_varint_reader_done(dwnx_varint_reader *vird) {
  return vird->left == 0;
}

typedef enum dwnx_record_read_state {
  DWNX_RECORD_READ_STATE_RECORD_SIZE,
  DWNX_RECORD_READ_STATE_FRAME_TYPE,
  DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_LEN,
  DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_PARAMS,
  DWNX_RECORD_READ_STATE_STREAM_STREAM_ID,
  DWNX_RECORD_READ_STATE_STREAM_OFFSET,
  DWNX_RECORD_READ_STATE_STREAM_LENGTH,
  DWNX_RECORD_READ_STATE_STREAM_DATA,
  DWNX_RECORD_READ_STATE_RESET_STREAM_STREAM_ID,
  DWNX_RECORD_READ_STATE_RESET_STREAM_APP_ERROR_CODE,
  DWNX_RECORD_READ_STATE_RESET_STREAM_FINAL_SIZE,
  DWNX_RECORD_READ_STATE_STOP_SENDING_STREAM_ID,
  DWNX_RECORD_READ_STATE_STOP_SENDING_APP_ERROR_CODE,
  DWNX_RECORD_READ_STATE_MAX_DATA_MAX_DATA,
  DWNX_RECORD_READ_STATE_MAX_STREAM_DATA_STREAM_ID,
  DWNX_RECORD_READ_STATE_MAX_STREAM_DATA_MAX_STREAM_DATA,
  DWNX_RECORD_READ_STATE_MAX_STREAMS_MAX_STREAMS,
} dwnx_record_read_state;

typedef struct dwnx_record_reader {
  /* state is the state of this reader. */
  dwnx_record_read_state state;
  /* fr is the frame currently received. */
  dwnx_frame fr;
  /* record_left is the number of bytes left in the current record. */
  size_t record_left;
  /* field_left is the number of bytes left in the field currently
     reading.  This field is not used for every field.*/
  size_t field_left;
  /* buf points to the buffer when we need to buffer data. */
  dwnx_buf buf;
} dwnx_record_reader;

/* dwnx_record_reader_reset resets per-frame or per-record state.
   |rcrd|->buf is freed. */
void dwnx_record_reader_reset(dwnx_record_reader *rcrd, const dwnx_mem *mem);

/* dwnx_record_reader_avail returns the number of bytes to read from
   the given data of length |len|, taking into account the remainder
   of the record. */
size_t dwnx_record_reader_avail(dwnx_record_reader *rcrd, size_t len);

/* dwnx_record_reader_field_avail returns the number of bytes to read
   from the given data of length |len|, taking into account the
   remainder of the field (see |rcrd|->field_left).  This function
   assumes that |rcrd|->field_left has been initialized and set to the
   valid value. */
size_t dwnx_record_reader_field_avail(dwnx_record_reader *rcrd, size_t len);

/* dwnx_record_reader_buf_avail returns the number of bytes to read
   from the given data of length |len|, taking into account the
   remainder of the buf (see |rcrd->buf).  This function assumes that
   |rcrd|->buf has been initialized. */
size_t dwnx_record_reader_buf_avail(dwnx_record_reader *rcrd, size_t len);

#endif /* !defined(DWNX_RECORD_READ_STATE_H) */
