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
#include "dwnx_record_reader.h"

#include <string.h>
#include <assert.h>

#include "dwnx_conv.h"
#include "dwnx_macro.h"
#include "dwnx_net.h"
#include "dwnx_mem.h"

void dwnx_varint_reader_reset(dwnx_varint_reader *vird) {
  *vird = (dwnx_varint_reader){0};
}

dwnx_ssize dwnx_varint_reader_read(dwnx_varint_reader *vird,
                                   const uint8_t *data, size_t datalen,
                                   int fin) {
  size_t len, vlen;
  uint8_t *p;
  const uint8_t *begin = data, *end = data + datalen;

  assert(begin != end);

  if (vird->left == 0) {
    assert(vird->acc == 0);

    vlen = dwnx_get_uvarintlen(begin);
    len = dwnx_min(vlen, (size_t)(end - begin));
    if (vlen <= len) {
      dwnx_get_uvarint(&vird->acc, begin);
      return (dwnx_ssize)vlen;
    }

    if (fin) {
      return DWNX_ERR_INVALID_ARGUMENT;
    }

    p = (uint8_t *)&vird->acc + (sizeof(vird->acc) - vlen);
    memcpy(p, begin, len);
    *p &= 0x3FU;
    vird->left = vlen - len;

    return (dwnx_ssize)len;
  }

  len = dwnx_min(vird->left, (size_t)(end - begin));
  p = (uint8_t *)&vird->acc + (sizeof(vird->acc) - vird->left);
  memcpy(p, begin, len);
  vird->left -= len;

  if (vird->left == 0) {
    vird->acc = dwnx_ntohl64(vird->acc);
  } else if (fin) {
    return DWNX_ERR_INVALID_ARGUMENT;
  }

  return (dwnx_ssize)len;
}

void dwnx_record_reader_reset(dwnx_record_reader *rcrd, const dwnx_mem *mem) {
  dwnx_mem_free(mem, rcrd->buf.begin);

  *rcrd = (dwnx_record_reader){0};
}

void dwnx_record_reader_next_frame(dwnx_record_reader *rcrd,
                                   const dwnx_mem *mem) {
  if (rcrd->buf.begin) {
    dwnx_mem_free(mem, rcrd->buf.begin);
    rcrd->buf = (dwnx_buf){0};
  }

  rcrd->state = DWNX_RECORD_READ_STATE_FRAME_TYPE;
}

size_t dwnx_record_reader_avail(dwnx_record_reader *rcrd, size_t len) {
  return dwnx_min(len, rcrd->record_left);
}

size_t dwnx_record_reader_field_avail(dwnx_record_reader *rcrd, size_t len) {
  return dwnx_min(rcrd->field_left, dwnx_record_reader_avail(rcrd, len));
}

size_t dwnx_record_reader_buf_avail(dwnx_record_reader *rcrd, size_t len) {
  return dwnx_min(dwnx_buf_left(&rcrd->buf),
                  dwnx_record_reader_avail(rcrd, len));
}
