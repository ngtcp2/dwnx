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
#include "client_base.h"

#include <cassert>
#include <array>
#include <fstream>

#include "debug.h"
#include "template.h"
#include "util.h"

using namespace dwnx;
using namespace std::literals;

extern Config config;

ClientBase::ClientBase() { dwnx_ccerr_default(&last_error_); }

ClientBase::~ClientBase() {
  dwnx_conn_del(conn_);

  if (qlog_) {
    fclose(qlog_);
  }
}

dwnx_conn *ClientBase::conn() const { return conn_; }

void qlog_write_cb(void *user_data, uint32_t flags, const void *data,
                   size_t datalen) {
  auto c = static_cast<ClientBase *>(user_data);
  c->write_qlog(data, datalen);
}

void ClientBase::write_qlog(const void *data, size_t datalen) {
  assert(qlog_);
  fwrite(data, 1, datalen, qlog_);
}

void ClientBase::ticket_received() { ticket_received_ = true; }
