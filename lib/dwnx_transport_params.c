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
#include "dwnx_transport_params.h"

#include <assert.h>

#include "dwnx_conv.h"

void dwnx_transport_params_default(dwnx_transport_params *params) {
  *params = (dwnx_transport_params){
    .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
  };
}

/*
 * varint_paramlen returns the length of a single transport parameter
 * which has variable integer in its parameter.
 */
static size_t varint_paramlen(dwnx_transport_param_id id, uint64_t param) {
  size_t valuelen = dwnx_put_uvarintlen(param);
  return dwnx_put_uvarintlen(id) + dwnx_put_uvarintlen(valuelen) + valuelen;
}

/*
 * write_varint_param writes parameter |id| of the given |value| in
 * varint encoding.  It returns p + the number of bytes written.
 */
static uint8_t *write_varint_param(uint8_t *p, dwnx_transport_param_id id,
                                   uint64_t value) {
  p = dwnx_put_uvarint(p, id);
  p = dwnx_put_uvarint(p, dwnx_put_uvarintlen(value));
  return dwnx_put_uvarint(p, value);
}

dwnx_ssize dwnx_transport_params_encode(uint8_t *dest, size_t destlen,
                                        const dwnx_transport_params *params) {
  uint8_t *p;
  size_t len = 0;

  if (params->initial_max_stream_data_bidi_local) {
    len +=
      varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
                      params->initial_max_stream_data_bidi_local);
  }

  if (params->initial_max_stream_data_bidi_remote) {
    len +=
      varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE,
                      params->initial_max_stream_data_bidi_remote);
  }

  if (params->initial_max_stream_data_uni) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_UNI,
                           params->initial_max_stream_data_uni);
  }

  if (params->initial_max_data) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA,
                           params->initial_max_data);
  }

  if (params->initial_max_streams_bidi) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI,
                           params->initial_max_streams_bidi);
  }

  if (params->initial_max_streams_uni) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI,
                           params->initial_max_streams_uni);
  }

  if (params->max_idle_timeout) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT,
                           params->max_idle_timeout / DWNX_MILLISECONDS);
  }

  if (params->max_record_size != DWNX_DEFAULT_MAX_RECORD_SIZE) {
    len += varint_paramlen(DWNX_TRANSPORT_PARAM_MAX_RECORD_SIZE,
                           params->max_record_size);
  }

  if (dest == NULL && destlen == 0) {
    return (dwnx_ssize)len;
  }

  if (destlen < len) {
    return DWNX_ERR_NOBUF;
  }

  p = dest;

  if (params->initial_max_stream_data_bidi_local) {
    p = write_varint_param(
      p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
      params->initial_max_stream_data_bidi_local);
  }

  if (params->initial_max_stream_data_bidi_remote) {
    p = write_varint_param(
      p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE,
      params->initial_max_stream_data_bidi_remote);
  }

  if (params->initial_max_stream_data_uni) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_UNI,
                           params->initial_max_stream_data_uni);
  }

  if (params->initial_max_data) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA,
                           params->initial_max_data);
  }

  if (params->initial_max_streams_bidi) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI,
                           params->initial_max_streams_bidi);
  }

  if (params->initial_max_streams_uni) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI,
                           params->initial_max_streams_uni);
  }

  if (params->max_idle_timeout) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT,
                           params->max_idle_timeout / DWNX_MILLISECONDS);
  }

  if (params->max_record_size != DWNX_DEFAULT_MAX_RECORD_SIZE) {
    p = write_varint_param(p, DWNX_TRANSPORT_PARAM_MAX_RECORD_SIZE,
                           params->max_record_size);
  }

  assert((size_t)(p - dest) == len);

  return (dwnx_ssize)len;
}

/*
 * decode_varint decodes a single varint from the buffer pointed by
 * |*pp| of length |end - *pp|.  If it decodes an integer
 * successfully, it stores the integer in |*pdest|, increment |*pp| by
 * the number of bytes read from |*pp|, and returns 0.  Otherwise it
 * returns -1.
 */
static int decode_varint(uint64_t *pdest, const uint8_t **pp,
                         const uint8_t *end) {
  const uint8_t *p = *pp;
  size_t len;

  if (p == end) {
    return -1;
  }

  len = dwnx_get_uvarintlen(p);
  if ((size_t)(end - p) < len) {
    return -1;
  }

  *pp = dwnx_get_uvarint(pdest, p);

  return 0;
}

/*
 * decode_varint_param decodes length prefixed value from the buffer
 * pointed by |*pp| of length |end - *pp|.  The length and value are
 * encoded in varint form.  If it decodes a value successfully, it
 * stores the value in |*pdest|, increment |*pp| by the number of
 * bytes read from |*pp|, and returns 0.  Otherwise it returns -1.
 */
static int decode_varint_param(uint64_t *pdest, const uint8_t **pp,
                               const uint8_t *end) {
  const uint8_t *p = *pp;
  uint64_t valuelen;

  if (decode_varint(&valuelen, &p, end) != 0) {
    return -1;
  }

  if (p == end) {
    return -1;
  }

  if ((uint64_t)(end - p) < valuelen) {
    return -1;
  }

  if (dwnx_get_uvarintlen(p) != valuelen) {
    return -1;
  }

  *pp = dwnx_get_uvarint(pdest, p);

  return 0;
}

int dwnx_transport_params_decode(dwnx_transport_params *dest,
                                 const uint8_t *data, size_t datalen) {
  const uint8_t *p, *end;
  uint64_t param_type;
  uint64_t valuelen;

  /* Set default values */
  *dest = (dwnx_transport_params){
    .max_record_size = DWNX_DEFAULT_MAX_RECORD_SIZE,
  };

  p = end = data;

  if (datalen) {
    end += datalen;
  }

  for (; (size_t)(end - p) >= 2;) {
    if (decode_varint(&param_type, &p, end) != 0) {
      return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
    }

    switch (param_type) {
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL:
      if (decode_varint_param(&dest->initial_max_stream_data_bidi_local, &p,
                              end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE:
      if (decode_varint_param(&dest->initial_max_stream_data_bidi_remote, &p,
                              end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_UNI:
      if (decode_varint_param(&dest->initial_max_stream_data_uni, &p, end) !=
          0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA:
      if (decode_varint_param(&dest->initial_max_data, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI:
      if (decode_varint_param(&dest->initial_max_streams_bidi, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      if (dest->initial_max_streams_bidi > DWNX_MAX_STREAMS) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI:
      if (decode_varint_param(&dest->initial_max_streams_uni, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      if (dest->initial_max_streams_uni > DWNX_MAX_STREAMS) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT:
      if (decode_varint_param(&dest->max_idle_timeout, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      if (dest->max_idle_timeout > UINT64_MAX / DWNX_MILLISECONDS) {
        dest->max_idle_timeout = UINT64_MAX;
      }
      dest->max_idle_timeout *= DWNX_MILLISECONDS;
      break;
    case DWNX_TRANSPORT_PARAM_MAX_RECORD_SIZE:
      if (decode_varint_param(&dest->max_record_size, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      break;
    case DWNX_TRANSPORT_PARAM_MAX_UDP_PAYLOAD_SIZE:
    case DWNX_TRANSPORT_PARAM_STATELESS_RESET_TOKEN:
    case DWNX_TRANSPORT_PARAM_ACK_DELAY_EXPONENT:
    case DWNX_TRANSPORT_PARAM_PREFERRED_ADDRESS:
    case DWNX_TRANSPORT_PARAM_DISABLE_ACTIVE_MIGRATION:
    case DWNX_TRANSPORT_PARAM_ORIGINAL_DESTINATION_CONNECTION_ID:
    case DWNX_TRANSPORT_PARAM_RETRY_SOURCE_CONNECTION_ID:
    case DWNX_TRANSPORT_PARAM_INITIAL_SOURCE_CONNECTION_ID:
    case DWNX_TRANSPORT_PARAM_MAX_ACK_DELAY:
    case DWNX_TRANSPORT_PARAM_ACTIVE_CONNECTION_ID_LIMIT:
      return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
    default:
      /* Ignore unknown parameter */
      if (decode_varint(&valuelen, &p, end) != 0) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      if ((size_t)(end - p) < valuelen) {
        return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
      }
      p += valuelen;
      break;
    }
  }

  if (end - p != 0) {
    return DWNX_ERR_MALFORMED_TRANSPORT_PARAM;
  }

  return 0;
}
