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
#ifndef DWNX_H
#define DWNX_H

/* Define WIN32 when build target is Win32 API (borrowed from
   libcurl) */
#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32
#endif /* (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32) */

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4324)
#endif /* defined(_MSC_VER) */

#include <stdlib.h>
#if defined(_MSC_VER) && (_MSC_VER < 1800)
/* MSVC < 2013 does not have inttypes.h because it is not C99
   compliant.  See compiler macros and version number in
   https://sourceforge.net/p/predef/wiki/Compilers/ */
#  include <stdint.h>
#else /* !(defined(_MSC_VER) && (_MSC_VER < 1800)) */
#  include <inttypes.h>
#endif /* !(defined(_MSC_VER) && (_MSC_VER < 1800)) */
#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>

#include <dwnx/version.h>

#ifdef DWNX_STATICLIB
#  define DWNX_EXTERN
#elif defined(WIN32)
#  ifdef BUILDING_DWNX
#    define DWNX_EXTERN __declspec(dllexport)
#  else /* !defined(BUILDING_DWNX) */
#    define DWNX_EXTERN __declspec(dllimport)
#  endif /* !defined(BUILDING_DWNX) */
#else    /* !(defined(DWNX_STATICLIB) || defined(WIN32)) */
#  ifdef BUILDING_DWNX
#    define DWNX_EXTERN __attribute__((visibility("default")))
#  else /* !defined(BUILDING_DWNX) */
#    define DWNX_EXTERN
#  endif /* !defined(BUILDING_DWNX) */
#endif   /* !(defined(DWNX_STATICLIB) || defined(WIN32)) */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/**
 * @macrosection
 *
 * QUIC specific macros
 */

/**
 * @macro
 *
 * :macro:`DWNX_MAX_VARINT` is the maximum value which can be encoded
 * in variable-length integer encoding.
 */
#define DWNX_MAX_VARINT ((1ULL << 62) - 1)

#define DWNX_DEFAULT_MAX_RECORD_SIZE 16382

/**
 * @macrosection
 *
 * dwnx library error codes
 */

/**
 * @macro
 *
 * :macro:`DWNX_ERR_INVALID_ARGUMENT` indicates that a passed argument
 * is invalid.
 */
#define DWNX_ERR_INVALID_ARGUMENT -201
/**
 * @macro
 *
 * :macro:`DWNX_ERR_NOBUF` indicates that a provided buffer does not
 * have enough space to store data.
 */
#define DWNX_ERR_NOBUF -202
/**
 * @macro
 *
 * :macro:`DWNX_ERR_PROTO` indicates a general protocol error.
 */
#define DWNX_ERR_PROTO -203
/**
 * @macro
 *
 * :macro:`DWNX_ERR_INVALID_STATE` indicates that a requested
 * operation is not allowed at the current connection state.
 */
#define DWNX_ERR_INVALID_STATE -204
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_ID_BLOCKED` indicates that there is no
 * spare stream ID available.
 */
#define DWNX_ERR_STREAM_ID_BLOCKED -206
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_IN_USE` indicates that a stream ID is
 * already in use.
 */
#define DWNX_ERR_STREAM_IN_USE -207
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_DATA_BLOCKED` indicates that stream data
 * cannot be sent because of flow control.
 */
#define DWNX_ERR_STREAM_DATA_BLOCKED -208
/**
 * @macro
 *
 * :macro:`DWNX_ERR_FLOW_CONTROL` indicates flow control error.
 */
#define DWNX_ERR_FLOW_CONTROL -209
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_LIMIT` indicates that a remote endpoint
 * opens more streams that is permitted.
 */
#define DWNX_ERR_STREAM_LIMIT -211
/**
 * @macro
 *
 * :macro:`DWNX_ERR_FINAL_SIZE` indicates that inconsistent final size
 * of a stream.
 */
#define DWNX_ERR_FINAL_SIZE -212
/**
 * @macro
 *
 * :macro:`DWNX_ERR_REQUIRED_TRANSPORT_PARAM` indicates that a
 * required transport parameter is missing.
 */
#define DWNX_ERR_REQUIRED_TRANSPORT_PARAM -215
/**
 * @macro
 *
 * :macro:`DWNX_ERR_MALFORMED_TRANSPORT_PARAM` indicates that a
 * transport parameter is malformed.
 */
#define DWNX_ERR_MALFORMED_TRANSPORT_PARAM -216
/**
 * @macro
 *
 * :macro:`DWNX_ERR_FRAME_ENCODING` indicates there is an error in
 * frame encoding.
 */
#define DWNX_ERR_FRAME_ENCODING -217
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_SHUT_WR` indicates no more data can be sent
 * to a stream.
 */
#define DWNX_ERR_STREAM_SHUT_WR -219
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_NOT_FOUND` indicates that a stream was not
 * found.
 */
#define DWNX_ERR_STREAM_NOT_FOUND -220
/**
 * @macro
 *
 * :macro:`DWNX_ERR_STREAM_STATE` indicates that a requested operation
 * is not allowed at the current stream state.
 */
#define DWNX_ERR_STREAM_STATE -221
/**
 * @macro
 *
 * :macro:`DWNX_ERR_CLOSING` indicates that connection is in closing
 * state.
 */
#define DWNX_ERR_CLOSING -223
/**
 * @macro
 *
 * :macro:`DWNX_ERR_DRAINING` indicates that connection is in draining
 * state.
 */
#define DWNX_ERR_DRAINING -224
/**
 * @macro
 *
 * :macro:`DWNX_ERR_TRANSPORT_PARAM` indicates a general transport
 * parameter error.
 */
#define DWNX_ERR_TRANSPORT_PARAM -225
/**
 * @macro
 *
 * :macro:`DWNX_ERR_INTERNAL` indicates an internal error.
 */
#define DWNX_ERR_INTERNAL -228
/**
 * @macro
 *
 * :macro:`DWNX_ERR_WRITE_MORE` indicates
 * :macro:`DWNX_WRITE_STREAM_FLAG_MORE` is used and a function call
 * succeeded.
 */
#define DWNX_ERR_WRITE_MORE -230
/**
 * @macro
 *
 * :macro:`DWNX_ERR_IDLE_CLOSE` indicates the connection should be
 * closed silently because of idle timeout.
 */
#define DWNX_ERR_IDLE_CLOSE -238
/**
 * @macro
 *
 * :macro:`DWNX_ERR_FATAL` indicates that error codes less than this
 * value is fatal error.  When this error is returned, an endpoint
 * should close connection immediately.
 */
#define DWNX_ERR_FATAL -500
/**
 * @macro
 *
 * :macro:`DWNX_ERR_NOMEM` indicates out of memory.
 */
#define DWNX_ERR_NOMEM -501
/**
 * @macro
 *
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` indicates that user defined
 * callback function failed.
 */
#define DWNX_ERR_CALLBACK_FAILURE -502

/**
 * @macrosection
 *
 * QUIC transport error code
 */

/**
 * @macro
 *
 * :macro:`DWNX_NO_ERROR` is QUIC transport error code ``NO_ERROR``.
 */
#define DWNX_NO_ERROR 0x0U

/**
 * @macro
 *
 * :macro:`DWNX_INTERNAL_ERROR` is QUIC transport error code
 * ``INTERNAL_ERROR``.
 */
#define DWNX_INTERNAL_ERROR 0x1U

/**
 * @macro
 *
 * :macro:`DWNX_CONNECTION_REFUSED` is QUIC transport error code
 * ``CONNECTION_REFUSED``.
 */
#define DWNX_CONNECTION_REFUSED 0x2U

/**
 * @macro
 *
 * :macro:`DWNX_FLOW_CONTROL_ERROR` is QUIC transport error code
 * ``FLOW_CONTROL_ERROR``.
 */
#define DWNX_FLOW_CONTROL_ERROR 0x3U

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_LIMIT_ERROR` is QUIC transport error code
 * ``STREAM_LIMIT_ERROR``.
 */
#define DWNX_STREAM_LIMIT_ERROR 0x4U

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_STATE_ERROR` is QUIC transport error code
 * ``STREAM_STATE_ERROR``.
 */
#define DWNX_STREAM_STATE_ERROR 0x5U

/**
 * @macro
 *
 * :macro:`DWNX_FINAL_SIZE_ERROR` is QUIC transport error code
 * ``FINAL_SIZE_ERROR``.
 */
#define DWNX_FINAL_SIZE_ERROR 0x6U

/**
 * @macro
 *
 * :macro:`DWNX_FRAME_ENCODING_ERROR` is QUIC transport error code
 * ``FRAME_ENCODING_ERROR``.
 */
#define DWNX_FRAME_ENCODING_ERROR 0x7U

/**
 * @macro
 *
 * :macro:`DWNX_TRANSPORT_PARAMETER_ERROR` is QUIC transport error
 * code ``TRANSPORT_PARAMETER_ERROR``.
 */
#define DWNX_TRANSPORT_PARAMETER_ERROR 0x8U

/**
 * @macro
 *
 * :macro:`DWNX_PROTOCOL_VIOLATION` is QUIC transport error code
 * ``PROTOCOL_VIOLATION``.
 */
#define DWNX_PROTOCOL_VIOLATION 0xAU

/**
 * @macro
 *
 * :macro:`DWNX_APPLICATION_ERROR` is QUIC transport error code
 * ``APPLICATION_ERROR``.
 */
#define DWNX_APPLICATION_ERROR 0xCU

/**
 * @typedef
 *
 * :type:`dwnx_ssize` is signed counterpart of size_t.
 */
typedef ptrdiff_t dwnx_ssize;

/**
 * @functypedef
 *
 * :type:`dwnx_malloc` is a custom memory allocator to replace
 * :manpage:`malloc(3)`.  The |user_data| is
 * :member:`dwnx_mem.user_data`.
 */
typedef void *(*dwnx_malloc)(size_t size, void *user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_free` is a custom memory allocator to replace
 * :manpage:`free(3)`.  The |user_data| is
 * :member:`dwnx_mem.user_data`.
 */
typedef void (*dwnx_free)(void *ptr, void *user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_calloc` is a custom memory allocator to replace
 * :manpage:`calloc(3)`.  The |user_data| is the
 * :member:`dwnx_mem.user_data`.
 */
typedef void *(*dwnx_calloc)(size_t nmemb, size_t size, void *user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_realloc` is a custom memory allocator to replace
 * :manpage:`realloc(3)`.  The |user_data| is the
 * :member:`dwnx_mem.user_data`.
 */
typedef void *(*dwnx_realloc)(void *ptr, size_t size, void *user_data);

/**
 * @struct
 *
 * :type:`dwnx_mem` is a custom memory allocator.  The
 * :member:`user_data` field is passed to each allocator function.
 * This can be used, for example, to achieve per-connection memory
 * pool.
 *
 * In the following example code, ``my_malloc``, ``my_free``,
 * ``my_calloc`` and ``my_realloc`` are the replacement of the
 * standard allocators :manpage:`malloc(3)`, :manpage:`free(3)`,
 * :manpage:`calloc(3)` and :manpage:`realloc(3)` respectively::
 *
 *     void *my_malloc_cb(size_t size, void *user_data) {
 *       (void)user_data;
 *       return my_malloc(size);
 *     }
 *
 *     void my_free_cb(void *ptr, void *user_data) {
 *       (void)user_data;
 *       my_free(ptr);
 *     }
 *
 *     void *my_calloc_cb(size_t nmemb, size_t size, void *user_data) {
 *       (void)user_data;
 *       return my_calloc(nmemb, size);
 *     }
 *
 *     void *my_realloc_cb(void *ptr, size_t size, void *user_data) {
 *       (void)user_data;
 *       return my_realloc(ptr, size);
 *     }
 *
 *     void conn_new() {
 *       dwnx_mem mem = {
 *         .malloc = my_malloc_cb,
 *         .free = my_free_cb,
 *         .calloc = my_calloc_cb,
 *         .realloc = my_realloc_cb,
 *       };
 *
 *       ...
 *     }
 */
typedef struct dwnx_mem {
  /**
   * :member:`user_data` is an arbitrary user supplied data.  This
   * is passed to each allocator function.
   */
  void *user_data;
  /**
   * :member:`malloc` is a custom allocator function to replace
   * :manpage:`malloc(3)`.
   */
  dwnx_malloc malloc;
  /**
   * :member:`free` is a custom allocator function to replace
   * :manpage:`free(3)`.
   */
  dwnx_free free;
  /**
   * :member:`calloc` is a custom allocator function to replace
   * :manpage:`calloc(3)`.
   */
  dwnx_calloc calloc;
  /**
   * :member:`realloc` is a custom allocator function to replace
   * :manpage:`realloc(3)`.
   */
  dwnx_realloc realloc;
} dwnx_mem;

/**
 * @function
 *
 * `dwnx_mem_default` returns the default, system standard memory
 * allocator.
 */
DWNX_EXTERN const dwnx_mem *dwnx_mem_default(void);

/**
 * @struct
 *
 * :type:`dwnx_vec` is struct iovec compatible structure to reference
 * arbitrary array of bytes.
 */
typedef struct dwnx_vec {
  /**
   * :member:`base` points to the data.
   */
  uint8_t *base;
  /**
   * :member:`len` is the number of bytes which the buffer pointed by
   * base contains.
   */
  size_t len;
} dwnx_vec;

/**
 * @typedef
 *
 * :type:`dwnx_tstamp` is a timestamp with nanosecond resolution.
 * ``UINT64_MAX`` is an invalid value, and it is often used to
 * indicate that no value is set.
 */
typedef uint64_t dwnx_tstamp;

/**
 * @typedef
 *
 * :type:`dwnx_duration` is a period of time in nanosecond resolution.
 * ``UINT64_MAX`` is an invalid value, and it is often used to
 * indicate that no value is set.
 */
typedef uint64_t dwnx_duration;

/**
 * @macrosection
 *
 * Time related macros
 */

/**
 * @macro
 *
 * :macro:`DWNX_NANOSECONDS` is a count of tick which corresponds to
 * 1 nanosecond.
 */
#define DWNX_NANOSECONDS ((dwnx_duration)1ULL)

/**
 * @macro
 *
 * :macro:`DWNX_MICROSECONDS` is a count of tick which corresponds
 * to 1 microsecond.
 */
#define DWNX_MICROSECONDS ((dwnx_duration)(1000ULL * DWNX_NANOSECONDS))

/**
 * @macro
 *
 * :macro:`DWNX_MILLISECONDS` is a count of tick which corresponds
 * to 1 millisecond.
 */
#define DWNX_MILLISECONDS ((dwnx_duration)(1000ULL * DWNX_MICROSECONDS))

/**
 * @macro
 *
 * :macro:`DWNX_SECONDS` is a count of tick which corresponds to 1
 * second.
 */
#define DWNX_SECONDS ((dwnx_duration)(1000ULL * DWNX_MILLISECONDS))

/**
 * @macro
 *
 * :macro:`DWNX_MINUTES` is a count of tick which corresponds to 1
 * minute.
 */
#define DWNX_MINUTES ((dwnx_duration)(60ULL * DWNX_SECONDS))

typedef struct dwnx_transport_params {
  /**
   * :member:`initial_max_stream_data_bidi_local` is the size of flow
   * control window of locally initiated stream.  This is the number
   * of bytes that the remote endpoint can send, and the local
   * endpoint must ensure that it has enough buffer to receive them.
   */
  uint64_t initial_max_stream_data_bidi_local;
  /**
   * :member:`initial_max_stream_data_bidi_remote` is the size of flow
   * control window of remotely initiated stream.  This is the number
   * of bytes that the remote endpoint can send, and the local
   * endpoint must ensure that it has enough buffer to receive them.
   */
  uint64_t initial_max_stream_data_bidi_remote;
  /**
   * :member:`initial_max_stream_data_uni` is the size of flow control
   * window of remotely initiated unidirectional stream.  This is the
   * number of bytes that the remote endpoint can send, and the local
   * endpoint must ensure that it has enough buffer to receive them.
   */
  uint64_t initial_max_stream_data_uni;
  /**
   * :member:`initial_max_data` is the connection level flow control
   * window.
   */
  uint64_t initial_max_data;
  /**
   * :member:`initial_max_streams_bidi` is the number of concurrent
   * streams that the remote endpoint can create.
   */
  uint64_t initial_max_streams_bidi;
  /**
   * :member:`initial_max_streams_uni` is the number of concurrent
   * unidirectional streams that the remote endpoint can create.
   */
  uint64_t initial_max_streams_uni;
  /**
   * :member:`max_idle_timeout` is a duration during which sender
   * allows quiescent.  0 means no idle timeout.  It must not be
   * UINT64_MAX.
   */
  dwnx_duration max_idle_timeout;
  uint64_t max_record_size;
} dwnx_transport_params;

DWNX_EXTERN void dwnx_transport_params_default(dwnx_transport_params *params);

typedef struct dwnx_conn dwnx_conn;

/**
 * @macrosection
 *
 * STREAM frame data flags
 */

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_DATA_FLAG_NONE` indicates no flag set.
 */
#define DWNX_STREAM_DATA_FLAG_NONE 0x00U

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_DATA_FLAG_FIN` indicates that this chunk of
 * data is final piece of an incoming stream.
 */
#define DWNX_STREAM_DATA_FLAG_FIN 0x01U

/**
 * @functypedef
 *
 * :type:`dwnx_recv_stream_data` is invoked when stream data is
 * received.  The stream is specified by |stream_id|.  |flags| is the
 * bitwise-OR of zero or more of :macro:`DWNX_STREAM_DATA_FLAG_*
 * <DWNX_STREAM_DATA_FLAG_NONE>`.  If |flags| &
 * :macro:`DWNX_STREAM_DATA_FLAG_FIN` is nonzero, this portion of the
 * data is the last data in this stream.  |offset| is the offset where
 * this data begins.  The library ensures that data is passed to the
 * application in the non-decreasing order of |offset| without any
 * overlap.  The data is passed as |data| of length |datalen|.
 * |datalen| may be 0 if and only if |fin| is nonzero.
 *
 * The callback function must return 0 if it succeeds, or
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` which makes the library return
 * immediately.
 */
typedef int (*dwnx_recv_stream_data)(dwnx_conn *conn, uint32_t flags,
                                     int64_t stream_id, uint64_t offset,
                                     const uint8_t *data, size_t datalen,
                                     void *user_data, void *stream_user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_stream_open` is a callback function which is called
 * when remote stream is opened by a remote endpoint.  This function
 * is not called if stream is opened by implicitly (we might
 * reconsider this behaviour later).
 *
 * The implementation of this callback should return 0 if it succeeds.
 * Returning :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call
 * return immediately.
 */
typedef int (*dwnx_stream_open)(dwnx_conn *conn, int64_t stream_id,
                                void *user_data);

/**
 * @macrosection
 *
 * Stream close flags for :type:`dwnx_stream_close` callback.
 */

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_CLOSE_FLAG_NONE` indicates no flag set.
 */
#define DWNX_STREAM_CLOSE_FLAG_NONE 0x00U

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_CLOSE_FLAG_RX_APP_ERROR_CODE_SET` indicates
 * that rx_app_error_code parameter is set.
 */
#define DWNX_STREAM_CLOSE_FLAG_RX_APP_ERROR_CODE_SET 0x01U

/**
 * @macro
 *
 * :macro:`DWNX_STREAM_CLOSE_FLAG_TX_APP_ERROR_CODE_SET` indicates
 * that tx_app_error_code parameter is set.
 */
#define DWNX_STREAM_CLOSE_FLAG_TX_APP_ERROR_CODE_SET 0x02U

/**
 * @functypedef
 *
 * :type:`dwnx_stream_close` is invoked when a stream is closed.  This
 * callback is not called when QUIC connection is closed before
 * existing streams are closed.  |flags| is the bitwise-OR of zero or
 * more of :macro:`DWNX_STREAM_CLOSE_FLAG_*
 * <DWNX_STREAM_CLOSE_FLAG_NONE>`.  |rx_app_error_code| indicates the
 * error code that shut down the receiving side of the stream if
 * :macro:`DWNX_STREAM_CLOSE_FLAG_RX_APP_ERROR_CODE_SET` is set in
 * |flags|.  |tx_app_error_code| indicates the error code that shut
 * down the sending side of the stream if
 * :macro:`DWNX_STREAM_CLOSE_FLAG_TX_APP_ERROR_CODE_SET` is set in
 * |flags|.
 *
 * Because QUIC can close the send and receive sides of a stream
 * independently, this callback has 2 application error codes for both
 * directions.  No error code means that its direction of a stream is
 * closed cleanly.  For example, a client gets STOP_SENDING frame from
 * a server, and it sends back RESET_STREAM frame with the error code
 * included in STOP_SENDING frame.  This error code is reported as
 * |tx_app_error_code| and
 * :macro:`DWNX_STREAM_CLOSE_FLAG_TX_APP_ERROR_CODE_SET` is set in
 * |flags|.  Meanwhile, the client receives the response body without
 * any error.  Then
 * :macro:`DWNX_STREAM_CLOSE_FLAG_RX_APP_ERROR_CODE_SET` is not set in
 * |flags|.
 *
 * The implementation of this callback should return 0 if it succeeds.
 * Returning :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library
 * call return immediately.
 */
typedef int (*dwnx_stream_close)(dwnx_conn *conn, uint32_t flags,
                                 int64_t stream_id, uint64_t rx_app_error_code,
                                 uint64_t tx_app_error_code, void *user_data,
                                 void *stream_user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_stream_reset` is invoked when a stream identified by
 * |stream_id| is reset by a remote endpoint.
 *
 * The implementation of this callback should return 0 if it succeeds.
 * Returning :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call
 * return immediately.
 */
typedef int (*dwnx_stream_reset)(dwnx_conn *conn, int64_t stream_id,
                                 uint64_t final_size, uint64_t app_error_code,
                                 void *user_data, void *stream_user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_recv_stop_sending` is invoked when a STOP_SENDING frame
 * is received from a remote endpoint for a stream identified by
 * |stream_id|.  |app_error_code| is the application error code carried
 * by the STOP_SENDING frame.  This callback is called at most
 * once per stream.
 *
 * The callback function must return 0 if it succeeds.  Returning
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call return
 * immediately.
 */
typedef int (*dwnx_recv_stop_sending)(dwnx_conn *conn, int64_t stream_id,
                                      uint64_t app_error_code, void *user_data,
                                      void *stream_user_data);

typedef struct dwnx_callbacks {
  /**
   * :member:`recv_stream_data` is a callback function which is
   * invoked when stream data, which includes application data, is
   * received.  This callback function is optional.
   */
  dwnx_recv_stream_data recv_stream_data;
  /**
   * :member:`stream_open` is a callback function which is invoked
   * when new remote stream is opened by a remote endpoint.  This
   * callback function is optional.
   */
  dwnx_stream_open stream_open;
  /**
   * :member:`stream_close` is a callback function which is invoked
   * when a stream is closed.  This callback function is optional.
   */
  dwnx_stream_close stream_close;
  /**
   * :member:`stream_reset` is a callback function which is invoked
   * when a stream is reset by a remote endpoint.  This callback
   * function is optional.
   */
  dwnx_stream_reset stream_reset;
  /**
   * :member:`recv_stop_sending` is a callback function which is invoked
   * when a STOP_SENDING frame is received from a remote endpoint.  This
   * callback function is optional.
   */
  dwnx_recv_stop_sending recv_stop_sending;
} dwnx_callbacks;

DWNX_EXTERN int dwnx_conn_server_new(dwnx_conn **pconn,
                                     const dwnx_callbacks *callbacks,
                                     const dwnx_transport_params *params,
                                     const dwnx_mem *mem, void *user_data);

DWNX_EXTERN int dwnx_conn_client_new(dwnx_conn **pconn,
                                     const dwnx_callbacks *callbacks,
                                     const dwnx_transport_params *params,
                                     const dwnx_mem *mem, void *user_data);

DWNX_EXTERN void dwnx_conn_del(dwnx_conn *conn);

DWNX_EXTERN int dwnx_conn_read(dwnx_conn *conn, const uint8_t *data,
                               size_t datalen, dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_conn_extend_max_offset` extends max data offset by
 * |datalen|.  This function only extends connection-level flow
 * control window.
 */
DWNX_EXTERN void dwnx_conn_extend_max_offset(dwnx_conn *conn, uint64_t datalen);

/**
 * @function
 *
 * `dwnx_err_is_fatal` returns nonzero if |liberr| is a fatal error.
 * |liberr| must be one of dwnx library error codes (which is defined
 * as :macro:`DWNX_ERR_* <DWNX_ERR_INVALID_ARGUMENT>` macros).
 */
DWNX_EXTERN int dwnx_err_is_fatal(int liberr);

#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* defined(_MSC_VER) */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(DWNX_H) */
