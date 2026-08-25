/*
 * dwnx
 *
 * Copyright (c) 2016 dwnx contributors
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
#ifndef SERVER_H
#define SERVER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#include <vector>
#include <unordered_map>
#include <string>
#include <deque>
#include <string_view>
#include <memory>
#include <span>
#include <optional>

#include <dwnx/dwnx.h>

#include <ev.h>

#include "server_base.h"
#include "network.h"
#include "shared.h"
#include "util.h"

#ifdef WITH_EXAMPLE_HTTP3_PROTO_CODEC
#  include "http3_server_proto_codec.h"
#endif // WITH_EXAMPLE_HTTP3_PROTO_CODEC

#ifdef WITH_EXAMPLE_HQ_PROTO_CODEC
#  include <http-parser/http_parser.h>

#  include "hq_server_proto_codec.h"
#endif // WITH_EXAMPLE_HQ_PROTO_CODEC

using namespace dwnx;

class Handler;

enum FileEntryFlag {
  FILE_ENTRY_TYPE_DIR = 0x1,
};

struct FileEntry {
  uint64_t len{};
  void *map{};
  int fd{};
  uint8_t flags{};
};

std::string make_status_body(unsigned int status_code);

struct Request {
  std::string path;
  struct {
    int32_t urgency;
    int inc;
  } pri{};
};

struct Stream {
  Stream(int64_t stream_id, Handler *handler);

  std::expected<void, Error> start_response();
  std::expected<FileEntry, Error> open_file(const std::filesystem::path &path);
  void map_file(const FileEntry &fe);
  std::expected<void, Error>
  send_status_response(ProtoCodec *pc, unsigned int status_code,
                       const std::vector<HTTPHeader> &extra_headers = {});
  std::expected<void, Error> send_redirect_response(ProtoCodec *pc,
                                                    unsigned int status_code,
                                                    std::string_view path);
  std::expected<uint64_t, Error> find_dyn_length(std::string_view path);
  void http_acked_stream_data(uint64_t datalen);
  std::expected<Request, Error> request_path();

  int64_t stream_id;
  Handler *handler;
  // uri is request uri/path.
  std::string uri;
  std::string method;
  std::string authority;
  std::string status_resp_body;
  // resp_data is a pointer to the response data.  It might be the
  // memory which maps file denoted by fd, or status_resp_body.
  std::span<const uint8_t> resp_data;
  // dynresp is true if dynamic data response is enabled.
  bool dynresp{};
  // dyndataleft is the number of dynamic data left to send.
  uint64_t dyndataleft{};
  // dynbuflen is the number of bytes in-flight.
  uint64_t dynbuflen{};
#ifdef WITH_EXAMPLE_HQ_PROTO_CODEC
  http_parser htp;
  // eos gets true when one HTTP request message is seen.
  bool eos{};
#endif // WITH_EXAMPLE_HQ_PROTO_CODEC
};

class Server;

// Endpoint is a local endpoint.
struct Endpoint {
  ev_io rev;
  int fd;
};

class Handler : public HandlerBase {
public:
  Handler(struct ev_loop *loop, int fd, Server *server);
  ~Handler();

  std::expected<void, Error> init(SSL_CTX *ssl_ctx);
  std::expected<void, Error> init_ssl(SSL_CTX *ssl_ctx, AppProtocol app_proto);

  std::expected<void, Error> on_read();
  std::expected<void, Error> on_write();

  std::expected<void, Error> tls_handshake();
  std::expected<void, Error> read_data();
  std::expected<void, Error> write_data();
  std::expected<void, Error> write_streams();
  std::expected<void, Error> feed_data(std::span<const uint8_t> data);
  void update_timer();
  std::expected<void, Error> handle_expiry();
  void signal_write();
  std::expected<void, Error> handshake_completed();

  Server *server() const;
  std::expected<void, Error>
  recv_transport_params(const dwnx_transport_params *params);
  std::expected<void, Error> recv_stream_data(uint32_t flags, int64_t stream_id,
                                              std::span<const uint8_t> data);
  std::expected<void, Error> acked_stream_data_offset(int64_t stream_id,
                                                      uint64_t datalen);
  void on_stream_open(int64_t stream_id);
  std::expected<void, Error>
  on_stream_close(int64_t stream_id, std::optional<uint64_t> rx_app_error_code,
                  std::optional<uint64_t> tx_app_error_code);
  std::expected<void, Error> handle_error();

  void extend_max_remote_streams_bidi(uint64_t max_streams);
  Stream *find_stream(int64_t stream_id) const;
  std::expected<void, Error> on_stream_reset(int64_t stream_id);
  std::expected<void, Error> on_stream_stop_sending(int64_t stream_id);
  std::expected<void, Error> extend_max_stream_data(int64_t stream_id,
                                                    uint64_t max_data);
  std::expected<void, Error>
  write_stream_data_offset(int64_t stream_id, uint64_t offset, size_t len);
  void shutdown_read(int64_t stream_id, uint64_t app_error_code);

  void write_qlog(const void *data, size_t datalen);

  void start_rev();
  void start_wev();
  std::expected<void, Error>
  send_packet_or_blocked(std::span<const uint8_t> data);
  std::expected<std::span<const uint8_t>, Error>
  send_packet(std::span<const uint8_t> data);
  void on_send_blocked(std::span<const uint8_t> data);
  std::expected<void, Error> send_blocked_packet();

  std::expected<void, Error> start_response(Stream *stream);

private:
  struct ev_loop *loop_;
  Server *server_;
  int fd_;
  std::function<std::expected<void,Error>(Handler&)> read_;
  std::function<std::expected<void,Error>(Handler&)> write_;
  ev_io rev_;
  ev_io wev_;
  ev_timer timer_;
  FILE *qlog_{};
  std::unique_ptr<ProtoCodec> proto_codec_;
  std::unordered_map<int64_t, std::unique_ptr<Stream>> streams_;

  struct {
    bool send_blocked;
    // blocked field is effective only when send_blocked is true.
    struct {
      std::span<const uint8_t> data;
    } blocked;
  } tx_{};
  std::array<uint8_t, 16_k> txbuf_;
};

class Server {
public:
  Server(struct ev_loop *loop, SSL_CTX *ssl_ctx);
  ~Server();

  std::expected<void, Error> init(const char *addr, const char *port);
  std::expected<void, Error> add_endpoint(const char *addr, const char *port,
                                          int af);
  void accept_connection(int fd);

private:
  struct ev_loop *loop_;
  SSL_CTX *ssl_ctx_;
  std::vector<Endpoint> endpoints_;
};

#endif // !defined(SERVER_H)
