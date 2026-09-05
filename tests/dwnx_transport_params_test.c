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
#include "dwnx_str.h"
#include "dwnx_macro.h"
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
  static const uint64_t prohibited_params[] = {
    DWNX_TRANSPORT_PARAM_ORIGINAL_DESTINATION_CONNECTION_ID,
    DWNX_TRANSPORT_PARAM_STATELESS_RESET_TOKEN,
    DWNX_TRANSPORT_PARAM_MAX_UDP_PAYLOAD_SIZE,
    DWNX_TRANSPORT_PARAM_ACK_DELAY_EXPONENT,
    DWNX_TRANSPORT_PARAM_MAX_ACK_DELAY,
    DWNX_TRANSPORT_PARAM_DISABLE_ACTIVE_MIGRATION,
    DWNX_TRANSPORT_PARAM_PREFERRED_ADDRESS,
    DWNX_TRANSPORT_PARAM_ACTIVE_CONNECTION_ID_LIMIT,
    DWNX_TRANSPORT_PARAM_INITIAL_SOURCE_CONNECTION_ID,
    DWNX_TRANSPORT_PARAM_RETRY_SOURCE_CONNECTION_ID,
  };
  dwnx_transport_params params, src;
  uint8_t rawbuf[1024];
  dwnx_buf buf;
  dwnx_ssize nwrite;
  size_t i;
  int rv;

  dwnx_buf_init(&buf, rawbuf, sizeof(rawbuf));

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

  /* Transport parameter is prematurely truncated inside type */
  dwnx_buf_reset(&buf);

  buf.last = dwnx_put_uvarint(buf.last, DWNX_MAX_VARINT);
  --buf.last;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* initial_max_streams_bidi is larger than DWNX_MAX_STREAMS */
  dwnx_transport_params_default(&src);
  src.initial_max_streams_bidi = DWNX_MAX_STREAMS + 1;

  dwnx_buf_reset(&buf);
  nwrite = dwnx_transport_params_encode(buf.last, dwnx_buf_left(&buf), &src);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* initial_max_streams_uni is larger than or DWNX_MAX_STREAMS */
  dwnx_transport_params_default(&src);
  src.initial_max_streams_uni = DWNX_MAX_STREAMS + 1;

  dwnx_buf_reset(&buf);
  nwrite = dwnx_transport_params_encode(buf.last, dwnx_buf_left(&buf), &src);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* maximum max_idle_timeout */
  dwnx_transport_params_default(&src);
  src.max_idle_timeout = 18446744073709 * DWNX_MILLISECONDS;

  dwnx_buf_reset(&buf);
  nwrite = dwnx_transport_params_encode(buf.last, dwnx_buf_left(&buf), &src);

  assert_ptrdiff(0, <, nwrite);

  buf.last += nwrite;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(0, ==, rv);
  assert_uint64(18446744073709 * DWNX_MILLISECONDS, ==,
                params.max_idle_timeout);

  /* max_idle_timeout is too large, overflows uint64_t when multiplied
     by DWNX_MILLISECONDS. */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT);
  buf.last = dwnx_put_uvarint(buf.last, dwnx_put_uvarintlen(18446744073710));
  buf.last = dwnx_put_uvarint(buf.last, 18446744073710);

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(0, ==, rv);
  assert_uint64(18446744073709 * DWNX_MILLISECONDS, ==,
                params.max_idle_timeout);

  /* Ignore unknown transport parameters */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, 0xDEADBEEF);
  buf.last = dwnx_put_uvarint(buf.last, 100);
  buf.last = dwnx_setmem(buf.last, 0, 100);
  buf.last = dwnx_put_uvarint(buf.last, 0xCACECAFE);
  buf.last = dwnx_put_uvarint(buf.last, 0);
  buf.last = dwnx_put_uvarint(buf.last, DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA);
  buf.last = dwnx_put_uvarint(buf.last, dwnx_put_uvarintlen(1000000007));
  buf.last = dwnx_put_uvarint(buf.last, 1000000007);

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(0, ==, rv);
  assert_uint64(1000000007, ==, params.initial_max_data);

  /* Prematurely truncated unknown transport parameter value length */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, 0xDEADBEEF);
  dwnx_put_uvarint(buf.last, 100);
  ++buf.last;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* The value of unknown transport parameter is truncated */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, 0xDEADBEEF);
  buf.last = dwnx_put_uvarint(buf.last, 78);
  buf.last = dwnx_setmem(buf.last, 0, 77);

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* The length of transport parameter value is missing */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, 0xDEADBEEF);

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* The value length of max_data does not match the encoded
     integer */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(buf.last, DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA);
  buf.last = dwnx_put_uvarint(buf.last, 2);
  buf.last = dwnx_put_uvarint(buf.last, 63);
  *buf.last++ = 0;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* Prematurely truncated length of Connection ID parameter */
  dwnx_buf_reset(&buf);
  buf.last = dwnx_put_uvarint(
    buf.last, DWNX_TRANSPORT_PARAM_ORIGINAL_DESTINATION_CONNECTION_ID);
  dwnx_put_uvarint(buf.last, 100);
  ++buf.last;

  rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

  assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);

  /* prohibited transport parameters */
  for (i = 0; i < dwnx_arraylen(prohibited_params); ++i) {
    dwnx_buf_reset(&buf);
    buf.last = dwnx_put_uvarint(buf.last, prohibited_params[i]);
    *buf.last++ = 0;

    rv = dwnx_transport_params_decode(&params, buf.pos, dwnx_buf_len(&buf));

    assert_int(DWNX_ERR_MALFORMED_TRANSPORT_PARAM, ==, rv);
  }
}
