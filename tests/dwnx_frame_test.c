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
#include "dwnx_frame_test.h"

#include <stdio.h>

#include "dwnx_frame.h"
#include "dwnx_conv.h"
#include "dwnx_macro.h"
#include "dwnx_test_helper.h"

static const MunitTest tests[] = {
  munit_void_test(test_dwnx_frame_encode_qx_transport_parameters),
  munit_void_test(test_dwnx_frame_encode_stream),
  munit_void_test(test_dwnx_frame_encode_reset_stream),
  munit_void_test(test_dwnx_frame_encode_stop_sending),
  munit_void_test(test_dwnx_frame_encode_max_data),
  munit_void_test(test_dwnx_frame_encode_max_stream_data),
  munit_void_test(test_dwnx_frame_encode_max_streams),
  munit_void_test(test_dwnx_frame_encode_data_blocked),
  munit_void_test(test_dwnx_frame_encode_stream_data_blocked),
  munit_void_test(test_dwnx_frame_encode_connection_close),
  munit_void_test(test_dwnx_frame_encode_qx_ping),
  munit_test_end(),
};

const MunitSuite frame_suite = {
  .prefix = "/frame",
  .tests = tests,
};

void test_dwnx_frame_encode_qx_transport_parameters(void) {
  uint8_t buf[256];
  static const dwnx_transport_params params = {
    .initial_max_stream_data_bidi_local = 1000000007,
    .initial_max_stream_data_bidi_remote = 961748941,
    .initial_max_stream_data_uni = 982451653,
    .initial_max_data = 1000000009,
    .initial_max_streams_bidi = 908,
    .initial_max_streams_uni = 16383,
    .max_idle_timeout = 16363 * DWNX_MILLISECONDS,
    .max_record_size = 5983223322,
  };
  dwnx_transport_params nparams;
  dwnx_frame_qx_transport_parameters fr;
  dwnx_frame_qx_transport_parameters nfr = {
    .params = &nparams,
  };
  dwnx_ssize nwrite, nread;
  size_t paramslen = (size_t)dwnx_transport_params_encode(NULL, 0, &params);
  size_t framelen = dwnx_put_uvarintlen(DWNX_FRAME_QX_TRANSPORT_PARAMETERS) +
                    dwnx_put_uvarintlen(paramslen) + paramslen;

  fr = (dwnx_frame_qx_transport_parameters){
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params = &params,
  };

  nwrite = dwnx_frame_encode_qx_transport_parameters(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nwrite);

  nread = dwnx_frame_decode_qx_transport_parameters(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(params.initial_max_stream_data_bidi_local, ==,
                nparams.initial_max_stream_data_bidi_local);
  assert_uint64(params.initial_max_stream_data_bidi_remote, ==,
                nparams.initial_max_stream_data_bidi_remote);
  assert_uint64(params.initial_max_stream_data_uni, ==,
                nparams.initial_max_stream_data_uni);
  assert_uint64(params.initial_max_data, ==, nparams.initial_max_data);
  assert_uint64(params.initial_max_streams_bidi, ==,
                nparams.initial_max_streams_bidi);
  assert_uint64(params.initial_max_streams_uni, ==,
                nparams.initial_max_streams_uni);
  assert_uint64(params.max_idle_timeout, ==, nparams.max_idle_timeout);
  assert_uint64(params.max_record_size, ==, nparams.max_record_size);

  nwrite = dwnx_frame_encode_qx_transport_parameters(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, nwrite);
}

void test_dwnx_frame_encode_stream(void) {
  static const uint8_t data[] = "0123456789abcdef0";
  uint8_t buf[256];
  dwnx_vec datav, ndatav;
  dwnx_frame_stream fr;
  dwnx_frame_stream nfr = {
    .data = &ndatav,
  };
  dwnx_ssize rv, nread;
  size_t framelen;

  /* 32 bits Stream ID + 62 bits Offset + Data Length */
  fr = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .stream_id = 0xF1F2F3F4U,
    .offset = 0x31F2F3F4F5F6F7F8ULL,
    .len = dwnx_strlen_lit(data),
    .datacnt = 1,
    .data = &datav,
  };
  datav = (dwnx_vec){
    .len = dwnx_strlen_lit(data),
    .base = (uint8_t *)data,
  };

  framelen = 1 + 8 + 8 + 1 + 17;

  rv = dwnx_frame_encode_stream(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_stream(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint8(DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT, ==, nfr.flags);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.datacnt, ==, nfr.datacnt);
  assert_size(fr.data[0].len, ==, nfr.data[0].len);
  assert_memory_equal(fr.data[0].len, fr.data[0].base, nfr.data[0].base);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  nfr = (dwnx_frame_stream){
    .data = &ndatav,
  };

  /* 6 bits Stream ID + No Offset + Data Length */
  fr = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .stream_id = 0x31,
    .len = dwnx_strlen_lit(data),
    .datacnt = 1,
    .data = &datav,
  };
  datav = (dwnx_vec){
    .len = dwnx_strlen_lit(data),
    .base = (uint8_t *)data,
  };

  framelen = 1 + 1 + 1 + 17;

  rv = dwnx_frame_encode_stream(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_stream(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint8(DWNX_STREAM_LEN_BIT, ==, nfr.flags);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.datacnt, ==, nfr.datacnt);
  assert_size(fr.data[0].len, ==, nfr.data[0].len);
  assert_memory_equal(fr.data[0].len, fr.data[0].base, nfr.data[0].base);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  nfr = (dwnx_frame_stream){
    .data = &ndatav,
  };

  /* Fin + 32 bits Stream ID + 62 bits Offset + Data Length */
  fr = (dwnx_frame_stream){
    .type = DWNX_FRAME_STREAM,
    .fin = 1,
    .stream_id = 0xF1F2F3F4U,
    .offset = 0x31F2F3F4F5F6F7F8ULL,
    .len = dwnx_strlen_lit(data),
    .datacnt = 1,
    .data = &datav,
  };
  datav = (dwnx_vec){
    .len = dwnx_strlen_lit(data),
    .base = (uint8_t *)data,
  };

  framelen = 1 + 8 + 8 + 1 + 17;

  rv = dwnx_frame_encode_stream(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_stream(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint8(DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT,
               ==, nfr.flags);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.datacnt, ==, nfr.datacnt);
  assert_size(fr.data[0].len, ==, nfr.data[0].len);
  assert_memory_equal(fr.data[0].len, fr.data[0].base, nfr.data[0].base);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_reset_stream(void) {
  uint8_t buf[32];
  dwnx_frame_reset_stream fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 4 + 4 + 8;

  fr = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 1000000007,
    .app_error_code = 0xE1E2,
    .final_size = 0x31F2F3F4F5F6F7F8ULL,
  };

  rv = dwnx_frame_encode_reset_stream(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_reset_stream(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.app_error_code, ==, nfr.app_error_code);
  assert_uint64(fr.final_size, ==, nfr.final_size);

  rv = dwnx_frame_encode_reset_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_stop_sending(void) {
  uint8_t buf[16];
  dwnx_frame_stop_sending fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8 + 4;

  fr = (dwnx_frame_stop_sending){
    .type = DWNX_FRAME_STOP_SENDING,
    .stream_id = 0xF1F2F3F4U,
    .app_error_code = 0xE1E2U,
  };

  rv = dwnx_frame_encode_stop_sending(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_stop_sending(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.app_error_code, ==, nfr.app_error_code);

  rv = dwnx_frame_encode_stop_sending(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_data(void) {
  uint8_t buf[16];
  dwnx_frame_max_data fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8;

  fr = (dwnx_frame_max_data){
    .type = DWNX_FRAME_MAX_DATA,
    .max_data = 0x31F2F3F4F5F6F7F8ULL,
  };

  rv = dwnx_frame_encode_max_data(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_max_data(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.max_data, ==, nfr.max_data);

  rv = dwnx_frame_encode_max_data(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_stream_data(void) {
  uint8_t buf[17];
  dwnx_frame_max_stream_data fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8 + 8;

  fr = (dwnx_frame_max_stream_data){
    .type = DWNX_FRAME_MAX_STREAM_DATA,
    .stream_id = 0xF1F2F3F4U,
    .max_stream_data = 0x35F6F7F8F9FAFBFCULL,
  };

  rv = dwnx_frame_encode_max_stream_data(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_max_stream_data(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.max_stream_data, ==, nfr.max_stream_data);

  rv = dwnx_frame_encode_max_stream_data(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_streams(void) {
  uint8_t buf[16];
  dwnx_frame_max_streams fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8;

  fr = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_BIDI,
    .max_streams = 0xF1F2F3F4U,
  };

  rv = dwnx_frame_encode_max_streams(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_max_streams(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.max_streams, ==, nfr.max_streams);

  rv = dwnx_frame_encode_max_streams(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_data_blocked(void) {
  uint8_t buf[16];
  dwnx_frame_data_blocked fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8;

  fr = (dwnx_frame_data_blocked){
    .type = DWNX_FRAME_DATA_BLOCKED,
    .offset = DWNX_MAX_VARINT,
  };

  rv = dwnx_frame_encode_data_blocked(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_data_blocked(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.offset, ==, nfr.offset);

  rv = dwnx_frame_encode_data_blocked(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_stream_data_blocked(void) {
  uint8_t buf[32];
  dwnx_frame_stream_data_blocked fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 8 + 8;

  fr = (dwnx_frame_stream_data_blocked){
    .type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .stream_id = (int64_t)(DWNX_MAX_VARINT >> 1),
    .offset = DWNX_MAX_VARINT,
  };

  rv = dwnx_frame_encode_stream_data_blocked(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_stream_data_blocked(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);

  rv = dwnx_frame_encode_stream_data_blocked(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_connection_close(void) {
  static const uint8_t reason[] = "hello world";
  uint8_t buf[256];
  dwnx_frame_connection_close fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 1 + 1 + 1 + 1 + dwnx_strlen_lit(reason);

  fr = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE,
    .error_code = DWNX_PROTOCOL_VIOLATION,
    .frame_type = DWNX_FRAME_STREAM_DATA_BLOCKED,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = reason,
  };

  rv = dwnx_frame_encode_connection_close(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_connection_close(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.error_code, ==, nfr.error_code);
  assert_uint64(fr.frame_type, ==, nfr.frame_type);
  assert_size(fr.reasonlen, ==, nfr.reasonlen);
  assert_memory_equal(fr.reasonlen, fr.reason, nfr.reason);

  rv = dwnx_frame_encode_connection_close(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  /* Without reason */
  framelen = 1 + 1 + 1 + 1;

  fr = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE,
    .error_code = DWNX_PROTOCOL_VIOLATION,
    .frame_type = DWNX_FRAME_STREAM_DATA_BLOCKED,
  };

  rv = dwnx_frame_encode_connection_close(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_connection_close(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.error_code, ==, nfr.error_code);
  assert_uint64(fr.frame_type, ==, nfr.frame_type);
  assert_size(fr.reasonlen, ==, nfr.reasonlen);
  assert_null(nfr.reason);

  rv = dwnx_frame_encode_connection_close(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  /* CONNECTION_CLOSE (0x1D) */
  framelen = 1 + 1 + 1 + dwnx_strlen_lit(reason);

  fr = (dwnx_frame_connection_close){
    .type = DWNX_FRAME_CONNECTION_CLOSE_APP,
    .error_code = DWNX_PROTOCOL_VIOLATION,
    .reasonlen = dwnx_strlen_lit(reason),
    .reason = reason,
  };

  rv = dwnx_frame_encode_connection_close(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_connection_close(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.error_code, ==, nfr.error_code);
  assert_size(fr.reasonlen, ==, nfr.reasonlen);
  assert_memory_equal(fr.reasonlen, fr.reason, nfr.reason);

  rv = dwnx_frame_encode_connection_close(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_qx_ping(void) {
  uint8_t buf[16];
  dwnx_frame_qx_ping fr, nfr;
  dwnx_ssize rv, nread;
  size_t framelen = 8 + 4;

  fr = (dwnx_frame_qx_ping){
    .type = DWNX_FRAME_QX_PING_REQUEST,
    .seq = 64111,
  };

  rv = dwnx_frame_encode_qx_ping(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  nread = dwnx_frame_decode_qx_ping(&nfr, buf, framelen);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nread);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.seq, ==, nfr.seq);

  rv = dwnx_frame_encode_qx_ping(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}
