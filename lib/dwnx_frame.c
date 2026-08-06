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
#include "dwnx_frame.h"

#include <assert.h>

#include "dwnx_conv.h"
#include "dwnx_vec.h"
#include "dwnx_str.h"
#include "dwnx_transport_params.h"
#include "dwnx_macro.h"
#include "dwnx_unreachable.h"

dwnx_ssize dwnx_frame_encode(uint8_t *out, size_t outlen,
                             const dwnx_frame *fr) {
  switch (fr->hd.type) {
  case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
    return dwnx_frame_encode_qx_transport_parameters(
      out, outlen, &fr->qx_transport_parameters);
  case DWNX_FRAME_RESET_STREAM:
    return dwnx_frame_encode_reset_stream(out, outlen, &fr->reset_stream);
  case DWNX_FRAME_STOP_SENDING:
    return dwnx_frame_encode_stop_sending(out, outlen, &fr->stop_sending);
  case DWNX_FRAME_STREAM:
    return dwnx_frame_encode_stream(out, outlen, &fr->stream);
  case DWNX_FRAME_MAX_DATA:
    return dwnx_frame_encode_max_data(out, outlen, &fr->max_data);
  case DWNX_FRAME_MAX_STREAM_DATA:
    return dwnx_frame_encode_max_stream_data(out, outlen, &fr->max_stream_data);
  case DWNX_FRAME_MAX_STREAMS_BIDI:
  case DWNX_FRAME_MAX_STREAMS_UNI:
    return dwnx_frame_encode_max_streams(out, outlen, &fr->max_streams);
  default:
    dwnx_unreachable();
  }
}

dwnx_ssize dwnx_frame_encode_qx_transport_parameters(
  uint8_t *out, size_t outlen, const dwnx_frame_qx_transport_parameters *fr) {
  size_t paramslen = (size_t)dwnx_transport_params_encode(NULL, 0, fr->params);
  size_t len =
    dwnx_put_uvarintlen(fr->type) + dwnx_put_uvarintlen(paramslen) + paramslen;
  uint8_t *p;
  dwnx_ssize nwrite;
  (void)nwrite;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  p = dwnx_put_uvarint(p, fr->type);
  p = dwnx_put_uvarint(p, paramslen);

  nwrite = dwnx_transport_params_encode(p, paramslen, fr->params);

  assert(nwrite >= 0);
  assert((size_t)nwrite == paramslen);

  p += nwrite;

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_stream(uint8_t *out, size_t outlen,
                                    const dwnx_frame_stream *fr) {
  size_t len = 1;
  uint8_t flags = DWNX_STREAM_LEN_BIT;
  uint8_t *p;
  size_t i;
  size_t left, nwrite;

  if (fr->fin) {
    flags |= DWNX_STREAM_FIN_BIT;
  }

  if (fr->offset) {
    flags |= DWNX_STREAM_OFF_BIT;
    len += dwnx_put_uvarintlen(fr->offset);
  }

  len += dwnx_put_uvarintlen((uint64_t)fr->stream_id);
  len += dwnx_put_uvarintlen(fr->len);
  len += fr->len;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = flags | DWNX_FRAME_STREAM;

  p = dwnx_put_uvarint(p, (uint64_t)fr->stream_id);

  if (fr->offset) {
    p = dwnx_put_uvarint(p, fr->offset);
  }

  p = dwnx_put_uvarint(p, fr->len);

  left = fr->len;

  for (i = 0; i < fr->datacnt && left; ++i) {
    if (fr->data[i].len == 0) {
      continue;
    }

    nwrite = dwnx_min(left, fr->data[i].len);
    p = dwnx_cpymem(p, fr->data[i].base, nwrite);
    left -= nwrite;
  }

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_reset_stream(uint8_t *out, size_t outlen,
                                          const dwnx_frame_reset_stream *fr) {
  size_t len = 1 + dwnx_put_uvarintlen((uint64_t)fr->stream_id) +
               dwnx_put_uvarintlen(fr->app_error_code) +
               dwnx_put_uvarintlen(fr->final_size);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_RESET_STREAM;
  p = dwnx_put_uvarint(p, (uint64_t)fr->stream_id);
  p = dwnx_put_uvarint(p, fr->app_error_code);
  p = dwnx_put_uvarint(p, fr->final_size);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_stop_sending(uint8_t *out, size_t outlen,
                                          const dwnx_frame_stop_sending *fr) {
  size_t len = 1 + dwnx_put_uvarintlen((uint64_t)fr->stream_id) +
               dwnx_put_uvarintlen(fr->app_error_code);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_STOP_SENDING;
  p = dwnx_put_uvarint(p, (uint64_t)fr->stream_id);
  p = dwnx_put_uvarint(p, fr->app_error_code);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_max_data(uint8_t *out, size_t outlen,
                                      const dwnx_frame_max_data *fr) {
  size_t len = 1 + dwnx_put_uvarintlen(fr->max_data);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_MAX_DATA;
  p = dwnx_put_uvarint(p, fr->max_data);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize
dwnx_frame_encode_max_stream_data(uint8_t *out, size_t outlen,
                                  const dwnx_frame_max_stream_data *fr) {
  size_t len = 1 + dwnx_put_uvarintlen((uint64_t)fr->stream_id) +
               dwnx_put_uvarintlen(fr->max_stream_data);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_MAX_STREAM_DATA;
  p = dwnx_put_uvarint(p, (uint64_t)fr->stream_id);
  p = dwnx_put_uvarint(p, fr->max_stream_data);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_max_streams(uint8_t *out, size_t outlen,
                                         const dwnx_frame_max_streams *fr) {
  size_t len = 1 + dwnx_put_uvarintlen(fr->max_streams);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = (uint8_t)fr->type;
  p = dwnx_put_uvarint(p, fr->max_streams);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}
