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
#include "dwnx_strm.h"
#include "dwnx_test_helper.h"

static const MunitTest tests[] = {
  munit_void_test(test_dwnx_conn_recv_transport_params),
  munit_void_test(test_dwnx_conn_recv_stream),
  munit_void_test(test_dwnx_conn_recv_reset_stream),
  munit_void_test(test_dwnx_conn_recv_stop_sending),
  munit_void_test(test_dwnx_conn_recv_max_data),
  munit_void_test(test_dwnx_conn_recv_max_stream_data),
  munit_void_test(test_dwnx_conn_recv_max_streams_bidi),
  munit_void_test(test_dwnx_conn_recv_max_streams_uni),
  munit_void_test(test_dwnx_conn_recv_data_blocked),
  munit_void_test(test_dwnx_conn_recv_stream_data_blocked),
  munit_void_test(test_dwnx_conn_recv_streams_blocked_bidi),
  munit_void_test(test_dwnx_conn_recv_streams_blocked_uni),
  munit_void_test(test_dwnx_conn_recv_connection_close),
  munit_void_test(test_dwnx_conn_recv_connection_close_app),
  munit_void_test(test_dwnx_conn_recv_padding),
  munit_void_test(test_dwnx_conn_recv_qx_ping),
  munit_void_test(test_dwnx_conn_extend_max_stream_offset),
  munit_void_test(test_dwnx_conn_writev_stream),
  munit_test_end(),
};

const MunitSuite conn_suite = {
  .prefix = "/conn",
  .tests = tests,
};

static const uint8_t nulldata[1 << 20];

typedef struct userdata {
  struct {
    size_t ncalled;
    int64_t stream_id;
    uint64_t offset;
    size_t datalen;
    uint32_t flags;
  } recv_stream_data;
  struct {
    size_t ncalled;
    int64_t stream_id;
  } stream_open;
  struct {
    size_t ncalled;
    int64_t stream_id;
    uint64_t final_size;
    uint64_t app_error_code;
  } stream_reset;
  struct {
    size_t ncalled;
    int64_t stream_id;
    uint64_t app_error_code;
  } recv_stop_sending;
  struct {
    size_t ncalled;
    int64_t stream_id;
    uint64_t max_data;
  } extend_max_stream_data;
  struct {
    size_t ncalled;
    uint64_t max_streams;
  } extend_max_local_streams_bidi;
  struct {
    size_t ncalled;
    uint64_t max_streams;
  } extend_max_local_streams_uni;
} userdata;

typedef struct conn_options {
  const dwnx_callbacks *callbacks;
  const dwnx_transport_params *params;
  const dwnx_mem *mem;
  void *user_data;
} conn_options;

static const dwnx_frame_qx_transport_parameters empty_params_fr = {
  .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
  .params =
    &(dwnx_transport_params){
      .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
    },
};

static int recv_stream_data(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                            uint64_t offset, const uint8_t *data,
                            size_t datalen, void *user_data,
                            void *stream_user_data) {
  userdata *ud = user_data;
  (void)conn;
  (void)data;
  (void)stream_user_data;

  ++ud->recv_stream_data.ncalled;
  ud->recv_stream_data.stream_id = stream_id;
  ud->recv_stream_data.offset = offset;
  ud->recv_stream_data.datalen = datalen;
  ud->recv_stream_data.flags = flags;

  return 0;
}

static int stream_open(dwnx_conn *conn, int64_t stream_id, void *user_data) {
  userdata *ud = user_data;
  (void)conn;

  ++ud->stream_open.ncalled;
  ud->stream_open.stream_id = stream_id;

  return 0;
}

static int stream_reset(dwnx_conn *conn, int64_t stream_id, uint64_t final_size,
                        uint64_t app_error_code, void *user_data,
                        void *stream_user_data) {
  userdata *ud = user_data;
  (void)conn;
  (void)stream_user_data;

  ++ud->stream_reset.ncalled;
  ud->stream_reset.stream_id = stream_id;
  ud->stream_reset.final_size = final_size;
  ud->stream_reset.app_error_code = app_error_code;

  return 0;
}

static int recv_stop_sending(dwnx_conn *conn, int64_t stream_id,
                             uint64_t app_error_code, void *user_data,
                             void *stream_user_data) {
  userdata *ud = user_data;
  (void)conn;
  (void)stream_user_data;

  ++ud->recv_stop_sending.ncalled;
  ud->recv_stop_sending.stream_id = stream_id;
  ud->recv_stop_sending.app_error_code = app_error_code;

  return 0;
}

static int extend_max_stream_data(dwnx_conn *conn, int64_t stream_id,
                                  uint64_t max_data, void *user_data,
                                  void *stream_user_data) {
  userdata *ud = user_data;
  (void)conn;
  (void)stream_user_data;

  ++ud->extend_max_stream_data.ncalled;
  ud->extend_max_stream_data.stream_id = stream_id;
  ud->extend_max_stream_data.max_data = max_data;

  return 0;
}

static int extend_max_local_streams_bidi(dwnx_conn *conn, uint64_t max_streams,
                                         void *user_data) {
  userdata *ud = user_data;
  (void)conn;

  ++ud->extend_max_local_streams_bidi.ncalled;
  ud->extend_max_local_streams_bidi.max_streams = max_streams;

  return 0;
}

static int extend_max_local_streams_uni(dwnx_conn *conn, uint64_t max_streams,
                                        void *user_data) {
  userdata *ud = user_data;
  (void)conn;

  ++ud->extend_max_local_streams_uni.ncalled;
  ud->extend_max_local_streams_uni.max_streams = max_streams;

  return 0;
}

static void server_default_transport_params(dwnx_transport_params *params) {
  dwnx_transport_params_default(params);
  params->initial_max_streams_bidi = 10;
  params->initial_max_streams_uni = 10;
  params->initial_max_stream_data_bidi_local = 64 * 1024;
  params->initial_max_stream_data_bidi_remote = 64 * 1024;
  params->initial_max_stream_data_uni = 64 * 1024;
  params->initial_max_data = 128 * 1024;
}

static void setup_default_server_with_options(dwnx_conn **pconn,
                                              conn_options opts) {
  dwnx_callbacks callbacks = {0};
  dwnx_transport_params params;
  int rv;

  if (!opts.callbacks) {
    opts.callbacks = &callbacks;
  }

  if (!opts.params) {
    server_default_transport_params(&params);
    opts.params = &params;
  }

  rv = dwnx_conn_server_new(pconn, opts.callbacks, opts.params, opts.mem,
                            opts.user_data);

  assert_int(0, ==, rv);
}

static void setup_default_server(dwnx_conn **pconn) {
  setup_default_server_with_options(pconn, (conn_options){0});
}

static void client_default_transport_params(dwnx_transport_params *params) {
  dwnx_transport_params_default(params);
  params->initial_max_streams_bidi = 10;
  params->initial_max_streams_uni = 10;
  params->initial_max_stream_data_bidi_local = 64 * 1024;
  params->initial_max_stream_data_bidi_remote = 64 * 1024;
  params->initial_max_stream_data_uni = 64 * 1024;
  params->initial_max_data = 128 * 1024;
}

static void setup_default_client_with_options(dwnx_conn **pconn,
                                              conn_options opts) {
  dwnx_callbacks callbacks = {0};
  dwnx_transport_params params;
  int rv;

  if (!opts.callbacks) {
    opts.callbacks = &callbacks;
  }

  if (!opts.params) {
    client_default_transport_params(&params);
    opts.params = &params;
  }

  rv = dwnx_conn_client_new(pconn, opts.callbacks, opts.params, opts.mem,
                            opts.user_data);

  assert_int(0, ==, rv);
}

static void setup_default_client(dwnx_conn **pconn) {
  setup_default_client_with_options(pconn, (conn_options){0});
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
  static const dwnx_callbacks callbacks = {
    .stream_open = stream_open,
    .recv_stream_data = recv_stream_data,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  int rv;
  userdata ud;
  conn_options opts;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  /* With all bits set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(0, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.recv_stream_data.ncalled);
  assert_int64(0, ==, ud.recv_stream_data.stream_id);
  assert_uint64(0, ==, ud.recv_stream_data.offset);
  assert_size(100, ==, ud.recv_stream_data.datalen);
  assert_uint32(DWNX_STREAM_DATA_FLAG_FIN, ==, ud.recv_stream_data.flags);

  dwnx_conn_del(conn);

  /* With FIN and OFF bits set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(0, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.recv_stream_data.ncalled);
  assert_int64(0, ==, ud.recv_stream_data.stream_id);
  assert_uint64(0, ==, ud.recv_stream_data.offset);
  assert_size(100, ==, ud.recv_stream_data.datalen);
  assert_uint32(DWNX_STREAM_DATA_FLAG_FIN, ==, ud.recv_stream_data.flags);

  dwnx_conn_del(conn);

  /* With FIN bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(0, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.recv_stream_data.ncalled);
  assert_int64(0, ==, ud.recv_stream_data.stream_id);
  assert_uint64(0, ==, ud.recv_stream_data.offset);
  assert_size(100, ==, ud.recv_stream_data.datalen);
  assert_uint32(DWNX_STREAM_DATA_FLAG_FIN, ==, ud.recv_stream_data.flags);

  dwnx_conn_del(conn);

  /* Without bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(0, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.recv_stream_data.ncalled);
  assert_int64(0, ==, ud.recv_stream_data.stream_id);
  assert_uint64(0, ==, ud.recv_stream_data.offset);
  assert_size(100, ==, ud.recv_stream_data.datalen);
  assert_uint32(DWNX_STREAM_DATA_FLAG_NONE, ==, ud.recv_stream_data.flags);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_reset_stream(void) {
  static const dwnx_callbacks callbacks = {
    .stream_open = stream_open,
    .stream_reset = stream_reset,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  int rv;
  userdata ud;
  conn_options opts;
  size_t i;

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .stream_id = 4,
    .len = 99,
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr[1].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 4,
    .app_error_code = 1000000007,
    .final_size = 64 * 1024,
  };

  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(4, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.stream_reset.ncalled);
  assert_int64(4, ==, ud.stream_reset.stream_id);
  assert_uint64(1000000007, ==, ud.stream_reset.app_error_code);
  assert_uint64(64 * 1024, ==, ud.stream_reset.final_size);

  /* Receiving another RESET_STREAM is treated as error */
  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr[1], 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[1].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 4,
    .app_error_code = 1000000007,
    .final_size = 64 * 1024,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr[1], 1);

  ud = (userdata){0};

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_size(0, ==, ud.stream_reset.ncalled);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_size(1, ==, ud.stream_reset.ncalled);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_reset.ncalled);
  assert_int64(4, ==, ud.stream_reset.stream_id);
  assert_uint64(1000000007, ==, ud.stream_reset.app_error_code);
  assert_uint64(64 * 1024, ==, ud.stream_reset.final_size);

  dwnx_conn_del(conn);

  /* Flow control error */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr[1].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 4,
    .app_error_code = 1000000007,
    .final_size = 64 * 1024 + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Final size error */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr[1].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 4,
    .app_error_code = 1000000007,
    .final_size = 98,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FINAL_SIZE, ==, rv);

  dwnx_conn_del(conn);

  /* RESET_STREAM after seeing fin */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT,
    .stream_id = 4,
    .len = 99,
  };
  fr[1].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 4,
    .app_error_code = 1000000007,
    .final_size = 98,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_stop_sending(void) {
  static const dwnx_callbacks callbacks = {
    .stream_open = stream_open,
    .recv_stop_sending = recv_stop_sending,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  int rv;
  userdata ud;
  conn_options opts;
  size_t i;

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .stream_id = 4,
    .len = 99,
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr[1].stop_sending = (dwnx_frame_stop_sending){
    .type = DWNX_FRAME_STOP_SENDING,
    .stream_id = 4,
    .app_error_code = 1000000007,
  };

  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(4, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.recv_stop_sending.ncalled);
  assert_int64(4, ==, ud.recv_stop_sending.stream_id);
  assert_uint64(1000000007, ==, ud.recv_stop_sending.app_error_code);

  /* Receiving another STOP_SENDING is just fine. */
  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr[1], 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(0, ==, ud.recv_stop_sending.ncalled);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[1].stop_sending = (dwnx_frame_stop_sending){
    .type = DWNX_FRAME_STOP_SENDING,
    .stream_id = 4,
    .app_error_code = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr[1], 1);

  ud = (userdata){0};

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_size(0, ==, ud.stream_reset.ncalled);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.recv_stop_sending.ncalled);
  assert_int64(4, ==, ud.recv_stop_sending.stream_id);
  assert_uint64(1000000007, ==, ud.recv_stop_sending.app_error_code);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_max_data(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_data = (dwnx_frame_max_data){
    .type = DWNX_FRAME_MAX_DATA,
    .max_data = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_uint64(1000000007, ==, conn->tx.max_offset);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_data = (dwnx_frame_max_data){
    .type = DWNX_FRAME_MAX_DATA,
    .max_data = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf); ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_uint64(1000000007, ==, conn->tx.max_offset);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_max_stream_data(void) {
  static const dwnx_callbacks callbacks = {
    .stream_open = stream_open,
    .extend_max_stream_data = extend_max_stream_data,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  dwnx_strm *strm;
  userdata ud;
  conn_options opts;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_stream_data = (dwnx_frame_max_stream_data){
    .type = DWNX_FRAME_MAX_STREAM_DATA,
    .stream_id = 4,
    .max_stream_data = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(4, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.extend_max_stream_data.ncalled);
  assert_int64(4, ==, ud.extend_max_stream_data.stream_id);
  assert_uint64(1000000007, ==, ud.extend_max_stream_data.max_data);

  strm = dwnx_conn_find_stream(conn, 4);

  assert_not_null(strm);
  assert_uint64(1000000007, ==, strm->tx.max_offset);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_stream_data = (dwnx_frame_max_stream_data){
    .type = DWNX_FRAME_MAX_STREAM_DATA,
    .stream_id = 4,
    .max_stream_data = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_size(0, ==, ud.extend_max_stream_data.ncalled);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.stream_open.ncalled);
  assert_int64(4, ==, ud.stream_open.stream_id);
  assert_size(1, ==, ud.extend_max_stream_data.ncalled);
  assert_int64(4, ==, ud.extend_max_stream_data.stream_id);
  assert_uint64(1000000007, ==, ud.extend_max_stream_data.max_data);

  strm = dwnx_conn_find_stream(conn, 4);

  assert_not_null(strm);
  assert_uint64(1000000007, ==, strm->tx.max_offset);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_max_streams_bidi(void) {
  static const dwnx_callbacks callbacks = {
    .extend_max_local_streams_bidi = extend_max_local_streams_bidi,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  conn_options opts;
  userdata ud;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_streams = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_BIDI,
    .max_streams = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.extend_max_local_streams_bidi.ncalled);
  assert_uint64(1000000007, ==, ud.extend_max_local_streams_bidi.max_streams);
  assert_uint64(1000000007, ==, conn->tx.bidi.max_streams);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_streams = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_BIDI,
    .max_streams = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_size(0, ==, ud.extend_max_local_streams_bidi.ncalled);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.extend_max_local_streams_bidi.ncalled);
  assert_uint64(1000000007, ==, ud.extend_max_local_streams_bidi.max_streams);
  assert_uint64(1000000007, ==, conn->tx.bidi.max_streams);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_max_streams_uni(void) {
  static const dwnx_callbacks callbacks = {
    .extend_max_local_streams_uni = extend_max_local_streams_uni,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  conn_options opts;
  userdata ud;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_streams = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_UNI,
    .max_streams = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.extend_max_local_streams_uni.ncalled);
  assert_uint64(1000000007, ==, ud.extend_max_local_streams_uni.max_streams);
  assert_uint64(1000000007, ==, conn->tx.uni.max_streams);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.max_streams = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_UNI,
    .max_streams = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  ud = (userdata){0};

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_size(0, ==, ud.extend_max_local_streams_uni.ncalled);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(1, ==, ud.extend_max_local_streams_uni.ncalled);
  assert_uint64(1000000007, ==, ud.extend_max_local_streams_uni.max_streams);
  assert_uint64(1000000007, ==, conn->tx.uni.max_streams);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_data_blocked(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.data_blocked = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = 64 * 1024,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.data_blocked = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = 64 * 1024,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf); ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_stream_data_blocked(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 4,
    .offset = 64 * 1024,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 4,
    .offset = 64 * 1024,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf); ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_streams_blocked_bidi(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
    .max_streams = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
    .max_streams = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_streams_blocked_uni(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
    .max_streams = 1000000007,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
    .max_streams = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_connection_close(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  const uint8_t reason[] = "bye";
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.connection_close = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE,
    .error_code = 1000000007,
    .frame_type = 1000000009,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = (uint8_t *)reason,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_DRAINING, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.connection_close = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE,
    .error_code = 1000000007,
    .frame_type = 1000000009,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = (uint8_t *)reason,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(DWNX_ERR_DRAINING, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_connection_close_app(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  const uint8_t reason[] = "bye";
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.connection_close = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE_APP,
    .error_code = 1000000007,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = (uint8_t *)reason,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_DRAINING, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.connection_close = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE_APP,
    .error_code = 1000000007,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = (uint8_t *)reason,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(DWNX_ERR_DRAINING, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_padding(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.padding = (dwnx_frame_padding){
    .type = DWNX_FRAME_PADDING,
    .len = 100,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_qx_ping(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  int rv;
  size_t i;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.qx_ping = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_REQUEST,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_int64(0, ==, conn->rx.ping.last_seq);

  fr.qx_ping = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_REQUEST,
    .seq = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_int64(1000000007, ==, conn->rx.ping.last_seq);

  /* Receiving same sequence number is treated as error */
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  fr.qx_ping = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_REQUEST,
    .seq = 1000000007,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  for (i = 0; i < dwnx_buf_len(&buf) - 1; ++i) {
    rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

    assert_int(0, ==, rv);
    assert_int64(-1, ==, conn->rx.ping.last_seq);
  }

  rv = dwnx_conn_read(conn, buf.pos + i, 1, ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_int64(1000000007, ==, conn->rx.ping.last_seq);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_extend_max_stream_offset(void) {
  static const dwnx_transport_params remote_params = {
    .initial_max_streams_bidi = 1,
    .initial_max_streams_uni = 1,
    .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
  };
  static const dwnx_frame_qx_transport_parameters params_fr = {
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params = &remote_params,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_strm *strm;
  dwnx_tstamp ts = 0;
  int64_t stream_id;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  /* Extending the max stream offset of non-existent stream is
     noop. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_params_fr, ++ts);

  rv = dwnx_conn_extend_max_stream_offset(conn, 0, 1000000007);

  assert_int(0, ==, rv);

  dwnx_conn_del(conn);

  /* Attempt to extend the max stream offset against local
     unidirectional stream */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_uni_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);
  assert_int64(3, ==, stream_id);

  rv = dwnx_conn_extend_max_stream_offset(conn, stream_id, 999);

  assert_int(DWNX_ERR_INVALID_ARGUMENT, ==, rv);

  dwnx_conn_del(conn);

  /* Increase stream data limit */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);
  assert_int64(1, ==, stream_id);

  rv = dwnx_conn_extend_max_stream_offset(conn, stream_id, 999);

  assert_int(0, ==, rv);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_uint64(65536 + 999, ==, strm->rx.unsent_max_offset);
  assert_false(dwnx_strm_is_tx_queued(strm));

  rv = dwnx_conn_extend_max_stream_offset(conn, stream_id, 15386);

  assert_int(0, ==, rv);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_uint64(65536 + 999 + 15386, ==, strm->rx.unsent_max_offset);
  assert_true(dwnx_strm_is_tx_queued(strm));

  dwnx_conn_del(conn);
}

void test_dwnx_conn_writev_stream(void) {
  static const dwnx_transport_params remote_params = {
    .initial_max_streams_bidi = 1,
    .initial_max_streams_uni = 1,
    .initial_max_stream_data_bidi_remote = 24 * 1024,
    .initial_max_stream_data_bidi_local = 48 * 1024,
    .initial_max_stream_data_uni = 32 * 1024,
    .initial_max_data = 64 * 1024,
    .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
  };
  static const dwnx_frame_qx_transport_parameters params_fr = {
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params = &remote_params,
  };
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_ssize nwrite, nread;
  size_t rclen;
  dwnx_frd frd;
  dwnx_frame fr;
  int64_t stream_id;
  dwnx_ssize datalen;
  dwnx_strm *strm;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_frd_init(&frd);

  /* Just write transport parameters */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(2, <, nwrite);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);
  assert_uint64(
    64 * 1024, ==,
    fr.qx_transport_parameters.params->initial_max_stream_data_bidi_remote);

  dwnx_conn_del(conn);

  /* Write STREAM frame */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff((dwnx_ssize)dwnx_buf_left(&buf), ==, nwrite);
  assert_ptrdiff(16339, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(0, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  dwnx_conn_del(conn);

  /* Write empty STREAM frame */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id, NULL,
                                  0, ++ts);

  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);
  assert_ptrdiff(0, ==, datalen);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(0, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(0, ==, fr.stream.datacnt);

  dwnx_conn_del(conn);

  /* Write STREAM frame with FIN */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_FIN, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff((dwnx_ssize)dwnx_buf_left(&buf), ==, nwrite);
  assert_ptrdiff(16339, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(0, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_FIN, stream_id,
                                  nulldata, 99, ++ts);

  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);
  assert_ptrdiff(99, ==, datalen);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_true(fr.stream.fin);
  assert_uint64(16339, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_true(strm->flags & DWNX_STRM_FLAG_SHUT_WR);
  assert_uint64(16339 + 99, ==, strm->tx.offset);

  dwnx_conn_del(conn);

  /* Write control frames */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_conn_extend_max_offset(conn, 1000000007);

  rv = dwnx_conn_extend_max_stream_offset(conn, stream_id, 1000000009);

  assert_int(0, ==, rv);

  dwnx_conn_extend_max_streams_bidi(conn, 999);
  dwnx_conn_extend_max_streams_uni(conn, 9999);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff((dwnx_ssize)dwnx_buf_left(&buf), ==, nwrite);
  assert_ptrdiff(16322, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_MAX_DATA, ==, fr.hd.type);
  assert_uint64(128 * 1024 + 1000000007, ==, fr.max_data.max_data);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_MAX_STREAM_DATA, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.max_stream_data.stream_id);
  assert_uint64(64 * 1024 + 1000000009, ==, fr.max_stream_data.max_stream_data);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_MAX_STREAMS_BIDI, ==, fr.hd.type);
  assert_uint64(10 + 999, ==, fr.max_streams.max_streams);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_MAX_STREAMS_UNI, ==, fr.hd.type);
  assert_uint64(10 + 9999, ==, fr.max_streams.max_streams);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(0, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  dwnx_conn_del(conn);

  /* Stream blocked by stream-level flow control */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &params_fr, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff((dwnx_ssize)dwnx_buf_left(&buf), ==, nwrite);
  assert_ptrdiff(16339, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(0, ==, fr.stream.offset);
  assert_uint64((uint64_t)datalen, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);
  assert_ptrdiff(8237, ==, datalen);

  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(DWNX_ERR_STREAM_DATA_BLOCKED, ==, nwrite);
  assert_ptrdiff(-1, ==, datalen);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_size(dwnx_buf_len(&buf), ==, rclen);
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(16339, ==, fr.stream.offset);
  assert_uint64(8237, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  dwnx_conn_del(conn);
}
