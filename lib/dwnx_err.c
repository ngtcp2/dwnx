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
#include "dwnx_err.h"

int dwnx_err_is_fatal(int liberr) { return liberr < DWNX_ERR_FATAL; }

uint64_t dwnx_err_infer_quic_transport_error_code(int liberr) {
  switch (liberr) {
  case 0:
    return DWNX_NO_ERROR;
  case DWNX_ERR_FRAME_ENCODING:
    return DWNX_FRAME_ENCODING_ERROR;
  case DWNX_ERR_FLOW_CONTROL:
    return DWNX_FLOW_CONTROL_ERROR;
  case DWNX_ERR_STREAM_LIMIT:
    return DWNX_STREAM_LIMIT_ERROR;
  case DWNX_ERR_FINAL_SIZE:
    return DWNX_FINAL_SIZE_ERROR;
  case DWNX_ERR_REQUIRED_TRANSPORT_PARAM:
  case DWNX_ERR_MALFORMED_TRANSPORT_PARAM:
  case DWNX_ERR_TRANSPORT_PARAM:
    return DWNX_TRANSPORT_PARAMETER_ERROR;
  case DWNX_ERR_INVALID_ARGUMENT:
  case DWNX_ERR_NOMEM:
  case DWNX_ERR_CALLBACK_FAILURE:
  case DWNX_ERR_INTERNAL:
    return DWNX_INTERNAL_ERROR;
  case DWNX_ERR_STREAM_STATE:
    return DWNX_STREAM_STATE_ERROR;
  default:
    return DWNX_PROTOCOL_VIOLATION;
  }
}
