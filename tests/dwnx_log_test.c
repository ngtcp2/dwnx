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
#include "dwnx_log_test.h"

#include <stdio.h>

#include "dwnx_log.h"
#include "dwnx_macro.h"
#include "dwnx_test_helper.h"

static const MunitTest tests[] = {
  munit_void_test(test_dwnx_log_info),
  munit_void_test(test_dwnx_log_infof),
  munit_void_test(test_dwnx_log_fr),
  munit_void_test(test_dwnx_log_rcd),
  munit_test_end(),
};

const MunitSuite log_suite = {
  .prefix = "/log",
  .tests = tests,
};

typedef struct log_data {
  char buf[DWNX_LOG_BUFLEN];
  const char *expected[256];
  size_t idx;
} log_data;

static void log_write(void *user_data, char *msg, size_t len) {
  log_data *ld = user_data;

  assert_size(len, ==, strlen(msg));
  assert_size(dwnx_arraylen(ld->expected), >, ld->idx);
  assert_not_null(ld->expected[ld->idx]);
  assert_string_equal(ld->expected[ld->idx], msg);

  ++ld->idx;
}

static void log_init(dwnx_log *log, log_data *ld) {
  dwnx_log_init(log, 0xDEADBEEF, log_write, ld->buf, 0, ld);
  log->last_ts = DWNX_SECONDS + 123 * DWNX_MILLISECONDS;
}

void test_dwnx_log_info(void) {
  log_data ld;
  dwnx_log log;

  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef con message without formatting directive",
      },
  };

  log_init(&log, &ld);

  dwnx_log_infof(&log, DWNX_LOG_EVENT_CON,
                 "message without formatting directive");

  assert_null(ld.expected[ld.idx]);
}

void test_dwnx_log_infof(void) {
  log_data ld;
  dwnx_log log;

  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef con message with formatting directive "
        "888",
      },
  };

  log_init(&log, &ld);

  dwnx_log_infof(&log, DWNX_LOG_EVENT_CON, "message ", "with", " formatting ",
                 "directive", " ", 888);

  assert_null(ld.expected[ld.idx]);
}

void test_dwnx_log_fr(void) {
  log_data ld;
  dwnx_log log;
  uint8_t reason[66] = {0};

  memcpy(reason + sizeof(reason) - dwnx_strlen_lit("this is the reason") - 2,
         "this is the reason", dwnx_strlen_lit("this is the reason"));

  /* QX_TRANSPORT_PARAMETERS */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_stream_data_bidi_local=4294967296",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_stream_data_bidi_remote=4294967297",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_stream_data_uni=4294967298",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_data=4294967299",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_streams_bidi=4294967300",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "initial_max_streams_uni=4294967301",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "max_idle_timeout=18446744073709",
        "I00001123 0x00000000deadbeef frm rx "
        "QX_TRANSPORT_PARAMETERS(0x3f5153300d0a0d0a) "
        "max_record_size=4294967303",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log,
                 &(dwnx_frame){
                   .qx_transport_parameters =
                     {
                       .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
                       .params =
                         &(dwnx_transport_params){
                           .initial_max_stream_data_bidi_local = 4294967296,
                           .initial_max_stream_data_bidi_remote = 4294967297,
                           .initial_max_stream_data_uni = 4294967298,
                           .initial_max_data = 4294967299,
                           .initial_max_streams_bidi = 4294967300,
                           .initial_max_streams_uni = 4294967301,
                           .max_idle_timeout = UINT64_MAX - 1,
                           .max_record_size = 4294967303,
                         },
                     },
                 });

  assert_null(ld.expected[ld.idx]);

  /* STREAM (fin and uni) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx STREAM(0x9) id=0x3b9aca07 fin=1 "
        "offset=4852383 len=123 uni=1",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){.stream = {
                                       .type = DWNX_FRAME_STREAM,
                                       .flags = DWNX_STREAM_FIN_BIT,
                                       .fin = 1,
                                       .stream_id = 1000000007,
                                       .offset = 4852383,
                                       .len = 123,
                                     }});

  assert_null(ld.expected[ld.idx]);

  /* STREAM (bidi) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm tx STREAM(0x8) id=0x3b9aca09 fin=0 "
        "offset=4852383 len=123 uni=0",
      },
  };

  log_init(&log, &ld);

  dwnx_log_tx_fr(&log, &(dwnx_frame){.stream = {
                                       .type = DWNX_FRAME_STREAM,
                                       .stream_id = 1000000009,
                                       .offset = 4852383,
                                       .len = 123,
                                     }});

  assert_null(ld.expected[ld.idx]);

  /* PADDING */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx PADDING(0x0) len=99999",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .padding =
                           {
                             .type = DWNX_FRAME_PADDING,
                             .len = 99999,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* RESET_STREAM */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx RESET_STREAM(0x4) id=0x3b9aca09 "
        "app_error_code=(unknown)(0x66e2311a) final_size=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .reset_stream =
                           {
                             .type = DWNX_FRAME_RESET_STREAM,
                             .stream_id = 1000000009,
                             .app_error_code = 0x66E2311A,
                             .final_size = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* CONNECTION_CLOSE (transport) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx CONNECTION_CLOSE(0x1c) "
        "error_code=CONNECTION_REFUSED(0x2) frame_type=0x4 reason_len=0 "
        "reason=[]",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .connection_close =
                           {
                             .type = DWNX_FRAME_CONNECTION_CLOSE,
                             .error_code = DWNX_CONNECTION_REFUSED,
                             .frame_type = DWNX_FRAME_RESET_STREAM,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* CONNECTION_CLOSE (transport with reason) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx CONNECTION_CLOSE(0x1c) "
        "error_code=CONNECTION_REFUSED(0x2) frame_type=0x4 reason_len=66 "
        "reason=[.............................................."
        "this is the reason]",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .connection_close =
                           {
                             .type = DWNX_FRAME_CONNECTION_CLOSE,
                             .error_code = DWNX_CONNECTION_REFUSED,
                             .frame_type = DWNX_FRAME_RESET_STREAM,
                             .reasonlen = sizeof(reason),
                             .reason = reason,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* CONNECTION_CLOSE (application) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx CONNECTION_CLOSE(0x1d) "
        "error_code=(unknown)(0x2) frame_type=0x0 reason_len=0 "
        "reason=[]",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .connection_close =
                           {
                             .type = DWNX_FRAME_CONNECTION_CLOSE_APP,
                             .error_code = DWNX_CONNECTION_REFUSED,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* MAX_DATA */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx MAX_DATA(0x10) "
        "max_data=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .max_data =
                           {
                             .type = DWNX_FRAME_MAX_DATA,
                             .max_data = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* MAX_STREAM_DATA */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx MAX_STREAM_DATA(0x11) "
        "id=0x3b9aca09 max_stream_data=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .max_stream_data =
                           {
                             .type = DWNX_FRAME_MAX_STREAM_DATA,
                             .stream_id = 1000000009,
                             .max_stream_data = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* MAX_STREAMS (bidi) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx MAX_STREAMS(0x12) "
        "max_streams=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .max_streams =
                           {
                             .type = DWNX_FRAME_MAX_STREAMS_BIDI,
                             .max_streams = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* MAX_STREAMS (uni) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx MAX_STREAMS(0x13) "
        "max_streams=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .max_streams =
                           {
                             .type = DWNX_FRAME_MAX_STREAMS_UNI,
                             .max_streams = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* QX_PING (request) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx QX_PING(0x348c67529ef8c7bd) "
        "seq=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .qx_ping =
                           {
                             .type = DWNX_FRAME_QX_PING_REQUEST,
                             .seq = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* QX_PING (response) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx QX_PING(0x348c67529ef8c7be) "
        "seq=1000000009",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .qx_ping =
                           {
                             .type = DWNX_FRAME_QX_PING_RESPONSE,
                             .seq = 1000000009,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* DATA_BLOCKED */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx DATA_BLOCKED(0x14) "
        "max_data=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .data_blocked =
                           {
                             .type = DWNX_FRAME_DATA_BLOCKED,
                             .max_data = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* STREAM_DATA_BLOCKED */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx STREAM_DATA_BLOCKED(0x15) "
        "id=0x3b9aca09 max_stream_data=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .stream_data_blocked =
                           {
                             .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
                             .stream_id = 1000000009,
                             .max_stream_data = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* STREAMS_BLOCKED (bidi) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx STREAMS_BLOCKED(0x16) "
        "max_streams=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .streams_blocked =
                           {
                             .type = DWNX_FRAME_STREAMS_BLOCKED_BIDI,
                             .max_streams = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* STREAMS_BLOCKED (uni) */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx STREAMS_BLOCKED(0x17) "
        "max_streams=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .streams_blocked =
                           {
                             .type = DWNX_FRAME_STREAMS_BLOCKED_UNI,
                             .max_streams = 1000000007,
                           },
                       });

  assert_null(ld.expected[ld.idx]);

  /* STOP_SENDING */
  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef frm rx STOP_SENDING(0x5) "
        "id=0x3b9aca09 app_error_code=(unknown)(0xf)",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_fr(&log, &(dwnx_frame){
                         .stop_sending =
                           {
                             .type = DWNX_FRAME_STOP_SENDING,
                             .stream_id = 1000000009,
                             .app_error_code = 0xF,
                           },
                       });

  assert_null(ld.expected[ld.idx]);
}

void test_dwnx_log_rcd(void) {
  log_data ld;
  dwnx_log log;

  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef rcd rx record len=1000000007",
      },
  };

  log_init(&log, &ld);

  dwnx_log_rx_rcd(&log, 1000000007);

  assert_null(ld.expected[ld.idx]);

  ld = (log_data){
    .expected =
      {
        "I00001123 0x00000000deadbeef rcd tx record len=111",
      },
  };

  log_init(&log, &ld);

  dwnx_log_tx_rcd(&log, 111);

  assert_null(ld.expected[ld.idx]);
}
