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
#include "dwnx_ratelim.h"

#include <assert.h>

#include "dwnx_macro.h"

void dwnx_ratelim_init(dwnx_ratelim *rlim, uint64_t burst, uint64_t rate,
                       dwnx_tstamp ts) {
  burst = dwnx_min(burst, DWNX_RATELIM_MAX_BURST);

  *rlim = (dwnx_ratelim){
    .burst = burst,
    .rate = rate,
    .tokens = burst,
    .ts = ts,
  };
}

/* ratelim_update updates rlim->tokens with the current |ts|. */
static void ratelim_update(dwnx_ratelim *rlim, dwnx_tstamp ts) {
  uint64_t d, gain, gps;

  assert(ts >= rlim->ts);

  if (ts == rlim->ts) {
    return;
  }

  d = ts - rlim->ts;
  rlim->ts = ts;

  if (rlim->rate <= (UINT64_MAX - rlim->carry) / d) {
    gain = rlim->rate * d + rlim->carry;
    gps = gain / DWNX_SECONDS;

    if (gps < rlim->burst && rlim->tokens < rlim->burst - gps) {
      rlim->tokens += gps;
      rlim->carry = gain % DWNX_SECONDS;

      return;
    }
  }

  rlim->tokens = rlim->burst;
  rlim->carry = 0;
}

int dwnx_ratelim_drain(dwnx_ratelim *rlim, uint64_t n, dwnx_tstamp ts) {
  ratelim_update(rlim, ts);

  if (rlim->tokens < n) {
    return -1;
  }

  rlim->tokens -= n;

  return 0;
}
