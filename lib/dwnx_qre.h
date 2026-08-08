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
#ifndef DWNX_QRE_H
#define DWNX_QRE_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_buf.h"
#include "dwnx_frame.h"

#define DWNX_QRE_FLAG_NONE 0x00U
/* DWNX_QRE_FLAG_STARTED indicates that the encoding has started. */
#define DWNX_QRE_FLAG_STARTED 0x01U

typedef struct dwnx_qre {
  dwnx_buf buf;
  uint32_t flags;
} dwnx_qre;

void dwnx_qre_init(dwnx_qre *qre);

void dwnx_qre_start(dwnx_qre *qre, uint8_t *buf, size_t buflen);

int dwnx_qre_has_started(const dwnx_qre *qre);

dwnx_ssize dwnx_qre_stream_max_datalen(const dwnx_qre *qre, int64_t stream_id,
                                       uint64_t offset, uint64_t len);

int dwnx_qre_encode_frame(dwnx_qre *qre, const dwnx_frame *fr);

size_t dwnx_qre_final(dwnx_qre *qre);

size_t dwnx_qre_left(const dwnx_qre *qre);

#endif /* !defined(DWNX_QRE_H) */
