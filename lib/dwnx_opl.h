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
#ifndef DWNX_OPL_H
#define DWNX_OPL_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

typedef struct dwnx_opl_entry dwnx_opl_entry;

struct dwnx_opl_entry {
  dwnx_opl_entry *next;
};

/*
 * dwnx_opl is an object memory pool.
 */
typedef struct dwnx_opl {
  dwnx_opl_entry *head;
} dwnx_opl;

/*
 * dwnx_opl_init initializes |opl|.
 */
void dwnx_opl_init(dwnx_opl *opl);

/*
 * dwnx_opl_push inserts |ent| to |opl| head.
 */
void dwnx_opl_push(dwnx_opl *opl, dwnx_opl_entry *ent);

/*
 * dwnx_opl_pop removes the first dwnx_opl_entry from |opl| and
 * returns it.  If |opl| does not have any entry, it returns NULL.
 */
dwnx_opl_entry *dwnx_opl_pop(dwnx_opl *opl);

void dwnx_opl_clear(dwnx_opl *opl);

#endif /* !defined(DWNX_OPL_H) */
