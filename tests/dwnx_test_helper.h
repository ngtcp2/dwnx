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
#ifndef DWNX_TEST_HELPER_H
#define DWNX_TEST_HELPER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#define MUNIT_ENABLE_ASSERT_ALIASES

#include "munit.h"

#include "dwnx_buf.h"
#include "dwnx_frame.h"

typedef struct dwnx_conn dwnx_conn;

#define dwnx_check_recordlen(BUF, LEN)                                         \
  do {                                                                         \
    size_t n;                                                                  \
    uint64_t reclen;                                                           \
                                                                               \
    assert_size(0, <, dwnx_buf_len((BUF)));                                    \
                                                                               \
    n = dwnx_get_uvarintlen((BUF)->pos);                                       \
                                                                               \
    assert_size(dwnx_buf_len((BUF)), >, n);                                    \
                                                                               \
    (BUF)->pos = (uint8_t *)dwnx_get_uvarint(&reclen, (BUF)->pos);             \
    assert_uint64((uint64_t)(LEN), ==, reclen);                                \
  } while (0);

void dwnx_write_frame(dwnx_buf *dest, const dwnx_frame *fr);

void dwnx_write_record(dwnx_buf *dest, const dwnx_frame *fr, size_t n);

void dwnx_read_transport_params(dwnx_conn *conn,
                                const dwnx_transport_params *remote_params,
                                dwnx_tstamp ts);

const uint8_t *dwnx_read_recordlen(uint64_t *plen, const uint8_t *data,
                                   size_t datalen);

int64_t dwnx_nth_local_bidi_stream_id(dwnx_conn *conn, uint64_t n);

int64_t dwnx_nth_remote_bidi_stream_id(dwnx_conn *conn, uint64_t n);

int64_t dwnx_nth_local_uni_stream_id(dwnx_conn *conn, uint64_t n);

int64_t dwnx_nth_remote_uni_stream_id(dwnx_conn *conn, uint64_t n);

#endif /* !defined(DWNX_TEST_HELPER_H) */
