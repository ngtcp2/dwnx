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

/* DWNX_QRE_RECORDLEN_SIZE is the number of bytes that the record size
   field occupies.  We always encode the record size in this size.  We
   do not allow record length > 16382. */
#define DWNX_QRE_RECORDLEN_SIZE 2

typedef struct dwnx_log dwnx_log;

/*
 * dwnx_qre is a QMux record encoder.  It writes a sequence of frames
 * and then writes the record header when finalizing the record.
 */
typedef struct dwnx_qre {
  /* buf points to the buffer to write QMux record. */
  dwnx_buf buf;
  /* log is the logger. */
  dwnx_log *log;
  /* flags is the bitwise-OR of zero or more of DWNX_QRE_FLAG_*. */
  uint32_t flags;
} dwnx_qre;

/*
 * dwnx_qre_init initializes |qre|.  |log| must not be NULL.
 */
void dwnx_qre_init(dwnx_qre *qre, dwnx_log *log);

/*
 * dwnx_qre_start starts QMux record encoding.  |buf| of length
 * |buflen| is the buffer to write a QMux record.  This function
 * assumes |buflen| >= DWNX_QRE_RECORDLEN_SIZE.
 */
void dwnx_qre_start(dwnx_qre *qre, uint8_t *buf, size_t buflen);

/*
 * dwnx_qre_has_started returns nonzero if dwnx_qre_start has been
 * called.
 */
int dwnx_qre_has_started(const dwnx_qre *qre);

/*
 * dwnx_qre_stream_max_datalen returns the maximum stream data that
 * can be written to the remaining QMux record.  |stream_id|
 * identifies the stream.  |offset| is the offset of the stream data.
 * |len| is the length of the available data.  If STREAM frame cannot
 * be encoded due to the insufficient remaining buffer size, this
 * function returns -1.
 */
dwnx_ssize dwnx_qre_stream_max_datalen(const dwnx_qre *qre, int64_t stream_id,
                                       uint64_t offset, uint64_t len);

/*
 * dwnx_qre_encode_frame encodes |fr|.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOBUF
 *     No space to encode |fr|.
 */
int dwnx_qre_encode_frame(dwnx_qre *qre, const dwnx_frame *fr);

/*
 * dwnx_qre_final finalizes QMux record.  It returns the number of
 * bytes written to the buffer.  If no frame is encoded, it returns 0.
 * This function unsets DWNX_QRE_FLAG_STARTED flag.
 */
size_t dwnx_qre_final(dwnx_qre *qre);

/*
 * dwnx_qre_reset unsets DWNX_QRE_FLAG_STARTED flag.
 */
void dwnx_qre_reset(dwnx_qre *qre);

/*
 * dwnx_qre_left returns the number of bytes left in the current
 * buffer.
 */
size_t dwnx_qre_left(const dwnx_qre *qre);

#endif /* !defined(DWNX_QRE_H) */
