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

/**
 * @macro
 *
 * :macro:`DWNX_DEFAULT_MAX_RECORD_SIZE` is the default maximum QMux
 * record size.
 */
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

/**
 * @struct
 *
 * :type:`dwnx_transport_params` represents QUIC transport parameters.
 */
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
  /**
   * :member:`max_record_size` is the maximum QMux record size that
   * the sender accepts.  It must be greater than or equal to
   * :macro:`DWNX_DEFAULT_MAX_RECORD_SIZE`.
   */
  uint64_t max_record_size;
} dwnx_transport_params;

/**
 * @function
 *
 * `dwnx_transport_params_default` initializes |params| with the
 * default values.  This function first fills |params| with 0, and
 * sets the default values to the following fields:
 *
 * - :member:`max_record_size <dwnx_transport_params.max_record_size>`
 *   = :macro:`DWNX_DEFAULT_MAX_RECORD_SIZE`
 */
DWNX_EXTERN void dwnx_transport_params_default(dwnx_transport_params *params);

/**
 * @struct
 *
 * :type:`dwnx_conn` represents a single QMux connection.
 */
typedef struct dwnx_conn dwnx_conn;

/**
 * @functypedef
 *
 * :type:`dwnx_log_write` is a callback function for logging.
 * |user_data| is the same object passed to `dwnx_conn_client_new` or
 * `dwnx_conn_server_new`.  The caller guarantees that the memory
 * region [|msg|, |msg| + |len|], inclusive, are writable, and
 * |msg|[|len|] == '\0'.  If application needs to emit a single line
 * with a line terminator, one can do msg[len] = '\n', and write |len|
 * + 1 bytes from |msg|.
 */
typedef void (*dwnx_log_write)(void *user_data, char *msg, size_t len);

/**
 * @struct
 *
 * :type:`dwnx_settings` defines QMux connection settings.
 */
typedef struct dwnx_settings {
  /**
   * :member:`conn_id` is the identifier of this connection.
   * Currently, it is used in a log header so that people can
   * distinguish the particular connection from the others.
   */
  uint64_t conn_id;
  /**
   * :member:`initial_ts` is an initial timestamp given to the
   * library.
   */
  dwnx_tstamp initial_ts;
  /**
   * :member:`log_write` is the callback function when a single log
   * message is emitted.  If this field is NULL, logging is disabled.
   */
  dwnx_log_write log_write;
} dwnx_settings;

/**
 * @function
 *
 * `dwnx_settings_default` initializes |settings| with the default
 * values.  Currently, it sets 0 to all fields.
 */
DWNX_EXTERN void dwnx_settings_default(dwnx_settings *settings);

/**
 * @functypedef
 *
 * :type:`dwnx_recv_transport_params` is invoked when transport
 * parameters |params| are received from the remote endpoint.
 *
 * The callback function must return 0 if it succeeds, or
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` which makes the library return
 * immediately.
 */
typedef int (*dwnx_recv_transport_params)(dwnx_conn *conn,
                                          const dwnx_transport_params *params,
                                          void *user_data);

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
 * :type:`dwnx_stream_stop_sending` is invoked when a stream is no
 * longer read by a local endpoint before it receives all stream data.
 * This function is called at most once per stream.  |app_error_code|
 * is the error code passed to `dwnx_conn_shutdown_stream_read` or
 * `dwnx_conn_shutdown_stream`.
 *
 * The callback function must return 0 if it succeeds.  Returning
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call return
 * immediately.
 */
typedef int (*dwnx_stream_stop_sending)(dwnx_conn *conn, int64_t stream_id,
                                        uint64_t app_error_code,
                                        void *user_data,
                                        void *stream_user_data);

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

/**
 * @functypedef
 *
 * :type:`dwnx_extend_max_stream_data` is a callback function which is
 * invoked when max stream data is extended.  |stream_id| identifies
 * the stream.  |max_data| is a cumulative number of bytes an endpoint
 * can send on this stream.
 *
 * The callback function must return 0 if it succeeds.  Returning
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call return
 * immediately.
 */
typedef int (*dwnx_extend_max_stream_data)(dwnx_conn *conn, int64_t stream_id,
                                           uint64_t max_data, void *user_data,
                                           void *stream_user_data);

/**
 * @functypedef
 *
 * :type:`dwnx_extend_max_streams` is a callback function which is
 * called every time max stream ID is strictly extended.
 * |max_streams| is the cumulative number of streams which an endpoint
 * can open.
 *
 * The callback function must return 0 if it succeeds.  Returning
 * :macro:`DWNX_ERR_CALLBACK_FAILURE` makes the library call return
 * immediately.
 */
typedef int (*dwnx_extend_max_streams)(dwnx_conn *conn, uint64_t max_streams,
                                       void *user_data);

/**
 * @struct
 *
 * :type:`dwnx_callbacks` holds a set of callback functions.
 */
typedef struct dwnx_callbacks {
  /**
   * :member:`recv_transport_params` is a callback function which is
   * invoked when transport parameters are received from the remote
   * endpoint.
   */
  dwnx_recv_transport_params recv_transport_params;
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
   * :member:`stream_stop_sending` is a callback function which is
   * invoked when a local endpoint no longer reads from a stream
   * before it receives all stream data.  This callback function is
   * optional.
   */
  dwnx_stream_stop_sending stream_stop_sending;
  /**
   * :member:`recv_stop_sending` is a callback function which is invoked
   * when a STOP_SENDING frame is received from a remote endpoint.  This
   * callback function is optional.
   */
  dwnx_recv_stop_sending recv_stop_sending;
  /**
   * :member:`extend_max_stream_data` is callback function which is
   * invoked when the maximum offset of stream data that a local
   * endpoint can send is increased.  This callback function is
   * optional.
   */
  dwnx_extend_max_stream_data extend_max_stream_data;
  /**
   * :member:`extend_max_local_streams_bidi` is a callback function
   * which is invoked when the number of bidirectional stream which a
   * local endpoint can open is increased.  This callback function is
   * optional.
   */
  dwnx_extend_max_streams extend_max_local_streams_bidi;
  /**
   * :member:`extend_max_local_streams_uni` is a callback function
   * which is invoked when the number of unidirectional stream which a
   * local endpoint can open is increased.  This callback function is
   * optional.
   */
  dwnx_extend_max_streams extend_max_local_streams_uni;
  /**
   * :member:`extend_max_remote_streams_bidi` is a callback function
   * which is invoked when the number of bidirectional streams which a
   * remote endpoint can open is increased.  This callback function is
   * optional.
   */
  dwnx_extend_max_streams extend_max_remote_streams_bidi;
  /**
   * :member:`extend_max_remote_streams_uni` is a callback function
   * which is invoked when the number of unidirectional streams which
   * a remote endpoint can open is increased.  This callback function
   * is optional.
   */
  dwnx_extend_max_streams extend_max_remote_streams_uni;
} dwnx_callbacks;

/**
 * @function
 *
 * `dwnx_conn_server_new` creates new :type:`dwnx_conn` as a server.
 * If it succeeds, it assigns the pointer to the object to |*pconn|.
 * |callbacks|, |settings|, and |params| must not be NULL, and the
 * function makes a copy of each of them.  |params| is the local
 * transport parameters, and sent to a remote endpoint during
 * handshake.  |user_data| is the arbitrary pointer which is passed to
 * the user-defined callback functions.  |mem| is a memory allocator.
 * If |mem| is NULL, the memory allocator returned by
 * `dwnx_mem_default()` is used.
 *
 * Call `dwnx_conn_del` to free memory allocated for |*pconn|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory.
 */
DWNX_EXTERN int dwnx_conn_server_new(dwnx_conn **pconn,
                                     const dwnx_callbacks *callbacks,
                                     const dwnx_settings *settings,
                                     const dwnx_transport_params *params,
                                     const dwnx_mem *mem, void *user_data);

/**
 * @function
 *
 * `dwnx_conn_client_new` creates new :type:`dwnx_conn` as a client.
 * If it succeeds, it assigns the pointer to the object to |*pconn|.
 * |callbacks|, |settings|, and |params| must not be NULL, and the
 * function makes a copy of each of them.  |params| is the local
 * transport parameters, and sent to a remote endpoint during
 * handshake.  |user_data| is the arbitrary pointer which is passed to
 * the user-defined callback functions.  |mem| is a memory allocator.
 * If |mem| is NULL, the memory allocator returned by
 * `dwnx_mem_default()` is used.
 *
 * Call `dwnx_conn_del` to free memory allocated for |*pconn|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory.
 */
DWNX_EXTERN int dwnx_conn_client_new(dwnx_conn **pconn,
                                     const dwnx_callbacks *callbacks,
                                     const dwnx_settings *settings,
                                     const dwnx_transport_params *params,
                                     const dwnx_mem *mem, void *user_data);

/**
 * @function
 *
 * `dwnx_conn_del` frees resources allocated for |conn|.  It also
 * frees memory pointed by |conn|.
 */
DWNX_EXTERN void dwnx_conn_del(dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_read` processes the incoming data pointed by |data| of
 * length |datalen|.  |ts| is the timestamp of this call.  Normally,
 * this function processes all input data.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * TBD
 *
 * In general, when one of negative error codes is returned, the QMux
 * connection must be closed, and |conn| must be deleted by
 * `dwnx_conn_del`.
 */
DWNX_EXTERN int dwnx_conn_read(dwnx_conn *conn, const uint8_t *data,
                               size_t datalen, dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_conn_extend_max_stream_offset` extends the maximum stream
 * data that a remote endpoint can send by |datalen|.  |stream_id|
 * specifies the stream ID.  This function only extends stream-level
 * flow control window.
 *
 * This function returns 0 if a stream denoted by |stream_id| is not
 * found.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory.
 * :macro:`DWNX_ERR_INVALID_ARGUMENT`
 *     |stream_id| refers to a local unidirectional stream.
 */
DWNX_EXTERN int dwnx_conn_extend_max_stream_offset(dwnx_conn *conn,
                                                   int64_t stream_id,
                                                   uint64_t datalen);

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
 * `dwnx_conn_extend_max_streams_bidi` extends the number of maximum
 * remote bidirectional streams that a remote endpoint can open by
 * |n|.
 *
 * The library does not increase maximum stream limit automatically.
 * The exception is when a stream is closed without
 * :member:`dwnx_callbacks.stream_open` callback being called.  In
 * this case, stream limit is increased automatically.
 */
DWNX_EXTERN void dwnx_conn_extend_max_streams_bidi(dwnx_conn *conn, size_t n);

/**
 * @function
 *
 * `dwnx_conn_extend_max_streams_uni` extends the number of maximum
 * remote unidirectional streams that a remote endpoint can open by
 * |n|.
 *
 * The library does not increase maximum stream limit automatically.
 * The exception is when a stream is closed without
 * :member:`dwnx_callbacks.stream_open` callback being called.  In
 * this case, stream limit is increased automatically.
 */
DWNX_EXTERN void dwnx_conn_extend_max_streams_uni(dwnx_conn *conn, size_t n);

/**
 * @function
 *
 * `dwnx_conn_open_bidi_stream` opens new bidirectional stream.  The
 * |stream_user_data| is the user data specific to the stream.  The
 * stream ID of the opened stream is stored in |*pstream_id|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 * :macro:`DWNX_ERR_STREAM_ID_BLOCKED`
 *     The remote endpoint does not allow |stream_id| yet.
 */
DWNX_EXTERN int dwnx_conn_open_bidi_stream(dwnx_conn *conn, int64_t *pstream_id,
                                           void *stream_user_data);

/**
 * @function
 *
 * `dwnx_conn_open_uni_stream` opens new unidirectional stream.  The
 * |stream_user_data| is the user data specific to the stream.  The
 * stream ID of the opened stream is stored in |*pstream_id|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 * :macro:`DWNX_ERR_STREAM_ID_BLOCKED`
 *     The remote endpoint does not allow |stream_id| yet.
 */
DWNX_EXTERN int dwnx_conn_open_uni_stream(dwnx_conn *conn, int64_t *pstream_id,
                                          void *stream_user_data);

/**
 * @function
 *
 * `dwnx_conn_shutdown_stream` closes a stream denoted by |stream_id|
 * abruptly.  |app_error_code| is one of application error codes, and
 * indicates the reason of shutdown.  Successful call of this function
 * does not immediately erase the state of the stream.  The actual
 * deletion is done when the remote endpoint sends acknowledgement.
 * Calling this function is equivalent to call
 * `dwnx_conn_shutdown_stream_read`, and
 * `dwnx_conn_shutdown_stream_write` sequentially with the following
 * differences.  If |stream_id| refers to a local unidirectional
 * stream, this function only shutdowns write side of the stream.  If
 * |stream_id| refers to a remote unidirectional stream, this function
 * only shutdowns read side of the stream.
 *
 * |flags| is currently unused, and should be set to 0.
 *
 * This function returns 0 if a stream denoted by |stream_id| is not
 * found.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 */
DWNX_EXTERN int dwnx_conn_shutdown_stream(dwnx_conn *conn, uint32_t flags,
                                          int64_t stream_id,
                                          uint64_t app_error_code);

/**
 * @function
 *
 * `dwnx_conn_shutdown_stream_write` closes write-side of a stream
 * denoted by |stream_id| abruptly.  |app_error_code| is one of
 * application error codes, and indicates the reason of shutdown.  If
 * this function succeeds, no further application data is sent to the
 * remote endpoint.  It discards all data which has not been
 * acknowledged yet.
 *
 * |flags| is currently unused, and should be set to 0.
 *
 * This function returns 0 if a stream denoted by |stream_id| is not
 * found.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 * :macro:`DWNX_ERR_INVALID_ARGUMENT`
 *     |stream_id| refers to a remote unidirectional stream.
 */
DWNX_EXTERN int dwnx_conn_shutdown_stream_write(dwnx_conn *conn, uint32_t flags,
                                                int64_t stream_id,
                                                uint64_t app_error_code);

/**
 * @function
 *
 * `dwnx_conn_shutdown_stream_read` closes read-side of a stream
 * denoted by |stream_id| abruptly.  |app_error_code| is one of
 * application error codes, and indicates the reason of shutdown.  If
 * this function succeeds, no further application data is forwarded to
 * an application layer.
 *
 * |flags| is currently unused, and should be set to 0.
 *
 * This function returns 0 if a stream denoted by |stream_id| is not
 * found.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 * :macro:`DWNX_ERR_INVALID_ARGUMENT`
 *     |stream_id| refers to a local unidirectional stream.
 */
DWNX_EXTERN int dwnx_conn_shutdown_stream_read(dwnx_conn *conn, uint32_t flags,
                                               int64_t stream_id,
                                               uint64_t app_error_code);

/**
 * @function
 *
 * `dwnx_conn_get_streams_bidi_left` returns the number of
 * bidirectional streams which the local endpoint can open without
 * violating stream concurrency limit.
 */
DWNX_EXTERN uint64_t dwnx_conn_get_streams_bidi_left(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_get_streams_uni_left` returns the number of
 * unidirectional streams which the local endpoint can open without
 * violating stream concurrency limit.
 */
DWNX_EXTERN uint64_t dwnx_conn_get_streams_uni_left(const dwnx_conn *conn);

/**
 * @macrosection
 *
 * Write stream data flags
 */

/**
 * @macro
 *
 * :macro:`DWNX_WRITE_STREAM_FLAG_NONE` indicates no flag set.
 */
#define DWNX_WRITE_STREAM_FLAG_NONE 0x00U

/**
 * @macro
 *
 * :macro:`DWNX_WRITE_STREAM_FLAG_FIN` indicates that a passed data is
 * the final part of a stream.
 */
#define DWNX_WRITE_STREAM_FLAG_FIN 0x02U

/**
 * @function
 *
 * `dwnx_conn_writev_stream` writes a single QMux record.  The caller
 * can optionally pass the stream data.  The buffer of the record is
 * pointed by |dest| of length |destlen|.  It returns the number of
 * bytes written to the buffer pointed by |dest| if it succeeds.
 *
 * |destlen| should be at least :macro:`DWNX_DEFAULT_MAX_RECORD_SIZE`.
 * The caller may provide a smaller sized buffer if the full sized TLS
 * record cannot be sent because the congestion window is not wide
 * open.
 *
 * Specifying -1 to |stream_id| means no new stream data to send.
 *
 * If |stream_id| is not -1, the stream data is specified as vector of
 * data |datav|.  |datavcnt| specifies the number of :type:`dwnx_vec`
 * that |datav| includes.  The number of data encoded in STREAM frame
 * is stored in |*pdatalen| if it is not NULL and this function
 * succeeds, or it returns :macro:`DWNX_ERR_WRITE_MORE`.
 *
 * If all given data is encoded as STREAM frame in |dest|, and if
 * |flags| & :macro:`DWNX_WRITE_STREAM_FLAG_FIN` is nonzero, fin flag
 * is set to outgoing STREAM frame.  Otherwise, fin flag in STREAM
 * frame is not set.
 *
 * This record may contain frames other than STREAM frame.  The record
 * might not contain STREAM frame if other frames occupy the frame.
 * In that case, |*pdatalen| would be -1 if |pdatalen| is not NULL.
 *
 * Empty data is treated specially, and it is only accepted if no
 * data, including the empty data, is submitted to a stream or
 * :macro:`DWNX_WRITE_STREAM_FLAG_FIN` is set in |flags|.  If 0 length
 * STREAM frame is successfully serialized, |*pdatalen| would be 0.
 *
 * This function may return :macro:`DWNX_ERR_WRITE_MORE` error code.
 * It indicates that there are more spaces in the record, the caller
 * should call this function again to send another stream data.  If no
 * stream data is available, specify |stream_id| to -1.
 *
 * This function may return :macro:`DWNX_ERR_STREAM_DATA_BLOCKED`
 * error code.  It indicates that the flow control prevents from the
 * data to be sent.  In this case, |*pdatalen| is -1.
 *
 * This function may return :macro:`DWNX_ERR_STREAM_SHUT_WR` error
 * code.  It indicates that the write side of the stream has been
 * closed.  In this case, |*pdatalen| is -1.
 *
 * If the other negative error codes are returned, QMux connection
 * must be closed.
 *
 * The rule of this function call is keep calling this function until
 * it returns 0 or a positive integer, or the negative error code
 * other than :macro:`DWNX_ERR_WRITE_MORE`,
 * :macro:`DWNX_ERR_STREAM_DATA_BLOCKED`, and
 * :macro:`DWNX_ERR_STREAM_SHUT_WR`.  If the function returns 0, it
 * means that there is nothing to send.
 *
 * This function must not be called from inside the callback
 * functions.
 *
 * This function returns the number of bytes written in |dest| if it
 * succeeds, or one of the following negative error codes:
 *
 * :macro:`DWNX_ERR_NOMEM`
 *     Out of memory
 * :macro:`DWNX_ERR_STREAM_NOT_FOUND`
 *     Stream does not exist
 * :macro:`DWNX_ERR_STREAM_SHUT_WR`
 *     Stream is half closed (local); or stream is being reset.
 * :macro:`DWNX_ERR_CALLBACK_FAILURE`
 *     User callback failed
 * :macro:`DWNX_ERR_INVALID_ARGUMENT`
 *     The total length of stream data is too large.
 * :macro:`DWNX_ERR_STREAM_DATA_BLOCKED`
 *     Stream is blocked because of flow control.
 * :macro:`DWNX_ERR_WRITE_MORE`
 *     Application can call this function to pack more stream data
 *     into the same record.  See above to know how it works.
 * :macro:`DWNX_ERR_NOBUF`
 *     Buffer is too small.
 *
 * If any other negative error is returned, close the connection.
 */
DWNX_EXTERN dwnx_ssize
dwnx_conn_writev_stream(dwnx_conn *conn, uint8_t *dest, size_t destlen,
                        dwnx_ssize *pdatalen, uint32_t flags, int64_t stream_id,
                        const dwnx_vec *datav, size_t datavcnt, dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_conn_write_stream` works like `dwnx_conn_writev_stream`, but
 * it can accept a single stream data vector.
 */
DWNX_EXTERN dwnx_ssize dwnx_conn_write_stream(dwnx_conn *conn, uint8_t *dest,
                                              size_t destlen,
                                              dwnx_ssize *pdatalen,
                                              uint32_t flags, int64_t stream_id,
                                              const uint8_t *data,
                                              size_t datalen, dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_conn_is_local_stream` returns nonzero if |stream_id| denotes
 * a locally initiated stream.
 */
DWNX_EXTERN int dwnx_conn_is_local_stream(const dwnx_conn *conn,
                                          int64_t stream_id);

/**
 * @function
 *
 * `dwnx_conn_is_server` returns nonzero if |conn| is initialized as
 * server.
 */
DWNX_EXTERN int dwnx_conn_is_server(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_get_timestamp` returns the latest timestamp that is
 * known to |conn|.
 */
DWNX_EXTERN dwnx_tstamp dwnx_conn_get_timestamp(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_get_max_data_left` returns the number of bytes that this
 * local endpoint can send in this connection without violating
 * connection-level flow control.
 */
DWNX_EXTERN uint64_t dwnx_conn_get_max_data_left(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_get_local_transport_params` returns a pointer to the
 * local QUIC transport parameters.
 */
DWNX_EXTERN const dwnx_transport_params *
dwnx_conn_get_local_transport_params(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_get_expiry` returns the next expiry time.  It returns
 * ``UINT64_MAX`` if there is no next expiry.
 *
 * Call `dwnx_conn_handle_expiry` when the expiry time has passed.
 */
DWNX_EXTERN dwnx_tstamp dwnx_conn_get_expiry(const dwnx_conn *conn);

/**
 * @function
 *
 * `dwnx_conn_handle_expiry` handles expired timer.
 *
 * If it returns :macro:`DWNX_ERR_IDLE_CLOSE`, it means that an idle
 * timer has fired for this particular connection.  In this case, just
 * close the underlying connection.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * :macro:`DWNX_ERR_IDLE_CLOSE`
 *     The idle timer has fired.
 */
DWNX_EXTERN int dwnx_conn_handle_expiry(dwnx_conn *conn, dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_strerror` returns the text representation of |liberr|.
 * |liberr| must be one of dwnx library error codes (which is defined
 * as :macro:`DWNX_ERR_* <DWNX_ERR_INVALID_ARGUMENT>` macros).
 */
DWNX_EXTERN const char *dwnx_strerror(int liberr);

/**
 * @function
 *
 * `dwnx_err_is_fatal` returns nonzero if |liberr| is a fatal error.
 * |liberr| must be one of dwnx library error codes (which is defined
 * as :macro:`DWNX_ERR_* <DWNX_ERR_INVALID_ARGUMENT>` macros).
 */
DWNX_EXTERN int dwnx_err_is_fatal(int liberr);

/**
 * @function
 *
 * `dwnx_err_infer_quic_transport_error_code` returns a QUIC transport
 * error code which corresponds to |liberr|.  |liberr| must be one of
 * dwnx library error codes (which is defined as :macro:`DWNX_ERR_*
 * <DWNX_ERR_INVALID_ARGUMENT>` macros).
 */
DWNX_EXTERN uint64_t dwnx_err_infer_quic_transport_error_code(int liberr);

/**
 * @enum
 *
 * :type:`dwnx_ccerr_type` defines connection error type.
 */
typedef enum dwnx_ccerr_type {
  /**
   * :enum:`DWNX_CCERR_TYPE_TRANSPORT` indicates the QUIC transport
   * error, and the error code is QUIC transport error code.
   */
  DWNX_CCERR_TYPE_TRANSPORT,
  /**
   * :enum:`DWNX_CCERR_TYPE_APPLICATION` indicates an application
   * error, and the error code is application error code.
   */
  DWNX_CCERR_TYPE_APPLICATION,
  /**
   * :enum:`DWNX_CCERR_TYPE_IDLE_CLOSE` is a special case of QUIC
   * transport error, and it indicates that connection is closed
   * because of idle timeout.
   */
  DWNX_CCERR_TYPE_IDLE_CLOSE
} dwnx_ccerr_type;

/**
 * @struct
 *
 * :type:`dwnx_ccerr` contains connection error code, its type, a
 * frame type that caused this error, and the optional reason phrase.
 */
typedef struct dwnx_ccerr {
  /**
   * :member:`type` is the type of this error.
   */
  dwnx_ccerr_type type;
  /**
   * :member:`error_code` is the error code for connection closure.
   * Its interpretation depends on :member:`type`.
   */
  uint64_t error_code;
  /**
   * :member:`frame_type` is the type of QUIC frame which triggers
   * this connection error.  This field is set to 0 if the frame type
   * is unknown.
   */
  uint64_t frame_type;
  /**
   * :member:`reason` points to the buffer which contains a reason
   * phrase.  It may be NULL if there is no reason phrase.  If it is
   * received from a remote endpoint, it is truncated to at most 1024
   * bytes.
   */
  const uint8_t *reason;
  /**
   * :member:`reasonlen` is the length of data pointed by
   * :member:`reason`.
   */
  size_t reasonlen;
} dwnx_ccerr;

/**
 * @function
 *
 * `dwnx_ccerr_default` initializes |ccerr| with the default values.
 * It sets the following fields:
 *
 * - :member:`type <dwnx_ccerr.type>` =
 *   :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_TRANSPORT`
 * - :member:`error_code <dwnx_ccerr.error_code>` =
 *   :macro:`DWNX_NO_ERROR`.
 * - :member:`frame_type <dwnx_ccerr.frame_type>` = 0
 * - :member:`reason <dwnx_ccerr.reason>` = NULL
 * - :member:`reasonlen <dwnx_ccerr.reasonlen>` = 0
 */
DWNX_EXTERN void dwnx_ccerr_default(dwnx_ccerr *ccerr);

/**
 * @function
 *
 * `dwnx_ccerr_set_transport_error` sets :member:`ccerr->type
 * <dwnx_ccerr.type>` to
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_TRANSPORT`, and
 * :member:`ccerr->error_code <dwnx_ccerr.error_code>` to
 * |error_code|.  |reason| is the reason phrase of length |reasonlen|.
 * This function does not make a copy of the reason phrase.
 */
DWNX_EXTERN void dwnx_ccerr_set_transport_error(dwnx_ccerr *ccerr,
                                                uint64_t error_code,
                                                const uint8_t *reason,
                                                size_t reasonlen);

/**
 * @function
 *
 * `dwnx_ccerr_set_liberr` sets type and error_code based on |liberr|.
 *
 * |reason| is the reason phrase of length |reasonlen|.  This function
 * does not make a copy of the reason phrase.
 *
 * If |liberr| is :macro:`DWNX_ERR_IDLE_CLOSE`, :member:`ccerr->type
 * <dwnx_ccerr.type>` is set to
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_IDLE_CLOSE`, and
 * :member:`ccerr->error_code <dwnx_ccerr.error_code>` to
 * :macro:`DWNX_NO_ERROR`.
 *
 * Otherwise, :member:`ccerr->type <dwnx_ccerr.type>` is set to
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_TRANSPORT`, and
 * :member:`ccerr->error_code <dwnx_ccerr.error_code>` is set to an
 * error code inferred by |liberr| (see
 * `dwnx_err_infer_quic_transport_error_code`).
 */
DWNX_EXTERN void dwnx_ccerr_set_liberr(dwnx_ccerr *ccerr, int liberr,
                                       const uint8_t *reason, size_t reasonlen);

/**
 * @function
 *
 * `dwnx_ccerr_set_application_error` sets :member:`ccerr->type
 * <dwnx_ccerr.type>` to
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_APPLICATION`, and
 * :member:`ccerr->error_code <dwnx_ccerr.error_code>` to
 * |error_code|.  |reason| is the reason phrase of length |reasonlen|.
 * This function does not make a copy of the reason phrase.
 */
DWNX_EXTERN void dwnx_ccerr_set_application_error(dwnx_ccerr *ccerr,
                                                  uint64_t error_code,
                                                  const uint8_t *reason,
                                                  size_t reasonlen);

/**
 * @function
 *
 * `dwnx_conn_write_connection_close` writes a QMux record which only
 * contains a single CONNECTION_CLOSE frame (either type 0x1C or 0x1D)
 * in the buffer pointed by |dest| whose capacity is |destlen|.
 *
 * If :member:`ccerr->type <dwnx_ccerr.type>` ==
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_TRANSPORT`, this function
 * sends CONNECTION_CLOSE (type 0x1C) frame.  If :member:`ccerr->type
 * <dwnx_ccerr.type>` ==
 * :enum:`dwnx_ccerr_type.DWNX_CCERR_TYPE_APPLICATION`, it sends
 * CONNECTION_CLOSE (type 0x1D) frame.  Otherwise, it does not produce
 * any data, and returns 0.
 *
 * After successful production of QMux record, |conn| enters closing
 * state.
 *
 * If |conn| is in either draining or closing state, this function
 * returns 0 without writing any data.
 *
 * This function returns the number of bytes written in |dest| if it
 * succeeds, or one of the following negative error codes:
 *
 * :macro:`DWNX_ERR_NOBUF`
 *     Buffer is too small.
 */
DWNX_EXTERN dwnx_ssize dwnx_conn_write_connection_close(dwnx_conn *conn,
                                                        uint8_t *dest,
                                                        size_t destlen,
                                                        const dwnx_ccerr *ccerr,
                                                        dwnx_tstamp ts);

/**
 * @function
 *
 * `dwnx_is_bidi_stream` returns nonzero if |stream_id| denotes
 * bidirectional stream.
 */
DWNX_EXTERN int dwnx_is_bidi_stream(int64_t stream_id);

#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* defined(_MSC_VER) */

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(DWNX_H) */
