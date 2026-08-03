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
#ifndef DWNX_BALLOC_H
#define DWNX_BALLOC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_buf.h"

typedef struct dwnx_memblock_hd dwnx_memblock_hd;

/*
 * dwnx_memblock_hd is the header of memory block.
 */
struct dwnx_memblock_hd {
  union {
    dwnx_memblock_hd *next;
    uint64_t pad;
  };
};

/*
 * dwnx_balloc is a custom memory allocator.  It allocates |blklen|
 * bytes of memory at once on demand, and returns its slice when the
 * allocation is requested.
 */
typedef struct dwnx_balloc {
  /* mem is the underlying memory allocator. */
  const dwnx_mem *mem;
  /* blklen is the size of memory block. */
  size_t blklen;
  /* head points to the list of memory block allocated so far. */
  dwnx_memblock_hd *head;
  /* buf wraps the current memory block for allocation requests. */
  dwnx_buf buf;
} dwnx_balloc;

/*
 * dwnx_balloc_init initializes |balloc| with |blklen| which is the
 * size of memory block.  |blklen| must be divisible by 16.
 */
void dwnx_balloc_init(dwnx_balloc *balloc, size_t blklen, const dwnx_mem *mem);

/*
 * dwnx_balloc_free releases all allocated memory blocks.
 */
void dwnx_balloc_free(dwnx_balloc *balloc);

/*
 * dwnx_balloc_get allocates |n| bytes of memory and assigns its
 * pointer to |*pbuf|.
 *
 * It returns 0 if it succeeds, or one of the following negative error
 * codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_balloc_get(dwnx_balloc *balloc, void **pbuf, size_t n);

/*
 * dwnx_balloc_clear releases all allocated memory blocks and
 * initializes its state.
 */
void dwnx_balloc_clear(dwnx_balloc *balloc);

#endif /* !defined(DWNX_BALLOC_H) */
