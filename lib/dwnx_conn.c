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
#include "dwnx_conn.h"

#include <assert.h>

#include "dwnx_mem.h"
#include "dwnx_vec.h"
#include "dwnx_str.h"
#include "dwnx_transport_params.h"
#include "dwnx_unreachable.h"
#include "dwnx_strm.h"
#include "dwnx_conv.h"

/*
 * conn_local_stream returns nonzero if |stream_id| indicates that it
 * is the stream initiated by local endpoint.
 */
static int conn_local_stream(const dwnx_conn *conn, int64_t stream_id) {
  return (uint8_t)(stream_id & 1) == conn->server;
}

/*
 * bidi_stream returns nonzero if |stream_id| is a bidirectional
 * stream ID.
 */
static int bidi_stream(int64_t stream_id) { return (stream_id & 0x2) == 0; }

static int conn_call_stream_open(dwnx_conn *conn, dwnx_strm *strm) {
  int rv;

  if (!conn->callbacks.stream_open) {
    return 0;
  }

  rv = conn->callbacks.stream_open(conn, strm->stream_id, conn->user_data);
  if (rv != 0) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}

static int conn_call_recv_stream_data(dwnx_conn *conn, dwnx_strm *strm,
                                      uint32_t flags, uint64_t offset,
                                      const uint8_t *data, size_t datalen) {
  int rv;

  if (!conn->callbacks.recv_stream_data) {
    return 0;
  }

  rv = conn->callbacks.recv_stream_data(conn, flags, strm->stream_id, offset,
                                        data, datalen, conn->user_data,
                                        strm->stream_user_data);
  if (rv != 0) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}

static int conn_new(dwnx_conn **pconn, const dwnx_callbacks *callbacks,
                    const dwnx_transport_params *params, const dwnx_mem *mem,
                    void *user_data) {
  dwnx_conn *conn;

  assert(params);
  assert(params->initial_max_stream_data_bidi_local <= DWNX_MAX_VARINT);
  assert(params->initial_max_stream_data_bidi_remote <= DWNX_MAX_VARINT);
  assert(params->initial_max_data <= DWNX_MAX_VARINT);
  assert(params->initial_max_streams_bidi <= DWNX_MAX_VARINT);
  assert(params->initial_max_streams_uni <= DWNX_MAX_VARINT);
  assert(params->max_idle_timeout != UINT64_MAX);
  assert(params->max_idle_timeout / DWNX_MILLISECONDS <= DWNX_MAX_VARINT);
  assert(params->max_record_size <= DWNX_MAX_VARINT);
  assert(params->max_record_size >= DWNX_DEFAULT_MAX_RECORD_SIZE);

  if (!mem) {
    mem = dwnx_mem_default();
  }

  conn = dwnx_mem_calloc(mem, 1, sizeof(*conn));
  if (!conn) {
    return DWNX_ERR_NOBUF;
  }

  conn->mem = mem;
  conn->callbacks = *callbacks;
  conn->local.transport_params = *params;
  /* We do not let application increase max record size. */
  conn->local.transport_params.max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE;
  params = &conn->local.transport_params;

  dwnx_transport_params_default(&conn->remote.transport_params);

  conn->user_data = user_data;

  /* TODO: Specify seed */
  dwnx_map_init(&conn->strms, /* seed = */ 0, mem);
  dwnx_idtr_init(&conn->bidi.idtr, mem);
  dwnx_idtr_init(&conn->uni.idtr, mem);

  /* Apply flow control limits */
  conn->rx.window = conn->rx.unsent_max_offset = conn->rx.max_offset =
    params->initial_max_data;
  conn->rx.bidi.unsent_max_streams = params->initial_max_streams_bidi;
  conn->rx.bidi.max_streams = params->initial_max_streams_bidi;
  conn->rx.uni.unsent_max_streams = params->initial_max_streams_uni;
  conn->rx.uni.max_streams = params->initial_max_streams_uni;

  *pconn = conn;

  return 0;
}

int dwnx_conn_server_new(dwnx_conn **pconn, const dwnx_callbacks *callbacks,
                         const dwnx_transport_params *params,
                         const dwnx_mem *mem, void *user_data) {
  int rv;

  rv = conn_new(pconn, callbacks, params, mem, user_data);
  if (rv != 0) {
    return rv;
  }

  (*pconn)->server = 1;

  return 0;
}

int dwnx_conn_client_new(dwnx_conn **pconn, const dwnx_callbacks *callbacks,
                         const dwnx_transport_params *params,
                         const dwnx_mem *mem, void *user_data) {
  return conn_new(pconn, callbacks, params, mem, user_data);
}

static int delete_strm(void *data, void *ptr) {
  dwnx_strm *strm = data;
  dwnx_conn *conn = ptr;

  dwnx_strm_free(strm);
  dwnx_mem_free(conn->mem, strm);

  return 0;
}

void dwnx_conn_del(dwnx_conn *conn) {
  const dwnx_mem *mem;

  if (!conn) {
    return;
  }

  mem = conn->mem;

  dwnx_idtr_free(&conn->uni.idtr);
  dwnx_idtr_free(&conn->bidi.idtr);
  dwnx_map_each(&conn->strms, delete_strm, conn);
  dwnx_map_free(&conn->strms);
  dwnx_mem_free(mem, conn);
}

int dwnx_conn_read(dwnx_conn *conn, const uint8_t *data, size_t datalen,
                   dwnx_tstamp ts) {
  const uint8_t *p, *end;
  const dwnx_mem *mem = conn->mem;
  dwnx_varint_reader *vird = &conn->rx.vird;
  dwnx_record_reader *rcrd = &conn->rx.rcrd;
  dwnx_ssize nread;
  dwnx_strm *strm;
  int rv;
  uint8_t *buf;
  size_t len;
  uint64_t vint;
  (void)ts;

  if (datalen == 0) {
    return 0;
  }

  p = data;
  end = p + datalen;

  for (; p != end;) {
    switch (rcrd->state) {
    case DWNX_RECORD_READ_STATE_RECORD_SIZE:
      nread =
        dwnx_varint_reader_read(vird, p, (size_t)(end - p), /* fin = */ 0);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      vint = vird->acc;
      dwnx_varint_reader_reset(vird);

      if (vint > conn->local.transport_params.max_record_size ||
          vint == 0 /* This is most likely a bug of the remote
                            endpoint. */) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      rcrd->record_left = vint;

      rcrd->state = DWNX_RECORD_READ_STATE_FRAME_TYPE;

      if (p == end) {
        return 0;
      }

      /* Fall through */
    case DWNX_RECORD_READ_STATE_FRAME_TYPE:
      len = dwnx_record_reader_avail(rcrd, (size_t)(end - p));
      nread = dwnx_varint_reader_read(vird, p, len, rcrd->record_left == len);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      rcrd->record_left -= (size_t)nread;

      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      vint = vird->acc;
      dwnx_varint_reader_reset(vird);

      if (!(conn->flags & DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN) &&
          vint != DWNX_FRAME_QX_TRANSPORT_PARAMETERS) {
        return DWNX_ERR_PROTO;
      }

      switch (vint) {
      case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
        if (conn->flags & DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN) {
          return DWNX_ERR_PROTO;
        }

        conn->flags |= DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN;

        if (rcrd->record_left == 0) {
          return DWNX_ERR_FRAME_ENCODING;
        }

        rcrd->fr.qx_transport_parameters.type = vint;
        rcrd->state = DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_LEN;

        break;
      default:
        if ((vint & ~(DWNX_FRAME_STREAM - 1)) == DWNX_FRAME_STREAM) {
          if (rcrd->record_left == 0) {
            return DWNX_ERR_FRAME_ENCODING;
          }

          rcrd->fr.stream.type = DWNX_FRAME_STREAM;
          rcrd->fr.stream.flags = (uint8_t)(vint & 0x7U);
          rcrd->fr.stream.fin =
            (rcrd->fr.stream.flags & DWNX_STREAM_FIN_BIT) != 0;

          rcrd->state = DWNX_RECORD_READ_STATE_STREAM_STREAM_ID;

          break;
        }

        return DWNX_ERR_FRAME_ENCODING;
      }

      break;
    case DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_LEN:
      len = dwnx_record_reader_avail(rcrd, (size_t)(end - p));
      nread = dwnx_varint_reader_read(vird, p, len, rcrd->record_left == len);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      rcrd->record_left -= (size_t)nread;

      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      vint = vird->acc;
      dwnx_varint_reader_reset(vird);

      if (vint == 0) {
        rv = dwnx_conn_recv_transport_params(conn, NULL, 0);
        if (rv != 0) {
          return rv;
        }

        goto frame_done;
      }

      /* Because our max record size is constant (we do not allow
         application to change it), the size is smaller than
         DWNX_DEFAULT_MAX_RECORD_SIZE. */
      if (vint > rcrd->record_left) {
        return DWNX_ERR_INTERNAL;
      }

      if (p + vint <= end) {
        rv = dwnx_conn_recv_transport_params(conn, p, vint);
        if (rv != 0) {
          return rv;
        }

        p += vint;
        rcrd->record_left -= vint;

        goto frame_done;
      }

      buf = dwnx_mem_malloc(mem, vint);
      if (!buf) {
        return DWNX_ERR_NOMEM;
      }

      dwnx_buf_init(&rcrd->buf, buf, vint);

      rcrd->state = DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_PARAMS;

      if (p == end) {
        return 0;
      }

      /* Fall through */
    case DWNX_RECORD_READ_STATE_QX_TRANSPORT_PARAMETERS_PARAMS:
      len = dwnx_record_reader_buf_avail(rcrd, (size_t)(end - p));
      rcrd->buf.last = dwnx_cpymem(rcrd->buf.last, p, len);

      p += len;
      rcrd->record_left -= len;

      if (dwnx_buf_left(&rcrd->buf)) {
        return 0;
      }

      rv = dwnx_conn_recv_transport_params(conn, rcrd->buf.pos,
                                           dwnx_buf_len(&rcrd->buf));
      if (rv != 0) {
        return rv;
      }

      goto frame_done;
    case DWNX_RECORD_READ_STATE_STREAM_STREAM_ID:
      len = dwnx_record_reader_avail(rcrd, (size_t)(end - p));
      nread = dwnx_varint_reader_read(vird, p, len, rcrd->record_left == len);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      rcrd->record_left -= (size_t)nread;

      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      rcrd->fr.stream.stream_id = (int64_t)vird->acc;
      dwnx_varint_reader_reset(vird);

      if (rcrd->fr.stream.flags & DWNX_STREAM_OFF_BIT) {
        if (rcrd->record_left == 0) {
          return DWNX_ERR_FRAME_ENCODING;
        }

        rcrd->state = DWNX_RECORD_READ_STATE_STREAM_OFFSET;
      } else if (rcrd->fr.stream.flags & DWNX_STREAM_LEN_BIT) {
        if (rcrd->record_left == 0) {
          return DWNX_ERR_FRAME_ENCODING;
        }

        rcrd->state = DWNX_RECORD_READ_STATE_STREAM_LENGTH;

        break;
      } else {
        rcrd->fr.stream.len = rcrd->record_left;
        rcrd->field_left = rcrd->fr.stream.len;

        rv = dwnx_conn_recv_stream(conn, &rcrd->fr.stream, ts);
        if (rv != 0) {
          return rv;
        }

        if (rcrd->record_left == 0) {
          goto frame_done;
        }

        rcrd->state = DWNX_RECORD_READ_STATE_STREAM_DATA;

        break;
      }

      if (p == end) {
        return 0;
      }

      /* Fall through */
    case DWNX_RECORD_READ_STATE_STREAM_OFFSET:
      len = dwnx_record_reader_avail(rcrd, (size_t)(end - p));
      nread = dwnx_varint_reader_read(vird, p, len, rcrd->record_left == len);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      rcrd->record_left -= (size_t)nread;

      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      rcrd->fr.stream.offset = vird->acc;
      dwnx_varint_reader_reset(vird);

      if (rcrd->fr.stream.flags & DWNX_STREAM_LEN_BIT) {
        if (rcrd->record_left == 0) {
          return DWNX_ERR_FRAME_ENCODING;
        }

        rcrd->state = DWNX_RECORD_READ_STATE_STREAM_LENGTH;
      } else {
        rcrd->fr.stream.len = rcrd->record_left;
        rcrd->field_left = rcrd->fr.stream.len;

        rv = dwnx_conn_recv_stream(conn, &rcrd->fr.stream, ts);
        if (rv != 0) {
          return rv;
        }

        if (rcrd->record_left == 0) {
          goto frame_done;
        }

        rcrd->state = DWNX_RECORD_READ_STATE_STREAM_DATA;

        break;
      }

      if (p == end) {
        return 0;
      }

      /* Fall through */
    case DWNX_RECORD_READ_STATE_STREAM_LENGTH:
      len = dwnx_record_reader_avail(rcrd, (size_t)(end - p));
      nread = dwnx_varint_reader_read(vird, p, len, rcrd->record_left == len);
      if (nread < 0) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      p += nread;
      rcrd->record_left -= (size_t)nread;

      if (!dwnx_varint_reader_done(vird)) {
        return 0;
      }

      rcrd->fr.stream.len = vird->acc;
      dwnx_varint_reader_reset(vird);

      if (rcrd->fr.stream.len > rcrd->record_left) {
        return DWNX_ERR_FRAME_ENCODING;
      }

      rcrd->field_left = rcrd->fr.stream.len;

      rv = dwnx_conn_recv_stream(conn, &rcrd->fr.stream, ts);
      if (rv != 0) {
        return rv;
      }

      if (rcrd->record_left == 0) {
        goto frame_done;
      }

      rcrd->state = DWNX_RECORD_READ_STATE_STREAM_DATA;

      if (p == end) {
        return 0;
      }

      /* Fall through */
    case DWNX_RECORD_READ_STATE_STREAM_DATA:
      strm = dwnx_conn_find_stream(conn, rcrd->fr.stream.stream_id);

      assert(strm);

      len = dwnx_record_reader_field_avail(rcrd, (size_t)(end - p));

      rv = conn_call_recv_stream_data(
        conn, strm,
        (rcrd->fr.stream.fin && rcrd->field_left == len)
          ? DWNX_STREAM_DATA_FLAG_FIN
          : DWNX_STREAM_DATA_FLAG_NONE,
        strm->rx.last_offset - rcrd->field_left, p, len);
      if (rv != 0) {
        return rv;
      }

      p += len;
      rcrd->record_left -= len;
      rcrd->field_left -= len;

      if (rcrd->field_left) {
        return 0;
      }

      goto frame_done;
    default:
      dwnx_unreachable();
    }

    continue;

  frame_done:
    if (rcrd->record_left) {
      dwnx_record_reader_next_frame(rcrd, mem);
    } else {
      dwnx_record_reader_reset(rcrd, mem);
    }
  }

  return 0;
}

int dwnx_conn_recv_transport_params(dwnx_conn *conn, const uint8_t *data,
                                    size_t datalen) {
  return dwnx_transport_params_decode(&conn->remote.transport_params, data,
                                      datalen);
}

/*
 * conn_max_data_violated returns nonzero if receiving |datalen|
 * violates connection flow control on local endpoint.
 */
static int conn_max_data_violated(const dwnx_conn *conn, uint64_t datalen) {
  return conn->rx.max_offset - conn->rx.offset < datalen;
}

int dwnx_conn_recv_stream(dwnx_conn *conn, const dwnx_frame_stream *fr,
                          dwnx_tstamp ts) {
  dwnx_idtr *idtr;
  dwnx_strm *strm;
  uint64_t fr_end_offset;
  int local_stream;
  int bidi;
  int rv;
  (void)ts;

  local_stream = conn_local_stream(conn, fr->stream_id);
  bidi = bidi_stream(fr->stream_id);

  if (bidi) {
    if (local_stream) {
      if (conn->tx.bidi.next_stream_id <= fr->stream_id) {
        return DWNX_ERR_STREAM_STATE;
      }
    } else if (conn->rx.bidi.max_streams < dwnx_ord_stream_id(fr->stream_id)) {
      return DWNX_ERR_STREAM_LIMIT;
    }

    idtr = &conn->bidi.idtr;
  } else {
    if (local_stream) {
      return DWNX_ERR_STREAM_STATE;
    }
    if (conn->rx.uni.max_streams < dwnx_ord_stream_id(fr->stream_id)) {
      return DWNX_ERR_STREAM_LIMIT;
    }

    idtr = &conn->uni.idtr;
  }

  if (DWNX_MAX_VARINT - fr->len < fr->offset) {
    return DWNX_ERR_FLOW_CONTROL;
  }

  strm = dwnx_conn_find_stream(conn, fr->stream_id);
  if (strm == NULL) {
    if (local_stream) {
      /* The stream has been closed. */
      return DWNX_ERR_PROTO;
    }

    rv = dwnx_idtr_open(idtr, fr->stream_id);
    if (rv != 0) {
      if (dwnx_err_is_fatal(rv)) {
        return rv;
      }

      assert(rv == DWNX_ERR_STREAM_IN_USE);

      return DWNX_ERR_PROTO;
    }

    rv = dwnx_conn_create_stream(conn, &strm, fr->stream_id, NULL);
    if (rv != 0) {
      return rv;
    }

    rv = conn_call_stream_open(conn, strm);
    if (rv != 0) {
      return rv;
    }
  }

  if (strm->flags & DWNX_STRM_FLAG_SHUT_RD) {
    return DWNX_ERR_PROTO;
  }

  if (strm->rx.last_offset != fr->offset) {
    return DWNX_ERR_PROTO;
  }

  fr_end_offset = fr->offset + fr->len;

  if (strm->rx.max_offset < fr_end_offset ||
      conn_max_data_violated(conn, fr->len)) {
    return DWNX_ERR_FLOW_CONTROL;
  }

  conn->rx.offset += fr->len;

  strm->rx.last_offset = fr_end_offset;

  if (fr->fin) {
    dwnx_strm_shutdown(strm, DWNX_STRM_FLAG_SHUT_RD);

    if (fr->len == 0) {
      rv = conn_call_recv_stream_data(conn, strm, DWNX_STREAM_DATA_FLAG_FIN,
                                      fr->offset, NULL, 0);
      if (rv != 0) {
        return rv;
      }
    }

    /* TODO: Delete stream if both ends are closed. */
  }

  return 0;
}

dwnx_strm *dwnx_conn_find_stream(const dwnx_conn *conn, int64_t stream_id) {
  return dwnx_map_find(&conn->strms, (uint64_t)stream_id);
}

int dwnx_conn_create_stream(dwnx_conn *conn, dwnx_strm **pstrm,
                            int64_t stream_id, void *stream_user_data) {
  dwnx_strm *strm;
  int rv;
  uint64_t max_rx_offset;
  uint64_t max_tx_offset;
  int local_stream = conn_local_stream(conn, stream_id);
  int bidi = bidi_stream(stream_id);

  strm = dwnx_mem_malloc(conn->mem, sizeof(*strm));
  if (!strm) {
    return DWNX_ERR_NOMEM;
  }

  if (bidi_stream(stream_id)) {
    if (local_stream) {
      max_rx_offset =
        conn->local.transport_params.initial_max_stream_data_bidi_local;
      max_tx_offset =
        conn->remote.transport_params.initial_max_stream_data_bidi_remote;
    } else {
      max_rx_offset =
        conn->local.transport_params.initial_max_stream_data_bidi_remote;
      max_tx_offset =
        conn->remote.transport_params.initial_max_stream_data_bidi_local;
    }
  } else if (local_stream) {
    max_rx_offset = 0;
    max_tx_offset = conn->remote.transport_params.initial_max_stream_data_uni;
  } else {
    max_rx_offset = conn->local.transport_params.initial_max_stream_data_uni;
    max_tx_offset = 0;
  }

  dwnx_strm_init(strm, stream_id, DWNX_STRM_FLAG_NONE, max_rx_offset,
                 max_tx_offset, stream_user_data, conn->mem);

  if (!bidi) {
    if (local_stream) {
      dwnx_strm_shutdown(strm, DWNX_STRM_FLAG_SHUT_RD);
    } else {
      dwnx_strm_shutdown(strm, DWNX_STRM_FLAG_SHUT_WR);
    }
  }

  rv = dwnx_map_insert(&conn->strms, (dwnx_map_key_type)strm->stream_id, strm);
  if (rv != 0) {
    assert(rv != DWNX_ERR_INVALID_ARGUMENT);
    goto fail;
  }

  *pstrm = strm;

  return 0;

fail:
  dwnx_strm_free(strm);
  dwnx_mem_free(conn->mem, strm);

  return rv;
}
