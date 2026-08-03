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
#include "dwnx_gaptr.h"

#include <string.h>
#include <assert.h>

void dwnx_gaptr_init(dwnx_gaptr *gaptr, const dwnx_mem *mem) {
  dwnx_ksl_init(&gaptr->gap, dwnx_ksl_range_compar, dwnx_ksl_range_search,
                sizeof(dwnx_range), mem);

  gaptr->mem = mem;
}

static int gaptr_gap_init(dwnx_gaptr *gaptr) {
  static const dwnx_range end = {
    .end = UINT64_MAX,
  };

  return dwnx_ksl_insert(&gaptr->gap, NULL, &end, NULL);
}

void dwnx_gaptr_free(dwnx_gaptr *gaptr) {
  if (gaptr == NULL) {
    return;
  }

  dwnx_ksl_free(&gaptr->gap);
}

int dwnx_gaptr_push(dwnx_gaptr *gaptr, uint64_t offset, uint64_t datalen) {
  int rv;
  dwnx_range k, m, l, r;
  dwnx_range q = {
    .begin = offset,
    .end = offset + datalen,
  };
  dwnx_ksl_it it;

  if (dwnx_ksl_len(&gaptr->gap) == 0) {
    rv = gaptr_gap_init(gaptr);
    if (rv != 0) {
      return rv;
    }
  }

  it = dwnx_ksl_lower_bound_search(&gaptr->gap, &q,
                                   dwnx_ksl_range_exclusive_search);

  for (; !dwnx_ksl_it_end(&it);) {
    k = *(dwnx_range *)dwnx_ksl_it_key(&it);
    m = dwnx_range_intersect(&q, &k);
    if (!dwnx_range_len(&m)) {
      break;
    }

    if (dwnx_range_eq(&k, &m)) {
      dwnx_ksl_remove_hint(&gaptr->gap, &it, &it, &k);
      continue;
    }

    dwnx_range_cut(&l, &r, &k, &m);

    if (dwnx_range_len(&l)) {
      dwnx_ksl_update_key(&gaptr->gap, &k, &l);

      if (dwnx_range_len(&r)) {
        rv = dwnx_ksl_insert(&gaptr->gap, &it, &r, NULL);
        if (rv != 0) {
          return rv;
        }
      }
    } else if (dwnx_range_len(&r)) {
      dwnx_ksl_update_key(&gaptr->gap, &k, &r);
    }

    dwnx_ksl_it_next(&it);
  }

  return 0;
}

uint64_t dwnx_gaptr_first_gap_offset(const dwnx_gaptr *gaptr) {
  dwnx_ksl_it it;

  if (dwnx_ksl_len(&gaptr->gap) == 0) {
    return 0;
  }

  it = dwnx_ksl_begin(&gaptr->gap);

  return ((dwnx_range *)dwnx_ksl_it_key(&it))->begin;
}

dwnx_range dwnx_gaptr_get_first_gap_after(const dwnx_gaptr *gaptr,
                                          uint64_t offset) {
  dwnx_ksl_it it;

  if (dwnx_ksl_len(&gaptr->gap) == 0) {
    dwnx_range r = {
      .end = UINT64_MAX,
    };
    return r;
  }

  it = dwnx_ksl_lower_bound_search(&gaptr->gap,
                                   &(dwnx_range){
                                     .begin = offset,
                                     .end = offset + 1,
                                   },
                                   dwnx_ksl_range_exclusive_search);

  assert(!dwnx_ksl_it_end(&it));

  return *(dwnx_range *)dwnx_ksl_it_key(&it);
}

int dwnx_gaptr_is_pushed(const dwnx_gaptr *gaptr, uint64_t offset,
                         uint64_t datalen) {
  dwnx_range q = {
    .begin = offset,
    .end = offset + datalen,
  };
  dwnx_ksl_it it;
  dwnx_range m;

  if (dwnx_ksl_len(&gaptr->gap) == 0) {
    return 0;
  }

  it = dwnx_ksl_lower_bound_search(&gaptr->gap, &q,
                                   dwnx_ksl_range_exclusive_search);

  assert(!dwnx_ksl_it_end(&it));

  m = dwnx_range_intersect(&q, (dwnx_range *)dwnx_ksl_it_key(&it));

  return dwnx_range_len(&m) == 0;
}

void dwnx_gaptr_drop_first_gap(dwnx_gaptr *gaptr) {
  dwnx_ksl_it it;
  dwnx_range r;

  if (dwnx_ksl_len(&gaptr->gap) == 0) {
    return;
  }

  it = dwnx_ksl_begin(&gaptr->gap);

  assert(!dwnx_ksl_it_end(&it));

  r = *(dwnx_range *)dwnx_ksl_it_key(&it);

  dwnx_ksl_remove_hint(&gaptr->gap, NULL, &it, &r);
}
