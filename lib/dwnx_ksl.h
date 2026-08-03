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
#ifndef DWNX_KSL_H
#define DWNX_KSL_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* defined(HAVE_CONFIG_H) */

#include <stdlib.h>

#include <dwnx/dwnx.h>

#include "dwnx_objalloc.h"
#include "dwnx_range.h"

#define DWNX_KSL_DEGR 16
/* DWNX_KSL_MAX_NBLK is the maximum number of nodes which a single
   block can contain. */
#define DWNX_KSL_MAX_NBLK (2 * DWNX_KSL_DEGR)
/* DWNX_KSL_MIN_NBLK is the minimum number of nodes which a single
   block other than root must contain. */
#define DWNX_KSL_MIN_NBLK DWNX_KSL_DEGR

/*
 * dwnx_ksl_key represents key in dwnx_ksl.
 */
typedef void dwnx_ksl_key;

typedef struct dwnx_ksl_node dwnx_ksl_node;

typedef struct dwnx_ksl_blk dwnx_ksl_blk;

/*
 * dwnx_ksl_node is a node which contains either dwnx_ksl_blk or
 * opaque data.  If a node is an internal node, it contains
 * dwnx_ksl_blk.  Otherwise, it has data.
 */
struct dwnx_ksl_node {
  union {
    dwnx_ksl_blk *blk;
    void *data;
  };
};

/*
 * dwnx_ksl_blk contains dwnx_ksl_node objects.
 */
struct dwnx_ksl_blk {
  union {
    struct {
      /* next points to the next block if leaf field is nonzero. */
      dwnx_ksl_blk *next;
      /* prev points to the previous block if leaf field is
         nonzero. */
      dwnx_ksl_blk *prev;
      dwnx_ksl_node nodes[DWNX_KSL_MAX_NBLK];
      /* keys is a pointer to the buffer to include DWNX_KSL_MAX_NBLK
         keys.  Because the length of key is unknown until
         dwnx_ksl_init is called, the actual buffer will be allocated
         after this object. */
      uint8_t *keys;
      /* n is the number of nodes this object contains in nodes. */
      uint32_t n;
      /* aligned_keylen is the length of the single key including
         alignment. */
      uint16_t aligned_keylen;
      /* leaf is nonzero if this block contains leaf nodes. */
      uint8_t leaf;
    };

    dwnx_opl_entry oplent;
  };
};

dwnx_objalloc_decl(ksl_blk, dwnx_ksl_blk, oplent)

/*
 * dwnx_ksl_compar is a function type which returns nonzero if key
 * |lhs| should be placed before |rhs|.  It returns 0 otherwise.
 */
typedef int (*dwnx_ksl_compar)(const dwnx_ksl_key *lhs,
                               const dwnx_ksl_key *rhs);

typedef struct dwnx_ksl dwnx_ksl;

/*
 * dwnx_ksl_search is a function to search for the first element in
 * |blk|->nodes which is not ordered before |key|.  It returns the
 * index of such element.  It returns |blk|->n if there is no such
 * element.
 */
typedef size_t (*dwnx_ksl_search)(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                  const dwnx_ksl_key *key);

/*
 * dwnx_ksl_search_def is a macro to implement dwnx_ksl_search with
 * COMPAR which is supposed to be dwnx_ksl_compar.
 */
#define dwnx_ksl_search_def(NAME, COMPAR)                                      \
  static size_t ksl_##NAME##_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,    \
                                    const dwnx_ksl_key *key) {                 \
    size_t i;                                                                  \
    uint8_t *node_key;                                                         \
                                                                               \
    for (i = 0, node_key = blk->keys; i < blk->n && COMPAR(node_key, key);     \
         ++i, node_key += ksl->aligned_keylen)                                 \
      ;                                                                        \
                                                                               \
    return i;                                                                  \
  }

typedef struct dwnx_ksl_it dwnx_ksl_it;

/*
 * dwnx_ksl_it is a bidirectional iterator to iterate nodes.
 */
struct dwnx_ksl_it {
  dwnx_ksl_blk *blk;
  size_t i;
};

/*
 * dwnx_ksl is a deterministic paged skip list.
 */
struct dwnx_ksl {
  dwnx_objalloc blkalloc;
  /* root points to the root block. */
  dwnx_ksl_blk *root;
  /* front points to the first leaf block. */
  dwnx_ksl_blk *front;
  /* back points to the last leaf block. */
  dwnx_ksl_blk *back;
  dwnx_ksl_compar compar;
  dwnx_ksl_search search;
  /* n is the number of elements stored. */
  size_t n;
  /* keylen is the size of key */
  size_t keylen;
  size_t aligned_keylen;
};

/*
 * dwnx_ksl_init initializes |ksl|.  |compar| specifies compare
 * function.  |search| is a search function which must use |compar|.
 * |keylen| is the length of key and must be at least
 * sizeof(uint64_t).
 */
void dwnx_ksl_init(dwnx_ksl *ksl, dwnx_ksl_compar compar,
                   dwnx_ksl_search search, size_t keylen, const dwnx_mem *mem);

/*
 * dwnx_ksl_free frees resources allocated for |ksl|.  If |ksl| is
 * NULL, this function does nothing.  It does not free the memory
 * region pointed by |ksl| itself.
 */
void dwnx_ksl_free(dwnx_ksl *ksl);

/*
 * dwnx_ksl_insert inserts |key| with its associated |data|.  On
 * successful insertion, the iterator points to the inserted node is
 * stored in |*it| if |it| is not NULL.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 * DWNX_ERR_INVALID_ARGUMENT
 *     |key| already exists.
 */
int dwnx_ksl_insert(dwnx_ksl *ksl, dwnx_ksl_it *it, const dwnx_ksl_key *key,
                    void *data);

/*
 * dwnx_ksl_remove removes the |key| from |ksl|.
 *
 * This function assigns the iterator to |*it|, which points to the
 * node which is located at the right next of the removed node if |it|
 * is not NULL.  If |key| is not found, no deletion takes place and
 * the return value of dwnx_ksl_end(ksl) is assigned to |*it| if |it|
 * is not NULL.
 *
 * This function returns 0 if it succeeds, or one of the following
 * negative error codes:
 *
 * DWNX_ERR_INVALID_ARGUMENT
 *     |key| does not exist.
 */
int dwnx_ksl_remove(dwnx_ksl *ksl, dwnx_ksl_it *it, const dwnx_ksl_key *key);

/*
 * dwnx_ksl_remove_hint removes the |key| from |ksl|.  |hint| must
 * point to the same node denoted by |key|.  |hint| is used to remove
 * a node efficiently in some cases.  Other than that, it behaves
 * exactly like dwnx_ksl_remove.  |it| and |hint| can point to the
 * same object.
 */
int dwnx_ksl_remove_hint(dwnx_ksl *ksl, dwnx_ksl_it *it,
                         const dwnx_ksl_it *hint, const dwnx_ksl_key *key);

/*
 * dwnx_ksl_lower_bound returns the iterator which points to the first
 * node which has the key which is equal to |key| or the last node
 * which satisfies !compar(&node->key, key).  If there is no such
 * node, it returns the iterator which satisfies dwnx_ksl_it_end(it)
 * != 0.
 */
dwnx_ksl_it dwnx_ksl_lower_bound(const dwnx_ksl *ksl, const dwnx_ksl_key *key);

/*
 * dwnx_ksl_lower_bound_search works like dwnx_ksl_lower_bound, but it
 * takes custom function |search| to do lower bound search.
 */
dwnx_ksl_it dwnx_ksl_lower_bound_search(const dwnx_ksl *ksl,
                                        const dwnx_ksl_key *key,
                                        dwnx_ksl_search search);

/*
 * dwnx_ksl_update_key replaces the key of nodes which has |old_key|
 * with |new_key|.  |new_key| must be strictly greater than the
 * previous node and strictly smaller than the next node.
 */
void dwnx_ksl_update_key(dwnx_ksl *ksl, const dwnx_ksl_key *old_key,
                         const dwnx_ksl_key *new_key);

/*
 * dwnx_ksl_begin returns the iterator which points to the first node.
 * If there is no node in |ksl|, it returns the iterator which
 * satisfies both dwnx_ksl_it_begin(it) != 0 and dwnx_ksl_it_end(it)
 * != 0.
 */
dwnx_ksl_it dwnx_ksl_begin(const dwnx_ksl *ksl);

/*
 * dwnx_ksl_end returns the iterator which points to the node
 * following the last node.  The returned object satisfies
 * dwnx_ksl_it_end().  If there is no node in |ksl|, it returns the
 * iterator which satisfies dwnx_ksl_it_begin(it) != 0 and
 * dwnx_ksl_it_end(it) != 0.
 */
dwnx_ksl_it dwnx_ksl_end(const dwnx_ksl *ksl);

/*
 * dwnx_ksl_len returns the number of elements stored in |ksl|.
 */
size_t dwnx_ksl_len(const dwnx_ksl *ksl);

/*
 * dwnx_ksl_clear removes all elements stored in |ksl|.
 */
void dwnx_ksl_clear(dwnx_ksl *ksl);

/*
 * dwnx_ksl_blk_nth_key returns the |n|th key under |blk|.
 */
static inline const dwnx_ksl_key *dwnx_ksl_blk_nth_key(const dwnx_ksl_blk *blk,
                                                       size_t n) {
  return blk->keys + n * blk->aligned_keylen;
}

#ifndef WIN32
/*
 * dwnx_ksl_print prints its internal state in stderr.  It assumes
 * that the key is of type int64_t.  This function should be used for
 * the debugging purpose only.
 */
void dwnx_ksl_print(const dwnx_ksl *ksl);
#endif /* !defined(WIN32) */

/*
 * dwnx_ksl_it_init initializes |it|.
 */
void dwnx_ksl_it_init(dwnx_ksl_it *it, dwnx_ksl_blk *blk, size_t i);

/*
 * dwnx_ksl_it_get returns the data associated to the node which |it|
 * points to.  It is undefined to call this function when
 * dwnx_ksl_it_end(it) returns nonzero.
 */
static inline void *dwnx_ksl_it_get(const dwnx_ksl_it *it) {
  return it->blk->nodes[it->i].data;
}

/*
 * dwnx_ksl_it_next advances the iterator by one.  It is undefined if
 * this function is called when dwnx_ksl_it_end(it) returns nonzero.
 */
static inline void dwnx_ksl_it_next(dwnx_ksl_it *it) {
  if (++it->i == it->blk->n && it->blk->next) {
    it->blk = it->blk->next;
    it->i = 0;
  }
}

/*
 * dwnx_ksl_it_prev moves backward the iterator by one.  It is
 * undefined if this function is called when dwnx_ksl_it_begin(it)
 * returns nonzero.
 */
void dwnx_ksl_it_prev(dwnx_ksl_it *it);

/*
 * dwnx_ksl_it_end returns nonzero if |it| points to the one beyond
 * the last node.
 */
static inline int dwnx_ksl_it_end(const dwnx_ksl_it *it) {
  return it->blk->n == it->i && it->blk->next == NULL;
}

/*
 * dwnx_ksl_it_begin returns nonzero if |it| points to the first node.
 * |it| might satisfy both dwnx_ksl_it_begin(it) != 0 and
 * dwnx_ksl_it_end(it) != 0 if the skip list has no node.
 */
int dwnx_ksl_it_begin(const dwnx_ksl_it *it);

/*
 * dwnx_ksl_key returns the key of the node which |it| points to.  It
 * is undefined to call this function when dwnx_ksl_it_end(it) returns
 * nonzero.
 */
static inline const dwnx_ksl_key *dwnx_ksl_it_key(const dwnx_ksl_it *it) {
  return dwnx_ksl_blk_nth_key(it->blk, it->i);
}

/*
 * dwnx_ksl_range_compar is an implementation of dwnx_ksl_compar.
 * |lhs| and |rhs| must point to dwnx_range object, and the function
 * returns nonzero if ((const dwnx_range *)lhs)->begin < ((const
 * dwnx_range *)rhs)->begin.
 */
static inline int dwnx_ksl_range_compar(const dwnx_ksl_key *lhs,
                                        const dwnx_ksl_key *rhs) {
  const dwnx_range *a = (const dwnx_range *)lhs, *b = (const dwnx_range *)rhs;
  return a->begin < b->begin;
}

/*
 * dwnx_ksl_range_search is an implementation of dwnx_ksl_search that
 * uses dwnx_ksl_range_compar.
 */
size_t dwnx_ksl_range_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                             const dwnx_ksl_key *key);

/*
 * dwnx_ksl_range_exclusive_compar is an implementation of
 * dwnx_ksl_compar.  |lhs| and |rhs| must point to dwnx_range object,
 * and the function returns nonzero if ((const dwnx_range
 * *)lhs)->begin < ((const dwnx_range *)rhs)->begin, and the 2 ranges
 * do not intersect.
 */
static inline int dwnx_ksl_range_exclusive_compar(const dwnx_ksl_key *lhs,
                                                  const dwnx_ksl_key *rhs) {
  const dwnx_range *a = (const dwnx_range *)lhs, *b = (const dwnx_range *)rhs;
  return a->begin < b->begin &&
         !(dwnx_max(a->begin, b->begin) < dwnx_min(a->end, b->end));
}

/*
 * dwnx_ksl_range_exclusive_search is an implementation of
 * dwnx_ksl_search that uses dwnx_ksl_range_exclusive_compar.
 */
size_t dwnx_ksl_range_exclusive_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                       const dwnx_ksl_key *key);

/*
 * dwnx_ksl_uint64_less is an implementation of dwnx_ksl_compar.
 * |lhs| and |rhs| must point to uint64_t objects, and the function
 * returns nonzero if *(uint64_t *)|lhs| < *(uint64_t *)|rhs|.
 */
static inline int dwnx_ksl_uint64_less(const dwnx_ksl_key *lhs,
                                       const dwnx_ksl_key *rhs) {
  return *(const uint64_t *)lhs < *(const uint64_t *)rhs;
}

/*
 * dwnx_ksl_uint64_less_search is an implementation of dwnx_ksl_search
 * that uses dwnx_ksl_uint64_less.
 */
size_t dwnx_ksl_uint64_less_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                   const dwnx_ksl_key *key);

/*
 * dwnx_ksl_int64_greater is an implementation of dwnx_ksl_compar.
 * |lhs| and |rhs| must point to int64_t objects, and the function
 * returns nonzero if *(int64_t *)|lhs| > *(int64_t *)|rhs|.
 */
static inline int dwnx_ksl_int64_greater(const dwnx_ksl_key *lhs,
                                         const dwnx_ksl_key *rhs) {
  return *(const int64_t *)lhs > *(const int64_t *)rhs;
}

/*
 * dwnx_ksl_int64_greater_search is an implementation of
 * dwnx_ksl_search that uses dwnx_ksl_int64_greater.
 */
size_t dwnx_ksl_int64_greater_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                     const dwnx_ksl_key *key);

#endif /* !defined(DWNX_KSL_H) */
