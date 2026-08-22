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
#ifndef DWNX_CONN_H
#define DWNX_CONN_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_record_reader.h"
#include "dwnx_map.h"
#include "dwnx_idtr.h"
#include "dwnx_pq.h"
#include "dwnx_qre.h"
#include "dwnx_log.h"

#define DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN 0x01U
#define DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SENT 0x02U

typedef struct dwnx_strm dwnx_strm;

struct dwnx_conn {
  const dwnx_mem *mem;
  void *user_data;

  struct {
    dwnx_transport_params transport_params;
  } local;

  struct {
    dwnx_transport_params transport_params;
  } remote;

  struct {
    dwnx_idtr idtr;
  } bidi;

  struct {
    dwnx_idtr idtr;
  } uni;

  struct {
    dwnx_varint_reader vird;
    dwnx_record_reader rcrd;

    /* unsent_max_offset is the maximum offset that remote endpoint
       can send without extending MAX_DATA.  This limit is not yet
       notified to the remote endpoint. */
    uint64_t unsent_max_offset;
    /* offset is the cumulative sum of stream data received for this
       connection. */
    uint64_t offset;
    /* max_offset is the maximum offset that remote endpoint can
       send. */
    uint64_t max_offset;
    /* window is the connection-level flow control window size. */
    uint64_t window;

    struct {
      /* last_seq is the last sequence number in QX_PING_REQUEST frame
         received so far.  It is initialized as -1. */
      int64_t last_seq;
      /* last_resp_seq is the last sequence number in QX_PING_RESPONSE
         frame sent to the remote endpoint.  It is initialized as -1.
         The invariant last_seq >= last_resp_seq must hold. */
      int64_t last_resp_seq;
    } ping;

    struct {
      /* unsent_max_streams is the maximum number of streams of peer
         initiated bidirectional stream which the local endpoint can
         accept.  This limit is not yet notified to the remote
         endpoint. */
      uint64_t unsent_max_streams;
      /* max_streams is the maximum number of streams of peer
         initiated bidirectional stream which the local endpoint can
         accept. */
      uint64_t max_streams;
    } bidi;

    struct {
      /* unsent_max_streams is the maximum number of streams of peer
         initiated unidirectional stream which the local endpoint can
         accept.  This limit is not yet notified to the remote
         endpoint. */
      uint64_t unsent_max_streams;
      /* max_streams is the maximum number of streams of peer
         initiated unidirectional stream which the local endpoint can
         accept. */
      uint64_t max_streams;
    } uni;
  } rx;

  struct {
    /* strmq contains dwnx_strm which has frames to send. */
    dwnx_pq strmq;
    dwnx_qre qre;
    /* offset is the offset the local endpoint has sent to the remote
       endpoint. */
    uint64_t offset;
    /* max_offset is the maximum offset that local endpoint can
       send. */
    uint64_t max_offset;
    /* last_blocked_offset is the offset that was sent along with
       DATA_BLOCKED frame last time. */
    uint64_t last_blocked_offset;

    struct {
      /* last_seq is the last sequence number in QX_PING_REQUEST frame
         sent so far.  It is initialized as -1. */
      int64_t last_seq;
    } ping;

    struct {
      /* max_streams is the maximum number of bidirectional streams which
         the local endpoint can open. */
      uint64_t max_streams;
      /* next_stream_id is the bidirectional stream ID which the local
         endpoint opens next. */
      int64_t next_stream_id;
    } bidi;

    struct {
      /* max_streams is the maximum number of unidirectional streams
         which the local endpoint can open. */
      uint64_t max_streams;
      /* next_stream_id is the unidirectional stream ID which the
         local endpoint opens next. */
      int64_t next_stream_id;
    } uni;
  } tx;

  dwnx_settings settings;
  dwnx_callbacks callbacks;
  dwnx_map strms;
  dwnx_log log;
  dwnx_tstamp idle_ts;
  uint32_t flags;
  int server;
};

/*
 * dwnx_conn_find_stream returns a stream whose stream ID is
 * |stream_id|.  If no such stream is found, it returns NULL.
 */
dwnx_strm *dwnx_conn_find_stream(const dwnx_conn *conn, int64_t stream_id);

int dwnx_conn_create_stream(dwnx_conn *conn, dwnx_strm **pstrm,
                            int64_t stream_id, void *stream_user_data);

int dwnx_conn_close_stream_if_shut_rdwr(dwnx_conn *conn, dwnx_strm *strm);

int dwnx_conn_close_stream(dwnx_conn *conn, dwnx_strm *strm);

uint64_t dwnx_conn_tx_strmq_first_cycle(const dwnx_conn *conn);

/*
 * dwnx_conn_tx_strmq_top returns the dwnx_strm which sits on the top
 * of queue.  |conn|->tx.strmq must not be empty.
 */
dwnx_strm *dwnx_conn_tx_strmq_top(dwnx_conn *conn);

/*
 * dwnx_conn_tx_strmq_pop pops the dwnx_strm from the queue.
 * |conn|->tx.strmq must not be empty.
 */
void dwnx_conn_tx_strmq_pop(dwnx_conn *conn);

/*
 * dwnx_conn_tx_strmq_push pushes |strm| into |conn|->tx.strmq.
 * Caller should set |strm|->cycle before calling this function.
 *
 *  This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_conn_tx_strmq_push(dwnx_conn *conn, dwnx_strm *strm);

/*
 * dwnx_conn_tx_strmq_push_if_not pushes |strm| into |conn|->tx.strmq
 * if it is not pushed yet.  See dwnx_conn_tx_strmq_push for
 * preconditions.
 *
 *  This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_conn_tx_strmq_push_if_not(dwnx_conn *conn, dwnx_strm *strm);

typedef enum dwnx_vmsg_type {
  DWNX_VMSG_TYPE_STREAM,
} dwnx_vmsg_type;

typedef struct dwnx_vmsg_stream {
  /* strm is a stream that data is sent to. */
  dwnx_strm *strm;
  /* data is the pointer to dwnx_vec array which contains the stream
     data to send. */
  const dwnx_vec *data;
  /* datacnt is the number of dwnx_vec pointed by data. */
  size_t datacnt;
  /* pdatalen is the pointer to the variable which the number of bytes
     written is assigned to if pdatalen is not NULL. */
  dwnx_ssize *pdatalen;
  /* flags is bitwise OR of zero or more of
     DWNX_WRITE_STREAM_FLAG_*. */
  uint32_t flags;
} dwnx_vmsg_stream;

typedef struct dwnx_vmsg {
  dwnx_vmsg_type type;
  union {
    dwnx_vmsg_stream stream;
  };
} dwnx_vmsg;

dwnx_ssize dwnx_conn_write_vmsg(dwnx_conn *conn, uint8_t *dest, size_t destlen,
                                dwnx_vmsg *vmsg, dwnx_tstamp ts);

int dwnx_conn_write_transport_params(dwnx_conn *conn, dwnx_tstamp ts);

int dwnx_conn_write_ctrl_frames(dwnx_conn *conn, dwnx_tstamp ts);

int dwnx_conn_write_data_blocked(dwnx_conn *conn, dwnx_tstamp ts);

int dwnx_conn_write_stream_data_blocked(dwnx_conn *conn, dwnx_strm *strm,
                                        dwnx_tstamp ts);

int dwnx_conn_write_max_streams(dwnx_conn *conn, dwnx_tstamp ts);

int dwnx_conn_write_stream_frame(dwnx_conn *conn, dwnx_ssize *pdatalen,
                                 dwnx_strm *strm, uint32_t flags,
                                 const dwnx_vec *datav, size_t datavcnt,
                                 dwnx_tstamp ts);

dwnx_tstamp dwnx_conn_get_idle_expiry(const dwnx_conn *conn);

#endif /* !defined(DWNX_CONN_H) */
