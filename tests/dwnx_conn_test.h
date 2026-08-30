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
#ifndef DWNX_CONN_TEST_H
#define DWNX_CONN_TEST_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#define MUNIT_ENABLE_ASSERT_ALIASES

#include "munit.h"

extern const MunitSuite conn_suite;

munit_void_test_decl(test_dwnx_conn_recv_transport_params)
munit_void_test_decl(test_dwnx_conn_recv_stream)
munit_void_test_decl(test_dwnx_conn_recv_stream_data)
munit_void_test_decl(test_dwnx_conn_recv_reset_stream)
munit_void_test_decl(test_dwnx_conn_recv_stop_sending)
munit_void_test_decl(test_dwnx_conn_recv_max_data)
munit_void_test_decl(test_dwnx_conn_recv_max_stream_data)
munit_void_test_decl(test_dwnx_conn_recv_max_streams_bidi)
munit_void_test_decl(test_dwnx_conn_recv_max_streams_uni)
munit_void_test_decl(test_dwnx_conn_recv_data_blocked)
munit_void_test_decl(test_dwnx_conn_recv_stream_data_blocked)
munit_void_test_decl(test_dwnx_conn_recv_streams_blocked_bidi)
munit_void_test_decl(test_dwnx_conn_recv_streams_blocked_uni)
munit_void_test_decl(test_dwnx_conn_recv_connection_close)
munit_void_test_decl(test_dwnx_conn_recv_connection_close_app)
munit_void_test_decl(test_dwnx_conn_recv_padding)
munit_void_test_decl(test_dwnx_conn_recv_qx_ping)
munit_void_test_decl(test_dwnx_conn_extend_max_stream_offset)
munit_void_test_decl(test_dwnx_conn_extend_max_offset)
munit_void_test_decl(test_dwnx_conn_extend_max_streams)
munit_void_test_decl(test_dwnx_conn_writev_stream)
munit_void_test_decl(test_dwnx_conn_send_stream_data_blocked)
munit_void_test_decl(test_dwnx_conn_handle_expiry)
munit_void_test_decl(test_dwnx_conn_write_connection_close)
munit_void_test_decl(test_dwnx_conn_encode_0rtt_transport_params)
munit_void_test_decl(test_dwnx_conn_validate_early_transport_params)
munit_void_test_decl(test_dwnx_conn_tls_early_data_rejected)
munit_void_test_decl(test_dwnx_conn_stream_close)
munit_void_test_decl(test_dwnx_conn_read)
munit_void_test_decl(test_dwnx_conn_close_stream)
munit_void_test_decl(test_dwnx_conn_shutdown_stream)
munit_void_test_decl(test_dwnx_conn_open_bidi_stream)
munit_void_test_decl(test_dwnx_conn_open_uni_stream)
munit_void_test_decl(test_dwnx_conn_is_local_stream)
munit_void_test_decl(test_dwnx_conn_is_server)
munit_void_test_decl(test_dwnx_conn_get_timestamp)
munit_void_test_decl(test_dwnx_conn_get_local_transport_params)
munit_void_test_decl(test_dwnx_conn_get_idle_expiry)
munit_void_test_decl(test_dwnx_is_bidi_stream)
munit_void_test_decl(test_dwnx_ccerr)

#endif /* !defined(DWNX_CONN_TEST_H) */
