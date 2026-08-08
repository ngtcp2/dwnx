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
#include <chrono>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <memory>
#include <fstream>
#include <iomanip>

#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <libgen.h>

#include <urlparse.h>

#include "server.h"
#include "network.h"
#include "debug.h"
#include "util.h"
#include "shared.h"
#include "http.h"
#include "template.h"

using namespace dwnx;
using namespace std::literals;

namespace {
auto randgen = util::make_mt19937();
} // namespace

Config config;

Stream::Stream(int64_t stream_id, Handler *handler)
  : stream_id{stream_id}, handler{handler} {
#ifdef WITH_EXAMPLE_HQ_PROTO_CODEC
  htp.data = this;
  http_parser_init(&htp, HTTP_REQUEST);
#endif // WITH_EXAMPLE_HQ_PROTO_CODEC
}

std::string make_status_body(unsigned int status_code) {
  auto status_string = util::format_uint(status_code);
  auto reason_phrase = http::get_reason_phrase(status_code);

  std::string body;
  body = "<html><head><title>";
  body += status_string;
  body += ' ';
  body += reason_phrase;
  body += "</title></head><body><h1>";
  body += status_string;
  body += ' ';
  body += reason_phrase;
  body += "</h1><hr><address>";
  body += DWNX_SERVER;
  body += " at port ";
  body += util::format_uint(config.port);
  body += "</address>";
  body += "</body></html>";
  return body;
}

std::expected<void, Error> Stream::start_response() {
  return handler->start_response(this);
}

std::expected<Request, Error> Stream::request_path() {
  urlparse_url u;
  Request req{
    .pri{
      .urgency = -1,
      .inc = -1,
    },
  };
  auto is_connect = method == "CONNECT";

  if (auto rv = urlparse_parse_url(uri.data(), uri.size(), is_connect, &u);
      rv != 0) {
    return std::unexpected{Error::INVALID_ARGUMENT};
  }

  if (u.field_set & (1 << URLPARSE_PATH)) {
    req.path = util::get_string(uri, u, URLPARSE_PATH);
    if (req.path.find('%') != std::string::npos) {
      req.path = util::percent_decode(req.path);
    }

    assert(!req.path.empty());

    if (req.path[0] != '/') {
      return std::unexpected{Error::INVALID_ARGUMENT};
    }

    if (req.path.back() == '/') {
      req.path += "index.html";
    }

    auto maybe_norm_path = util::normalize_path(req.path);
    if (!maybe_norm_path) {
      return std::unexpected{maybe_norm_path.error()};
    }

    req.path = std::move(*maybe_norm_path);
  } else {
    req.path = "/index.html";
  }

  if (u.field_set & (1 << URLPARSE_QUERY)) {
    static constexpr auto urgency_prefix = "u="sv;
    static constexpr auto inc_prefix = "i="sv;
    auto q = util::get_string(uri, u, URLPARSE_QUERY);
    for (auto p = std::ranges::begin(q); p != std::ranges::end(q);) {
      if (util::istarts_with(std::string_view{p, std::ranges::end(q)},
                             urgency_prefix)) {
        auto urgency_start = p + urgency_prefix.size();
        auto urgency_end =
          std::ranges::find(urgency_start, std::ranges::end(q), '&');
        if (urgency_start + 1 == urgency_end && '0' <= *urgency_start &&
            *urgency_start <= '7') {
          req.pri.urgency = *urgency_start - '0';
        }
        if (urgency_end == std::ranges::end(q)) {
          break;
        }
        p = urgency_end + 1;
        continue;
      }
      if (util::istarts_with(std::string_view{p, std::ranges::end(q)},
                             inc_prefix)) {
        auto inc_start = p + inc_prefix.size();
        auto inc_end = std::ranges::find(inc_start, std::ranges::end(q), '&');
        if (inc_start + 1 == inc_end &&
            (*inc_start == '0' || *inc_start == '1')) {
          req.pri.inc = *inc_start - '0';
        }
        if (inc_end == std::ranges::end(q)) {
          break;
        }
        p = inc_end + 1;
        continue;
      }

      p = std::ranges::find(p, std::ranges::end(q), '&');
      if (p == std::ranges::end(q)) {
        break;
      }
      ++p;
    }
  }
  return req;
}

namespace {
std::unordered_map<std::string, FileEntry> file_cache;
} // namespace

std::expected<FileEntry, Error>
Stream::open_file(const std::filesystem::path &path) {
  auto it = file_cache.find(path.native());
  if (it != std::ranges::end(file_cache)) {
    return (*it).second;
  }

  auto fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    return std::unexpected{Error::SYSCALL};
  }

  struct stat st{};
  if (fstat(fd, &st) != 0) {
    close(fd);
    return std::unexpected{Error::SYSCALL};
  }

  FileEntry fe;
  if (st.st_mode & S_IFDIR) {
    fe.flags |= FILE_ENTRY_TYPE_DIR;
    fe.fd = -1;
    close(fd);
  } else {
    fe.fd = fd;
    fe.len = static_cast<size_t>(st.st_size);
    if (fe.len) {
      fe.map = mmap(nullptr, fe.len, PROT_READ, MAP_SHARED, fd, 0);
      if (fe.map == MAP_FAILED) {
        std::println(stderr, "mmap: {}", strerror(errno));
        close(fd);
        return std::unexpected{Error::SYSCALL};
      }
    }
  }

  file_cache.emplace(path.native(), fe);

  return fe;
}

void Stream::map_file(const FileEntry &fe) {
  resp_data = {static_cast<const uint8_t *>(fe.map), fe.len};
}

std::expected<uint64_t, Error> Stream::find_dyn_length(std::string_view path) {
  assert(path[0] == '/');

  if (path.size() == 1) {
    return std::unexpected{Error::INVALID_ARGUMENT};
  }

  uint64_t n = 0;

  for (auto it = std::ranges::begin(path) + 1; it != std::ranges::end(path);
       ++it) {
    if (*it < '0' || '9' < *it) {
      return std::unexpected{Error::INVALID_ARGUMENT};
    }
    auto d = static_cast<uint64_t>(*it - '0');
    if (n > (((1ULL << 62) - 1) - d) / 10) {
      return std::unexpected{Error::INVALID_ARGUMENT};
    }
    n = n * 10 + d;
    if (n > config.max_dyn_length) {
      return std::unexpected{Error::INVALID_ARGUMENT};
    }
  }

  return n;
}

void Stream::http_acked_stream_data(uint64_t datalen) {
  if (!dynresp) {
    return;
  }

  assert(dynbuflen >= datalen);

  dynbuflen -= datalen;
}

namespace {
void readcb(struct ev_loop *loop, ev_io *w, int revents) {
  auto h = static_cast<Handler *>(w->data);

  if (!h->on_read() || !h->on_write()) {
    delete h;
  }
}
} // namespace

namespace {
void writecb(struct ev_loop *loop, ev_io *w, int revents) {
  auto h = static_cast<Handler *>(w->data);

  if (!h->on_write()) {
    delete h;
  }
}
} // namespace

namespace {
void timeoutcb(struct ev_loop *loop, ev_timer *w, int revents) {
  auto h = static_cast<Handler *>(w->data);

  if (!config.quiet) {
    std::println(stderr, "Timer expired");
  }

  delete h;
}
} // namespace

Handler::Handler(struct ev_loop *loop, int fd, Server *server)
  : loop_{loop}, server_{server}, fd_{fd} {
  ev_io_init(&rev_, readcb, fd, EV_READ);
  rev_.data = this;
  ev_io_init(&wev_, writecb, fd, EV_WRITE);
  wev_.data = this;
  ev_timer_init(&timer_, timeoutcb, 0.,
                static_cast<double>(config.timeout) / DWNX_SECONDS);
  timer_.data = this;
}

Handler::~Handler() {
  if (!config.quiet) {
    std::println(stderr, "Closing QMux connection");
  }

  ev_timer_stop(loop_, &timer_);
  ev_io_stop(loop_, &wev_);
  ev_io_stop(loop_, &rev_);

  if (ssl_) {
    SSL_set_shutdown(ssl_, SSL_get_shutdown(ssl_) | SSL_RECEIVED_SHUTDOWN);
    ERR_clear_error();
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
  }

  close(fd_);

  if (qlog_) {
    fclose(qlog_);
  }
}

std::expected<void, Error> Handler::handshake_completed() {
  if (!config.quiet) {
    std::println(stderr, "Negotiated cipher suite is {}",
                 SSL_get_cipher_name(ssl_));

    auto group = std::string_view{SSL_get_group_name(SSL_get_group_id(ssl_))};
    if (!group.empty()) {
      std::println(stderr, "Negotiated group is {}", group);
    }

    std::println(stderr, "Negotiated ALPN is {}",
                 util::get_selected_alpn(ssl_));
  }

  return {};
}

namespace {
int recv_transport_params(dwnx_conn *conn, const dwnx_transport_params *params,
                          void *user_data) {
  auto h = static_cast<Handler *>(user_data);

  if (!h->recv_transport_params(params)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

std::expected<void, Error>
Handler::recv_transport_params(const dwnx_transport_params *params) {
  return proto_codec_->setup_codec();
}

namespace {
int recv_stream_data(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                     uint64_t offset, const uint8_t *data, size_t datalen,
                     void *user_data, void *stream_user_data) {
  auto h = static_cast<Handler *>(user_data);

  if (!h->recv_stream_data(flags, stream_id, {data, datalen})) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }

  return 0;
}
} // namespace

namespace {
int stream_open(dwnx_conn *conn, int64_t stream_id, void *user_data) {
  auto h = static_cast<Handler *>(user_data);
  h->on_stream_open(stream_id);
  return 0;
}
} // namespace

void Handler::on_stream_open(int64_t stream_id) {
  if (!dwnx_is_bidi_stream(stream_id)) {
    return;
  }

  assert(!streams_.contains(stream_id));

  streams_.emplace(stream_id, std::make_unique<Stream>(stream_id, this));
}

Stream *Handler::find_stream(int64_t stream_id) const {
  auto it = streams_.find(stream_id);
  if (it == std::ranges::end(streams_)) {
    return nullptr;
  }

  return (*it).second.get();
}

namespace {
int stream_close(dwnx_conn *conn, uint32_t flags, int64_t stream_id,
                 uint64_t rx_app_error_code, uint64_t tx_app_error_code,
                 void *user_data, void *stream_user_data) {
  auto h = static_cast<Handler *>(user_data);
  if (!h->on_stream_close(stream_id,
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
  auto h = static_cast<Handler *>(user_data);
  if (!h->on_stream_reset(stream_id)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }
  return 0;
}
} // namespace

std::expected<void, Error> Handler::on_stream_reset(int64_t stream_id) {
  return proto_codec_->on_stream_reset(stream_id);
}

namespace {
int stream_stop_sending(dwnx_conn *conn, int64_t stream_id,
                        uint64_t app_error_code, void *user_data,
                        void *stream_user_data) {
  auto h = static_cast<Handler *>(user_data);
  if (!h->on_stream_stop_sending(stream_id)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }
  return 0;
}
} // namespace

std::expected<void, Error> Handler::on_stream_stop_sending(int64_t stream_id) {
  return proto_codec_->on_stream_stop_sending(stream_id);
}

namespace {
int extend_max_remote_streams_bidi(dwnx_conn *conn, uint64_t max_streams,
                                   void *user_data) {
  auto h = static_cast<Handler *>(user_data);
  h->extend_max_remote_streams_bidi(max_streams);
  return 0;
}
} // namespace

void Handler::extend_max_remote_streams_bidi(uint64_t max_streams) {
  proto_codec_->extend_max_remote_streams_bidi(max_streams);
}

namespace {
int extend_max_stream_data(dwnx_conn *conn, int64_t stream_id,
                           uint64_t max_data, void *user_data,
                           void *stream_user_data) {
  auto h = static_cast<Handler *>(user_data);
  if (!h->extend_max_stream_data(stream_id, max_data)) {
    return DWNX_ERR_CALLBACK_FAILURE;
  }
  return 0;
}
} // namespace

std::expected<void, Error> Handler::extend_max_stream_data(int64_t stream_id,
                                                           uint64_t max_data) {
  return proto_codec_->extend_max_stream_data(stream_id, max_data);
}

std::expected<void, Error> Handler::start_response(Stream *stream) {
  return proto_codec_->start_response(stream);
}

void Handler::write_qlog(const void *data, size_t datalen) {
  assert(qlog_);
  fwrite(data, 1, datalen, qlog_);
}

std::expected<void, Error> Handler::init(SSL_CTX *ssl_ctx) {
  static constexpr auto callbacks = dwnx_callbacks{
    .recv_transport_params = ::recv_transport_params,
    .recv_stream_data = ::recv_stream_data,
    .stream_open = ::stream_open,
    .stream_close = ::stream_close,
    .stream_reset = ::stream_reset,
    .stream_stop_sending = ::stream_stop_sending,
    .extend_max_stream_data = ::extend_max_stream_data,
    .extend_max_remote_streams_bidi = ::extend_max_remote_streams_bidi,
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

  if (auto rv = dwnx_conn_server_new(&conn_, &callbacks, &settings, &params,
                                     nullptr, this);
      rv != 0) {
    std::println(stderr, "dwnx_conn_server_new: {}", dwnx_strerror(rv));
    return std::unexpected{Error::QUIC};
  }

  proto_codec_ = std::make_unique<ProtoCodec>(this, last_error_);

  if (auto rv = init_ssl(ssl_ctx, ProtoCodec::protocol); !rv) {
    return rv;
  }

  read_ = &Handler::tls_handshake;
  write_ = &Handler::tls_handshake;

  start_rev();
  update_timer();

  return {};
}

std::expected<void, Error> Handler::init_ssl(SSL_CTX *ssl_ctx,
                                             AppProtocol app_proto) {
  ssl_ = SSL_new(ssl_ctx);
  if (!ssl_) {
    std::println(stderr, "SSL_new: {}",
                 ERR_error_string(ERR_get_error(), nullptr));

    return std::unexpected{Error::CRYPTO};
  }

  SSL_set_app_data(ssl_, this);
  SSL_set_accept_state(ssl_);
  SSL_set_fd(ssl_, fd_);

  return {};
}

std::expected<void, Error> Handler::tls_handshake() {
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

  read_ = &Handler::read_data;
  write_ = &Handler::write_data;

  ev_feed_event(loop_, &rev_, EV_READ);

  return handshake_completed();
}

std::expected<void, Error> Handler::feed_data(std::span<const uint8_t> data) {
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

std::expected<void, Error> Handler::on_read() { return read_(*this); }

std::expected<void, Error> Handler::on_write() { return write_(*this); }

std::expected<void, Error> Handler::handle_expiry() { return {}; }

std::expected<void, Error> Handler::read_data() {
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

std::expected<void, Error> Handler::write_data() {
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

  update_timer();

  return {};
}

std::expected<void, Error> Handler::write_streams() {
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
Handler::send_packet_or_blocked(std::span<const uint8_t> data) {
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

std::expected<std::span<const uint8_t>, Error>
Handler::send_packet(std::span<const uint8_t> data) {
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
      return {};
    case SSL_ERROR_WANT_READ:
      // renegotiation started
    default:
      return std::unexpected{Error::CRYPTO};
    }
  }

  return data.subspan(as_unsigned(nwrite));
}

void Handler::on_send_blocked(std::span<const uint8_t> data) {
  assert(!tx_.send_blocked);

  tx_.send_blocked = true;
  tx_.blocked.data = data;

  start_wev();
}

std::expected<void, Error> Handler::send_blocked_packet() {
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

void Handler::start_rev() { ev_io_start(loop_, &rev_); }

void Handler::start_wev() { ev_io_start(loop_, &wev_); }

std::expected<void, Error> Handler::handle_error() { return {}; }

void Handler::update_timer() { ev_timer_again(loop_, &timer_); }

std::expected<void, Error>
Handler::recv_stream_data(uint32_t flags, int64_t stream_id,
                          std::span<const uint8_t> data) {
  if (!config.quiet && !config.no_quic_dump) {
    debug::print_stream_data(stream_id, data);
  }

  return proto_codec_->recv_stream_data(flags, stream_id, data);
}

Server *Handler::server() const { return server_; }

std::expected<void, Error>
Handler::on_stream_close(int64_t stream_id,
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
    if (dwnx_is_bidi_stream(stream_id)) {
      dwnx_conn_extend_max_streams_bidi(conn_, 1);
    }

    // TODO We might later add uni stream extension here.
  }

  auto it = streams_.find(stream_id);
  if (it != std::ranges::end(streams_)) {
    streams_.erase(it);
  }

  return {};
}

void Handler::shutdown_read(int64_t stream_id, uint64_t app_error_code) {
  dwnx_conn_shutdown_stream_read(conn_, 0, stream_id, app_error_code);
}

Server::Server(struct ev_loop *loop, SSL_CTX *ssl_ctx)
  : loop_{loop}, ssl_ctx_{ssl_ctx} {}

Server::~Server() {
  for (auto &ep : endpoints_) {
    ev_io_stop(loop_, &ep.rev);
  }
}

namespace {
std::expected<int, Error> create_sock(Address &local_addr, const char *addr,
                                      const char *port, int family) {
  addrinfo hints{
    .ai_flags = AI_PASSIVE,
    .ai_family = family,
    .ai_socktype = SOCK_STREAM,
  };
  addrinfo *res, *rp;
  int val = 1;

  if (strcmp(addr, "*") == 0) {
    addr = nullptr;
  }

  if (auto rv = getaddrinfo(addr, port, &hints, &res); rv != 0) {
    std::println(stderr, "getaddrinfo: {}", gai_strerror(rv));
    return std::unexpected{Error::LIBC};
  }

  auto res_d = defer([res] { freeaddrinfo(res); });

  int fd = -1;

  for (rp = res; rp; rp = rp->ai_next) {
    auto maybe_fd = util::create_nonblock_socket(rp->ai_family, rp->ai_socktype,
                                                 rp->ai_protocol);
    if (!maybe_fd) {
      continue;
    }

    fd = *maybe_fd;

    if (rp->ai_family == AF_INET6) {
      if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &val,
                     static_cast<socklen_t>(sizeof(val))) == -1) {
        close(fd);
        continue;
      }
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val,
                   static_cast<socklen_t>(sizeof(val))) == -1) {
      close(fd);
      continue;
    }

    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == -1) {
      close(fd);
      continue;
    }

    if (listen(fd, 1000) != -1) {
      break;
    }

    close(fd);
  }

  if (!rp) {
    std::println(stderr, "Could not bind");
    return std::unexpected{Error::SYSCALL};
  }

  sockaddr_storage ss;
  socklen_t len = sizeof(ss);
  if (getsockname(fd, reinterpret_cast<sockaddr *>(&ss), &len) == -1) {
    std::println(stderr, "getsockname: {}", strerror(errno));
    close(fd);
    return std::unexpected{Error::SYSCALL};
  }

  local_addr.set(reinterpret_cast<const sockaddr *>(&ss));

  return fd;
}

} // namespace

namespace {
void acceptcb(struct ev_loop *loop, ev_io *w, int revents) {
  auto s = static_cast<Server *>(w->data);

  s->accept_connection(w->fd);
}
} // namespace

void Server::accept_connection(int server_fd) {
  constexpr size_t max_num_accept = 10;

  for (size_t i = 0; i < max_num_accept; ++i) {
#ifdef HAVE_ACCEPT4
    auto fd = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK);
#else  // !defined(HAVE_ACCEPT4)
    auto fd = accept(server_fd, nullptr, nullptr);
#endif // !defined(HAVE_ACCEPT4)
    if (fd == -1) {
      break;
    }
#ifndef HAVE_ACCEPT4
    (void)util::make_socket_nonblocking(fd);
#endif // !defined(HAVE_ACCEPT4)

    auto h = std::make_unique<Handler>(loop_, fd, this);
    if (!h->init(ssl_ctx_)) {
      continue;
    }

    h.release();
  }
}

std::expected<void, Error> Server::add_endpoint(const char *addr,
                                                const char *port, int af) {
  Address dest;
  auto maybe_fd = create_sock(dest, addr, port, af);
  if (!maybe_fd) {
    return std::unexpected{maybe_fd.error()};
  }

  endpoints_.emplace_back();

  auto &ep = endpoints_.back();
  ev_io_init(&ep.rev, acceptcb, *maybe_fd, EV_READ);
  ep.rev.data = this;

  return {};
}

namespace {
int alpn_select_proto_h3_cb(SSL *ssl, const unsigned char **out,
                            unsigned char *outlen, const unsigned char *in,
                            unsigned int inlen, void *arg) {
  for (auto s = std::span{in, inlen}; s.size() >= H3_ALPN_V1.size();
       s = s.subspan(s[0] + 1)) {
    if (std::ranges::equal(H3_ALPN_V1, s.first(H3_ALPN_V1.size()))) {
      *out = &s[1];
      *outlen = s[0];
      return SSL_TLSEXT_ERR_OK;
    }
  }

  if (!config.quiet) {
    std::println(stderr, "Client did not present ALPN {}",
                 as_string_view(H3_ALPN_V1.subspan(1)));
  }

  return SSL_TLSEXT_ERR_ALERT_FATAL;
}
} // namespace

namespace {
int alpn_select_proto_hq_cb(SSL *ssl, const unsigned char **out,
                            unsigned char *outlen, const unsigned char *in,
                            unsigned int inlen, void *arg) {
  for (auto s = std::span{in, inlen}; s.size() >= HQ_ALPN_V1.size();
       s = s.subspan(s[0] + 1)) {
    if (std::ranges::equal(HQ_ALPN_V1, s.first(HQ_ALPN_V1.size()))) {
      *out = &s[1];
      *outlen = s[0];
      return SSL_TLSEXT_ERR_OK;
    }
  }

  if (!config.quiet) {
    std::println(stderr, "Client did not present ALPN {}",
                 as_string_view(HQ_ALPN_V1.subspan(1)));
  }

  return SSL_TLSEXT_ERR_ALERT_FATAL;
}
} // namespace

namespace {
int verify_cb(int preverify_ok, X509_STORE_CTX *ctx) {
  // We don't verify the client certificate.  Just request it for the
  // testing purpose.
  return 1;
}
} // namespace

namespace {
std::expected<SSL_CTX *, Error> create_ssl_ctx(const char *private_key_file,
                                               const char *cert_file,
                                               AppProtocol app_proto) {
  auto ssl_ctx = SSL_CTX_new(TLS_server_method());
  if (!ssl_ctx) {
    std::println(stderr, "SSL_CTX_new: {}",
                 ERR_error_string(ERR_get_error(), nullptr));

    return std::unexpected{Error::CRYPTO};
  }

  constexpr auto ssl_opts = (SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS) |
                            SSL_OP_SINGLE_ECDH_USE |
                            SSL_OP_CIPHER_SERVER_PREFERENCE;

  SSL_CTX_set_options(ssl_ctx, ssl_opts);

  if (SSL_CTX_set1_groups_list(ssl_ctx, config.groups) != 1) {
    std::println(stderr, "SSL_CTX_set1_groups_list failed");
    return std::unexpected{Error::CRYPTO};
  }

  SSL_CTX_set_mode(ssl_ctx, SSL_MODE_RELEASE_BUFFERS);

  switch (app_proto) {
  case AppProtocol::H3:
    SSL_CTX_set_alpn_select_cb(ssl_ctx, alpn_select_proto_h3_cb, nullptr);
    break;
  case AppProtocol::HQ:
    SSL_CTX_set_alpn_select_cb(ssl_ctx, alpn_select_proto_hq_cb, nullptr);
    break;
  }

  SSL_CTX_set_default_verify_paths(ssl_ctx);

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

  if (SSL_CTX_check_private_key(ssl_ctx) != 1) {
    std::println(stderr, "SSL_CTX_check_private_key: {}",
                 ERR_error_string(ERR_get_error(), nullptr));
    return std::unexpected{Error::CRYPTO};
  }

  if (config.verify_client) {
    SSL_CTX_set_verify(ssl_ctx,
                       SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE |
                         SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       verify_cb);
  }

  return ssl_ctx;
}
} // namespace

std::expected<void, Error> Server::init(const char *addr, const char *port) {
  endpoints_.reserve(2);

  auto ready = false;
  auto error = Error::INTERNAL;

  if (!util::numeric_host(addr, AF_INET6)) {
    if (auto rv = add_endpoint(addr, port, AF_INET); !rv) {
      error = rv.error();
    } else {
      ready = true;
    }
  }
  if (!util::numeric_host(addr, AF_INET)) {
    if (auto rv = add_endpoint(addr, port, AF_INET6); !rv) {
      error = rv.error();
    } else {
      ready = true;
    }
  }
  if (!ready) {
    return std::unexpected{error};
  }

  for (auto &ep : endpoints_) {
    ev_io_start(loop_, &ep.rev);
  }

  return {};
}

namespace {
const char *prog = "server";
} // namespace

namespace {
void print_usage(FILE *out) {
  std::println(
    out,
    "Usage: {} [OPTIONS] <ADDR> <PORT> <PRIVATE_KEY_FILE> <CERTIFICATE_FILE>",
    prog);
}
} // namespace

namespace {
void print_help() {
  print_usage(stdout);

  Config config;

  std::cout << R"(
  <ADDR>      Address to listen to.  '*' binds to any address.
  <PORT>      Port
  <PRIVATE_KEY_FILE>
              Path to private key file
  <CERTIFICATE_FILE>
              Path to certificate file
Options:
  --ciphers=<CIPHERS>
              Specify the cipher suite list to enable.
              Default: )"
            << config.ciphers << R"(
  --groups=<GROUPS>
              Specify the supported groups.
              Default: )"
            << config.groups << R"(
  -d, --htdocs=<PATH>
              Specify document root.  If this option is not specified,
              the document root is the current working directory.
  -q, --quiet Suppress debug output.
  --timeout=<DURATION>
              Specify idle timeout.
              Default: )"
            << util::format_duration(config.timeout) << R"(
  --mime-types-file=<PATH>
              Path  to file  that contains  MIME media  types and  the
              extensions.
              Default: )"
            << config.mime_types_file.native() << R"(
  --early-response
              Start  sending response  when  it  receives HTTP  header
              fields  without  waiting  for  request  body.   If  HTTP
              response data is written  before receiving request body,
              STOP_SENDING is sent.
  --verify-client
              Request a  client certificate.   At the moment,  we just
              request a certificate and no verification is done.
  --qlog-dir=<PATH>
              Path to  the directory where  qlog file is  stored.  The
              file name  of each qlog  is the Source Connection  ID of
              server.
  --no-quic-dump
              Disables printing QUIC STREAM and CRYPTO frame data out.
  --no-http-dump
              Disables printing HTTP response body out.
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
  --max-dyn-length=<SIZE>
              The maximum length of a dynamically generated content.
              Default: )"
            << util::format_uint_iec(config.max_dyn_length) << R"(
  --send-trailers
              Send trailer fields.
  --handshake-timeout=<DURATION>
              Set  the  QUIC handshake  timeout.   It  defaults to  no
              timeout.
  --ech-config-file=<PATH>
              Read private  key and  ECHConfig from <PATH>.   The file
              denoted  by   <PATH>  must   contain  private   key  and
              ECHConfigList   as   described   in   RFC   9934.    ECH
              configuration is only applied if an underlying TLS stack
              supports it.
  --origin=<ORIGIN>
              Specify the origin to send in ORIGIN frame.  Repeat to
              add multiple origins.
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
  if (argc) {
    prog = basename(argv[0]);
  }

  std::filesystem::path ech_config_file;

  for (;;) {
    static int flag = 0;
    static constexpr option long_opts[] = {
      {"help", no_argument, nullptr, 'h'},
      {"htdocs", required_argument, nullptr, 'd'},
      {"quiet", no_argument, nullptr, 'q'},
      {"ciphers", required_argument, &flag, 1},
      {"groups", required_argument, &flag, 2},
      {"timeout", required_argument, &flag, 3},
      {"mime-types-file", required_argument, &flag, 6},
      {"early-response", no_argument, &flag, 7},
      {"verify-client", no_argument, &flag, 8},
      {"qlog-dir", required_argument, &flag, 9},
      {"no-quic-dump", no_argument, &flag, 10},
      {"no-http-dump", no_argument, &flag, 11},
      {"max-data", required_argument, &flag, 12},
      {"max-stream-data-bidi-local", required_argument, &flag, 13},
      {"max-stream-data-bidi-remote", required_argument, &flag, 14},
      {"max-stream-data-uni", required_argument, &flag, 15},
      {"max-streams-bidi", required_argument, &flag, 16},
      {"max-streams-uni", required_argument, &flag, 17},
      {"max-dyn-length", required_argument, &flag, 18},
      {"send-trailers", no_argument, &flag, 22},
      {"max-window", required_argument, &flag, 23},
      {"max-stream-window", required_argument, &flag, 24},
      {"handshake-timeout", required_argument, &flag, 26},
      {"ech-config-file", required_argument, &flag, 33},
      {"origin", required_argument, &flag, 34},
      {},
    };

    auto optidx = 0;
    auto c = getopt_long(argc, argv, "d:hq", long_opts, &optidx);
    if (c == -1) {
      break;
    }
    switch (c) {
    case 'd': {
      // --htdocs
      auto path = realpath(optarg, nullptr);
      if (path == nullptr) {
        std::println(stderr, "path: invalid path {}", optarg);
        exit(EXIT_FAILURE);
      }
      config.htdocs = path;
      free(path);
      break;
    }
    case 'h':
      // --help
      print_help();
      exit(EXIT_SUCCESS);
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
      case 6:
        // --mime-types-file
        config.mime_types_file = optarg;
        break;
      case 7:
        // --early-response
        config.early_response = true;
        break;
      case 8:
        // --verify-client
        config.verify_client = true;
        break;
      case 9:
        // --qlog-dir
        config.qlog_dir = optarg;
        break;
      case 10:
        // --no-quic-dump
        config.no_quic_dump = true;
        break;
      case 11:
        // --no-http-dump
        config.no_http_dump = true;
        break;
      case 12:
        // --max-data
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-data: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_data = *n;
        }
        break;
      case 13:
        // --max-stream-data-bidi-local
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-bidi-local: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_bidi_local = *n;
        }
        break;
      case 14:
        // --max-stream-data-bidi-remote
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-bidi-remote: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_bidi_remote = *n;
        }
        break;
      case 15:
        // --max-stream-data-uni
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-stream-data-uni: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_stream_data_uni = *n;
        }
        break;
      case 16:
        // --max-streams-bidi
        if (auto n = util::parse_uint(optarg); !n) {
          std::println(stderr, "max-streams-bidi: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_streams_bidi = *n;
        }
        break;
      case 17:
        // --max-streams-uni
        if (auto n = util::parse_uint(optarg); !n) {
          std::println(stderr, "max-streams-uni: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_streams_uni = *n;
        }
        break;
      case 18:
        // --max-dyn-length
        if (auto n = util::parse_uint_iec(optarg); !n) {
          std::println(stderr, "max-dyn-length: invalid argument");
          exit(EXIT_FAILURE);
        } else {
          config.max_dyn_length = *n;
        }
        break;
      case 22:
        // --send-trailers
        config.send_trailers = true;
        break;
      case 33:
        // --ech-config-file
        ech_config_file = optarg;
        break;
      case 34: {
        // --origin
        auto origin = std::string_view{optarg};

        if (auto max = std::numeric_limits<uint16_t>::max();
            max < origin.size()) {
          std::println(stderr, "origin: must be less than or equal to {}", max);
          exit(EXIT_FAILURE);
        }

        if (!config.origin_list) {
          config.origin_list = std::vector<uint8_t>();
        }

        config.origin_list->push_back(static_cast<uint8_t>(origin.size() >> 8));
        config.origin_list->push_back(origin.size() & 0xFF);
        std::ranges::copy(origin, std::back_inserter(*config.origin_list));

        break;
      }
      }
      break;
    default:
      break;
    }
  }

  if (argc - optind < 4) {
    std::println(stderr, "Too few arguments");
    print_usage(stderr);
    exit(EXIT_FAILURE);
  }

  auto addr = argv[optind++];
  auto port = argv[optind++];
  auto private_key_file = argv[optind++];
  auto cert_file = argv[optind++];

  if (auto n = util::parse_uint(port); !n) {
    std::println(stderr, "port: invalid port number");
    exit(EXIT_FAILURE);
  } else if (*n > 65535) {
    std::println(stderr, "port: must not exceed 65535");
    exit(EXIT_FAILURE);
  } else {
    config.port = static_cast<uint16_t>(*n);
  }

  if (auto mt = util::read_mime_types(config.mime_types_file); !mt) {
    std::println(stderr,
                 "mime-types-file: Could not read MIME media types file {}",
                 config.mime_types_file.native());
  } else {
    config.mime_types = std::move(*mt);
  }

  if (!ech_config_file.empty()) {
    auto ech_config = util::read_ech_server_config(ech_config_file);
    if (!ech_config) {
      std::println(stderr,
                   "ech-config-file: Could not read private key and ECHConfig");
      exit(EXIT_FAILURE);
    }

    config.ech_config = std::move(*ech_config);
  }

  auto maybe_ssl_ctx =
    create_ssl_ctx(private_key_file, cert_file, ProtoCodec::protocol);
  if (!maybe_ssl_ctx) {
    exit(EXIT_FAILURE);
  }

  auto ssl_ctx = *maybe_ssl_ctx;

  std::println(stderr, "Using document root {}", config.htdocs.native());

  auto ev_loop_d = defer([] { ev_loop_destroy(EV_DEFAULT); });

  auto keylog_filename = getenv("SSLKEYLOGFILE");
  if (keylog_filename) {
    util::keylog_file.open(keylog_filename, std::ios_base::app);
    if (util::keylog_file) {
      util::enable_keylog(ssl_ctx);
    }
  }

  util::ignore_sigpipe();

  Server s(EV_DEFAULT, ssl_ctx);
  if (!s.init(addr, port)) {
    exit(EXIT_FAILURE);
  }

  ev_run(EV_DEFAULT, 0);

  return EXIT_SUCCESS;
}
