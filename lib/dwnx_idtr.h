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
#ifndef DWNX_IDTR_H
#define DWNX_IDTR_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_mem.h"
#include "dwnx_gaptr.h"

/*
 * dwnx_idtr tracks the usage of stream ID.
 */
typedef struct dwnx_idtr {
  /* gap maintains the range of an internal ID which is not used yet.
     Initially, its range is [0, UINT64_MAX).  The internal ID and
     stream ID are in the different number spaces.  See
     id_from_stream_id to convert a stream ID to an internal ID. */
  dwnx_gaptr gap;
} dwnx_idtr;

/*
 * dwnx_idtr_init initializes |idtr|.
 */
void dwnx_idtr_init(dwnx_idtr *idtr, const dwnx_mem *mem);

/*
 * dwnx_idtr_free frees resources allocated for |idtr|.
 */
void dwnx_idtr_free(dwnx_idtr *idtr);

/*
 * dwnx_idtr_open claims that |stream_id| is in use.
 *
 * It returns 0 if it succeeds, or one of the following negative error
 * codes:
 *
 * DWNX_ERR_STREAM_IN_USE
 *     |stream_id| has already been used.
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
int dwnx_idtr_open(dwnx_idtr *idtr, int64_t stream_id);

/*
 * dwnx_idtr_open returns nonzero if |stream_id| is in use.
 */
int dwnx_idtr_is_open(const dwnx_idtr *idtr, int64_t stream_id);

#endif /* !defined(DWNX_IDTR_H) */
