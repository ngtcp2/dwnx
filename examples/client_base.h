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
#ifndef CLIENT_BASE_H
#define CLIENT_BASE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#include <vector>
#include <deque>
#include <string>
#include <string_view>
#include <functional>
#include <filesystem>

#include <openssl/ssl.h>

#include <dwnx/dwnx.h>

#include "network.h"
#include "shared.h"
#include "util.h"

using namespace dwnx;

struct Request {
  std::string_view scheme;
  std::string authority;
  std::string path;
};

struct Config {
  bool scid_present{};
  // tx_loss_prob is probability of losing outgoing packet.
  double tx_loss_prob{};
  // rx_loss_prob is probability of losing incoming packet.
  double rx_loss_prob{};
  // fd is a file descriptor to read input for streams.
  int fd{-1};
  // ciphers is the list of enabled ciphers.
  const char *ciphers{util::crypto_default_ciphers()};
  // groups is the list of supported groups.
  const char *groups{util::crypto_default_groups()};
  // nstreams is the number of streams to open.
  size_t nstreams{};
  // data is the pointer to memory region which maps file denoted by
  // fd.
  uint8_t *data{};
  // datalen is the length of file denoted by fd.
  size_t datalen{};
  // quiet suppresses the output normally shown except for the error
  // messages.
  bool quiet{};
  // timeout is an idle timeout for QUIC connection.
  dwnx_duration timeout{30 * DWNX_SECONDS};
  // session_file is a path to a file to write, and read TLS session.
  std::filesystem::path session_file;
  // tp_file is a path to a file to write, and read QUIC transport
  // parameters.
  std::filesystem::path tp_file;
  std::string_view http_method{"GET"sv};
  // download is a path to a directory where a downloaded file is
  // saved.  If it is empty, no file is saved.
  std::filesystem::path download;
  // requests contains URIs to request.
  std::vector<Request> requests;
  // no_quic_dump is true if hexdump of QUIC STREAM and CRYPTO data
  // should be disabled.
  bool no_quic_dump{};
  // no_http_dump is true if hexdump of HTTP response body should be
  // disabled.
  bool no_http_dump{};
  // qlog_file is the path to write qlog.
  std::filesystem::path qlog_file;
  // qlog_dir is the path to directory where qlog is stored.  qlog_dir
  // and qlog_file are mutually exclusive.
  std::filesystem::path qlog_dir;
  // max_data is the initial connection-level flow control window.
  uint64_t max_data{24_m};
  // max_stream_data_bidi_local is the initial stream-level flow
  // control window for a bidirectional stream that the local endpoint
  // initiates.
  uint64_t max_stream_data_bidi_local{16_m};
  // max_stream_data_bidi_remote is the initial stream-level flow
  // control window for a bidirectional stream that the remote
  // endpoint initiates.
  uint64_t max_stream_data_bidi_remote{};
  // max_stream_data_uni is the initial stream-level flow control
  // window for a unidirectional stream.
  uint64_t max_stream_data_uni{16_m};
  // max_streams_bidi is the number of the concurrent bidirectional
  // streams.
  uint64_t max_streams_bidi{};
  // max_streams_uni is the number of the concurrent unidirectional
  // streams.
  uint64_t max_streams_uni{100};
  // exit_on_first_stream_close is the flag that if it is true, client
  // exits when a first HTTP stream gets closed.  It is not
  // necessarily the same time when the underlying QUIC stream closes
  // due to the QPACK synchronization.
  bool exit_on_first_stream_close{};
  // exit_on_all_streams_close is the flag that if it is true, client
  // exits when all HTTP streams get closed.
  bool exit_on_all_streams_close{};
  // disable_early_data disables early data.
  bool disable_early_data{};
  // static_secret is used to derive keying materials for Stateless
  // Retry token.
  std::array<uint8_t, 32> static_secret;
  // sni is the value sent in TLS SNI, overriding DNS name of the
  // remote host.
  std::string_view sni;
  // wait_for_ticket, if true, waits for a ticket to be received
  // before exiting on exit_on_first_stream_close or
  // exit_on_all_streams_close.
  bool wait_for_ticket{};
  // ech_config_list contains ECHConfigList.
  std::vector<uint8_t> ech_config_list;
  // ech_config_list_file is a path to a file to read and write
  // ECHConfigList.
  std::filesystem::path ech_config_list_file;
};

class ClientBase {
public:
  ClientBase();
  ~ClientBase();

  dwnx_conn *conn() const;

  void write_qlog(const void *data, size_t datalen);

  void ticket_received();

protected:
  SSL *ssl_{};
  FILE *qlog_{};
  dwnx_conn *conn_{};
  dwnx_ccerr last_error_;
  bool ticket_received_{};
};

void qlog_write_cb(void *user_data, uint32_t flags, const void *data,
                   size_t datalen);

#endif // !defined(CLIENT_BASE_H)
