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
#ifndef HTTP3_SERVER_PROTO_CODEC_H
#define HTTP3_SERVER_PROTO_CODEC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#include <vector>
#include <expected>
#include <optional>

#include <dwnx/dwnx.h>
#include <nghttp3/nghttp3.h>

#include "shared.h"
#include "server_base.h"

struct Stream;
class Handler;

namespace dwnx {

class ProtoCodec {
public:
  ProtoCodec(Handler *handler, dwnx_ccerr &last_error);
  ~ProtoCodec();

  std::expected<void, Error> acked_stream_data_offset(int64_t stream_id,
                                                      uint64_t datalen);

  std::expected<void, Error> on_stream_reset(int64_t stream_id);

  std::expected<void, Error> on_stream_stop_sending(int64_t stream_id);

  void extend_max_remote_streams_bidi(uint64_t max_streams);

  std::expected<void, Error> extend_max_stream_data(int64_t stream_id,
                                                    uint64_t max_data);

  std::expected<void, Error> write_stream_data_offset(int64_t stream_id,
                                                      size_t len);

  std::expected<void, Error> setup_codec();

  std::expected<std::span<const uint8_t>, Error>
  write_record(std::span<uint8_t> dest, dwnx_tstamp ts);

  std::expected<void, Error> recv_stream_data(uint32_t flags, int64_t stream_id,
                                              std::span<const uint8_t> data);

  std::expected<void, Error>
  on_stream_close(int64_t stream_id, std::optional<uint64_t> rx_app_error_code,
                  std::optional<uint64_t> tx_app_error_code);

  std::expected<void, Error> start_response(Stream *stream);

  // The following functions are made public so that they can be
  // called from nghttp3 callback functions.
  void http_acked_stream_data(Stream *stream, uint64_t datalen);

  void http_consume(int64_t stream_id, size_t nconsumed);

  void http_begin_request_headers(int64_t stream_id);

  void http_recv_request_header(Stream *stream, int32_t token,
                                nghttp3_rcbuf *name, nghttp3_rcbuf *value);

  std::expected<void, Error> http_end_request_headers(Stream *stream);

  std::expected<void, Error> http_stop_sending(int64_t stream_id,
                                               uint64_t app_error_code);

  std::expected<void, Error> http_end_stream(Stream *stream);

  std::expected<void, Error> http_reset_stream(int64_t stream_id,
                                               uint64_t app_error_code);

  static constexpr auto protocol = AppProtocol::H3;

private:
  std::expected<void, Error>
  send_status_response(Stream *stream, unsigned int status_code,
                       std::vector<HTTPHeader> extra_headers = {});

  std::expected<void, Error> send_redirect_response(Stream *stream,
                                                    unsigned int status_code,
                                                    std::string_view path);

  void http_stream_close(int64_t stream_id,
                         std::optional<uint64_t> rx_app_error_code,
                         std::optional<uint64_t> tx_app_error_code);

  Handler *handler_;
  dwnx_conn *conn_;
  dwnx_ccerr &last_error_;
  nghttp3_conn *httpconn_{};
};

} // namespace dwnx

#endif // !defined(HTTP3_SERVER_PROTO_CODEC_H)
