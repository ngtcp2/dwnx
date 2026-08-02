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
#ifndef DWNX_TRANSPORT_PARAMS_H
#define DWNX_TRANSPORT_PARAMS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

/* dwnx_transport_param_id is the registry of QUIC transport
   parameter ID. */
typedef uint64_t dwnx_transport_param_id;

/* QUIC transport parameters */
#define DWNX_TRANSPORT_PARAM_MAX_IDLE_TIMEOUT 0x01U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_DATA 0x04U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL 0x05U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE 0x06U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAM_DATA_UNI 0x07U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_BIDI 0x08U
#define DWNX_TRANSPORT_PARAM_INITIAL_MAX_STREAMS_UNI 0x09U

/* Prohibited QUIC transport parameters */
#define DWNX_TRANSPORT_PARAM_ORIGINAL_DESTINATION_CONNECTION_ID 0x00U
#define DWNX_TRANSPORT_PARAM_STATELESS_RESET_TOKEN 0x02U
#define DWNX_TRANSPORT_PARAM_MAX_UDP_PAYLOAD_SIZE 0x03U
#define DWNX_TRANSPORT_PARAM_ACK_DELAY_EXPONENT 0x0AU
#define DWNX_TRANSPORT_PARAM_MAX_ACK_DELAY 0x0BU
#define DWNX_TRANSPORT_PARAM_DISABLE_ACTIVE_MIGRATION 0x0CU
#define DWNX_TRANSPORT_PARAM_PREFERRED_ADDRESS 0x0DU
#define DWNX_TRANSPORT_PARAM_ACTIVE_CONNECTION_ID_LIMIT 0x0EU
#define DWNX_TRANSPORT_PARAM_INITIAL_SOURCE_CONNECTION_ID 0x0FU
#define DWNX_TRANSPORT_PARAM_RETRY_SOURCE_CONNECTION_ID 0x10U

/* QMux transport parameters */
#define DWNX_TRANSPORT_PARAM_MAX_RECORD_SIZE 0x0571C59429CD0845ULL

/* DWNX_MAX_STREAMS is the maximum number of streams. */
#define DWNX_MAX_STREAMS (1LL << 60)

dwnx_ssize dwnx_transport_params_encode(uint8_t *dest, size_t destlen,
                                        const dwnx_transport_params *params);

int dwnx_transport_params_decode(dwnx_transport_params *dest,
                                 const uint8_t *data, size_t datalen);

#endif /* !defined(DWNX_TRANSPORT_PARAMS_H) */
