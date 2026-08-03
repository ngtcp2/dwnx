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
#ifndef DWNX_RANGE_H
#define DWNX_RANGE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

/*
 * dwnx_range represents half-closed range [begin, end).
 */
typedef struct dwnx_range {
  uint64_t begin;
  uint64_t end;
} dwnx_range;

/*
 * dwnx_range_init initializes |r| with the range [|begin|, |end|).
 */
void dwnx_range_init(dwnx_range *r, uint64_t begin, uint64_t end);

/*
 * dwnx_range_intersect returns the intersection of |a| and |b|.  If
 * they do not overlap, it returns empty range.
 */
dwnx_range dwnx_range_intersect(const dwnx_range *a, const dwnx_range *b);

/*
 * dwnx_range_len returns the length of |r|.
 */
uint64_t dwnx_range_len(const dwnx_range *r);

/*
 * dwnx_range_eq returns nonzero if |a| equals |b|, such that a->begin
 * == b->begin and a->end == b->end hold.
 */
int dwnx_range_eq(const dwnx_range *a, const dwnx_range *b);

/*
 * dwnx_range_cut returns the left and right range after removing |b|
 * from |a|.  This function assumes that |a| completely includes |b|.
 * In other words, a->begin <= b->begin and b->end <= a->end hold.
 */
void dwnx_range_cut(dwnx_range *left, dwnx_range *right, const dwnx_range *a,
                    const dwnx_range *b);

/*
 * dwnx_range_not_after returns nonzero if the right edge of |a| does
 * not go beyond of the right edge of |b|.
 */
int dwnx_range_not_after(const dwnx_range *a, const dwnx_range *b);

#endif /* !defined(DWNX_RANGE_H) */
