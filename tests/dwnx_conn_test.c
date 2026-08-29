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
#include <unistd.h>
#include <errno.h>

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
  munit_void_test(test_dwnx_conn_send_stream_data_blocked),
  munit_void_test(test_dwnx_conn_handle_expiry),
  munit_void_test(test_dwnx_conn_write_connection_close),
  munit_void_test(test_dwnx_conn_encode_0rtt_transport_params),
  munit_void_test(test_dwnx_conn_validate_early_transport_params),
  munit_void_test(test_dwnx_conn_tls_early_data_rejected),
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
  const dwnx_settings *settings;
  const dwnx_transport_params *params;
  const dwnx_mem *mem;
  void *user_data;
} conn_options;

static const dwnx_transport_params empty_remote_params = {
  .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
};

static const dwnx_transport_params default_remote_params = {
  .initial_max_streams_bidi = 100,
  .initial_max_streams_uni = 100,
  .initial_max_stream_data_bidi_local = 64 * 1024,
  .initial_max_stream_data_bidi_remote = 64 * 1024,
  .initial_max_stream_data_uni = 64 * 1024,
  .initial_max_data = 128 * 1024,
  .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
};

static void genrand(uint8_t *dest, size_t destlen) { memset(dest, 0, destlen); }

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

static void log_write(void *user_data, char *msg, size_t len) {
  ssize_t nwrite;
  (void)user_data;

  msg[len++] = '\n';

  while ((nwrite = write(fileno(stderr), msg,
#ifdef WIN32
                         (unsigned int)
#endif /* WIN32 */
                           len)) == -1 &&
         errno == EINTR)
    ;

  assert_ssize((ssize_t)len, ==, nwrite);
}

static void server_default_callbacks(dwnx_callbacks *callbacks) {
  *callbacks = (dwnx_callbacks){
    .rand = genrand,
  };
}

static void server_default_settings(dwnx_settings *settings) {
  dwnx_settings_default(settings);
  settings->log_write = log_write;
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
  dwnx_callbacks callbacks;
  dwnx_settings settings;
  dwnx_transport_params params;
  int rv;

  if (!opts.callbacks) {
    server_default_callbacks(&callbacks);
    opts.callbacks = &callbacks;
  }

  if (!opts.settings) {
    server_default_settings(&settings);
    opts.settings = &settings;
  }

  if (!opts.params) {
    server_default_transport_params(&params);
    opts.params = &params;
  }

  rv = dwnx_conn_server_new(pconn, opts.callbacks, opts.settings, opts.params,
                            opts.mem, opts.user_data);

  assert_int(0, ==, rv);
}

static void setup_default_server(dwnx_conn **pconn) {
  setup_default_server_with_options(pconn, (conn_options){0});
}

static void client_default_callbacks(dwnx_callbacks *callbacks) {
  *callbacks = (dwnx_callbacks){
    .rand = genrand,
  };
}

static void client_default_settings(dwnx_settings *settings) {
  dwnx_settings_default(settings);
  settings->log_write = log_write;
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
  dwnx_callbacks callbacks;
  dwnx_settings settings;
  dwnx_transport_params params;
  int rv;

  if (!opts.callbacks) {
    client_default_callbacks(&callbacks);
    opts.callbacks = &callbacks;
  }

  if (!opts.settings) {
    client_default_settings(&settings);
    opts.settings = &settings;
  }

  if (!opts.params) {
    client_default_transport_params(&params);
    opts.params = &params;
  }

  rv = dwnx_conn_client_new(pconn, opts.callbacks, opts.settings, opts.params,
                            opts.mem, opts.user_data);

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
  dwnx_callbacks callbacks;
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  int rv;
  userdata ud;
  conn_options opts;

  server_default_callbacks(&callbacks);
  callbacks.stream_open = stream_open;
  callbacks.recv_stream_data = recv_stream_data;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  /* With all bits set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_write_record(&buf, fr, 1);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* With FIN and LEN bits set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* With OFF and LEN bits set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* With FIN bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* With OFF bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_OFF_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_OFF_BIT,
    .offset = 100,
    .len = 99,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_size(0, ==, ud.stream_open.ncalled);
  assert_size(1, ==, ud.recv_stream_data.ncalled);
  assert_int64(0, ==, ud.recv_stream_data.stream_id);
  assert_uint64(100, ==, ud.recv_stream_data.offset);
  assert_size(99, ==, ud.recv_stream_data.datalen);
  assert_uint32(DWNX_STREAM_DATA_FLAG_NONE, ==, ud.recv_stream_data.flags);

  dwnx_conn_del(conn);

  /* With LEN bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* Without bit set */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .len = 100,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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

  /* Unidirectional stream len == 0 with fin and len bits set,
     followed by another frame. */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
    .stream_id = 2,
  };
  fr[1].padding = (dwnx_frame_padding){
    .type = DWNX_FRAME_PADDING,
    .len = 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_reset_stream(void) {
  dwnx_callbacks callbacks;
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  int rv;
  userdata ud;
  conn_options opts;
  size_t i;
  int64_t stream_id;
  dwnx_ssize nwrite;
  dwnx_strm *strm;

  server_default_callbacks(&callbacks);
  callbacks.stream_open = stream_open;
  callbacks.stream_reset = stream_reset;

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .stream_id = 4,
    .len = 99,
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
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

  /* Receiving RESET_STREAM against the local stream that has already
     been closed */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &default_remote_params, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.pos, dwnx_buf_left(&buf), NULL,
                                  DWNX_WRITE_STREAM_FLAG_FIN, stream_id, NULL,
                                  0, ++ts);

  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);

  nwrite =
    dwnx_conn_write_stream(conn, buf.pos, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  fr[0].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = stream_id,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_null(strm);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving RESET_STREAM against the remote stream that has already
     been closed */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT,
    .stream_id = 2,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  strm = dwnx_conn_find_stream(conn, 2);

  assert_null(strm);

  fr[0].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 2,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_recv_stop_sending(void) {
  dwnx_callbacks callbacks;
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  dwnx_strm *strm;
  int rv;
  userdata ud;
  conn_options opts;
  size_t i;
  dwnx_ssize nwrite;

  server_default_callbacks(&callbacks);
  callbacks.stream_open = stream_open;
  callbacks.recv_stop_sending = recv_stop_sending;

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .stream_id = 4,
    .len = 99,
  };

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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

  /* Receives STREAM with fin and STOP_SENDING.  After writing
     RESET_STREAM, the stream must be closed. */
  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
    .stream_id = 4,
    .len = 99,
  };
  fr[1].stop_sending = (dwnx_frame_stop_sending){
    .type = DWNX_FRAME_STOP_SENDING,
    .stream_id = 4,
    .app_error_code = 772342,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  ud = (userdata){0};
  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  strm = dwnx_conn_find_stream(conn, 4);

  assert_not_null(strm);
  assert_true(strm->flags & DWNX_STRM_FLAG_SHUT_RD);
  assert_true(strm->flags & DWNX_STRM_FLAG_SHUT_WR);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_null(dwnx_conn_find_stream(conn, 4));

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_callbacks callbacks;
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

  server_default_callbacks(&callbacks);
  callbacks.stream_open = stream_open;
  callbacks.extend_max_stream_data = extend_max_stream_data;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_callbacks callbacks;
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  conn_options opts;
  userdata ud;
  size_t i;
  int rv;

  server_default_callbacks(&callbacks);
  callbacks.extend_max_local_streams_bidi = extend_max_local_streams_bidi;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_callbacks callbacks;
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_frame fr;
  dwnx_tstamp ts = 0;
  conn_options opts;
  userdata ud;
  size_t i;
  int rv;

  server_default_callbacks(&callbacks);
  callbacks.extend_max_local_streams_uni = extend_max_local_streams_uni;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  opts = (conn_options){
    .callbacks = &callbacks,
    .user_data = &ud,
  };
  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.data_blocked = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = conn->rx.max_offset,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receiving an offset that is larger than the local endpoint
     allows. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.data_blocked = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = conn->rx.max_offset + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.data_blocked = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = conn->rx.max_offset,
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
  dwnx_frame fr[2];
  dwnx_tstamp ts = 0;
  size_t i;
  int rv;
  dwnx_transport_params params, remote_params;
  uint8_t outbuf[16384];
  int64_t stream_id;
  dwnx_ssize nwrite;
  conn_options opts;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 4,
    .offset = 64 * 1024,
  };

  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the existing stream. */
  setup_default_server(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, outbuf, sizeof(outbuf), NULL,
                                  DWNX_WRITE_STREAM_FLAG_FIN, stream_id, NULL,
                                  0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = stream_id,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_local,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the local bidirectional
     stream that the local endpoint has not opened yet. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 1,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_local,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_STATE, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream that the local endpoint does not allow. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = dwnx_nth_remote_bidi_stream_id(
      conn, conn->local.transport_params.initial_max_streams_bidi + 1),
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_LIMIT, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the local unidirectional
     stream. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 3,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_STATE, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote unidirectional
     stream that the local endpoint does not allow. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = dwnx_nth_remote_uni_stream_id(
      conn, conn->local.transport_params.initial_max_streams_uni + 1),
    .offset = conn->local.transport_params.initial_max_stream_data_uni,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_LIMIT, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the local bidirectional
     stream that has been closed. */
  setup_default_server(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, outbuf, sizeof(outbuf), NULL,
                                  DWNX_WRITE_STREAM_FLAG_FIN, stream_id, NULL,
                                  0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  fr[0].reset_stream = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = stream_id,
  };
  fr[1].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = stream_id,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_local,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 2);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream that has been closed. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, outbuf, sizeof(outbuf), NULL,
                                  DWNX_WRITE_STREAM_FLAG_FIN, 0, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_null(dwnx_conn_find_stream(conn, 0));

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream and offset violates stream level flow control limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;
  params.initial_max_data = 100;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset =
      conn->local.transport_params.initial_max_stream_data_bidi_remote + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote unidirectional
     stream and offset violates stream level flow control limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_uni = 1;
  params.initial_max_data = 100;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 2,
    .offset = conn->local.transport_params.initial_max_stream_data_uni + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream and offset violates connection level flow control
     limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;
  params.initial_max_stream_data_bidi_remote = 100;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset = conn->local.transport_params.initial_max_data + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream that has been half-close remote. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_FIN_BIT | DWNX_STREAM_LEN_BIT,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream and offset is smaller than the last offset. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;
  params.initial_max_stream_data_bidi_remote = 100;
  params.initial_max_data = 100;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .len =
      (size_t)conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset =
      conn->local.transport_params.initial_max_stream_data_bidi_remote - 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream and offset violates stream level flow control limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;
  params.initial_max_stream_data_bidi_remote = 99;
  params.initial_max_data = 100;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .len =
      (size_t)conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset =
      conn->local.transport_params.initial_max_stream_data_bidi_remote + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receiving STREAM_DATA_BLOCKED against the remote bidirectional
     stream and offset violates connection level flow control limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1;
  params.initial_max_stream_data_bidi_remote = 100;
  params.initial_max_data = 99;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .flags = DWNX_STREAM_LEN_BIT,
    .len = (size_t)(conn->local.transport_params
                      .initial_max_stream_data_bidi_remote -
                    1),
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .offset = conn->local.transport_params.initial_max_stream_data_bidi_remote,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_FLOW_CONTROL, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr[0].stream_data_blocked = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = 4,
    .offset = 64 * 1024,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, fr, 1);

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
  dwnx_transport_params params;
  size_t i;
  int rv;
  conn_options opts;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
    .max_streams = conn->rx.bidi.max_streams,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receiving max_streams that is larger than the stream limit. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
    .max_streams = conn->rx.bidi.max_streams + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_LIMIT, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_bidi = 1000000007;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
    .max_streams = conn->rx.bidi.max_streams,
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
  dwnx_transport_params params;
  size_t i;
  int rv;
  conn_options opts;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  dwnx_transport_params_default(&params);
  params.initial_max_streams_uni = 999;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
    .max_streams = conn->rx.uni.max_streams,
  };

  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);

  dwnx_conn_del(conn);

  /* Receiving max_streams that is larger than the stream limit. */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_uni = 999;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
    .max_streams = conn->rx.uni.max_streams + 1,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_STREAM_LIMIT, ==, rv);

  dwnx_conn_del(conn);

  /* Receive 1 byte at a time */
  dwnx_transport_params_default(&params);
  params.initial_max_streams_uni = 1000000007;

  opts = (conn_options){
    .params = &params,
  };

  setup_default_server_with_options(&conn, opts);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.streams_blocked = (dwnx_frame_streams_blocked){
    .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
    .max_streams = conn->rx.uni.max_streams,
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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_ssize nwrite, nread;
  dwnx_frd frd;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_frd_init(&frd);

  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

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

  /* Respond to the ping request */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  nwrite =
    dwnx_conn_write_stream(conn, rawbuf, sizeof(rawbuf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  fr.qx_ping = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_REQUEST,
    .seq = 1000000009,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(0, ==, rv);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);
  assert_size(0, ==, conn->rx.rcrd.record_left);
  assert_int64(1000000009, ==, conn->rx.ping.last_seq);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_int64(1000000009, ==, conn->rx.ping.last_resp_seq);

  buf.last += nwrite;
  dwnx_check_recordlen(&buf, nwrite - DWNX_QRE_RECORDLEN_SIZE);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_QX_PING_RESPONSE, ==, fr.hd.type);
  assert_uint64(1000000009, ==, fr.qx_ping.seq);

  /* QX_PING_RESPONSE is only sent at most once per seq */
  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, ==, nwrite);

  dwnx_conn_del(conn);

  /* Receive QX_PING_RESPONSE with seq that is larger than seq we ever
     sent. */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  fr.qx_ping = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_RESPONSE,
  };

  dwnx_buf_reset(&buf);
  dwnx_write_record(&buf, &fr, 1);

  rv = dwnx_conn_read(conn, buf.pos, dwnx_buf_len(&buf), ++ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_extend_max_stream_offset(void) {
  static const dwnx_transport_params remote_params = {
    .initial_max_streams_bidi = 1,
    .initial_max_streams_uni = 1,
    .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
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
  dwnx_read_transport_params(conn, &empty_remote_params, ++ts);

  rv = dwnx_conn_extend_max_stream_offset(conn, 0, 1000000007);

  assert_int(0, ==, rv);

  dwnx_conn_del(conn);

  /* Attempt to extend the max stream offset against local
     unidirectional stream */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  rv = dwnx_conn_open_uni_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);
  assert_int64(3, ==, stream_id);

  rv = dwnx_conn_extend_max_stream_offset(conn, stream_id, 999);

  assert_int(DWNX_ERR_INVALID_ARGUMENT, ==, rv);

  dwnx_conn_del(conn);

  /* Increase stream data limit */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_ssize nwrite, nread;
  uint64_t rclen;
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
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(2, <, nwrite);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_QX_TRANSPORT_PARAMETERS, ==, fr.hd.type);
  assert_uint64(
    64 * 1024, ==,
    fr.qx_transport_parameters.params->initial_max_stream_data_bidi_remote);

  dwnx_conn_del(conn);

  /* Buffer is too small */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(
    conn, buf.last, 2, NULL, DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, nwrite);

  dwnx_conn_del(conn);

  /* Write STREAM frame */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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
  dwnx_read_transport_params(conn, &remote_params, ++ts);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);

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

  assert_uint64((uint64_t)dwnx_buf_len(&buf), ==, rclen);
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), >, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_false(fr.stream.fin);
  assert_uint64(16339, ==, fr.stream.offset);
  assert_uint64(8237, ==, fr.stream.len);
  assert_size(1, ==, fr.stream.datacnt);
  assert_size(fr.stream.len, ==, fr.stream.data[0].len);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM_DATA_BLOCKED, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream_data_blocked.stream_id);
  assert_uint64(16339 + 8237, ==, fr.stream_data_blocked.offset);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_send_stream_data_blocked(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_transport_params remote_params;
  dwnx_ssize datalen;
  dwnx_ssize nwrite, nread;
  dwnx_frd frd;
  dwnx_frame fr;
  dwnx_strm *strm;
  int64_t stream_id;
  uint64_t rclen;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_frd_init(&frd);

  /* Blocked by both stream and connection level flow control
     limits without writing any data */
  setup_default_client(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_ptrdiff(-1, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_DATA_BLOCKED, ==, fr.hd.type);
  assert_uint64(0, ==, fr.data_blocked.offset);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM_DATA_BLOCKED, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream_data_blocked.stream_id);
  assert_uint64(0, ==, fr.stream_data_blocked.offset);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_false(strm->flags & DWNX_STRM_FLAG_SEND_STREAM_DATA_BLOCKED);
  assert_uint64(strm->tx.offset, ==, strm->tx.last_blocked_offset);
  assert_uint64(conn->tx.offset, ==, conn->tx.last_blocked_offset);

  dwnx_conn_del(conn);

  /* Blocked by both stream and connection level flow control limits
     after writing some data */
  setup_default_client(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  remote_params.initial_max_stream_data_bidi_remote = 77;
  remote_params.initial_max_data = 77;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_ptrdiff(77, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_uint64(77, ==, fr.stream.len);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_DATA_BLOCKED, ==, fr.hd.type);
  assert_uint64(77, ==, fr.data_blocked.offset);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM_DATA_BLOCKED, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream_data_blocked.stream_id);
  assert_uint64(77, ==, fr.stream_data_blocked.offset);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_false(strm->flags & DWNX_STRM_FLAG_SEND_STREAM_DATA_BLOCKED);
  assert_uint64(strm->tx.offset, ==, strm->tx.last_blocked_offset);
  assert_uint64(conn->tx.offset, ==, conn->tx.last_blocked_offset);

  dwnx_conn_del(conn);

  /* No space to write stream blocked frame before writing any
     data. */
  setup_default_client(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  remote_params.initial_max_stream_data_bidi_remote = 77;
  remote_params.initial_max_data = 78;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, 84, &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, 77, ++ts);

  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);
  assert_ptrdiff(77, ==, datalen);

  nwrite = dwnx_conn_write_stream(conn, buf.last, 84, &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, 77, ++ts);

  assert_ptrdiff(DWNX_ERR_STREAM_DATA_BLOCKED, ==, nwrite);
  assert_ptrdiff(-1, ==, datalen);

  nwrite = dwnx_conn_write_stream(
    conn, buf.last, 84, NULL, DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_uint64(77, ==, fr.stream.len);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_true(strm->flags & DWNX_STRM_FLAG_SEND_STREAM_DATA_BLOCKED);
  assert_uint64(strm->tx.offset, !=, strm->tx.last_blocked_offset);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM_DATA_BLOCKED, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream_data_blocked.stream_id);
  assert_uint64(77, ==, fr.stream_data_blocked.offset);

  dwnx_conn_del(conn);

  /* No space to write stream blocked frame after writing some
     data. */
  setup_default_client(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.initial_max_streams_bidi = 1;
  remote_params.initial_max_stream_data_bidi_remote = 77;
  remote_params.initial_max_data = 77;
  dwnx_read_transport_params(conn, &remote_params, ++ts);

  dwnx_buf_reset(&buf);
  nwrite =
    dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), NULL,
                           DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ++ts);

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, 87, &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_ptrdiff(77, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff(0, <, nread);
  assert_uint64(DWNX_FRAME_STREAM, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream.stream_id);
  assert_uint64(77, ==, fr.stream.len);

  buf.pos += nread;
  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_DATA_BLOCKED, ==, fr.hd.type);
  assert_uint64(77, ==, fr.data_blocked.offset);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_true(strm->flags & DWNX_STRM_FLAG_SEND_STREAM_DATA_BLOCKED);
  assert_uint64(strm->tx.offset, !=, strm->tx.last_blocked_offset);
  assert_uint64(conn->tx.offset, ==, conn->tx.last_blocked_offset);

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_stream(conn, buf.last, dwnx_buf_left(&buf), &datalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_ptrdiff(-1, ==, datalen);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_STREAM_DATA_BLOCKED, ==, fr.hd.type);
  assert_int64(stream_id, ==, fr.stream_data_blocked.stream_id);
  assert_uint64(77, ==, fr.stream_data_blocked.offset);

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_false(strm->flags & DWNX_STRM_FLAG_SEND_STREAM_DATA_BLOCKED);
  assert_uint64(strm->tx.offset, ==, strm->tx.last_blocked_offset);
  assert_uint64(conn->tx.offset, ==, conn->tx.last_blocked_offset);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_handle_expiry(void) {
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_transport_params remote_params;
  dwnx_tstamp expiry;
  dwnx_ssize nwrite;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  setup_default_server(&conn);
  dwnx_transport_params_default(&remote_params);
  remote_params.max_idle_timeout = 5 * DWNX_SECONDS;
  ts += DWNX_SECONDS;
  dwnx_read_transport_params(conn, &remote_params, ts);

  expiry = dwnx_conn_get_expiry(conn);

  assert_uint64(6 * DWNX_SECONDS, ==, expiry);

  ts += DWNX_SECONDS;
  nwrite = dwnx_conn_write_stream(conn, buf.pos, dwnx_buf_left(&buf), NULL,
                                  DWNX_WRITE_STREAM_FLAG_NONE, -1, NULL, 0, ts);

  assert_ptrdiff(0, <, nwrite);

  expiry = dwnx_conn_get_expiry(conn);

  assert_uint64(7 * DWNX_SECONDS, ==, expiry);

  ts = expiry;

  rv = dwnx_conn_handle_expiry(conn, ts - DWNX_NANOSECONDS);

  assert_int(0, ==, rv);

  rv = dwnx_conn_handle_expiry(conn, ts);

  assert_int(DWNX_ERR_IDLE_CLOSE, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_write_connection_close(void) {
  static const uint8_t reason[] = "this is reason phrase";
  dwnx_conn *conn;
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_ccerr ccerr;
  dwnx_ssize nwrite, nread;
  dwnx_frd frd;
  dwnx_frame fr;
  uint64_t rclen;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));
  dwnx_frd_init(&frd);

  /* transport */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ts);

  ccerr = (dwnx_ccerr){
    .type = DWNX_CCERR_TYPE_TRANSPORT,
    .error_code = DWNX_INTERNAL_ERROR,
    .frame_type = DWNX_FRAME_QX_PING_REQUEST,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = reason,
  };

  nwrite = dwnx_conn_write_connection_close(conn, buf.pos, dwnx_buf_left(&buf),
                                            &ccerr, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_CLOSING, ==,
              conn->rx.rcrd.state);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_CONNECTION_CLOSE, ==, fr.hd.type);
  assert_uint64(ccerr.error_code, ==, fr.connection_close.error_code);
  assert_uint64(ccerr.frame_type, ==, fr.connection_close.frame_type);
  assert_size(ccerr.reasonlen, ==, fr.connection_close.reasonlen);
  assert_memory_equal(ccerr.reasonlen, ccerr.reason,
                      fr.connection_close.reason);

  /* Calling dwnx_conn_write_connection_close again returns 0. */
  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_connection_close(conn, buf.pos, dwnx_buf_left(&buf),
                                            &ccerr, ++ts);

  assert_ptrdiff(0, ==, nwrite);

  dwnx_conn_del(conn);

  /* transport without reason */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ts);

  ccerr = (dwnx_ccerr){
    .type = DWNX_CCERR_TYPE_TRANSPORT,
    .error_code = DWNX_INTERNAL_ERROR,
    .frame_type = DWNX_FRAME_QX_PING_REQUEST,
  };

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_connection_close(conn, buf.pos, dwnx_buf_left(&buf),
                                            &ccerr, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_CLOSING, ==,
              conn->rx.rcrd.state);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_CONNECTION_CLOSE, ==, fr.hd.type);
  assert_uint64(ccerr.error_code, ==, fr.connection_close.error_code);
  assert_uint64(ccerr.frame_type, ==, fr.connection_close.frame_type);
  assert_size(ccerr.reasonlen, ==, fr.connection_close.reasonlen);
  assert_null(fr.connection_close.reason);

  dwnx_conn_del(conn);

  /* application */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ts);

  ccerr = (dwnx_ccerr){
    .type = DWNX_CCERR_TYPE_APPLICATION,
    .error_code = DWNX_INTERNAL_ERROR,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = reason,
  };

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_connection_close(conn, buf.pos, dwnx_buf_left(&buf),
                                            &ccerr, ++ts);

  assert_ptrdiff(0, <, nwrite);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_CLOSING, ==,
              conn->rx.rcrd.state);

  buf.last += nwrite;
  buf.pos = (uint8_t *)dwnx_read_recordlen(&rclen, buf.pos, dwnx_buf_len(&buf));

  nread = dwnx_frd_decode(&frd, &fr, buf.pos, dwnx_buf_len(&buf));

  assert_ptrdiff((dwnx_ssize)dwnx_buf_len(&buf), ==, nread);
  assert_uint64(DWNX_FRAME_CONNECTION_CLOSE_APP, ==, fr.hd.type);
  assert_uint64(ccerr.error_code, ==, fr.connection_close.error_code);
  assert_size(ccerr.reasonlen, ==, fr.connection_close.reasonlen);
  assert_memory_equal(ccerr.reasonlen, ccerr.reason,
                      fr.connection_close.reason);

  dwnx_conn_del(conn);

  /* idle close */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &empty_remote_params, ts);

  ccerr = (dwnx_ccerr){
    .type = DWNX_CCERR_TYPE_IDLE_CLOSE,
  };

  dwnx_buf_reset(&buf);
  nwrite = dwnx_conn_write_connection_close(conn, buf.pos, dwnx_buf_left(&buf),
                                            &ccerr, ++ts);

  assert_ptrdiff(0, ==, nwrite);
  assert_enum(dwnx_record_read_state, DWNX_RECORD_READ_STATE_RECORD_SIZE, ==,
              conn->rx.rcrd.state);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_encode_0rtt_transport_params(void) {
  dwnx_conn *conn;
  uint8_t buf[256];
  dwnx_tstamp ts = 0;
  dwnx_transport_params remote_params, params;
  dwnx_ssize nwrite;
  int rv;

  dwnx_transport_params_default(&remote_params);

  remote_params.initial_max_streams_bidi = 1000000007;
  remote_params.initial_max_streams_uni = 1000000009;
  remote_params.initial_max_stream_data_bidi_local = UINT32_MAX;
  remote_params.initial_max_stream_data_bidi_remote = 999;
  remote_params.initial_max_stream_data_uni = 99929324311;
  remote_params.initial_max_data = 4521373001;
  remote_params.max_idle_timeout = 300 * DWNX_MILLISECONDS;

  /* client */
  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &remote_params, ts);

  nwrite = dwnx_conn_encode_0rtt_transport_params(conn, buf, sizeof(buf));

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_transport_params_decode(&params, buf, (size_t)nwrite);

  assert_int(0, ==, rv);
  assert_uint64(remote_params.initial_max_streams_bidi, ==,
                params.initial_max_streams_bidi);
  assert_uint64(remote_params.initial_max_streams_uni, ==,
                params.initial_max_streams_uni);
  assert_uint64(remote_params.initial_max_stream_data_bidi_local, ==,
                params.initial_max_stream_data_bidi_local);
  assert_uint64(remote_params.initial_max_stream_data_bidi_remote, ==,
                params.initial_max_stream_data_bidi_remote);
  assert_uint64(remote_params.initial_max_data, ==, params.initial_max_data);
  assert_uint64(0, ==, params.max_idle_timeout);

  dwnx_conn_del(conn);

  /* server */
  setup_default_server(&conn);
  dwnx_read_transport_params(conn, &remote_params, ts);

  nwrite = dwnx_conn_encode_0rtt_transport_params(conn, buf, sizeof(buf));

  assert_ptrdiff(0, <, nwrite);

  rv = dwnx_transport_params_decode(&params, buf, (size_t)nwrite);

  assert_int(0, ==, rv);
  assert_uint64(conn->local.transport_params.initial_max_streams_bidi, ==,
                params.initial_max_streams_bidi);
  assert_uint64(conn->local.transport_params.initial_max_streams_uni, ==,
                params.initial_max_streams_uni);
  assert_uint64(conn->local.transport_params.initial_max_stream_data_bidi_local,
                ==, params.initial_max_stream_data_bidi_local);
  assert_uint64(
    conn->local.transport_params.initial_max_stream_data_bidi_remote, ==,
    params.initial_max_stream_data_bidi_remote);
  assert_uint64(conn->local.transport_params.initial_max_data, ==,
                params.initial_max_data);
  assert_uint64(conn->local.transport_params.max_idle_timeout, ==,
                params.max_idle_timeout);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_validate_early_transport_params(void) {
  dwnx_conn *conn;
  uint8_t buf[256];
  dwnx_tstamp ts = 0;
  dwnx_transport_params early_params, remote_params;
  dwnx_ssize nwrite;
  int rv;

  dwnx_transport_params_default(&early_params);

  early_params.initial_max_streams_bidi = 1000000007;
  early_params.initial_max_streams_uni = 1000000009;
  early_params.initial_max_stream_data_bidi_local = 12323333;
  early_params.initial_max_stream_data_bidi_remote = 84843434;
  early_params.initial_max_stream_data_uni = 99929324311;
  early_params.initial_max_data = 4521373001;

  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &early_params, ts);

  nwrite = dwnx_conn_encode_0rtt_transport_params(conn, buf, sizeof(buf));

  assert_ptrdiff(0, <, nwrite);

  dwnx_conn_del(conn);

  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);
  assert_uint64(early_params.initial_max_streams_bidi, ==,
                conn->remote.transport_params.initial_max_streams_bidi);
  assert_uint64(early_params.initial_max_streams_uni, ==,
                conn->remote.transport_params.initial_max_streams_uni);
  assert_uint64(
    early_params.initial_max_stream_data_bidi_local, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_local);
  assert_uint64(
    early_params.initial_max_stream_data_bidi_remote, ==,
    conn->remote.transport_params.initial_max_stream_data_bidi_remote);
  assert_uint64(early_params.initial_max_data, ==,
                conn->remote.transport_params.initial_max_data);

  assert_uint64(early_params.initial_max_streams_bidi, ==,
                conn->early.transport_params.initial_max_streams_bidi);
  assert_uint64(early_params.initial_max_streams_uni, ==,
                conn->early.transport_params.initial_max_streams_uni);
  assert_uint64(
    early_params.initial_max_stream_data_bidi_local, ==,
    conn->early.transport_params.initial_max_stream_data_bidi_local);
  assert_uint64(
    early_params.initial_max_stream_data_bidi_remote, ==,
    conn->early.transport_params.initial_max_stream_data_bidi_remote);
  assert_uint64(early_params.initial_max_data, ==,
                conn->early.transport_params.initial_max_data);

  dwnx_read_transport_params(conn, &early_params, ++ts);

  dwnx_conn_del(conn);

  /* server reduced initial_max_streams_bidi */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_streams_bidi;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* server reduced initial_max_streams_uni */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_streams_uni;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* server reduced initial_max_stream_data_bidi_local */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_stream_data_bidi_local;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* server reduced initial_max_stream_data_bidi_remote */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_stream_data_bidi_remote;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* server reduced initial_max_stream_data_uni */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_stream_data_uni;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);

  /* server reduced initial_max_data */
  setup_default_client(&conn);

  rv =
    dwnx_conn_decode_and_set_0rtt_transport_params(conn, buf, (size_t)nwrite);

  assert_int(0, ==, rv);

  remote_params = early_params;
  --remote_params.initial_max_data;

  rv = dwnx_conn_read_transport_params(conn, &remote_params, ts);

  assert_int(DWNX_ERR_PROTO, ==, rv);

  dwnx_conn_del(conn);
}

void test_dwnx_conn_tls_early_data_rejected(void) {
  dwnx_conn *conn;
  uint8_t paramsbuf[256];
  uint8_t rawbuf[16384];
  dwnx_buf buf;
  dwnx_tstamp ts = 0;
  dwnx_transport_params early_params;
  dwnx_ssize nwrite;
  int rv;
  int64_t stream_id;
  dwnx_ssize ndatalen;
  dwnx_strm *strm;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

  dwnx_transport_params_default(&early_params);

  early_params.initial_max_streams_bidi = 100;
  early_params.initial_max_streams_uni = 100;
  early_params.initial_max_stream_data_bidi_local = 16384;
  early_params.initial_max_stream_data_bidi_remote = 1024;
  early_params.initial_max_stream_data_uni = 1024;
  early_params.initial_max_data = 2048;

  setup_default_client(&conn);
  dwnx_read_transport_params(conn, &early_params, ts);

  nwrite =
    dwnx_conn_encode_0rtt_transport_params(conn, paramsbuf, sizeof(paramsbuf));

  assert_ptrdiff(0, <, nwrite);

  dwnx_conn_del(conn);

  setup_default_client(&conn);

  rv = dwnx_conn_decode_and_set_0rtt_transport_params(conn, paramsbuf,
                                                      (size_t)nwrite);

  assert_int(0, ==, rv);
  assert_true(conn->flags & DWNX_CONN_FLAG_EARLY_TRANSPORT_PARAMS_SET);

  rv = dwnx_conn_open_bidi_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, buf.pos, dwnx_buf_left(&buf), &ndatalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);
  assert_ptrdiff(DWNX_ERR_WRITE_MORE, ==, nwrite);
  assert_ptrdiff(0, <, ndatalen);

  rv = dwnx_conn_open_uni_stream(conn, &stream_id, NULL);

  assert_int(0, ==, rv);

  nwrite = dwnx_conn_write_stream(conn, buf.pos, dwnx_buf_left(&buf), &ndatalen,
                                  DWNX_WRITE_STREAM_FLAG_NONE, stream_id,
                                  nulldata, sizeof(nulldata), ++ts);
  assert_ptrdiff(0, <, nwrite);
  assert_ptrdiff(0, <, ndatalen);

  rv = dwnx_conn_shutdown_stream_write(conn, 0, stream_id, DWNX_NO_ERROR);

  assert_int(0, ==, rv);
  assert_uint64(conn->remote.transport_params.initial_max_data, ==,
                conn->tx.offset);
  assert_uint64(conn->remote.transport_params.initial_max_data, ==,
                conn->tx.last_blocked_offset);
  assert_int64(4, ==, conn->tx.bidi.next_stream_id);
  assert_int64(6, ==, conn->tx.uni.next_stream_id);
  assert_size(2, ==, dwnx_map_size(&conn->strms));

  strm = dwnx_conn_find_stream(conn, stream_id);

  assert_not_null(strm);
  assert_true(dwnx_strm_is_tx_queued(strm));

  dwnx_conn_extend_max_streams_bidi(conn, 1);

  assert_uint64(conn->rx.bidi.max_streams + 1, ==,
                conn->rx.bidi.unsent_max_streams);

  dwnx_conn_extend_max_streams_uni(conn, 1);

  assert_uint64(conn->rx.uni.max_streams + 1, ==,
                conn->rx.uni.unsent_max_streams);

  dwnx_conn_extend_max_offset(conn, 1);

  assert_uint64(conn->rx.max_offset + 1, ==, conn->rx.unsent_max_offset);

  rv = dwnx_conn_tls_early_data_rejected(conn);

  assert_int(0, ==, rv);
  assert_true(conn->flags & DWNX_CONN_FLAG_EARLY_DATA_REJECTED);
  assert_false(conn->flags & DWNX_CONN_FLAG_EARLY_TRANSPORT_PARAMS_SET);
  assert_uint64(0, ==, conn->early.transport_params.initial_max_data);
  assert_uint64(0, ==, conn->remote.transport_params.initial_max_data);
  assert_uint64(0, ==, conn->tx.bidi.max_streams);
  assert_uint64(0, ==, conn->tx.uni.max_streams);
  assert_uint64(0, ==, conn->tx.offset);
  assert_uint64(0, ==, conn->tx.max_offset);
  assert_uint64(UINT64_MAX, ==, conn->tx.last_blocked_offset);
  assert_int64(0, ==, conn->tx.bidi.next_stream_id);
  assert_int64(2, ==, conn->tx.uni.next_stream_id);
  assert_size(0, ==, dwnx_map_size(&conn->strms));
  assert_true(dwnx_pq_empty(&conn->tx.strmq));
  assert_uint64(conn->rx.bidi.max_streams, ==,
                conn->rx.bidi.unsent_max_streams);
  assert_uint64(conn->rx.uni.max_streams, ==, conn->rx.uni.unsent_max_streams);
  assert_uint64(conn->rx.max_offset, ==, conn->rx.unsent_max_offset);

  dwnx_conn_del(conn);
}
