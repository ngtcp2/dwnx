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
#ifndef DWNX_OBJALLOC_H
#define DWNX_OBJALLOC_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <dwnx/dwnx.h>

#include "dwnx_balloc.h"
#include "dwnx_opl.h"
#include "dwnx_macro.h"
#include "dwnx_mem.h"

/*
 * dwnx_objalloc combines dwnx_balloc and dwnx_opl, and provides an
 * object pool with the custom allocator to reduce the allocation and
 * deallocation overheads for small objects.
 */
typedef struct dwnx_objalloc {
  dwnx_balloc balloc;
  dwnx_opl opl;
} dwnx_objalloc;

/*
 * dwnx_objalloc_init initializes |objalloc|.  |blklen| is directly
 * passed to dwnx_balloc_init.
 */
void dwnx_objalloc_init(dwnx_objalloc *objalloc, size_t blklen,
                        const dwnx_mem *mem);

/*
 * dwnx_objalloc_free releases all allocated resources.
 */
void dwnx_objalloc_free(dwnx_objalloc *objalloc);

/*
 * dwnx_objalloc_clear releases all allocated resources and
 * initializes its state.
 */
void dwnx_objalloc_clear(dwnx_objalloc *objalloc);

#ifndef NOMEMPOOL
#  define dwnx_objalloc_decl(NAME, TYPE, OPLENTFIELD)                          \
    inline static void dwnx_objalloc_##NAME##_init(                            \
      dwnx_objalloc *objalloc, size_t nmemb, const dwnx_mem *mem) {            \
      dwnx_objalloc_init(                                                      \
        objalloc, ((sizeof(TYPE) + 0xFU) & ~(size_t)0xFU) * nmemb, mem);       \
    }                                                                          \
                                                                               \
    TYPE *dwnx_objalloc_##NAME##_get(dwnx_objalloc *objalloc);                 \
                                                                               \
    TYPE *dwnx_objalloc_##NAME##_len_get(dwnx_objalloc *objalloc, size_t len); \
                                                                               \
    inline static void dwnx_objalloc_##NAME##_release(dwnx_objalloc *objalloc, \
                                                      TYPE *obj) {             \
      dwnx_opl_push(&objalloc->opl, &obj->OPLENTFIELD);                        \
    }

#  define dwnx_objalloc_def(NAME, TYPE, OPLENTFIELD)                           \
    TYPE *dwnx_objalloc_##NAME##_get(dwnx_objalloc *objalloc) {                \
      dwnx_opl_entry *oplent = dwnx_opl_pop(&objalloc->opl);                   \
      TYPE *obj;                                                               \
      int rv;                                                                  \
                                                                               \
      if (!oplent) {                                                           \
        rv = dwnx_balloc_get(&objalloc->balloc, (void **)&obj, sizeof(TYPE));  \
        if (rv != 0) {                                                         \
          return NULL;                                                         \
        }                                                                      \
                                                                               \
        return obj;                                                            \
      }                                                                        \
                                                                               \
      return dwnx_struct_of(oplent, TYPE, OPLENTFIELD);                        \
    }                                                                          \
                                                                               \
    TYPE *dwnx_objalloc_##NAME##_len_get(dwnx_objalloc *objalloc,              \
                                         size_t len) {                         \
      dwnx_opl_entry *oplent = dwnx_opl_pop(&objalloc->opl);                   \
      TYPE *obj;                                                               \
      int rv;                                                                  \
                                                                               \
      if (!oplent) {                                                           \
        rv = dwnx_balloc_get(&objalloc->balloc, (void **)&obj, len);           \
        if (rv != 0) {                                                         \
          return NULL;                                                         \
        }                                                                      \
                                                                               \
        return obj;                                                            \
      }                                                                        \
                                                                               \
      return dwnx_struct_of(oplent, TYPE, OPLENTFIELD);                        \
    }
#else /* defined(NOMEMPOOL) */
#  define dwnx_objalloc_decl(NAME, TYPE, OPLENTFIELD)                          \
    inline static void dwnx_objalloc_##NAME##_init(                            \
      dwnx_objalloc *objalloc, size_t nmemb, const dwnx_mem *mem) {            \
      dwnx_objalloc_init(                                                      \
        objalloc, ((sizeof(TYPE) + 0xFU) & ~(size_t)0xFU) * nmemb, mem);       \
    }                                                                          \
                                                                               \
    inline static TYPE *dwnx_objalloc_##NAME##_get(dwnx_objalloc *objalloc) {  \
      return dwnx_mem_malloc(objalloc->balloc.mem, sizeof(TYPE));              \
    }                                                                          \
                                                                               \
    inline static TYPE *dwnx_objalloc_##NAME##_len_get(                        \
      dwnx_objalloc *objalloc, size_t len) {                                   \
      return dwnx_mem_malloc(objalloc->balloc.mem, len);                       \
    }                                                                          \
                                                                               \
    inline static void dwnx_objalloc_##NAME##_release(dwnx_objalloc *objalloc, \
                                                      TYPE *obj) {             \
      dwnx_mem_free(objalloc->balloc.mem, obj);                                \
    }

#  define dwnx_objalloc_def(NAME, TYPE, OPLENTFIELD)
#endif /* defined(NOMEMPOOL) */

#endif /* !defined(DWNX_OBJALLOC_H) */
