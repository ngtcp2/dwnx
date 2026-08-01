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
#ifndef NGTCP2_H
#define NGTCP2_H

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

#ifdef _MSC_VER
#  pragma warning(pop)
#endif /* defined(_MSC_VER) */

typedef struct dwnx_conn dwnx_conn;

DWNX_EXTERN int dwnx_conn_server_new(dwnx_conn **pconn);

DWNX_EXTERN int dwnx_conn_client_new(dwnx_conn **pconn);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */

#endif /* !defined(DWNX_H) */
