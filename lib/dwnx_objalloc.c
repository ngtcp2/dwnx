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
#include "dwnx_objalloc.h"

void dwnx_objalloc_init(dwnx_objalloc *objalloc, size_t blklen,
                        const dwnx_mem *mem) {
  dwnx_balloc_init(&objalloc->balloc, blklen, mem);
  dwnx_opl_init(&objalloc->opl);
}

void dwnx_objalloc_free(dwnx_objalloc *objalloc) {
  dwnx_balloc_free(&objalloc->balloc);
}

void dwnx_objalloc_clear(dwnx_objalloc *objalloc) {
  dwnx_opl_clear(&objalloc->opl);
  dwnx_balloc_clear(&objalloc->balloc);
}
