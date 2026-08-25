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
#ifndef HTTP3_CLIENT_PROTO_CODEC_H
#define HTTP3_CLIENT_PROTO_CODEC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#include <vector>
#include <expected>
#include <optional>

#include <dwnx/dwnx.h>
#include <nghttp3/nghttp3.h>

#include "shared.h"
#include "client_base.h"

struct Stream;
class Client;

namespace dwnx {

class ProtoCodec {
public:
  ProtoCodec(Client *handler, dwnx_ccerr &last_error);
  ~ProtoCodec();

  std::expected<void, Error> recv_stream_data(uint32_t flags, int64_t stream_id,
                                              std::span<const uint8_t> data);

  std::expected<void, Error> acked_stream_data_offset(int64_t stream_id,
                                                      uint64_t datalen);

  std::expected<void, Error> extend_max_stream_data(int64_t stream_id,
                                                    uint64_t max_data);

  std::expected<void, Error> write_stream_data_offset(int64_t stream_id,
                                                      size_t len);

  void early_data_rejected();

  std::expected<void, Error>
  on_stream_close(int64_t stream_id, std::optional<uint64_t> rx_app_error_code,
                  std::optional<uint64_t> tx_app_error_code);

  std::expected<void, Error> on_stream_reset(int64_t stream_id);

  std::expected<void, Error> on_stream_stop_sending(int64_t stream_id);

  std::expected<void, Error> submit_request(const Stream *stream);

  std::expected<std::span<const uint8_t>, Error>
  write_record(std::span<uint8_t> dest, dwnx_tstamp ts);

  std::expected<void, Error> setup_codec();

  // The following functions are made public so that they can be
  // called from nghttp3 callback functions.
  std::expected<void, Error> stop_sending(int64_t stream_id,
                                          uint64_t app_error_code);

  std::expected<void, Error> reset_stream(int64_t stream_id,
                                          uint64_t app_error_code);

  void http_consume(int64_t stream_id, size_t nconsumed);

  void http_write_data(int64_t stream_id, std::span<const uint8_t> data);

  static constexpr auto protocol = AppProtocol::H3;
  static constexpr auto no_error = NGHTTP3_H3_NO_ERROR;

private:
  void http_stream_close(int64_t stream_id,
                         std::optional<uint64_t> rx_app_error_code,
                         std::optional<uint64_t> tx_app_error_code);

  Client *client_;
  dwnx_conn *conn_;
  dwnx_ccerr &last_error_;
  nghttp3_conn *httpconn_{};
};

} // namespace dwnx

#endif // !defined(HTTP3_CLIENT_PROTO_CODEC_H)
