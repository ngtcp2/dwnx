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
#include "dwnx_balloc.h"

#include <assert.h>

#include "dwnx_mem.h"

void dwnx_balloc_init(dwnx_balloc *balloc, size_t blklen, const dwnx_mem *mem) {
  assert((blklen & 0xFU) == 0);

  balloc->mem = mem;
  balloc->blklen = blklen;
  balloc->head = NULL;
  dwnx_buf_init(&balloc->buf, (void *)"", 0);
}

void dwnx_balloc_free(dwnx_balloc *balloc) {
  if (balloc == NULL) {
    return;
  }

  dwnx_balloc_clear(balloc);
}

void dwnx_balloc_clear(dwnx_balloc *balloc) {
  dwnx_memblock_hd *p, *next;

  for (p = balloc->head; p; p = next) {
    next = p->next;
    dwnx_mem_free(balloc->mem, p);
  }

  balloc->head = NULL;
  dwnx_buf_init(&balloc->buf, (void *)"", 0);
}

int dwnx_balloc_get(dwnx_balloc *balloc, void **pbuf, size_t n) {
  uint8_t *p;
  dwnx_memblock_hd *hd;

  assert(n <= balloc->blklen);

  if (dwnx_buf_left(&balloc->buf) < n) {
    p = dwnx_mem_malloc(balloc->mem,
                        sizeof(dwnx_memblock_hd) + 0x8U + balloc->blklen);
    if (p == NULL) {
      return DWNX_ERR_NOMEM;
    }

    hd = (void *)p;
    hd->next = balloc->head;
    balloc->head = hd;
    dwnx_buf_init(&balloc->buf,
                  (uint8_t *)(((uintptr_t)p + sizeof(dwnx_memblock_hd) + 0xFU) &
                              ~(uintptr_t)0xFU),
                  balloc->blklen);
  }

  assert(((uintptr_t)balloc->buf.last & 0xFU) == 0);

  *pbuf = balloc->buf.last;
  balloc->buf.last += (n + 0xFU) & ~(size_t)0xFU;

  return 0;
}
