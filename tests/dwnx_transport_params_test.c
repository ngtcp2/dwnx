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
#include "dwnx_transport_params_test.h"

#include <stdio.h>

#include "dwnx_transport_params.h"
#include "dwnx_conv.h"
#include "dwnx_test_helper.h"

static const MunitTest tests[] = {
  munit_void_test(test_dwnx_transport_params_encode),
  munit_void_test(test_dwnx_transport_params_decode),
  munit_test_end(),
};

const MunitSuite transport_params_suite = {
  .prefix = "/transport_params",
  .tests = tests,
};

static size_t varint_paramlen(dwnx_transport_param_id id, uint64_t value) {
  size_t valuelen = dwnx_put_uvarintlen(value);
  return dwnx_put_uvarintlen(id) + dwnx_put_uvarintlen(valuelen) + valuelen;
}

void test_dwnx_transport_params_encode(void) {
  dwnx_transport_params params, nparams = {0};
  uint8_t buf[512];
  dwnx_ssize nwrite;
  int rv;
  size_t i, len;

  params = (dwnx_transport_params){
    .initial_max_stream_data_bidi_local = 1000000007,
    .initial_max_stream_data_bidi_remote = 961748941,
    .initial_max_stream_data_uni = 982451653,
    .initial_max_data = 1000000009,
    .initial_max_streams_bidi = 908,
    .initial_max_streams_uni = 16383,
    .max_idle_timeout = 16363 * DWNX_MILLISECONDS,
    .max_record_size = 5983223322,
  };

  len =
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
                    params.initial_max_stream_data_bidi_local) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE,
                    params.initial_max_stream_data_bidi_remote) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_UNI,
                    params.initial_max_stream_data_uni) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA,
                    params.initial_max_data) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI,
                    params.initial_max_streams_bidi) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI,
                    params.initial_max_streams_uni) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT,
                    params.max_idle_timeout / DWNX_MILLISECONDS) +
    varint_paramlen(DWNX_TRANSPORT_PARAM_MAX_RECORD_SIZE,
                    params.max_record_size);

  nwrite = dwnx_transport_params_encode(NULL, 0, &params);

  assert_ptrdiff((dwnx_ssize)len, ==, nwrite);

  for (i = 0; i < len; ++i) {
    nwrite = dwnx_transport_params_encode(buf, i, &params);

    assert_ptrdiff(DWNX_ERR_NOBUF, ==, nwrite);
  }
  nwrite = dwnx_transport_params_encode(buf, i, &params);

  assert_ptrdiff((dwnx_ssize)i, ==, nwrite);

  for (i = 0; i < len; ++i) {
    rv = dwnx_transport_params_decode(&nparams, buf, i);

    assert(0 == rv || DWNX_ERR_MALFORMED_TRANSPORT_PARAM == rv);
  }

  rv = dwnx_transport_params_decode(&nparams, buf, len);

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
}

void test_dwnx_transport_params_decode(void) {
  dwnx_transport_params params;
  int rv;

  /* Decode from 0 length data */
  rv = dwnx_transport_params_decode(&params, NULL, 0);

  assert_int(0, ==, rv);
  assert_uint64(0, ==, params.initial_max_stream_data_bidi_local);
  assert_uint64(0, ==, params.initial_max_stream_data_bidi_remote);
  assert_uint64(0, ==, params.initial_max_stream_data_uni);
  assert_uint64(0, ==, params.initial_max_data);
  assert_uint64(0, ==, params.initial_max_streams_bidi);
  assert_uint64(0, ==, params.initial_max_streams_uni);
  assert_uint64(0, ==, params.max_idle_timeout);
  assert_uint64(DWNX_DEFAULT_MAX_RECORD_SIZE, ==, params.max_record_size);
}
