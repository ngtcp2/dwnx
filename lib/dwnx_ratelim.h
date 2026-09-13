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
#ifndef DWNX_RATELIM_H
#define DWNX_RATELIM_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

/* DWNX_RATELIM_MAX_BURST is the maximum value of the burst. */
#define DWNX_RATELIM_MAX_BURST (UINT64_MAX / DWNX_SECONDS)

typedef struct dwnx_ratelim {
  /* burst is the maximum number of tokens. */
  uint64_t burst;
  /* rate is the rate of token generation measured by token /
     second. */
  uint64_t rate;
  /* tokens is the amount of tokens available to drain. */
  uint64_t tokens;
  /* carry is the partial token gained in sub-second period.  It is
     added to the computation in the next update round. */
  uint64_t carry;
  /* ts is the last timestamp that is known to this object. */
  dwnx_tstamp ts;
} dwnx_ratelim;

/* dwnx_ratelim_init initializes |rlim| with the given parameters.
   |burst| is clamped to DWNX_RATELIM_MAX_BURST. */
void dwnx_ratelim_init(dwnx_ratelim *rlim, uint64_t burst, uint64_t rate,
                       dwnx_tstamp ts);

/* dwnx_ratelim_drain drains |n| from rlim->tokens.  It returns 0 if
   it succeeds, or -1. */
int dwnx_ratelim_drain(dwnx_ratelim *rlim, uint64_t n, dwnx_tstamp ts);

#endif /* !defined(DWNX_RATELIM_H) */
