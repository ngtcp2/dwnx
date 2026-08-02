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
#include "dwnx_conn_test.h"

#include <stdio.h>

#include "dwnx_conn.h"
#include "dwnx_conv.h"
#include "dwnx_transport_params.h"
#include "dwnx_test_helper.h"

static const MunitTest tests[] = {
  munit_void_test(test_dwnx_conn_recv_transport_params),
  munit_void_test(test_dwnx_conn_recv_stream),
  munit_test_end(),
};

const MunitSuite conn_suite = {
  .prefix = "/conn",
  .tests = tests,
};

typedef struct conn_options {
  const dwnx_transport_params *params;
  const dwnx_mem *mem;
  void *user_data;
} conn_options;

static void setup_default_server_with_options(dwnx_conn **pconn,
                                              conn_options opts) {
  dwnx_transport_params params;
  int rv;

  if (!opts.params) {
    dwnx_transport_params_default(&params);
    opts.params = &params;
  }

  rv = dwnx_conn_server_new(pconn, opts.params, opts.mem, opts.user_data);

  assert_int(0, ==, rv);
}

static void setup_default_server(dwnx_conn **pconn) {
  setup_default_server_with_options(pconn, (conn_options){0});
}

void test_dwnx_conn_recv_transport_params(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  int rv;
  size_t i;

  fr.qx_transport_parameters = (dwnx_frame_qx_transport_parameters){
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params =
      &(dwnx_transport_params){
        .initial_max_stream_data_bidi_local = 1000000007,
        .initial_max_stream_data_bidi_remote = 961748941,
        .initial_max_stream_data_uni = 982451653,
        .initial_max_data = 1000000009,
        .initial_max_streams_bidi = 908,
        .initial_max_streams_uni = 16383,
        .max_idle_timeout = 16363 * DWNX_MILLISECONDS,
        .max_record_size = 5983223322,
      },
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_write_record(&buf, &fr, 1);

  setup_default_server(&conn);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_uint64(
    1000000007, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_local);
  assert_uint64(
    961748941, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_remote);
  assert_uint64(982451653, ==,
                conn->remote.transport_params.initial_max_stream_data_uni);
  assert_uint64(1000000009, ==, conn->remote.transport_params.initial_max_data);
  assert_uint64(908, ==,
                conn->remote.transport_params.initial_max_streams_bidi);
  assert_uint64(16383, ==,
                conn->remote.transport_params.initial_max_streams_uni);
  assert_uint64(16363 * DWNX_MILLISECONDS, ==,
                conn->remote.transport_params.max_idle_timeout);
  assert_uint64(5983223322, ==, conn->remote.transport_params.max_record_size);

  /* Receiving QX_TRANSPORT_PARAMETERS frame second time is treated as
     error */
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Check state transition */
  setup_default_server(&conn);

  for (i = 0; i < dwnx_buf_len(&buf); ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_uint64(
    1000000007, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_local);
  assert_uint64(
    961748941, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_remote);
  assert_uint64(982451653, ==,
                conn->remote.transport_params.initial_max_stream_data_uni);
  assert_uint64(1000000009, ==, conn->remote.transport_params.initial_max_data);
  assert_uint64(908, ==,
                conn->remote.transport_params.initial_max_streams_bidi);
  assert_uint64(16383, ==,
                conn->remote.transport_params.initial_max_streams_uni);
  assert_uint64(16363 * DWNX_MILLISECONDS, ==,
                conn->remote.transport_params.max_idle_timeout);
  assert_uint64(5983223322, ==, conn->remote.transport_params.max_record_size);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_stream(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  int rv;

  fr[0].qx_transport_parameters = (dwnx_frame_qx_transport_parameters){
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params =
      &(dwnx_transport_params){
        .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
      },
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  /* With all bits set */
  setup_default_server(&conn);

  fr[1].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_write_record(&buf, fr, 2);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* With FIN and OFF bits set */
  setup_default_server(&conn);

  fr[1].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* With FIN bit set */
  setup_default_server(&conn);

  fr[1].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Without bit set */
  setup_default_server(&conn);

  fr[1].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}
