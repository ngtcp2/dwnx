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
#ifndef DWNX_CONN_H
#define DWNX_CONN_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_record_reader.h"

#define DWNX_CONN_FLAG_QX_TRANSPORT_PARAMETERS_SEEN 0x01U

struct dwnx_conn {
  const dwnx_mem *mem;
  void *user_data;

  struct {
    dwnx_transport_params transport_params;
  } local;

  struct {
    dwnx_transport_params transport_params;
  } remote;

  struct {
    dwnx_varint_reader vird;
    dwnx_record_reader rcrd;
  } rx;

  uint32_t flags;
  int server;
};

int dwnx_conn_recv_transport_params(dwnx_conn *conn, const uint8_t *data,
                                    size_t datalen);

#endif /* !defined(DWNX_CONN_H) */
