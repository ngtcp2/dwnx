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
#include "dwnx_strm.h"

void dwnx_strm_init(dwnx_strm *strm, int64_t stream_id, uint32_t flags,
                    uint64_t max_rx_offset, uint64_t max_tx_offset,
                    void *stream_user_data, const dwnx_mem *mem) {
  *strm = (dwnx_strm){
    .mem = mem,
    .stream_id = stream_id,
    .pe.index = DWNX_PQ_BAD_INDEX,
    .rx =
      {
        .max_offset = max_rx_offset,
        .unsent_max_offset = max_rx_offset,
        .window = max_rx_offset,
      },
    .tx =
      {
        .max_offset = max_tx_offset,
        .last_blocked_offset = UINT64_MAX,
      },
    .stream_user_data = stream_user_data,
    .flags = flags,
  };
}

void dwnx_strm_free(dwnx_strm *strm) { (void)strm; }

void dwnx_strm_shutdown(dwnx_strm *strm, uint32_t flags) {
  strm->flags |= flags & DWNX_STRM_FLAG_SHUT_RDWR;
}

int dwnx_strm_is_tx_queued(const dwnx_strm *strm) {
  return strm->pe.index != DWNX_PQ_BAD_INDEX;
}
