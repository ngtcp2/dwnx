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
#ifndef DWNX_MAP_H
#define DWNX_MAP_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_mem.h"

/* Implementation of unordered map */

typedef uint64_t dwnx_map_key_type;

typedef struct dwnx_map {
  dwnx_map_key_type *keys;
  void **data;
  /* psl is the Probe Sequence Length.  0 has special meaning that the
     element is not stored at i-th position if psl[i] == 0.  Because
     of this, the actual psl value is psl[i] - 1 if psl[i] > 0. */
  uint8_t *psl;
  const dwnx_mem *mem;
  uint64_t seed;
  size_t size;
  size_t hashbits;
} dwnx_map;

/*
 * dwnx_map_init initializes the map |map|.
 */
void dwnx_map_init(dwnx_map *map, uint64_t seed, const dwnx_mem *mem);

/*
 * dwnx_map_free deallocates any resources allocated for |map|.  The
 * stored entries are not freed by this function.  Use dwnx_map_each()
 * to free each entry.
 */
void dwnx_map_free(dwnx_map *map);

/*
 * dwnx_map_insert inserts the new |data| with the |key| to the map
 * |map|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_INVALID_ARGUMENT
 *     The item associated by |key| already exists.
 * DWNX_ERR_NOMEM
 *     Out of memory
 */
int dwnx_map_insert(dwnx_map *map, dwnx_map_key_type key, void *data);

/*
 * dwnx_map_find returns the entry associated by the key |key|.  If
 * there is no such entry, this function returns NULL.
 */
void *dwnx_map_find(const dwnx_map *map, dwnx_map_key_type key);

/*
 * dwnx_map_remove removes the entry associated by the key |key| from
 * the |map|.  The removed entry is not freed by this function.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_INVALID_ARGUMENT
 *     The entry associated by |key| does not exist.
 */
int dwnx_map_remove(dwnx_map *map, dwnx_map_key_type key);

/*
 * dwnx_map_clear removes all entries from |map|.  The removed entry
 * is not freed by this function.
 */
void dwnx_map_clear(dwnx_map *map);

/*
 * dwnx_map_size returns the number of items stored in the map |map|.
 */
size_t dwnx_map_size(const dwnx_map *map);

/*
 * dwnx_map_each applies the function |func| to each entry in the
 * |map| with the optional user supplied pointer |ptr|.
 *
 * If the |func| returns 0, this function calls the |func| with the
 * next entry.  If the |func| returns nonzero, it will not call the
 * |func| for further entries and return the return value of the
 * |func| immediately.  Thus, this function returns 0 if all the
 * invocations of the |func| return 0, or nonzero value which the last
 * invocation of |func| returns.
 */
int dwnx_map_each(const dwnx_map *map, int (*func)(void *data, void *ptr),
                  void *ptr);

#ifndef WIN32
void dwnx_map_print_distance(const dwnx_map *map);
#endif /* !defined(WIN32) */

#endif /* !defined(DWNX_MAP_H) */
