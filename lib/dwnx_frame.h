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
#ifndef DWNX_FRAME_H
#define DWNX_FRAME_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#define DWNX_FRAME_QX_TRANSPORT_PARAMETERS 0x3F5153300D0A0D0AULL

#define DWNX_FRAME_PADDING 0x00ULL
#define DWNX_FRAME_RESET_STREAM 0x04ULL
#define DWNX_FRAME_STOP_SENDING 0x05ULL
#define DWNX_FRAME_STREAM 0x08ULL
#define DWNX_FRAME_MAX_DATA 0x10ULL
#define DWNX_FRAME_MAX_STREAM_DATA 0x11ULL
#define DWNX_FRAME_MAX_STREAMS_BIDI 0x12ULL
#define DWNX_FRAME_MAX_STREAMS_UNI 0x13ULL
#define DWNX_FRAME_DATA_BLOCKED 0x14ULL
#define DWNX_FRAME_STREAM_DATA_BLOCKED 0x15ULL
#define DWNX_FRAME_STREAMS_BLOCKED_BIDI 0x16ULL
#define DWNX_FRAME_STREAMS_BLOCKED_UNI 0x17ULL
#define DWNX_FRAME_CONNECTION_CLOSE 0x1CULL
#define DWNX_FRAME_CONNECTION_CLOSE_APP 0x1DULL

/* STREAM frame specific macros */
#define DWNX_STREAM_FIN_BIT 0x01U
#define DWNX_STREAM_LEN_BIT 0x02U
#define DWNX_STREAM_OFF_BIT 0x04U

typedef struct dwnx_frame_hd {
  uint64_t type;
} dwnx_frame_hd;

typedef struct dwnx_frame_qx_transport_parameters {
  uint64_t type;
  dwnx_transport_params *params;
} dwnx_frame_qx_transport_parameters;

typedef struct dwnx_frame_stream {
  uint64_t type;
  uint8_t flags;
  int fin;
  int64_t stream_id;
  uint64_t offset;
  size_t len;
} dwnx_frame_stream;

typedef struct dwnx_frame_reset_stream {
  uint64_t type;
  int64_t stream_id;
  uint64_t app_error_code;
  uint64_t final_size;
} dwnx_frame_reset_stream;

typedef struct dwnx_frame_stop_sending {
  uint64_t type;
  int64_t stream_id;
  uint64_t app_error_code;
} dwnx_frame_stop_sending;

typedef union dwnx_frame {
  dwnx_frame_hd hd;
  dwnx_frame_qx_transport_parameters qx_transport_parameters;
  dwnx_frame_stream stream;
  dwnx_frame_reset_stream reset_stream;
  dwnx_frame_stop_sending stop_sending;
} dwnx_frame;

#endif /* !defined(DWNX_FRAME_H) */
