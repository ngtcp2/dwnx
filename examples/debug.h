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
#ifndef DEBUG_H
#define DEBUG_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif // defined(HAVE_CONFIG_H)

#ifndef __STDC_FORMAT_MACROS
// For travis and PRIu64
#  define __STDC_FORMAT_MACROS
#endif // !defined(__STDC_FORMAT_MACROS)

#include <cinttypes>
#include <string_view>
#include <span>

#include <dwnx/dwnx.h>
#include <nghttp3/nghttp3.h>

namespace dwnx {

namespace debug {

void print_stream_data(int64_t stream_id, std::span<const uint8_t> data);

void log_write(void *user_data, char *msg, size_t len);

void print_http_begin_request_headers(int64_t stream_id);

void print_http_begin_response_headers(int64_t stream_id);

void print_http_header(int64_t stream_id, const nghttp3_rcbuf *name,
                       const nghttp3_rcbuf *value, uint8_t flags);

void print_http_end_headers(int64_t stream_id);

void print_http_data(int64_t stream_id, std::span<const uint8_t> data);

void print_http_begin_trailers(int64_t stream_id);

void print_http_end_trailers(int64_t stream_id);

void print_http_request_headers(int64_t stream_id, const nghttp3_nv *nva,
                                size_t nvlen);

void print_http_response_headers(int64_t stream_id, const nghttp3_nv *nva,
                                 size_t nvlen);

void print_http_settings(const nghttp3_proto_settings *settings);

void print_http_origin(const uint8_t *origin, size_t originlen);

void print_http_end_origin();

} // namespace debug

} // namespace dwnx

#endif // !defined(DEBUG_H)
