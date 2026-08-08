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
#ifndef DWNX_LOG_H
#define DWNX_LOG_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_frame.h"
#include "dwnx_fmt.h"

#define DWNX_LOG_BUFLEN 1024

typedef struct dwnx_log {
  dwnx_log_write log_write;
  uint64_t conn_id;
  /* events is an event filter.  Only events set in this field are
     emitted. */
  uint8_t events;
  /* ts is the time point used to write time delta in the log. */
  dwnx_tstamp ts;
  /* last_ts is the most recent time point that this object is
     told. */
  dwnx_tstamp last_ts;
  /* user_data is user-defined opaque data which is passed to
     log_write. */
  void *user_data;
  /* conn_id is the identifier of the connection so that we can
     distinguish the logs of the particular connection from the
     others. */
  char *buf;
} dwnx_log;

/**
 * @enum
 *
 * :type:`dwnx_log_event` defines an event of dwnx library
 * internal logger.
 */
typedef enum dwnx_log_event {
  /**
   * :enum:`DWNX_LOG_EVENT_NONE` represents no event.
   */
  DWNX_LOG_EVENT_NONE,
  /**
   * :enum:`DWNX_LOG_EVENT_CON` is a connection (catch-all) event
   */
  DWNX_LOG_EVENT_CON = 0x1,
  /**
   * :enum:`DWNX_LOG_EVENT_RCD` is a QMux record event.
   */
  DWNX_LOG_EVENT_RCD = 0x2,
  /**
   * :enum:`DWNX_LOG_EVENT_FRM` is a QUIC frame event.
   */
  DWNX_LOG_EVENT_FRM = 0x4,
} dwnx_log_event;

void dwnx_log_init(dwnx_log *log, uint64_t conn_id, dwnx_log_write log_write,
                   char *buf, dwnx_tstamp ts, void *user_data);

void dwnx_log_rx_fr(dwnx_log *log, const dwnx_frame *fr);

void dwnx_log_tx_fr(dwnx_log *log, const dwnx_frame *fr);

void dwnx_log_rx_rcd(dwnx_log *log, size_t len);

void dwnx_log_tx_rcd(dwnx_log *log, size_t len);

uint64_t dwnx_log_timestamp(const dwnx_log *log);

static inline const char *dwnx_log_event_str(dwnx_log_event ev) {
  switch (ev) {
  case DWNX_LOG_EVENT_CON:
    return "con";
  case DWNX_LOG_EVENT_RCD:
    return "rcd";
  case DWNX_LOG_EVENT_FRM:
    return "frm";
  case DWNX_LOG_EVENT_NONE:
  default:
    return "non";
  }
}

#define DWNX_LOG_HD(LOG, EV)                                                   \
  "I", uintw(dwnx_log_timestamp(LOG), 8), " 0x",                               \
    hexw((LOG)->conn_id, sizeof((LOG)->conn_id) * 2), " ",                     \
    dwnx_log_event_str(EV), " "

#define dwnx_log_infof_raw(LOG, EV, ...)                                       \
  do {                                                                         \
    size_t log_nwrite;                                                         \
                                                                               \
    dwnx_fmt_format((LOG)->buf, &log_nwrite, DWNX_LOG_HD((LOG), (EV)),         \
                    __VA_ARGS__);                                              \
    (LOG)->log_write((LOG)->user_data, (LOG)->buf, log_nwrite);                \
  } while (0)

#define dwnx_log_infof(LOG, EV, ...)                                           \
  do {                                                                         \
    if (!(LOG)->log_write || !((LOG)->events & (EV))) {                        \
      break;                                                                   \
    }                                                                          \
                                                                               \
    dwnx_log_infof_raw((LOG), (EV), __VA_ARGS__);                              \
  } while (0)

#define dwnx_log_info(LOG, EV, ARG) dwnx_log_infof((LOG), (EV), (ARG))

#endif /* !defined(DWNX_LOG_H) */
