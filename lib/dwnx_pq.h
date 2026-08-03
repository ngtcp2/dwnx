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
#ifndef DWNX_PQ_H
#define DWNX_PQ_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_mem.h"

/* Implementation of priority queue */

/* DWNX_PQ_BAD_INDEX is the priority queue index which indicates that
   an entry is not queued.  Assigning this value to
   dwnx_pq_entry.index can check that the entry is queued or not. */
#define DWNX_PQ_BAD_INDEX SIZE_MAX

typedef struct dwnx_pq_entry {
  size_t index;
} dwnx_pq_entry;

/* dwnx_pq_less is a "less" function, that returns nonzero if |lhs| is
   considered to be less than |rhs|. */
typedef int (*dwnx_pq_less)(const dwnx_pq_entry *lhs, const dwnx_pq_entry *rhs);

typedef struct dwnx_pq {
  /* q is a pointer to an array that stores the items. */
  dwnx_pq_entry **q;
  /* mem is a memory allocator. */
  const dwnx_mem *mem;
  /* length is the number of items stored. */
  size_t length;
  /* capacity is the maximum number of items this queue can store.
     This is automatically extended when length is reached to this
     limit. */
  size_t capacity;
  /* less is the less function to compare items. */
  dwnx_pq_less less;
} dwnx_pq;

/*
 * dwnx_pq_init initializes |pq| with compare function |cmp|.
 */
void dwnx_pq_init(dwnx_pq *pq, dwnx_pq_less less, const dwnx_mem *mem);

/*
 * dwnx_pq_free deallocates any resources allocated for |pq|.  The
 * stored items are not freed by this function.
 */
void dwnx_pq_free(dwnx_pq *pq);

/*
 * dwnx_pq_push adds |item| to |pq|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_pq_push(dwnx_pq *pq, dwnx_pq_entry *item);

/*
 * dwnx_pq_top returns item at the top of |pq|.  It is undefined if
 * |pq| is empty.
 */
dwnx_pq_entry *dwnx_pq_top(const dwnx_pq *pq);

/*
 * dwnx_pq_pop pops item at the top of |pq|.  The popped item is not
 * freed by this function.  It is undefined if |pq| is empty.
 */
void dwnx_pq_pop(dwnx_pq *pq);

/*
 * dwnx_pq_empty returns nonzero if |pq| is empty.
 */
int dwnx_pq_empty(const dwnx_pq *pq);

/*
 * dwnx_pq_size returns the number of items |pq| contains.
 */
size_t dwnx_pq_size(const dwnx_pq *pq);

/*
 * dwnx_pq_remove removes |item| from |pq|.  |pq| must contain |item|
 * otherwise the behavior is undefined.
 */
void dwnx_pq_remove(dwnx_pq *pq, dwnx_pq_entry *item);

#endif /* !defined(DWNX_PQ_H) */
