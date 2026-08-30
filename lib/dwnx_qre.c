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
#include "dwnx_qre.h"

#include <assert.h>

#include "dwnx_conv.h"
#include "dwnx_macro.h"
#include "dwnx_log.h"

void dwnx_qre_init(dwnx_qre *qre, dwnx_log *log) {
  *qre = (dwnx_qre){
    .log = log,
  };
}

void dwnx_qre_start(dwnx_qre *qre, uint8_t *buf, size_t buflen) {
  assert(buflen >= DWNX_QRE_RECORDLEN_SIZE);

  buflen =
    dwnx_min(buflen, DWNX_QRE_RECORDLEN_SIZE + DWNX_DEFAULT_MAX_RECORD_SIZE);

  qre->flags |= DWNX_QRE_FLAG_STARTED;
  dwnx_buf_init(&qre->buf, buf, buflen);
  /* Leave the space for the record length */
  qre->buf.last += DWNX_QRE_RECORDLEN_SIZE;
}

int dwnx_qre_has_started(const dwnx_qre *qre) {
  return (qre->flags & DWNX_QRE_FLAG_STARTED) != 0;
}

dwnx_ssize dwnx_qre_stream_max_datalen(const dwnx_qre *qre, int64_t stream_id,
                                       uint64_t offset, uint64_t len) {
  size_t left = dwnx_buf_left(&qre->buf);
  size_t n = 1 /* type */ + dwnx_put_uvarintlen((uint64_t)stream_id) +
             (offset ? dwnx_put_uvarintlen(offset) : 0);

  assert(left <= DWNX_DEFAULT_MAX_RECORD_SIZE);

  if (left <= n) {
    return -1;
  }

  left -= n;

  if (left > 2 + 63 && len > 63) {
    len = dwnx_min(len, 16383);

    return (dwnx_ssize)dwnx_min(len, (uint64_t)(left - 2));
  }

  len = dwnx_min(len, 63);

  return (dwnx_ssize)dwnx_min(len, (uint64_t)(left - 1));
}

int dwnx_qre_encode_frame(dwnx_qre *qre, const dwnx_frame *fr) {
  dwnx_ssize nwrite;

  nwrite = dwnx_frame_encode(qre->buf.last, dwnx_buf_left(&qre->buf), fr);
  if (nwrite < 0) {
    return (int)nwrite;
  }

  dwnx_log_tx_fr(qre->log, fr);

  qre->buf.last += nwrite;

  return 0;
}

size_t dwnx_qre_final(dwnx_qre *qre) {
  size_t len;

  qre->flags &= ~DWNX_QRE_FLAG_STARTED;

  len = dwnx_buf_len(&qre->buf);
  if (len == DWNX_QRE_RECORDLEN_SIZE) {
    return 0;
  }

  dwnx_put_uvarintw(qre->buf.begin, len - DWNX_QRE_RECORDLEN_SIZE,
                    DWNX_QRE_RECORDLEN_SIZE);

  dwnx_log_tx_rcd(qre->log, len - DWNX_QRE_RECORDLEN_SIZE);

  return len;
}

void dwnx_qre_reset(dwnx_qre *qre) { qre->flags &= ~DWNX_QRE_FLAG_STARTED; }

size_t dwnx_qre_left(const dwnx_qre *qre) { return dwnx_buf_left(&qre->buf); }
