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
#include "dwnx_test_helper.h"

#include "dwnx_transport_params.h"
#include "dwnx_conv.h"
#include "dwnx_unreachable.h"

void dwnx_write_frame(dwnx_buf *dest, const dwnx_frame *fr) {
  uint8_t *p;
  dwnx_ssize nwrite;

  switch (fr->hd.type) {
  case DWNX_FRAME_QX_TRANSPORT_PARAMETERS:
    dest->last = dwnx_put_uvarint(dest->last, fr->qx_transport_parameters.type);
    p = dest->last;
    dest->last += 2;

    nwrite = dwnx_transport_params_encode(dest->last, dwnx_buf_left(dest),
                                          &fr->qx_transport_parameters.params);
    assert_ptrdiff(0, <, nwrite);

    dest->last += nwrite;
    dwnx_put_uvarintw(p, (size_t)nwrite, 2);

    return;
  default:
    dwnx_unreachable();
  }
}

void dwnx_write_record(dwnx_buf *dest, const dwnx_frame *fr, size_t n) {
  uint8_t *p = dest->last;

  dest->last += 2;

  for (; n; ++fr, --n) {
    dwnx_write_frame(dest, fr);
  }

  dwnx_put_uvarintw(p, (uint64_t)(dest->last - p - 2), 2);
}
