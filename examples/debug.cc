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
#include "debug.h"

#include <unistd.h>

#include <cassert>
#include <random>
#include <array>

#include "util.h"

using namespace std::literals;

namespace dwnx {

namespace debug {

namespace {
auto randgen = util::make_mt19937();
} // namespace

namespace {
auto *outfile = stderr;
} // namespace

void print_stream_data(int64_t stream_id, std::span<const uint8_t> data) {
  std::println(outfile, "Ordered STREAM data stream_id={:#x}", stream_id);
  (void)util::hexdump(outfile, data);
}

void log_write(void *user_data, char *msg, size_t len) {
  msg[len++] = '\n';

  while (write(fileno(stderr), msg, len) == -1 && errno == EINTR)
    ;
}

void print_http_begin_request_headers(int64_t stream_id) {
  std::println(outfile, "http: stream {:#x} request headers started",
               stream_id);
}

void print_http_begin_response_headers(int64_t stream_id) {
  std::println(outfile, "http: stream {:#x} response headers started",
               stream_id);
}

namespace {
void print_header(std::span<const uint8_t> name, std::span<const uint8_t> value,
                  uint8_t flags) {
  std::println(outfile, "[{}: {}]{}", as_string_view(name),
               as_string_view(value),
               (flags & NGHTTP3_NV_FLAG_NEVER_INDEX) ? "(sensitive)" : "");
}
} // namespace

namespace {
void print_header(const nghttp3_rcbuf *name, const nghttp3_rcbuf *value,
                  uint8_t flags) {
  auto namebuf = nghttp3_rcbuf_get_buf(name);
  auto valuebuf = nghttp3_rcbuf_get_buf(value);
  print_header({namebuf.base, namebuf.len}, {valuebuf.base, valuebuf.len},
               flags);
}
} // namespace

namespace {
void print_header(const nghttp3_nv &nv) {
  print_header({nv.name, nv.namelen}, {nv.value, nv.valuelen}, nv.flags);
}
} // namespace

void print_http_header(int64_t stream_id, const nghttp3_rcbuf *name,
                       const nghttp3_rcbuf *value, uint8_t flags) {
  std::print(outfile, "http: stream {:#x} ", stream_id);
  print_header(name, value, flags);
}

void print_http_end_headers(int64_t stream_id) {
  std::println(outfile, "http: stream {:#x} headers ended", stream_id);
}

void print_http_data(int64_t stream_id, std::span<const uint8_t> data) {
  std::println(outfile, "http: stream {:#x} body {} bytes", stream_id,
               data.size());
  (void)util::hexdump(outfile, data);
}

void print_http_begin_trailers(int64_t stream_id) {
  std::println(outfile, "http: stream {:#x} trailers started", stream_id);
}

void print_http_end_trailers(int64_t stream_id) {
  std::println(outfile, "http: stream {:#x} trailers ended", stream_id);
}

void print_http_request_headers(int64_t stream_id, const nghttp3_nv *nva,
                                size_t nvlen) {
  std::println(outfile, "http: stream {:#x} submit request headers", stream_id);
  for (size_t i = 0; i < nvlen; ++i) {
    auto &nv = nva[i];
    print_header(nv);
  }
}

void print_http_response_headers(int64_t stream_id, const nghttp3_nv *nva,
                                 size_t nvlen) {
  std::println(outfile, "http: stream {:#x} submit response headers",
               stream_id);
  for (size_t i = 0; i < nvlen; ++i) {
    auto &nv = nva[i];
    print_header(nv);
  }
}

void print_http_settings(const nghttp3_proto_settings *settings) {
  std::println(outfile, R"(http: remote settings
http: SETTINGS_MAX_FIELD_SECTION_SIZE={}
http: SETTINGS_QPACK_MAX_TABLE_CAPACITY={}
http: SETTINGS_QPACK_BLOCKED_STREAMS={}
http: SETTINGS_ENABLE_CONNECT_PROTOCOL={}
http: SETTINGS_H3_DATAGRAM={})",
               settings->max_field_section_size,
               settings->qpack_max_dtable_capacity,
               settings->qpack_blocked_streams,
               settings->enable_connect_protocol, settings->h3_datagram);
}

void print_http_origin(const uint8_t *origin, size_t originlen) {
  std::println(outfile, "http: origin [{}]",
               as_string_view(std::span{origin, originlen}));
}

void print_http_end_origin() { std::println(outfile, "http: origin ended"); }

} // namespace debug

} // namespace dwnx
