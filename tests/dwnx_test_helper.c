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
#include "dwnx_test_helper.h"

#include "dwnx_transport_params.h"
#include "dwnx_conv.h"
#include "dwnx_unreachable.h"
#include "dwnx_record_reader.h"
#include "dwnx_conn.h"
#include "dwnx_str.h"

void dwnx_write_frame(dwnx_buf *dest, const dwnx_frame *fr) {
  uint8_t *p;
  dwnx_ssize nwrite;

  switch (fr->hd.type) {
  case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
    dest->last = dwnx_put_uvarint(dest->last, fr->qx_transport_parameters.type);
    p = dest->last;
    dest->last += 2;

    nwrite = dwnx_transport_params_encode(dest->last, dwnx_buf_left(dest),
                                          fr->qx_transport_parameters.params);
    assert_ptrdiff(0, <=, nwrite);

    dest->last += nwrite;
    dwnx_put_uvarintw(p, (size_t)nwrite, 2);

    return;
  case DWNX_FRAME_QX_PING_REQUEST:
  case DWNX_FRAME_QX_PING_RESPONSE:
    dest->last = dwnx_put_uvarint(dest->last, fr->qx_ping.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->qx_ping.seq);

    return;
  case DWNX_FRAME_PADDING:
    memset(dest->last, 0, fr->padding.len);
    dest->last += fr->padding.len;

    return;
  case DWNX_FRAME_STREAM:
    /* fin must be indicated by DWNX_STREAM_FIN_BIT in flags */
    assert(0 == fr->stream.fin);

    dest->last =
      dwnx_put_uvarint(dest->last, fr->stream.type | fr->stream.flags);
    dest->last = dwnx_put_uvarint(dest->last, (uint64_t)fr->stream.stream_id);

    if (fr->stream.flags & DWNX_STREAM_OFF_BIT) {
      dest->last = dwnx_put_uvarint(dest->last, fr->stream.offset);
    }

    if (fr->stream.flags & DWNX_STREAM_LEN_BIT) {
      dest->last = dwnx_put_uvarint(dest->last, fr->stream.len);
    }

    memset(dest->last, 0, fr->stream.len);
    dest->last += fr->stream.len;

    return;
  case DWNX_FRAME_RESET_STREAM:
    dest->last = dwnx_put_uvarint(dest->last, fr->reset_stream.type);
    dest->last =
      dwnx_put_uvarint(dest->last, (uint64_t)fr->reset_stream.stream_id);
    dest->last = dwnx_put_uvarint(dest->last, fr->reset_stream.app_error_code);
    dest->last = dwnx_put_uvarint(dest->last, fr->reset_stream.final_size);

    return;
  case DWNX_FRAME_STOP_SENDING:
    dest->last = dwnx_put_uvarint(dest->last, fr->stop_sending.type);
    dest->last =
      dwnx_put_uvarint(dest->last, (uint64_t)fr->stop_sending.stream_id);
    dest->last = dwnx_put_uvarint(dest->last, fr->stop_sending.app_error_code);

    return;
  case DWNX_FRAME_MAX_DATA:
    dest->last = dwnx_put_uvarint(dest->last, fr->max_data.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->max_data.max_data);

    return;
  case DWNX_FRAME_MAX_STREAM_DATA:
    dest->last = dwnx_put_uvarint(dest->last, fr->max_stream_data.type);
    dest->last =
      dwnx_put_uvarint(dest->last, (uint64_t)fr->max_stream_data.stream_id);
    dest->last =
      dwnx_put_uvarint(dest->last, fr->max_stream_data.max_stream_data);

    return;
  case DWNX_FRAME_MAX_STREAMS_BIDI:
  case DWNX_FRAME_MAX_STREAMS_UNI:
    dest->last = dwnx_put_uvarint(dest->last, fr->max_streams.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->max_streams.max_streams);

    return;
  case DWNX_FRAME_DATA_BLOCKED:
    dest->last = dwnx_put_uvarint(dest->last, fr->data_blocked.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->data_blocked.offset);

    return;
  case DWNX_FRAME_STREAM_DATA_BLOCKED:
    dest->last = dwnx_put_uvarint(dest->last, fr->stream_data_blocked.type);
    dest->last =
      dwnx_put_uvarint(dest->last, (uint64_t)fr->stream_data_blocked.stream_id);
    dest->last = dwnx_put_uvarint(dest->last, fr->stream_data_blocked.offset);

    return;
  case DWNX_FRAME_STREAMS_BLOCKED_BIDI:
  case DWNX_FRAME_STREAMS_BLOCKED_UNI:
    dest->last = dwnx_put_uvarint(dest->last, fr->streams_blocked.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->streams_blocked.max_streams);

    return;
  case DWNX_FRAME_CONNECTION_CLOSE:
  case DWNX_FRAME_CONNECTION_CLOSE_APP:
    dest->last = dwnx_put_uvarint(dest->last, fr->connection_close.type);
    dest->last = dwnx_put_uvarint(dest->last, fr->connection_close.error_code);

    if (fr->connection_close.type == DWNX_FRAME_CONNECTION_CLOSE) {
      dest->last =
        dwnx_put_uvarint(dest->last, fr->connection_close.frame_type);
    }

    dest->last = dwnx_put_uvarint(dest->last, fr->connection_close.reasonlen);
    dest->last = dwnx_cpymem(dest->last, fr->connection_close.reason,
                             fr->connection_close.reasonlen);

    return;
  default:
    dwnx_unreachable();
  }
}

void dwnx_write_record(dwnx_buf *dest, const dwnx_frame *fr, size_t n) {
  uint8_t *p = dest->last;

  dest->last += 2;

  for (; n; ++fr, --n) {
    dwnx_write_frame(dest, fr);
  }

  dwnx_put_uvarintw(p, (uint64_t)(dest->last - p - 2), 2);
}

int dwnx_conn_read_transport_params(dwnx_conn *conn,
                                    const dwnx_transport_params *remote_params,
                                    dwnx_tstamp ts) {
  uint8_t rawbuf[16384];
  dwnx_buf buf;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_write_record(&buf,
                    &(dwnx_frame){
                      .qx_transport_parameters =
                        (dwnx_frame_qx_transport_parameters){
                          .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
                          .params = remote_params,
                        },
                    },
                    1);

  return dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ts);
}

void dwnx_read_transport_params(dwnx_conn *conn,
                                const dwnx_transport_params *remote_params,
                                dwnx_tstamp ts) {
  int rv;

  rv = dwnx_conn_read_transport_params(conn, remote_params, ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
}

const uint8_t *dwnx_read_recordlen(uint64_t *plen, const uint8_t *data,
                                   size_t datalen) {
  size_t n;

  assert_size(0, !=, datalen);

  n = dwnx_get_uvarintlen(data);

  assert_size(datalen, >=, n);

  return dwnx_get_uvarint(plen, data);
}

int64_t dwnx_nth_local_bidi_stream_id(dwnx_conn *conn, uint64_t n) {
  assert(n != 0);

  return (int64_t)(((n - 1) << 2) | (conn->server ? 0x1 : 0x0));
}

int64_t dwnx_nth_remote_bidi_stream_id(dwnx_conn *conn, uint64_t n) {
  assert(n != 0);

  return (int64_t)(((n - 1) << 2) | (conn->server ? 0x0 : 0x1));
}

int64_t dwnx_nth_local_uni_stream_id(dwnx_conn *conn, uint64_t n) {
  assert(n != 0);

  return (int64_t)(((n - 1) << 2) | 0x2 | (conn->server ? 0x1 : 0x0));
}

int64_t dwnx_nth_remote_uni_stream_id(dwnx_conn *conn, uint64_t n) {
  assert(n != 0);

  return (int64_t)(((n - 1) << 2) | 0x2 | (conn->server ? 0x0 : 0x1));
}
