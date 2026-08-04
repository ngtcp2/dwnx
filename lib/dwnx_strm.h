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
#ifndef DWNX_STRM_H
#define DWNX_STRM_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_pq.h"

/* DWNX_STRM_FLAG_NONE indicates that no flag is set. */
#define DWNX_STRM_FLAG_NONE 0x00U
/* DWNX_STRM_FLAG_SHUT_RD indicates that further reception of stream
   data is not allowed. */
#define DWNX_STRM_FLAG_SHUT_RD 0x01U
/* DWNX_STRM_FLAG_SHUT_WR indicates that further transmission of
   stream data is not allowed. */
#define DWNX_STRM_FLAG_SHUT_WR 0x02U
#define DWNX_STRM_FLAG_SHUT_RDWR                                               \
  (DWNX_STRM_FLAG_SHUT_RD | DWNX_STRM_FLAG_SHUT_WR)
/* DWNX_STRM_FLAG_RESET_STREAM indicates that RESET_STREAM is sent
   from the local endpoint.  In this case, DWNX_STRM_FLAG_SHUT_WR is
   also set. */
#define DWNX_STRM_FLAG_RESET_STREAM 0x04U
/* DWNX_STRM_FLAG_RESET_STREAM_RECVED indicates that RESET_STREAM is
   received from the remote endpoint.  In this case,
   DWNX_STRM_FLAG_SHUT_RD is also set. */
#define DWNX_STRM_FLAG_RESET_STREAM_RECVED 0x08U
/* DWNX_STRM_FLAG_STOP_SENDING indicates that STOP_SENDING is sent
   from the local endpoint. */
#define DWNX_STRM_FLAG_STOP_SENDING 0x10U
/* DWNX_STRM_FLAG_SEND_STOP_SENDING is set when STOP_SENDING frame
   should be sent. */
#define DWNX_STRM_FLAG_SEND_STOP_SENDING 0x200U
/* DWNX_STRM_FLAG_SEND_RESET_STREAM is set when RESET_STREAM frame
   should be sent. */
#define DWNX_STRM_FLAG_SEND_RESET_STREAM 0x400U
/* DWNX_STRM_FLAG_STOP_SENDING_RECVED indicates that STOP_SENDING is
   received from the remote endpoint.  In this case,
   DWNX_STRM_FLAG_SHUT_WR is also set. */
#define DWNX_STRM_FLAG_STOP_SENDING_RECVED 0x800U
/* DWNX_STRM_FLAG_RX_APP_ERROR_CODE_SET is set when
   dwnx_strm.rx.app_error_code is set. */
#define DWNX_STRM_FLAG_RX_APP_ERROR_CODE_SET 0x4000U
/* DWNX_STRM_FLAG_TX_RESET_STREAM_APP_ERROR_CODE_SET is set when
   dwnx_strm.tx.reset_stream_app_error_code is set. */
#define DWNX_STRM_FLAG_TX_RESET_STREAM_APP_ERROR_CODE_SET 0x8000U
/* DWNX_STRM_FLAG_TX_STOP_SENDING_APP_ERROR_CODE_SET is set when
   dwnx_strm.tx.stop_sending_app_error_code is set. */
#define DWNX_STRM_FLAG_TX_STOP_SENDING_APP_ERROR_CODE_SET 0x10000U

typedef struct dwnx_strm {
  const dwnx_mem *mem;
  int64_t stream_id;
  dwnx_pq_entry pe;
  uint64_t cycle;

  struct {
    /* last_offset is the largest offset of stream data received for
       this stream. */
    uint64_t last_offset;
    /* max_offset is the maximum offset that remote endpoint can send
       to this stream. */
    uint64_t max_offset;
    /* unsent_max_offset is the maximum offset that remote endpoint
       can send to this stream, and it is not notified to the remote
       endpoint.  unsent_max_offset >= max_offset must be hold. */
    uint64_t unsent_max_offset;
    /* window is the stream-level flow control window size. */
    uint64_t window;
    /* app_error_code is the application error code that is received
       in RESET_STREAM frame.  If this field is set,
       DWNX_STRM_FLAG_RX_APP_ERROR_CODE_SET is set.  This field is
       eventually passed to dwnx_stream_close callback as
       rx_app_error_code parameter. */
    uint64_t app_error_code;
  } rx;

  struct {
    /* offset is the next offset of new outgoing data.  In other
       words, it is the number of bytes sent in this stream without
       duplication. */
    uint64_t offset;
    /* max_tx_offset is the maximum offset that local endpoint can
       send for this stream. */
    uint64_t max_offset;
    /* last_blocked_offset is the largest offset where the
       transmission of stream data is blocked. */
    uint64_t last_blocked_offset;
    /* stop_sending_app_error_code is the application specific error
       code that is sent along with STOP_SENDING.  If this field is
       set, DWNX_STRM_FLAG_TX_STOP_SENDING_APP_ERROR_CODE_SET is set.
       This field is eventually passed to dwnx_stream_close callback
       as rx_app_error_code parameter. */
    uint64_t stop_sending_app_error_code;
    /* reset_stream_app_error_code is the application specific error
       code that is sent along with RESET_STREAM.  If this field is
       set, DWNX_STRM_FLAG_TX_RESET_STREAM_APP_ERROR_CODE_SET is set.
       This field is eventually passed to dwnx_stream_close callback
       as tx_app_error_code parameter. */
    uint64_t reset_stream_app_error_code;
  } tx;

  void *stream_user_data;
  uint32_t flags;
} dwnx_strm;

/*
 * dwnx_strm_init initializes |strm|.
 */
void dwnx_strm_init(dwnx_strm *strm, int64_t stream_id, uint32_t flags,
                    uint64_t max_rx_offset, uint64_t max_tx_offset,
                    void *stream_user_data, const dwnx_mem *mem);

/*
 * dwnx_strm_free deallocates memory allocated for |strm|.  This
 * function does not free the memory pointed by |strm| itself.
 */
void dwnx_strm_free(dwnx_strm *strm);

/*
 * dwnx_strm_shutdown shutdowns |strm|.  |flags| should be one of
 * DWNX_STRM_FLAG_SHUT_RD, DWNX_STRM_FLAG_SHUT_WR, and
 * DWNX_STRM_FLAG_SHUT_RDWR.
 */
void dwnx_strm_shutdown(dwnx_strm *strm, uint32_t flags);

/*
 * dwnx_strm_is_tx_queued returns nonzero if |strm| is queued.
 */
int dwnx_strm_is_tx_queued(const dwnx_strm *strm);

#endif /* !defined(DWNX_STRM_H) */
