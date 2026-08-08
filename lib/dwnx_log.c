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
#include "dwnx_log.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* defined(HAVE_UNISTD_H) */
#include <assert.h>
#include <string.h>

#include "dwnx_str.h"
#include "dwnx_vec.h"
#include "dwnx_macro.h"
#include "dwnx_conv.h"
#include "dwnx_unreachable.h"

void dwnx_log_init(dwnx_log *log, uint64_t conn_id, dwnx_log_write log_write,
                   char *buf, dwnx_tstamp ts, void *user_data) {
  log->conn_id = conn_id;
  log->log_write = log_write;
  log->events = 0xFF;
  log->ts = log->last_ts = ts;
  log->user_data = user_data;
  log->buf = buf;
}

/*
 * # Log header
 *
 * <LEVEL><TIMESTAMP> <CONN_ID> <EVENT>
 *
 * <LEVEL>:
 *   Log level.  I=Info, W=Warning, E=Error
 *
 * <TIMESTAMP>:
 *   Timestamp relative to dwnx_log.ts field in milliseconds
 *   resolution.
 *
 * <CONN_ID>:
 *   Connection ID in hex string.  This is not a QUIC Connection ID.
 *
 * <EVENT>:
 *   Event.  See dwnx_log_event.
 *
 * # Frame event
 *
 * <DIR> <FRAMENAME>(<FRAMETYPE>)
 *
 * <DIR>:
 *   Flow direction.  tx=transmission, rx=reception
 *
 * <FRAMENAME>:
 *   Frame name.  (e.g., STREAM, ACK, PING)
 *
 * <FRAMETYPE>:
 *   Frame type in hex string.
 */

static const char *strerrorcode(uint64_t error_code) {
  switch (error_code) {
  case DWNX_NO_ERROR:
    return "NO_ERROR";
  case DWNX_INTERNAL_ERROR:
    return "INTERNAL_ERROR";
  case DWNX_CONNECTION_REFUSED:
    return "CONNECTION_REFUSED";
  case DWNX_FLOW_CONTROL_ERROR:
    return "FLOW_CONTROL_ERROR";
  case DWNX_STREAM_LIMIT_ERROR:
    return "STREAM_LIMIT_ERROR";
  case DWNX_STREAM_STATE_ERROR:
    return "STREAM_STATE_ERROR";
  case DWNX_FINAL_SIZE_ERROR:
    return "FINAL_SIZE_ERROR";
  case DWNX_FRAME_ENCODING_ERROR:
    return "FRAME_ENCODING_ERROR";
  case DWNX_TRANSPORT_PARAMETER_ERROR:
    return "TRANSPORT_PARAMETER_ERROR";
  case DWNX_PROTOCOL_VIOLATION:
    return "PROTOCOL_VIOLATION";
  case DWNX_APPLICATION_ERROR:
    return "APPLICATION_ERROR";
  default:
    return "(unknown)";
  }
}

static const char *strapperrorcode(uint64_t app_error_code) {
  (void)app_error_code;
  return "(unknown)";
}

static void
log_fr_qx_transport_parameters(dwnx_log *log,
                               const dwnx_frame_qx_transport_parameters *fr,
                               const char *dir) {
  const dwnx_transport_params *params = fr->params;

#define DWNX_LOG_TP_HD(DIR, FR)                                                \
  (DIR), " QX_TRANSPORT_PARAMETERS(0x", hex((FR)->type), ") "

  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
                     "initial_max_stream_data_bidi_local=",
                     params->initial_max_stream_data_bidi_local);
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
                     "initial_max_stream_data_bidi_remote=",
                     params->initial_max_stream_data_bidi_remote);
  dwnx_log_infof_raw(
    log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
    "initial_max_stream_data_uni=", params->initial_max_stream_data_uni);
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
                     "initial_max_data=", params->initial_max_data);
  dwnx_log_infof_raw(
    log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
    "initial_max_streams_bidi=", params->initial_max_streams_bidi);
  dwnx_log_infof_raw(
    log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
    "initial_max_streams_uni=", params->initial_max_streams_uni);
  dwnx_log_infof_raw(
    log, DWNX_LOG_EVENT_FRM, DWNX_LOG_TP_HD(dir, fr),
    "max_idle_timeout=", params->max_idle_timeout / DWNX_MILLISECONDS);
#undef DWNX_LOG_TP_HD
}

static void log_fr_qx_ping(dwnx_log *log, const dwnx_frame_qx_ping *fr,
                           const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " QX_PING(0x", hex(fr->type),
                     ") seq=", fr->seq);
}

static void log_fr_padding(dwnx_log *log, const dwnx_frame_padding *fr,
                           const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " PADDING(0x", hex(fr->type),
                     ") len=", fr->len);
}

static void log_fr_reset_stream(dwnx_log *log,
                                const dwnx_frame_reset_stream *fr,
                                const char *dir) {
  dwnx_log_infof_raw(
    log, DWNX_LOG_EVENT_FRM, dir, " RESET_STREAM(0x", hex(fr->type), ") id=0x",
    hex(fr->stream_id), " app_error_code=", strapperrorcode(fr->app_error_code),
    "(0x", hex(fr->app_error_code), ") final_size=", fr->final_size);
}

static void log_fr_stop_sending(dwnx_log *log,
                                const dwnx_frame_stop_sending *fr,
                                const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " STOP_SENDING(0x",
                     hex(fr->type), ") id=0x", hex(fr->stream_id),
                     " app_error_code=", strapperrorcode(fr->app_error_code),
                     "(0x", hex(fr->app_error_code), ")");
}

static void log_fr_stream(dwnx_log *log, const dwnx_frame_stream *fr,
                          const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " STREAM(0x",
                     hex(fr->type | fr->flags), ") id=0x", hex(fr->stream_id),
                     " fin=", fr->fin, " offset=", fr->offset, " len=", fr->len,
                     " uni=", (fr->stream_id & 0x2) != 0);
}

static void log_fr_max_data(dwnx_log *log, const dwnx_frame_max_data *fr,
                            const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " MAX_DATA(0x",
                     hex(fr->type), ") max_data=", fr->max_data);
}

static void log_fr_max_stream_data(dwnx_log *log,
                                   const dwnx_frame_max_stream_data *fr,
                                   const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " MAX_STREAM_DATA(0x",
                     hex(fr->type), ") id=0x", hex(fr->stream_id),
                     " max_stream_data=", fr->max_stream_data);
}

static void log_fr_max_streams(dwnx_log *log, const dwnx_frame_max_streams *fr,
                               const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " MAX_STREAMS(0x",
                     hex(fr->type), ") max_streams=", fr->max_streams);
}

static void log_fr_data_blocked(dwnx_log *log,
                                const dwnx_frame_data_blocked *fr,
                                const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " DATA_BLOCKED(0x",
                     hex(fr->type), ") offset=", fr->offset);
}

static void log_fr_stream_data_blocked(dwnx_log *log,
                                       const dwnx_frame_stream_data_blocked *fr,
                                       const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " STREAM_DATA_BLOCKED(0x",
                     hex(fr->type), ") id=0x", hex(fr->stream_id),
                     " offset=", fr->offset);
}

static void log_fr_streams_blocked(dwnx_log *log,
                                   const dwnx_frame_streams_blocked *fr,
                                   const char *dir) {
  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " STREAMS_BLOCKED(0x",
                     hex(fr->type), ") max_streams=", fr->max_streams);
}

static void log_fr_connection_close(dwnx_log *log,
                                    const dwnx_frame_connection_close *fr,
                                    const char *dir) {
  size_t reasonlen = dwnx_min(64, fr->reasonlen);

  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_FRM, dir, " CONNECTION_CLOSE(0x",
                     hex(fr->type), ") error_code=",
                     fr->type == DWNX_FRAME_CONNECTION_CLOSE
                       ? strerrorcode(fr->error_code)
                       : strapperrorcode(fr->error_code),
                     "(0x", hex(fr->error_code), ") frame_type=0x",
                     hex(fr->frame_type), " reason_len=", fr->reasonlen,
                     " reason=[", ascii(fr->reason, reasonlen), "]");
}

static void log_fr(dwnx_log *log, const dwnx_frame *fr, const char *dir) {
  switch (fr->hd.type) {
  case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
    log_fr_qx_transport_parameters(log, &fr->qx_transport_parameters, dir);
    break;
  case DWNX_FRAME_QX_PING_REQUEST:
  case DWNX_FRAME_QX_PING_RESPONSE:
    log_fr_qx_ping(log, &fr->qx_ping, dir);
    break;
  case DWNX_FRAME_PADDING:
    log_fr_padding(log, &fr->padding, dir);
    break;
  case DWNX_FRAME_RESET_STREAM:
    log_fr_reset_stream(log, &fr->reset_stream, dir);
    break;
  case DWNX_FRAME_STOP_SENDING:
    log_fr_stop_sending(log, &fr->stop_sending, dir);
    break;
  case DWNX_FRAME_STREAM:
    log_fr_stream(log, &fr->stream, dir);
    break;
  case DWNX_FRAME_MAX_DATA:
    log_fr_max_data(log, &fr->max_data, dir);
    break;
  case DWNX_FRAME_MAX_STREAM_DATA:
    log_fr_max_stream_data(log, &fr->max_stream_data, dir);
    break;
  case DWNX_FRAME_MAX_STREAMS_BIDI:
  case DWNX_FRAME_MAX_STREAMS_UNI:
    log_fr_max_streams(log, &fr->max_streams, dir);
    break;
  case DWNX_FRAME_DATA_BLOCKED:
    log_fr_data_blocked(log, &fr->data_blocked, dir);
    break;
  case DWNX_FRAME_STREAM_DATA_BLOCKED:
    log_fr_stream_data_blocked(log, &fr->stream_data_blocked, dir);
    break;
  case DWNX_FRAME_STREAMS_BLOCKED_BIDI:
  case DWNX_FRAME_STREAMS_BLOCKED_UNI:
    log_fr_streams_blocked(log, &fr->streams_blocked, dir);
    break;
  case DWNX_FRAME_CONNECTION_CLOSE:
  case DWNX_FRAME_CONNECTION_CLOSE_APP:
    log_fr_connection_close(log, &fr->connection_close, dir);
    break;
  default:
    dwnx_unreachable();
  }
}

void dwnx_log_rx_fr(dwnx_log *log, const dwnx_frame *fr) {
  if (!log->log_write || !(log->events & DWNX_LOG_EVENT_FRM)) {
    return;
  }

  log_fr(log, fr, "rx");
}

void dwnx_log_tx_fr(dwnx_log *log, const dwnx_frame *fr) {
  if (!log->log_write || !(log->events & DWNX_LOG_EVENT_FRM)) {
    return;
  }

  log_fr(log, fr, "tx");
}

static void log_rcd(dwnx_log *log, size_t len, const char *dir) {
  if (!log->log_write || !(log->events & DWNX_LOG_EVENT_RCD)) {
    return;
  }

  dwnx_log_infof_raw(log, DWNX_LOG_EVENT_RCD, dir, " record len=", len);
}

void dwnx_log_rx_rcd(dwnx_log *log, size_t len) { log_rcd(log, len, "rx"); }

void dwnx_log_tx_rcd(dwnx_log *log, size_t len) { log_rcd(log, len, "tx"); }

uint64_t dwnx_log_timestamp(const dwnx_log *log) {
  return (log->last_ts - log->ts) / DWNX_MILLISECONDS;
}
