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

#define DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN 0x01U

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

  dwnx_callbacks callbacks;
  dwnx_map strms;
  uint32_t flags;
  int server;
};

int dwnx_conn_recv_transport_params(dwnx_conn *conn, const uint8_t *data,
                                    size_t datalen);

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
 * of queue.  tx_strmq must not be empty.
 */
dwnx_strm *dwnx_conn_tx_strmq_top(dwnx_conn *conn);

/*
 * dwnx_conn_tx_strmq_pop pops the dwnx_strm from the queue.  tx_strmq
 * must not be empty.
 */
void dwnx_conn_tx_strmq_pop(dwnx_conn *conn);

/*
 * dwnx_conn_tx_strmq_push pushes |strm| into tx_strmq.
 *
 *  This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_conn_tx_strmq_push(dwnx_conn *conn, dwnx_strm *strm);

#endif /* !defined(DWNX_CONN_H) */
