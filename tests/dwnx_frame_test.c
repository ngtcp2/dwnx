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
  dwnx_frame_qx_transport_parameters fr, nfr;
  dwnx_ssize nwrite;
  const uint8_t *p;
  size_t paramslen = (size_t)dwnx_transport_params_encode(NULL, 0, &params);
  uint64_t nparamslen;
  size_t framelen = dwnx_put_uvarintlen(DWNX_FRAME_QX_TRANSPORT_PARAMETERS) +
                    dwnx_put_uvarintlen(paramslen) + paramslen;
  int rv;

  fr = (dwnx_frame_qx_transport_parameters){
    .type = DWNX_FRAME_QX_TRANSPORT_PARAMETERS,
    .params = &params,
  };

  nwrite = dwnx_frame_encode_qx_transport_parameters(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, nwrite);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_uvarint(&nparamslen, p);

  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(paramslen, ==, nparamslen);

  rv = dwnx_transport_params_decode(&nparams, p, framelen - (size_t)(p - buf));

  assert_int(0, ==, rv);
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
  dwnx_vec datav;
  dwnx_frame_stream fr, nfr;
  dwnx_ssize rv;
  const uint8_t *p;
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

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.offset, p);
  p = dwnx_get_uvarint(&nfr.len, p);

  assert_uint64(fr.type | DWNX_STREAM_OFF_BIT | DWNX_STREAM_LEN_BIT, ==,
                nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.data[0].len, ==, (size_t)(buf + framelen - p));
  assert_memory_equal(fr.data[0].len, fr.data[0].base, p);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  nfr = (dwnx_frame_stream){0};

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

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.len, p);

  assert_uint64(fr.type | DWNX_STREAM_LEN_BIT, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.data[0].len, ==, (size_t)(buf + framelen - p));
  assert_memory_equal(fr.data[0].len, fr.data[0].base, p);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);

  nfr = (dwnx_frame_stream){0};

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

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.offset, p);
  p = dwnx_get_uvarint(&nfr.len, p);

  assert_uint64(fr.type | DWNX_STREAM_FIN_BIT | DWNX_STREAM_OFF_BIT |
                  DWNX_STREAM_LEN_BIT,
                ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.offset, ==, nfr.offset);
  assert_uint64(fr.len, ==, nfr.len);
  assert_size(fr.data[0].len, ==, (size_t)(buf + framelen - p));
  assert_memory_equal(fr.data[0].len, fr.data[0].base, p);

  rv = dwnx_frame_encode_stream(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_reset_stream(void) {
  uint8_t buf[32];
  dwnx_frame_reset_stream fr, nfr;
  dwnx_ssize rv;
  const uint8_t *p;
  size_t framelen = 1 + 4 + 4 + 8;

  fr = (dwnx_frame_reset_stream){
    .type = DWNX_FRAME_RESET_STREAM,
    .stream_id = 1000000007,
    .app_error_code = 0xE1E2,
    .final_size = 0x31F2F3F4F5F6F7F8ULL,
  };

  rv = dwnx_frame_encode_reset_stream(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.app_error_code, p);
  p = dwnx_get_uvarint(&nfr.final_size, p);

  assert_ptr_equal(buf + framelen, p);
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
  dwnx_ssize rv;
  const uint8_t *p;
  size_t framelen = 1 + 8 + 4;

  fr = (dwnx_frame_stop_sending){
    .type = DWNX_FRAME_STOP_SENDING,
    .stream_id = 0xF1F2F3F4U,
    .app_error_code = 0xE1E2U,
  };

  rv = dwnx_frame_encode_stop_sending(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.app_error_code, p);

  assert_ptr_equal(buf + framelen, p);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.app_error_code, ==, nfr.app_error_code);

  rv = dwnx_frame_encode_stop_sending(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_data(void) {
  uint8_t buf[16];
  dwnx_frame_max_data fr, nfr;
  dwnx_ssize rv;
  const uint8_t *p;
  size_t framelen = 1 + 8;

  fr = (dwnx_frame_max_data){
    .type = DWNX_FRAME_MAX_DATA,
    .max_data = 0x31F2F3F4F5F6F7F8ULL,
  };

  rv = dwnx_frame_encode_max_data(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_uvarint(&nfr.max_data, p);

  assert_ptr_equal(buf + framelen, p);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.max_data, ==, nfr.max_data);

  rv = dwnx_frame_encode_max_data(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_stream_data(void) {
  uint8_t buf[17];
  dwnx_frame_max_stream_data fr, nfr;
  dwnx_ssize rv;
  const uint8_t *p;
  size_t framelen = 1 + 8 + 8;

  fr = (dwnx_frame_max_stream_data){
    .type = DWNX_FRAME_MAX_STREAM_DATA,
    .stream_id = 0xF1F2F3F4U,
    .max_stream_data = 0x35F6F7F8F9FAFBFCULL,
  };

  rv = dwnx_frame_encode_max_stream_data(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_varint(&nfr.stream_id, p);
  p = dwnx_get_uvarint(&nfr.max_stream_data, p);

  assert_ptr_equal(buf + framelen, p);
  assert_uint64(fr.type, ==, nfr.type);
  assert_int64(fr.stream_id, ==, nfr.stream_id);
  assert_uint64(fr.max_stream_data, ==, nfr.max_stream_data);

  rv = dwnx_frame_encode_max_stream_data(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}

void test_dwnx_frame_encode_max_streams(void) {
  uint8_t buf[16];
  dwnx_frame_max_streams fr, nfr;
  dwnx_ssize rv;
  const uint8_t *p;
  size_t framelen = 1 + 8;

  fr = (dwnx_frame_max_streams){
    .type = DWNX_FRAME_MAX_STREAMS_BIDI,
    .max_streams = 0xF1F2F3F4U,
  };

  rv = dwnx_frame_encode_max_streams(buf, sizeof(buf), &fr);

  assert_ptrdiff((dwnx_ssize)framelen, ==, rv);

  p = buf;

  p = dwnx_get_uvarint(&nfr.type, p);
  p = dwnx_get_uvarint(&nfr.max_streams, p);

  assert_ptr_equal(buf + framelen, p);
  assert_uint64(fr.type, ==, nfr.type);
  assert_uint64(fr.max_streams, ==, nfr.max_streams);

  rv = dwnx_frame_encode_max_streams(buf, framelen - 1, &fr);

  assert_ptrdiff(DWNX_ERR_NOBUF, ==, rv);
}
