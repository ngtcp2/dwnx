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
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <memory>
#include <fstream>
#include <iomanip>

#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/mman.h>
#include <libgen.h>
#include <netinet/udp.h>

#include <urlparse.h>

#include "client.h"
#include "network.h"
#include "debug.h"
#include "util.h"
#include "shared.h"

using namespace dwnx;
using namespace std::literals;

namespace {
auto randgen = util::make_mt19937();
} // namespace

Config config;

Stream::Stream(const Request &req, int64_t stream_id)
  : req{req}, stream_id{stream_id} {}

Stream::~Stream() {
  if (fd != -1) {
    close(fd);
  }
}

std::expected<void, Error> Stream::open_file(std::string_view path) {
  assert(fd == -1);

  std::string_view filename;

  auto it = std::ranges::find(std::rbegin(path), std::rend(path), '/').base();
  if (it == std::ranges::end(path)) {
    filename = "index.html"sv;
  } else {
    filename = std::string_view{it, std::ranges::end(path)};
    if (filename == ".."sv || filename == "."sv) {
      std::println(stderr, "Invalid file name: {}", filename);
      return std::unexpected{Error::INVALID_ARGUMENT};
    }
  }

  auto fpath = config.download;
  fpath /= filename;

  fd = open(fpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    std::println(stderr, "open: Could not open file {}: {}", fpath.native(),
                 strerror(errno));
    return std::unexpected{Error::IO};
  }

  return {};
}

namespace {
void writecb(struct ev_loop *loop, ev_io *w, int revents) {
  auto c = static_cast<Client *>(w->data);

  if (!c->on_write()) {
    c->disconnect();
  }
}
} // namespace

namespace {
void readcb(struct ev_loop *loop, ev_io *w, int revents) {
  auto c = static_cast<Client *>(w->data);

  if (!c->on_read() || !c->on_write()) {
    c->disconnect();
  }
}
} // namespace

namespace {
void timeoutcb(struct ev_loop *loop, ev_timer *w, int revents) {
  auto c = static_cast<Client *>(w->data);

  if (auto rv = c->handle_expiry(); !rv) {
    return;
  }

  c->disconnect();
}
} // namespace

Client::Client(struct ev_loop *loop) : loop_{loop} {
  ev_io_init(&rev_, readcb, 0, EV_WRITE);
  rev_.data = this;
  ev_io_init(&wev_, writecb, 0, EV_WRITE);
  wev_.data = this;
  ev_timer_init(&timer_, timeoutcb, 0.,
                static_cast<double>(config.timeout) / DWNX_SECONDS);
  timer_.data = this;
}

Client::~Client() { disconnect(); }

void Client::disconnect() {
  tx_.send_blocked = false;

  (void)handle_error();

  ev_timer_stop(loop_, &timer_);

  ev_io_stop(loop_, &wev_);
  ev_io_stop(loop_, &rev_);

  if (ssl_) {
    SSL_set_shutdown(ssl_, SSL_get_shutdown(ssl_) | SSL_RECEIVED_SHUTDOWN);
    ERR_clear_error();
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }

  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
}

namespace {
int recv_transport_params(dwnx_conn *conn, const dwnx_transport_params *params,
                          void *user_data) {
  auto c = static_cast<Client *>(user_data);

  if (!c->recv_transport_params(params)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

std::expected<void, Error>
Client::recv_transport_params(const dwnx_transport_params *params) {
  return proto_codec_->setup_codec();
}

namespace {
int recv_stream_data(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                     uint64_t offset, const uint8_t *data, size_t datalen,
                     void *user_data, void *stream_user_data) {
  if (!config.quiet && !config.no_quic_dump) {
    debug::print_stream_data(stream_id, {data, datalen});
  }

  auto c = static_cast<Client *>(user_data);

  if (!c->recv_stream_data(flags, stream_id, {data, datalen})) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

std::expected<void, Error> Client::handshake_completed() {
  if (early_data_ && !SSL_early_data_accepted(ssl_)) {
    if (!config.quiet) {
      std::println(stderr, "Early data was rejected by server");
    }

    early_data_rejected();
  }

  if (!config.quiet) {
    std::println(stderr, "Negotiated cipher suite is {}",
                 SSL_get_cipher_name(ssl_));
    auto group = std::string_view{SSL_get_group_name(SSL_get_group_id(ssl_))};
    if (!group.empty()) {
      std::println(stderr, "Negotiated group is {}", group);
    }
    std::println(stderr, "Negotiated ALPN is {}",
                 util::get_selected_alpn(ssl_));

    if (!config.ech_config_list.empty() && SSL_ech_accepted(ssl_)) {
      std::println(stderr, "ECH was accepted");
    }
  }

  if (!config.tp_file.empty()) {
    // TODO: Save transport parameters for 0RTT.
  }

  return {};
}

bool Client::should_exit() const {
  return (!config.wait_for_ticket || ticket_received_) &&
         ((config.exit_on_first_stream_close &&
           (config.nstreams == 0 || nstreams_closed_)) ||
          (config.exit_on_all_streams_close &&
           config.nstreams == nstreams_done_ &&
           nstreams_closed_ == nstreams_done_));
}

namespace {
int stream_close(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                 uint64_t rx_app_error_code, uint64_t tx_app_error_code,
                 void *user_data, void *stream_user_data) {
  auto c = static_cast<Client *>(user_data);

  if (!c->on_stream_close(stream_id,
                          (flags & DWNX_STREAM_CLOSE_FLAG_RX_APP_ERROR_CODE_SET)
                            ? std::make_optional(rx_app_error_code)
                            : std::nullopt,
                          (flags & DWNX_STREAM_CLOSE_FLAG_TX_APP_ERROR_CODE_SET)
                            ? std::make_optional(tx_app_error_code)
                            : std::nullopt)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

namespace {
int stream_reset(dwnx_conn *conn, int64_t stream_id, uint64_t final_size,
                 uint64_t app_error_code, void *user_data,
                 void *stream_user_data) {
  auto c = static_cast<Client *>(user_data);

  if (!c->on_stream_reset(stream_id)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

namespace {
int stream_stop_sending(dwnx_conn *conn, int64_t stream_id,
                        uint64_t app_error_code, void *user_data,
                        void *stream_user_data) {
  auto c = static_cast<Client *>(user_data);

  if (!c->on_stream_stop_sending(stream_id)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

namespace {
int extend_max_local_streams_bidi(dwnx_conn *conn, uint64_t max_streams,
                                  void *user_data) {
  auto c = static_cast<Client *>(user_data);

  c->on_extend_max_streams();

  return 0;
}
} // namespace

namespace {
int extend_max_stream_data(dwnx_conn *conn, int64_t stream_id,
                           uint64_t max_data, void *user_data,
                           void *stream_user_data) {
  auto c = static_cast<Client *>(user_data);
  if (!c->extend_max_stream_data(stream_id, max_data)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }
  return 0;
}
} // namespace

std::expected<void, Error> Client::extend_max_stream_data(int64_t stream_id,
                                                          uint64_t max_data) {
  return proto_codec_->extend_max_stream_data(stream_id, max_data);
}

void Client::early_data_rejected() {
  proto_codec_->early_data_rejected();

  nstreams_done_ = 0;
  streams_.clear();
}

std::expected<void, Error> Client::init(int fd, const char *addr,
                                        const char *port, SSL_CTX *ssl_ctx) {
  fd_ = fd;
  addr_ = addr;
  port_ = port;

  ev_io_set(&rev_, fd, EV_READ);
  ev_io_set(&wev_, fd, EV_WRITE);

  static constexpr auto callbacks = dwnx_callbacks{
    .recv_transport_params = ::recv_transport_params,
    .recv_stream_data = ::recv_stream_data,
    .stream_close = stream_close,
    .stream_reset = stream_reset,
    .stream_stop_sending = stream_stop_sending,
    .extend_max_stream_data = ::extend_max_stream_data,
    .extend_max_local_streams_bidi = extend_max_local_streams_bidi,
  };

  dwnx_settings settings;
  dwnx_settings_default(&settings);
  settings.log_write = config.quiet ? nullptr : debug::log_write;
  settings.initial_ts = util::timestamp();
  (void)util::generate_secure_random(
    as_writable_uint8_span(std::span{&settings.conn_id, 1}));

  dwnx_transport_params params;
  dwnx_transport_params_default(&params);
  params.initial_max_stream_data_bidi_local = config.max_stream_data_bidi_local;
  params.initial_max_stream_data_bidi_remote =
    config.max_stream_data_bidi_remote;
  params.initial_max_stream_data_uni = config.max_stream_data_uni;
  params.initial_max_data = config.max_data;
  params.initial_max_streams_bidi = config.max_streams_bidi;
  params.initial_max_streams_uni = config.max_streams_uni;
  params.max_idle_timeout = config.timeout;

  auto rv =
    dwnx_conn_client_new(&conn_, &callbacks, &settings, &params, nullptr, this);

  if (rv != 0) {
    std::println(stderr, "dwnx_conn_client_new: {}", dwnx_strerror(rv));
    return std::unexpected{Error::QUIC};
  }

  proto_codec_ = std::make_unique<ProtoCodec>(this, last_error_);

  if (auto rv = init_ssl(ssl_ctx, ProtoCodec::protocol); !rv) {
    return rv;
  }

  if (early_data_ && !config.tp_file.empty()) {
    // TODO: Do early data
  }

  write_ = &Client::connected;

  start_wev();
  update_timer();

  return {};
}

std::expected<void, Error> Client::init_ssl(SSL_CTX *ssl_ctx,
                                            AppProtocol app_proto) {
  ssl_ = SSL_new(ssl_ctx);
  if (!ssl_) {
    std::println(stderr, "SSL_new: {}",
                 ERR_error_string(ERR_get_error(), nullptr));
    return std::unexpected{Error::CRYPTO};
  }

  SSL_set_app_data(ssl_, this);
  SSL_set_connect_state(ssl_);
  SSL_set_fd(ssl_, fd_);

  switch (app_proto) {
  case AppProtocol::H3:
    SSL_set_alpn_protos(ssl_, H3_ALPN.data(), H3_ALPN.size());
    break;
  case AppProtocol::HQ:
    SSL_set_alpn_protos(ssl_, HQ_ALPN.data(), HQ_ALPN.size());
    break;
  }

  if (!config.sni.empty()) {
    SSL_set_tlsext_host_name(ssl_, config.sni.data());
  } else if (util::numeric_host(addr_)) {
    // If remote host is numeric address, just send "localhost" as SNI
    // for now.
    SSL_set_tlsext_host_name(ssl_, "localhost");
  } else {
    SSL_set_tlsext_host_name(ssl_, addr_);
  }

  return {};
}

std::expected<void, Error> Client::connected() {
  if (!util::check_socket_connected(fd_)) {
    std::println(stderr, "Could not connect to the server");

    return std::unexpected{Error::CONNECT_FAIL};
  }

  read_ = &Client::tls_handshake;
  write_ = &Client::tls_handshake;

  start_rev();

  return {};
}

std::expected<void, Error> Client::tls_handshake() {
  ev_io_stop(loop_, &wev_);

  ERR_clear_error();

  auto rv = SSL_do_handshake(ssl_);
  if (rv <= 0) {
    auto err = SSL_get_error(ssl_, rv);
    switch (err) {
    case SSL_ERROR_WANT_READ:
      return {};
    case SSL_ERROR_WANT_WRITE:
      start_wev();
      update_timer();
      return {};
    default:
      std::println(stderr, "SSL_do_handshake: {}",
                   ERR_error_string(ERR_get_error(), NULL));

      return std::unexpected{Error::CRYPTO};
    }
  }

  read_ = &Client::read_data;
  write_ = &Client::write_data;

  ev_feed_event(loop_, &rev_, EV_READ);

  return handshake_completed();
}

std::expected<void, Error> Client::feed_data(std::span<const uint8_t> data) {
  if (!config.quiet) {
    std::println(stderr, "Read {} bytes from TLS stack", data.size());
  }

  if (auto rv =
        dwnx_conn_read(conn_, data.data(), data.size(), util::timestamp());
      rv != 0) {
    std::println(stderr, "dwnx_conn_read: {}", dwnx_strerror(rv));
    if (!last_error_.error_code) {
      dwnx_ccerr_set_liberr(&last_error_, rv, nullptr, 0);
    }

    return std::unexpected{Error::QUIC};
  }

  return {};
}

std::expected<void, Error> Client::on_read() { return read_(*this); }

std::expected<void, Error> Client::on_write() { return write_(*this); }

std::expected<void, Error> Client::handle_expiry() { return {}; }

std::expected<void, Error> Client::read_data() {
  std::array<uint8_t, 16_k> rawbuf;
  auto buf = std::span<uint8_t>{rawbuf};

  for (;;) {
    ERR_clear_error();

    auto nread = SSL_read(ssl_, buf.data(), static_cast<int>(buf.size()));

    if (nread <= 0) {
      auto err = SSL_get_error(ssl_, nread);
      switch (err) {
      case SSL_ERROR_WANT_READ:
        return {};
      case SSL_ERROR_WANT_WRITE:
        // renegotiation started
      default:
        return std::unexpected{Error::CRYPTO};
      }
    }

    if (auto rv = feed_data(buf.first(static_cast<size_t>(nread))); !rv) {
      return rv;
    }
  }

  return {};
}

std::expected<void, Error> Client::write_data() {
  if (tx_.send_blocked) {
    auto rv = send_blocked_packet();
    if (!rv) {
      return rv;
    }

    if (tx_.send_blocked) {
      return {};
    }
  }

  ev_io_stop(loop_, &wev_);

  if (auto rv = write_streams(); !rv) {
    return rv;
  }

  if (should_exit()) {
    dwnx_ccerr_set_application_error(&last_error_, ProtoCodec::no_error,
                                     nullptr, 0);

    return std::unexpected{Error::INTERNAL};
  }

  update_timer();

  return {};
}

std::expected<void, Error> Client::write_streams() {
  auto buf = std::span{txbuf_};

  for (;;) {
    auto maybe_data = proto_codec_->write_record(buf, util::timestamp());
    if (!maybe_data) {
      return std::unexpected{maybe_data.error()};
    }

    auto data = *maybe_data;
    if (data.empty()) {
      return {};
    }

    auto rv = send_packet_or_blocked(*maybe_data);
    if (!rv) {
      if (rv.error() == Error::SEND_BLOCKED) {
        return {};
      }

      return rv;
    }
  }
}

std::expected<void, Error>
Client::send_packet_or_blocked(std::span<const uint8_t> data) {
  auto maybe_rest = send_packet(data);
  if (!maybe_rest) {
    return std::unexpected{maybe_rest.error()};
  }

  auto rest = *maybe_rest;
  if (!rest.empty()) {
    on_send_blocked(rest);

    return std::unexpected{Error::SEND_BLOCKED};
  }

  return {};
}

void Client::update_timer() { ev_timer_again(loop_, &timer_); }

namespace {
std::expected<void, Error> connect_sock(Address &local_addr, int fd,
                                        const Address &remote_addr) {
  auto rv = connect(fd, remote_addr.as_sockaddr(), remote_addr.size());
  if (rv != 0 && errno != EINPROGRESS) {
    std::println(stderr, "connect: {}", strerror(errno));
    return std::unexpected{Error::SYSCALL};
  }

  sockaddr_storage ss;
  socklen_t len = sizeof(ss);
  if (getsockname(fd, reinterpret_cast<sockaddr *>(&ss), &len) == -1) {
    std::println(stderr, "getsockname: {}", strerror(errno));
    return std::unexpected{Error::SYSCALL};
  }

  local_addr.set(reinterpret_cast<const sockaddr *>(&ss));

  return {};
}
} // namespace

namespace {
std::expected<int, Error> tcp_sock(int family) {
  return util::create_nonblock_socket(family, SOCK_STREAM, IPPROTO_TCP);
}
} // namespace

namespace {
std::expected<int, Error> create_sock(Address &remote_addr, const char *addr,
                                      const char *port) {
  addrinfo hints{
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
  };
  addrinfo *res, *rp;

  if (auto rv = getaddrinfo(addr, port, &hints, &res); rv != 0) {
    std::println(stderr, "getaddrinfo: {}", gai_strerror(rv));
    return std::unexpected{Error::LIBC};
  }

  auto res_d = defer([res] { freeaddrinfo(res); });

  int fd = -1;

  for (rp = res; rp; rp = rp->ai_next) {
    auto maybe_fd = tcp_sock(rp->ai_family);
    if (!maybe_fd) {
      continue;
    }

    fd = *maybe_fd;

    break;
  }

  if (!rp) {
    std::println(stderr, "Could not create socket");
    return std::unexpected{Error::SYSCALL};
  }

  remote_addr.set(rp->ai_addr);

  return fd;
}
} // namespace

std::expected<std::span<const uint8_t>, Error>
Client::send_packet(std::span<const uint8_t> data) {
  if (!config.quiet) {
    std::println(stderr, "Send {} bytes", data.size());
  }

  ERR_clear_error();
  auto nwrite = SSL_write(ssl_, data.data(), static_cast<int>(data.size()));
  if (nwrite <= 0) {
    auto err = SSL_get_error(ssl_, nwrite);
    switch (err) {
    case SSL_ERROR_WANT_WRITE:
      start_wev();
      return data;
    case SSL_ERROR_WANT_READ:
      // renegotiation started
    default:
      return std::unexpected{Error::CRYPTO};
    }
  }

  return data.subspan(as_unsigned(nwrite));
}

void Client::on_send_blocked(std::span<const uint8_t> data) {
  assert(!tx_.send_blocked);

  tx_.send_blocked = true;
  tx_.blocked.data = data;

  start_wev();
}

std::expected<void, Error> Client::send_blocked_packet() {
  assert(tx_.send_blocked);

  auto &p = tx_.blocked;

  auto maybe_rest = send_packet(p.data);
  if (!maybe_rest) {
    return std::unexpected{maybe_rest.error()};
  }

  auto rest = *maybe_rest;
  if (!rest.empty()) {
    p.data = rest;

    start_wev();

    return {};
  }

  tx_.send_blocked = false;

  return {};
}

void Client::start_rev() { ev_io_start(loop_, &rev_); }

void Client::start_wev() { ev_io_start(loop_, &wev_); }

std::expected<void, Error> Client::handle_error() { return {}; }

std::expected<void, Error>
Client::on_stream_close(int64_t stream_id,
                        std::optional<uint64_t> rx_app_error_code,
                        std::optional<uint64_t> tx_app_error_code) {
  if (!config.quiet) {
    std::println(stderr, "QUIC stream {:#x} closed", stream_id);
  }

  if (auto rv = proto_codec_->on_stream_close(stream_id, rx_app_error_code,
                                              tx_app_error_code);
      !rv) {
    return rv;
  }

  if (!dwnx_conn_is_local_stream(conn_, stream_id)) {
    // TODO We might later add bidi stream extension here.
    if (!dwnx_is_bidi_stream(stream_id)) {
      dwnx_conn_extend_max_streams_uni(conn_, 1);
    }
  }

  auto it = streams_.find(stream_id);
  if (it != std::ranges::end(streams_)) {
    ++nstreams_closed_;

    streams_.erase(it);
  }

  return {};
}

std::expected<void, Error> Client::on_stream_reset(int64_t stream_id) {
  return proto_codec_->on_stream_reset(stream_id);
}

std::expected<void, Error> Client::on_stream_stop_sending(int64_t stream_id) {
  return proto_codec_->on_stream_stop_sending(stream_id);
}

std::expected<void, Error> Client::make_stream_early() {
  if (auto rv = setup_codec(); !rv) {
    return rv;
  }

  on_extend_max_streams();

  return {};
}

void Client::on_extend_max_streams() {
  int64_t stream_id;

  for (; nstreams_done_ < config.nstreams; ++nstreams_done_) {
    if (auto rv = dwnx_conn_open_bidi_stream(conn_, &stream_id, nullptr);
        rv != 0) {
      assert(DWNX_ERR_STREAM_ID_BLOCKED == rv);
      break;
    }

    auto stream = std::make_unique<Stream>(
      config.requests[nstreams_done_ % config.requests.size()], stream_id);

    if (!proto_codec_->submit_request(stream.get())) {
      break;
    }

    if (!config.download.empty()) {
      (void)stream->open_file(stream->req.path);
    }

    if (auto [_, rv] = streams_.try_emplace(stream_id, std::move(stream));
        !rv) {
      assert(0);
    }
  }
}

std::expected<void, Error>
Client::recv_stream_data(uint32_t flags, int64_t stream_id,
                         std::span<const uint8_t> data) {
  return proto_codec_->recv_stream_data(flags, stream_id, data);
}

std::expected<void, Error> Client::setup_codec() {
  return proto_codec_->setup_codec();
}

bool Client::get_early_data() const { return early_data_; }

Stream *Client::find_stream(int64_t stream_id) const {
  auto it = streams_.find(stream_id);
  if (it == std::ranges::end(streams_)) {
    return nullptr;
  }

  return (*it).second.get();
}

namespace {
std::expected<SSL_CTX *, Error> create_ssl_ctx(const char *private_key_file,
                                               const char *cert_file) {
  auto ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!ssl_ctx) {
    std::println(stderr, "SSL_CTX_new: {}",
                 ERR_error_string(ERR_get_error(), nullptr));
    return std::unexpected{Error::CRYPTO};
  }

  if (!SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_3_VERSION)) {
    std::println(stderr, "SSL_CTX_set_min_proto_version failed");
    return std::unexpected{Error::CRYPTO};
  }

  SSL_CTX_set_default_verify_paths(ssl_ctx);

  if (SSL_CTX_set1_groups_list(ssl_ctx, config.groups) != 1) {
    std::println(stderr, "SSL_CTX_set1_groups_list failed");
    return std::unexpected{Error::CRYPTO};
  }

  if (private_key_file && cert_file) {
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key_file,
                                    SSL_FILETYPE_PEM) != 1) {
      std::println(stderr, "SSL_CTX_use_PrivateKey_file: {}",
                   ERR_error_string(ERR_get_error(), nullptr));
      return std::unexpected{Error::CRYPTO};
    }

    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_file) != 1) {
      std::println(stderr, "SSL_CTX_use_certificate_chain_file: {}",
                   ERR_error_string(ERR_get_error(), nullptr));
      return std::unexpected{Error::CRYPTO};
    }
  }

  return ssl_ctx;
}
} // namespace

namespace {
std::expected<void, Error> run(Client &c, const char *addr, const char *port,
                               SSL_CTX *ssl_ctx) {
  Address remote_addr, local_addr;

  auto maybe_fd = create_sock(remote_addr, addr, port);
  if (!maybe_fd) {
    return std::unexpected{maybe_fd.error()};
  }

  auto fd = *maybe_fd;

  if (auto rv = connect_sock(local_addr, fd, remote_addr); !rv) {
    close(fd);
    return rv;
  }

  if (auto rv = c.init(fd, addr, port, ssl_ctx); !rv) {
    return rv;
  }

  ev_run(EV_DEFAULT, 0);

  return {};
}
} // namespace

namespace {
std::expected<Request, Error> parse_uri(std::string_view uri) {
  urlparse_url u;

  if (urlparse_parse_url(uri.data(), uri.size(), /* is_connect = */ 0, &u) !=
      0) {
    return std::unexpected{Error::INVALID_ARGUMENT};
  }

  if (!(u.field_set & (1 << URLPARSE_SCHEMA)) ||
      !(u.field_set & (1 << URLPARSE_HOST))) {
    return std::unexpected{Error::INVALID_ARGUMENT};
  }

  Request req;

  req.scheme = util::get_string(uri, u, URLPARSE_SCHEMA);

  auto host = std::string(util::get_string(uri, u, URLPARSE_HOST));
  if (util::numeric_host(host.c_str(), AF_INET6)) {
    req.authority = '[';
    req.authority += host;
    req.authority += ']';
  } else {
    req.authority = std::move(host);
  }

  if (u.field_set & (1 << URLPARSE_PORT)) {
    req.authority += ':';
    req.authority += util::get_string(uri, u, URLPARSE_PORT);
  }

  if (u.field_set & (1 << URLPARSE_PATH)) {
    req.path = util::get_string(uri, u, URLPARSE_PATH);
  } else {
    req.path = "/";
  }

  if (u.field_set & (1 << URLPARSE_QUERY)) {
    req.path += '?';
    req.path += util::get_string(uri, u, URLPARSE_QUERY);
  }

  return req;
}
} // namespace

namespace {
std::expected<void, Error> parse_requests(char **argv, size_t argvlen) {
  for (size_t i = 0; i < argvlen; ++i) {
    auto uri = std::string_view{argv[i]};
    auto maybe_req = parse_uri(uri);
    if (!maybe_req) {
      std::println(stderr, "Could not parse URI: {}", uri);
      return std::unexpected{maybe_req.error()};
    }
    config.requests.emplace_back(std::move(*maybe_req));
  }
  return {};
}
} // namespace

namespace {
const char *prog = "client";
} // namespace

namespace {
void print_usage(FILE *out) {
  std::println(out, "Usage: {} [OPTIONS] <HOST> <PORT> [<URI>...]", prog);
}
} // namespace

namespace {
void print_help() {
  print_usage(stdout);

  Config config;

  std::cout << R"(
  <HOST>      Remote server host (DNS name or IP address).  In case of
              DNS name, it will be sent in TLS SNI extension.
  <PORT>      Remote server port
  <URI>       Remote URI
Options:
  -d, --data=<PATH>
              Read data from <PATH>, and send them as STREAM data.
  -n, --nstreams=<N>
              The number of requests.  <URI>s are used in the order of
              appearance in the command-line.   If the number of <URI>
              list  is  less than  <N>,  <URI>  list is  wrapped.   It
              defaults to 0 which means the number of <URI> specified.
  -q, --quiet Suppress debug output.
  --timeout=<DURATION>
              Specify idle timeout.
              Default: )"
            << util::format_duration(config.timeout) << R"(
  --ciphers=<CIPHERS>
              Specify the cipher suite list to enable.
              Default: )"
            << config.ciphers << R"(
  --groups=<GROUPS>
              Specify the supported groups.
              Default: )"
            << config.groups << R"(
  --session-file=<PATH>
              Read/write  TLS session  from/to  <PATH>.   To resume  a
              session, the previous session must be supplied with this
              option.
  --tp-file=<PATH>
              Read/write QUIC transport parameters from/to <PATH>.  To
              send 0-RTT data, the  transport parameters received from
              the previous session must be supplied with this option.
  -m, --http-method=<METHOD>
              Specify HTTP method.  Default: )"
            << config.http_method << R"(
  --key=<PATH>
              The path to client private key PEM file.
  --cert=<PATH>
              The path to client certificate PEM file.
  --download=<PATH>
              The path to the directory  to save a downloaded content.
              It is  undefined if 2  concurrent requests write  to the
              same file.   If a request  path does not contain  a path
              component  usable  as  a   file  name,  it  defaults  to
              "index.html".
  --no-quic-dump
              Disables printing QUIC STREAM and CRYPTO frame data out.
  --no-http-dump
              Disables printing HTTP response body out.
  --qlog-file=<PATH>
              The path to write qlog.   This option and --qlog-dir are
              mutually exclusive.
  --qlog-dir=<PATH>
              Path to  the directory where  qlog file is  stored.  The
              file name  of each qlog  is the Source Connection  ID of
              client.   This  option   and  --qlog-file  are  mutually
              exclusive.
  --max-data=<SIZE>
              The initial connection-level flow control window.
              Default: )"
            << util::format_uint_iec(config.max_data) << R"(
  --max-stream-data-bidi-local=<SIZE>
              The  initial  stream-level  flow control  window  for  a
              bidirectional stream that the local endpoint initiates.
              Default: )"
            << util::format_uint_iec(config.max_stream_data_bidi_local) << R"(
  --max-stream-data-bidi-remote=<SIZE>
              The  initial  stream-level  flow control  window  for  a
              bidirectional stream that the remote endpoint initiates.
              Default: )"
            << util::format_uint_iec(config.max_stream_data_bidi_remote) << R"(
  --max-stream-data-uni=<SIZE>
              The  initial  stream-level  flow control  window  for  a
              unidirectional stream.
              Default: )"
            << util::format_uint_iec(config.max_stream_data_uni) << R"(
  --max-streams-bidi=<N>
              The number of the  concurrent bidirectional streams that
              the remote endpoint initiates.
              Default: )"
            << config.max_streams_bidi << R"(
  --max-streams-uni=<N>
              The number of the concurrent unidirectional streams that
              the remote endpoint initiates.
              Default: )"
            << config.max_streams_uni << R"(
  --exit-on-first-stream-close
              Exit  when  a  first  client initiated  HTTP  stream  is
              closed.
  --exit-on-all-streams-close
              Exit when all client initiated HTTP streams are closed.
  --wait-for-ticket
              Wait  for a  ticket  to be  received  before exiting  on
              --exit-on-first-stream-close                          or
              --exit-on-all-streams-close.   --session-file   must  be
              specified.
  --disable-early-data
              Disable early data.
  --sni=<DNSNAME>
              Send  <DNSNAME>  in TLS  SNI,  overriding  the DNS  name
              specified in <HOST>.
  --ech-config-list-file=<PATH>
              Read/write  ECHConfigList from/to  <PATH>.  ECH  is only
              attempted if  an underlying  TLS stack supports  it.  If
              the handshake  fails with ech_required alert,  ECH retry
              configs,  if  provided by  server,  will  be written  to
              <PATH>.
  -h, --help  Display this help and exit.

---

  The <SIZE> argument is an integer and an optional unit (e.g., 10K is
  10 * 1024).  Units are K, M and G (powers of 1024).

  The <DURATION> argument is an integer and an optional unit (e.g., 1s
  is 1 second and 500ms is 500  milliseconds).  Units are h, m, s, ms,
  us, or ns (hours,  minutes, seconds, milliseconds, microseconds, and
  nanoseconds respectively).  If  a unit is omitted, a  second is used
  as unit.

  The  <HEX> argument  is an  hex string  which must  start with  "0x"
  (e.g., 0x00000001).)"
            << std::endl;
}
} // namespace

int main(int argc, char **argv) {
  char *data_path = nullptr;
  const char *private_key_file = nullptr;
  const char *cert_file = nullptr;

  if (argc) {
    prog = basename(argv[0]);
  }

  for (;;) {
    static int flag = 0;
    static constexpr option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"data", required_argument, nullptr, 'd'},
      {"http-method", required_argument, nullptr, 'm'},
      {"nstreams", required_argument, nullptr, 'n'},
      {"quiet", no_argument, nullptr, 'q'},
      {"ciphers", required_argument, &flag, 1},
      {"groups", required_argument, &flag, 2},
      {"timeout", required_argument, &flag, 3},
      {"session-file", required_argument, &flag, 4},
      {"tp-file", required_argument, &flag, 5},
      {"key", required_argument, &flag, 12},
      {"cert", required_argument, &flag, 13},
      {"download", required_argument, &flag, 14},
      {"no-quic-dump", no_argument, &flag, 15},
      {"no-http-dump", no_argument, &flag, 16},
      {"qlog-file", required_argument, &flag, 17},
      {"max-data", required_argument, &flag, 18},
      {"max-stream-data-bidi-local", required_argument, &flag, 19},
      {"max-stream-data-bidi-remote", required_argument, &flag, 20},
      {"max-stream-data-uni", required_argument, &flag, 21},
      {"max-streams-bidi", required_argument, &flag, 22},
      {"max-streams-uni", required_argument, &flag, 23},
      {"exit-on-first-stream-close", no_argument, &flag, 24},
      {"disable-early-data", no_argument, &flag, 25},
      {"qlog-dir", required_argument, &flag, 26},
      {"exit-on-all-streams-close", no_argument, &flag, 28},
      {"sni", required_argument, &flag, 30},
      {"wait-for-ticket", no_argument, &flag, 41},
      {"ech-config-list-file", required_argument, &flag, 44},
      {},
    };

    auto optidx = 0;
    auto c = getopt_long(argc, argv, "d:hm:n:q", long_opts, &optidx);
    if (c == -1) {
      break;
    }
    switch (c) {
    case 'd':
      // --data
      data_path = optarg;
      break;
    case 'h':
      // --help
      print_help();
      exit(EXIT_SUCCESS);
    case 'm':
      // --http-method
      config.http_method = optarg;
      break;
    case 'n':
      // --streams
      if (auto n = util::parse_uint(optarg); !n) {
        std::println(stderr, "streams: invalid argument");
        exit(EXIT_FAILURE);
      } else if (*n > DWNX_MAX_VARINT) {
        std::println(stderr, "streams: must not exceed {}", DWNX_MAX_VARINT);
        exit(EXIT_FAILURE);
      } else {
        config.nstreams = *n;
      }
      break;
    case 'q':
      // --quiet
      config.quiet = true;
      break;
    case '?':
      print_usage(stderr);
      exit(EXIT_FAILURE);
    case 0:
      switch (flag) {
      case 1:
        // --ciphers
        if (util::crypto_default_ciphers()[0] == '\0') {
          std::println(stderr, "ciphers: not supported");
          exit(EXIT_FAILURE);
        }
        config.ciphers = optarg;
        break;
      case 2:
        // --groups
        config.groups = optarg;
        break;
      case 3:
        // --timeout
        if (auto t = util::parse_duration(optarg); !t) {
          std::println(stderr, "timeout: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.timeout = *t;
        }
        break;
      case 4:
        // --session-file
        config.session_file = optarg;
        break;
      case 5:
        // --tp-file
        config.tp_file = optarg;
        break;
      case 12:
        // --key
        private_key_file = optarg;
        break;
      case 13:
        // --cert
        cert_file = optarg;
        break;
      case 14:
        // --download
        config.download = optarg;
        break;
      case 15:
        // --no-quic-dump
        config.no_quic_dump = true;
        break;
      case 16:
        // --no-http-dump
        config.no_http_dump = true;
        break;
      case 17:
        // --qlog-file
        config.qlog_file = optarg;
        break;
      case 18:
        // --max-data
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-data: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_data = *n;
        }
        break;
      case 19:
        // --max-stream-data-bidi-local
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-bidi-local: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_bidi_local = *n;
        }
        break;
      case 20:
        // --max-stream-data-bidi-remote
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-bidi-remote: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_bidi_remote = *n;
        }
        break;
      case 21:
        // --max-stream-data-uni
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-uni: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_uni = *n;
        }
        break;
      case 22:
        // --max-streams-bidi
        if (auto n = util::parse_uint(optarg); !n) {
          std::println(stderr, "max-streams-bidi: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_streams_bidi = *n;
        }
        break;
      case 23:
        // --max-streams-uni
        if (auto n = util::parse_uint(optarg); !n) {
          std::println(stderr, "max-streams-uni: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_streams_uni = *n;
        }
        break;
      case 24:
        // --exit-on-first-stream-close
        config.exit_on_first_stream_close = true;
        break;
      case 25:
        // --disable-early-data
        config.disable_early_data = true;
        break;
      case 26:
        // --qlog-dir
        config.qlog_dir = optarg;
        break;
      case 28:
        // --exit-on-all-streams-close
        config.exit_on_all_streams_close = true;
        break;
      case 30:
        // --sni
        config.sni = optarg;
        break;
      case 41:
        // --wait-for-ticket
        config.wait_for_ticket = true;
        break;
      case 44:
        // --ech-config-list-file
        config.ech_config_list_file = optarg;
        break;
      }
      break;
    default:
      break;
    }
  }

  if (argc - optind < 2) {
    std::println(stderr, "Too few arguments");
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (!config.qlog_file.empty() && !config.qlog_dir.empty()) {
    std::println(stderr, "qlog-file and qlog-dir are mutually exclusive");
    exit(EXIT_FAILURE);
  }

  if (config.exit_on_first_stream_close && config.exit_on_all_streams_close) {
    std::println(stderr, "exit-on-first-stream-close and "
                         "exit-on-all-streams-close are mutually exclusive");
    exit(EXIT_FAILURE);
  }

  if (config.wait_for_ticket && config.session_file.empty()) {
    std::println(stderr, "wait-for-ticket: session-file must be specified");
    exit(EXIT_FAILURE);
  }

  if (data_path) {
    auto fd = open(data_path, O_RDONLY);
    if (fd == -1) {
      std::println(stderr, "data: Could not open file {}: {}", data_path,
                   strerror(errno));
      exit(EXIT_FAILURE);
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
      std::println(stderr, "data: Could not stat file {}: {}", data_path,
                   strerror(errno));
      exit(EXIT_FAILURE);
    }
    config.fd = fd;
    config.datalen = static_cast<size_t>(st.st_size);
    if (config.datalen) {
      auto addr = mmap(nullptr, config.datalen, PROT_READ, MAP_SHARED, fd, 0);
      if (addr == MAP_FAILED) {
        std::println(stderr, "data: Could not mmap file {}: {}", data_path,
                     strerror(errno));
        exit(EXIT_FAILURE);
      }
      config.data = static_cast<uint8_t *>(addr);
    }
  }

  if (!config.ech_config_list_file.empty()) {
    auto ech_config = util::read_file(config.ech_config_list_file);
    if (!ech_config) {
      std::println(stderr,
                   "ech-config-list-file: Could not read ECHConfigList");
    } else {
      config.ech_config_list = std::move(*ech_config);
    }
  }

  auto addr = argv[optind++];
  auto port = argv[optind++];

  if (!parse_requests(&argv[optind], static_cast<size_t>(argc - optind))) {
    exit(EXIT_FAILURE);
  }

  if (config.nstreams == 0) {
    config.nstreams = config.requests.size();
  }

  auto maybe_ssl_ctx = create_ssl_ctx(private_key_file, cert_file);
  if (!maybe_ssl_ctx) {
    exit(EXIT_FAILURE);
  }

  auto ssl_ctx = *maybe_ssl_ctx;
  auto ssl_ctx_d = defer([ssl_ctx] { SSL_CTX_free(ssl_ctx); });

  auto ev_loop_d = defer([] { ev_loop_destroy(EV_DEFAULT); });

  auto keylog_filename = getenv("SSLKEYLOGFILE");
  if (keylog_filename) {
    util::keylog_file.open(keylog_filename, std::ios_base::app);
    if (util::keylog_file) {
      util::enable_keylog(ssl_ctx);
    }
  }

  util::ignore_sigpipe();

  auto c = Client{EV_DEFAULT};

  if (!run(c, addr, port, ssl_ctx)) {
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
