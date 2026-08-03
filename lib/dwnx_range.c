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
#include "dwnx_range.h"
#include "dwnx_macro.h"

void dwnx_range_init(dwnx_range *r, uint64_t begin, uint64_t end) {
  *r = (dwnx_range){
    .begin = begin,
    .end = end,
  };
}

dwnx_range dwnx_range_intersect(const dwnx_range *a, const dwnx_range *b) {
  dwnx_range r;
  uint64_t begin = dwnx_max(a->begin, b->begin);
  uint64_t end = dwnx_min(a->end, b->end);

  if (begin < end) {
    dwnx_range_init(&r, begin, end);
  } else {
    r = (dwnx_range){0};
  }

  return r;
}

uint64_t dwnx_range_len(const dwnx_range *r) { return r->end - r->begin; }

int dwnx_range_eq(const dwnx_range *a, const dwnx_range *b) {
  return a->begin == b->begin && a->end == b->end;
}

void dwnx_range_cut(dwnx_range *left, dwnx_range *right, const dwnx_range *a,
                    const dwnx_range *b) {
  /* Assume that b is included in a */
  *left = (dwnx_range){
    .begin = a->begin,
    .end = b->begin,
  };
  *right = (dwnx_range){
    .begin = b->end,
    .end = a->end,
  };
}

int dwnx_range_not_after(const dwnx_range *a, const dwnx_range *b) {
  return a->end <= b->end;
}
