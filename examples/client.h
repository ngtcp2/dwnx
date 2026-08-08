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
#ifndef CLIENT_H
#define CLIENT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#include <vector>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <memory>
#include <span>
#include <optional>

#include <dwnx/dwnx.h>

#include <ev.h>

#include "client_base.h"
#include "network.h"
#include "shared.h"
#include "template.h"

#ifdef WITH_EXAMPLE_HTTP3_PROTO_CODEC
#  include "http3_client_proto_codec.h"
#endif // WITH_EXAMPLE_HTTP3_PROTO_CODEC

#ifdef WITH_EXAMPLE_HQ_PROTO_CODEC
#  include "hq_client_proto_codec.h"
#endif // WITH_EXAMPLE_HQ_PROTO_CODEC

using namespace dwnx;

struct Stream {
  Stream(const Request &req, int64_t stream_id);
  ~Stream();

  std::expected<void, Error> open_file(std::string_view path);

  Request req;
  int64_t stream_id;
  int fd{-1};
#ifdef WITH_EXAMPLE_HQ_PROTO_CODEC
  std::string rawreqbuf;
  std::span<const uint8_t> reqbuf;
#endif // WITH_EXAMPLE_HQ_PROTO_CODEC
};

class Client;

struct Endpoint {
  Address addr;
  ev_io rev;
  Client *client{};
  int fd{};
};

class Client : public ClientBase {
public:
  Client(struct ev_loop *loop);
  ~Client();

  std::expected<void, Error> init(int fd, const char *addr, const char *port,
                                  SSL_CTX *ssl_ctx);
  std::expected<void, Error> init_ssl(SSL_CTX *ssl_ctx, AppProtocol app_proto);
  void disconnect();

  std::expected<void, Error> on_read();
  std::expected<void, Error> on_write();

  std::expected<void, Error> tls_handshake();
  std::expected<void, Error> read_data();
  std::expected<void, Error> write_data();

  std::expected<void, Error> write_streams();
  std::expected<void, Error> feed_data(std::span<const uint8_t> data);
  std::expected<void, Error> handle_expiry();
  void update_timer();
  std::expected<void, Error> handshake_completed();

  std::expected<std::span<const uint8_t>, Error>
  send_packet(std::span<const uint8_t> data);
  std::expected<void, Error>
  send_packet_or_blocked(std::span<const uint8_t> data);
  std::expected<void, Error>
  on_stream_close(int64_t stream_id, std::optional<uint64_t> rx_app_error_code,
                  std::optional<uint64_t> tx_app_error_code);
  void on_extend_max_streams();
  std::expected<void, Error> handle_error();
  std::expected<void, Error> make_stream_early();

  std::expected<void, Error>
  recv_transport_params(const dwnx_transport_params *params);
  std::expected<void, Error> setup_codec();
  std::expected<void, Error> recv_stream_data(uint32_t flags, int64_t stream_id,
                                              std::span<const uint8_t> data);
  std::expected<void, Error> on_stream_reset(int64_t stream_id);
  std::expected<void, Error> on_stream_stop_sending(int64_t stream_id);
  std::expected<void, Error> extend_max_stream_data(int64_t stream_id,
                                                    uint64_t max_data);

  void on_send_blocked(std::span<const uint8_t> data);
  void start_rev();
  void start_wev();
  std::expected<void, Error> send_blocked_packet();

  bool get_early_data() const;
  void early_data_rejected();

  bool should_exit() const;

  Stream *find_stream(int64_t stream_id) const;

private:
  Address remote_addr_;
  int fd_{-1};
  struct ev_loop *loop_;
  ev_io rev_;
  ev_io wev_;
  ev_timer timer_;
  std::function<std::expected<void, Error>(Client&)> read_;
  std::function<std::expected<void, Error>(Client&)> write_;
  std::unordered_map<int64_t, std::unique_ptr<Stream>> streams_;
  std::unique_ptr<ProtoCodec> proto_codec_;
  // addr_ is the server host address.
  const char *addr_{};
  // port_ is the server port.
  const char *port_{};
  // nstreams_done_ is the number of streams opened.
  size_t nstreams_done_{};
  // nstreams_closed_ is the number of streams get closed.
  size_t nstreams_closed_{};
  // early_data_ is true if client attempts to do 0RTT data transfer.
  bool early_data_{};

  struct {
    bool send_blocked;
    // blocked field is effective only when send_blocked is true.
    struct {
      std::span<const uint8_t> data;
    } blocked;
  } tx_{};
  std::array<uint8_t, 16_k> txbuf_;
};

#endif // !defined(CLIENT_H)
