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
  case DWNX_FRAME_QX_PING_REQUEST:
  case DWNX_FRAME_QX_PING_RESPONSE:
    return dwnx_frame_encode_qx_ping(out, outlen, &fr->qx_ping);
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
  case DWNX_FRAME_DATA_BLOCKED:
    return dwnx_frame_encode_data_blocked(out, outlen, &fr->data_blocked);
  case DWNX_FRAME_STREAM_DATA_BLOCKED:
    return dwnx_frame_encode_stream_data_blocked(out, outlen,
                                                 &fr->stream_data_blocked);
  case DWNX_FRAME_CONNECTION_CLOSE:
  case DWNX_FRAME_CONNECTION_CLOSE_APP:
    return dwnx_frame_encode_connection_close(out, outlen,
                                              &fr->connection_close);
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

dwnx_ssize dwnx_frame_encode_qx_ping(uint8_t *out, size_t outlen,
                                     const dwnx_frame_qx_ping *fr) {
  size_t len = dwnx_put_uvarintlen(fr->type) + dwnx_put_uvarintlen(fr->seq);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  p = dwnx_put_uvarint(p, fr->type);
  p = dwnx_put_uvarint(p, fr->seq);

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

dwnx_ssize dwnx_frame_encode_data_blocked(uint8_t *out, size_t outlen,
                                          const dwnx_frame_data_blocked *fr) {
  size_t len = 1 + dwnx_put_uvarintlen(fr->offset);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_DATA_BLOCKED;
  p = dwnx_put_uvarint(p, fr->offset);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_encode_stream_data_blocked(
  uint8_t *out, size_t outlen, const dwnx_frame_stream_data_blocked *fr) {
  size_t len = 1 + dwnx_put_uvarintlen((uint64_t)fr->stream_id) +
               dwnx_put_uvarintlen(fr->offset);
  uint8_t *p;

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = DWNX_FRAME_STREAM_DATA_BLOCKED;
  p = dwnx_put_uvarint(p, (uint64_t)fr->stream_id);
  p = dwnx_put_uvarint(p, fr->offset);

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize
dwnx_frame_encode_connection_close(uint8_t *out, size_t outlen,
                                   const dwnx_frame_connection_close *fr) {
  size_t len = 1 + dwnx_put_uvarintlen(fr->error_code) +
               dwnx_put_uvarintlen(fr->reasonlen) + fr->reasonlen;
  uint8_t *p;

  if (fr->type == DWNX_FRAME_CONNECTION_CLOSE) {
    len += dwnx_put_uvarintlen(fr->frame_type);
  }

  if (outlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = out;

  *p++ = (uint8_t)fr->type;
  p = dwnx_put_uvarint(p, fr->error_code);

  if (fr->type == DWNX_FRAME_CONNECTION_CLOSE) {
    p = dwnx_put_uvarint(p, fr->frame_type);
  }

  p = dwnx_put_uvarint(p, fr->reasonlen);

  if (fr->reasonlen) {
    p = dwnx_cpymem(p, fr->reason, fr->reasonlen);
  }

  assert((size_t)(p - out) == len);

  return (dwnx_ssize)len;
}

void dwnx_frd_init(dwnx_frd *frd) { (void)frd; }

dwnx_ssize dwnx_frd_decode_buf(dwnx_frd *frd, dwnx_frame *dest,
                               dwnx_buf *payload) {
  dwnx_ssize nread;

  nread = dwnx_frd_decode(frd, dest, payload->pos, dwnx_buf_len(payload));
  if (nread > 0) {
    payload->pos += nread;
  }

  return nread;
}

dwnx_ssize dwnx_frd_decode(dwnx_frd *frd, dwnx_frame *dest,
                           const uint8_t *payload, size_t payloadlen) {
  uint64_t long_type;
  size_t vintlen;
  uint8_t type;

  if (payloadlen == 0) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  type = payload[0];

  switch (type) {
  case DWNX_FRAME_PADDING:
    return dwnx_frame_decode_padding(&dest->padding, payload, payloadlen);
  case DWNX_FRAME_RESET_STREAM:
    return dwnx_frame_decode_reset_stream(&dest->reset_stream, payload,
                                          payloadlen);
  case DWNX_FRAME_STOP_SENDING:
    return dwnx_frame_decode_stop_sending(&dest->stop_sending, payload,
                                          payloadlen);
  case DWNX_FRAME_MAX_DATA:
    return dwnx_frame_decode_max_data(&dest->max_data, payload, payloadlen);
  case DWNX_FRAME_MAX_STREAM_DATA:
    return dwnx_frame_decode_max_stream_data(&dest->max_stream_data, payload,
                                             payloadlen);
  case DWNX_FRAME_MAX_STREAMS_BIDI:
  case DWNX_FRAME_MAX_STREAMS_UNI:
    return dwnx_frame_decode_max_streams(&dest->max_streams, payload,
                                         payloadlen);
  case DWNX_FRAME_DATA_BLOCKED:
    return dwnx_frame_decode_data_blocked(&dest->data_blocked, payload,
                                          payloadlen);
  case DWNX_FRAME_STREAM_DATA_BLOCKED:
    return dwnx_frame_decode_stream_data_blocked(&dest->stream_data_blocked,
                                                 payload, payloadlen);
  case DWNX_FRAME_STREAMS_BLOCKED_BIDI:
  case DWNX_FRAME_STREAMS_BLOCKED_UNI:
    return dwnx_frame_decode_streams_blocked(&dest->streams_blocked, payload,
                                             payloadlen);
  case DWNX_FRAME_CONNECTION_CLOSE:
  case DWNX_FRAME_CONNECTION_CLOSE_APP:
    return dwnx_frame_decode_connection_close(&dest->connection_close, payload,
                                              payloadlen);
  default:
    if ((type & ~(DWNX_FRAME_STREAM - 1)) == DWNX_FRAME_STREAM) {
      dest->stream.data = &frd->buf.data;

      return dwnx_frame_decode_stream(&dest->stream, payload, payloadlen);
    }

    /* For frame types > 0xFF, use dwnx_get_uvarintlen and
       dwnx_get_uvarint to get a frame type, and then switch over it.
       Verify that payloadlen >= dwnx_get_uvarintlen(payload) before
       calling dwnx_get_uvarint(payload). */
    vintlen = dwnx_get_uvarintlen(payload);
    if (vintlen > payloadlen) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    dwnx_get_uvarint(&long_type, payload);

    switch (long_type) {
    case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
      dest->qx_transport_parameters.params = &frd->buf.params;

      return dwnx_frame_decode_qx_transport_parameters(
        &dest->qx_transport_parameters, payload, payloadlen);
    case DWNX_FRAME_QX_PING_REQUEST:
    case DWNX_FRAME_QX_PING_RESPONSE:
      return dwnx_frame_decode_qx_ping(&dest->qx_ping, payload, payloadlen);
    default:
      return DWNX_ERR_FRAME_ENCODING;
    }
  }
}

dwnx_ssize dwnx_frame_decode_padding(dwnx_frame_padding *dest,
                                     const uint8_t *payload,
                                     size_t payloadlen) {
  const uint8_t *p, *ep;

  assert(payloadlen > 0);

  p = payload + 1;
  ep = payload + payloadlen;

  for (; p != ep && *p == DWNX_FRAME_PADDING; ++p)
    ;

  dest->type = DWNX_FRAME_PADDING;
  dest->len = (size_t)(p - payload);

  return (dwnx_ssize)dest->len;
}

dwnx_ssize dwnx_frame_decode_qx_transport_parameters(
  dwnx_frame_qx_transport_parameters *dest, const uint8_t *payload,
  size_t payloadlen) {
  size_t len = 1 + 1;
  uint64_t vi;
  size_t n;
  size_t paramslen;
  const uint8_t *p;
  int rv;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  /* p = */ dwnx_get_uvarint(&vi, p);

  if (payloadlen - len < vi) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  paramslen = (size_t)vi;
  len += paramslen;

  p = payload;

  p = dwnx_get_uvarint(&dest->type, p);
  p += n;

  rv = dwnx_transport_params_decode((dwnx_transport_params *)dest->params, p,
                                    paramslen);
  if (rv != 0) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += paramslen;

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_qx_ping(dwnx_frame_qx_ping *dest,
                                     const uint8_t *payload,
                                     size_t payloadlen) {
  size_t len = 1 + 1;
  size_t n;
  const uint8_t *p;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload;

  p = dwnx_get_uvarint(&dest->type, p);
  p = dwnx_get_uvarint(&dest->seq, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_stream(dwnx_frame_stream *dest,
                                    const uint8_t *payload, size_t payloadlen) {
  uint8_t type;
  size_t len = 1 + 1;
  const uint8_t *p;
  size_t datalen = 0;
  size_t ndatalen = 0;
  size_t n;
  uint64_t vi;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  type = payload[0];

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  if (type & DWNX_STREAM_OFF_BIT) {
    ++len;
    if (payloadlen < len) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    n = dwnx_get_uvarintlen(p);
    len += n - 1;

    if (payloadlen < len) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    p += n;
  }

  if (type & DWNX_STREAM_LEN_BIT) {
    ++len;
    if (payloadlen < len) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    ndatalen = dwnx_get_uvarintlen(p);
    len += ndatalen - 1;

    if (payloadlen < len) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    /* p = */ dwnx_get_uvarint(&vi, p);
    if (payloadlen - len < vi) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    datalen = (size_t)vi;
    len += datalen;
  } else {
    len = payloadlen;
  }

  p = payload + 1;

  dest->type = DWNX_FRAME_STREAM;
  dest->flags = (uint8_t)(type & ~DWNX_FRAME_STREAM);
  dest->fin = (type & DWNX_STREAM_FIN_BIT) != 0;
  p = dwnx_get_varint(&dest->stream_id, p);

  if (type & DWNX_STREAM_OFF_BIT) {
    p = dwnx_get_uvarint(&dest->offset, p);
  } else {
    dest->offset = 0;
  }

  if (type & DWNX_STREAM_LEN_BIT) {
    p += ndatalen;
  } else {
    datalen = payloadlen - (size_t)(p - payload);
  }

  dest->len = datalen;

  if (datalen) {
    *(dwnx_vec *)&dest->data[0] = (dwnx_vec){
      .base = (uint8_t *)p,
      .len = datalen,
    };
    dest->datacnt = 1;
    p += datalen;
  } else {
    dest->datacnt = 0;
  }

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_reset_stream(dwnx_frame_reset_stream *dest,
                                          const uint8_t *payload,
                                          size_t payloadlen) {
  size_t len = 1 + 1 + 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;
  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;
  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  dest->type = DWNX_FRAME_RESET_STREAM;
  p = dwnx_get_varint(&dest->stream_id, p);
  p = dwnx_get_uvarint(&dest->app_error_code, p);
  p = dwnx_get_uvarint(&dest->final_size, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_stop_sending(dwnx_frame_stop_sending *dest,
                                          const uint8_t *payload,
                                          size_t payloadlen) {
  size_t len = 1 + 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;
  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  dest->type = DWNX_FRAME_STOP_SENDING;
  p = dwnx_get_varint(&dest->stream_id, p);
  p = dwnx_get_uvarint(&dest->app_error_code, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_max_data(dwnx_frame_max_data *dest,
                                      const uint8_t *payload,
                                      size_t payloadlen) {
  size_t len = 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  dest->type = DWNX_FRAME_MAX_DATA;
  p = dwnx_get_uvarint(&dest->max_data, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_max_stream_data(dwnx_frame_max_stream_data *dest,
                                             const uint8_t *payload,
                                             size_t payloadlen) {
  size_t len = 1 + 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  dest->type = DWNX_FRAME_MAX_STREAM_DATA;
  p = dwnx_get_varint(&dest->stream_id, p);
  p = dwnx_get_uvarint(&dest->max_stream_data, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_max_streams(dwnx_frame_max_streams *dest,
                                         const uint8_t *payload,
                                         size_t payloadlen) {
  size_t len = 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  dest->type = payload[0];
  p = dwnx_get_uvarint(&dest->max_streams, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_data_blocked(dwnx_frame_data_blocked *dest,
                                          const uint8_t *payload,
                                          size_t payloadlen) {
  size_t len = 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  dest->type = DWNX_FRAME_DATA_BLOCKED;
  p = dwnx_get_uvarint(&dest->offset, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize
dwnx_frame_decode_stream_data_blocked(dwnx_frame_stream_data_blocked *dest,
                                      const uint8_t *payload,
                                      size_t payloadlen) {
  size_t len = 1 + 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  dest->type = DWNX_FRAME_STREAM_DATA_BLOCKED;
  p = dwnx_get_varint(&dest->stream_id, p);
  p = dwnx_get_uvarint(&dest->offset, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_streams_blocked(dwnx_frame_streams_blocked *dest,
                                             const uint8_t *payload,
                                             size_t payloadlen) {
  size_t len = 1 + 1;
  const uint8_t *p;
  size_t n;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  dest->type = payload[0];
  p = dwnx_get_uvarint(&dest->max_streams, p);

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}

dwnx_ssize dwnx_frame_decode_connection_close(dwnx_frame_connection_close *dest,
                                              const uint8_t *payload,
                                              size_t payloadlen) {
  size_t len = 1 + 1 + 1;
  const uint8_t *p;
  size_t reasonlen;
  size_t nreasonlen;
  size_t n;
  uint8_t type;
  uint64_t vi;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  type = payload[0];

  p = payload + 1;

  n = dwnx_get_uvarintlen(p);
  len += n - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  p += n;

  if (type == DWNX_FRAME_CONNECTION_CLOSE) {
    ++len;

    n = dwnx_get_uvarintlen(p);
    len += n - 1;

    if (payloadlen < len) {
      return DWNX_ERR_FRAME_ENCODING;
    }

    p += n;
  }

  nreasonlen = dwnx_get_uvarintlen(p);
  len += nreasonlen - 1;

  if (payloadlen < len) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  dwnx_get_uvarint(&vi, p);
  if (payloadlen - len < vi) {
    return DWNX_ERR_FRAME_ENCODING;
  }

  reasonlen = (size_t)vi;
  len += reasonlen;

  p = payload + 1;

  dest->type = type;
  p = dwnx_get_uvarint(&dest->error_code, p);

  if (type == DWNX_FRAME_CONNECTION_CLOSE) {
    p = dwnx_get_uvarint(&dest->frame_type, p);
  } else {
    dest->frame_type = 0;
  }

  dest->reasonlen = reasonlen;
  p += nreasonlen;

  if (reasonlen == 0) {
    dest->reason = NULL;
  } else {
    dest->reason = (uint8_t *)p;
    p += reasonlen;
  }

  assert((size_t)(p - payload) == len);

  return (dwnx_ssize)len;
}
