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
#include "dwnx_ksl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "dwnx_macro.h"
#include "dwnx_mem.h"

static dwnx_ksl_blk null_blk;

dwnx_objalloc_def(ksl_blk, dwnx_ksl_blk, oplent)

#define DWNX_KSL_ALIGNED_BLKLEN ((sizeof(dwnx_ksl_blk) + 0x7U) & ~(size_t)0x7U)

static size_t ksl_blklen(size_t aligned_keylen) {
  return DWNX_KSL_ALIGNED_BLKLEN + DWNX_KSL_MAX_NBLK * aligned_keylen;
}

/*
 * ksl_set_nth_key sets |key| to |n|th node under |blk|.
 */
static void ksl_set_nth_key(const dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t n,
                            const dwnx_ksl_key *key) {
  memcpy(blk->keys + n * ksl->aligned_keylen, key, ksl->keylen);
}

void dwnx_ksl_init(dwnx_ksl *ksl, dwnx_ksl_compar compar,
                   dwnx_ksl_search search, size_t keylen, const dwnx_mem *mem) {
  size_t aligned_keylen;

  assert(keylen >= sizeof(uint64_t));

  aligned_keylen = (keylen + 0x7U) & ~(size_t)0x7U;

  assert(aligned_keylen <= UINT16_MAX);

  dwnx_objalloc_init(&ksl->blkalloc,
                     (ksl_blklen(aligned_keylen) + 0xFU) & ~(size_t)0xFU, mem);

  ksl->root = NULL;
  ksl->front = ksl->back = NULL;
  ksl->compar = compar;
  ksl->search = search;
  ksl->n = 0;
  ksl->keylen = keylen;
  ksl->aligned_keylen = aligned_keylen;
}

static dwnx_ksl_blk *ksl_blk_objalloc_new(dwnx_ksl *ksl) {
  dwnx_ksl_blk *blk = dwnx_objalloc_ksl_blk_len_get(
    &ksl->blkalloc, ksl_blklen(ksl->aligned_keylen));

  if (!blk) {
    return NULL;
  }

  blk->keys = (uint8_t *)blk + DWNX_KSL_ALIGNED_BLKLEN;
  blk->aligned_keylen = (uint16_t)ksl->aligned_keylen;

  return blk;
}

static void ksl_blk_objalloc_del(dwnx_ksl *ksl, dwnx_ksl_blk *blk) {
  dwnx_objalloc_ksl_blk_release(&ksl->blkalloc, blk);
}

static int ksl_root_init(dwnx_ksl *ksl) {
  dwnx_ksl_blk *root = ksl_blk_objalloc_new(ksl);

  if (!root) {
    return DWNX_ERR_NOMEM;
  }

  root->next = root->prev = NULL;
  root->n = 0;
  root->leaf = 1;

  ksl->root = root;
  ksl->front = ksl->back = root;

  return 0;
}

#ifdef NOMEMPOOL
/*
 * ksl_free_blk frees |blk| recursively.
 */
static void ksl_free_blk(dwnx_ksl *ksl, dwnx_ksl_blk *blk) {
  size_t i;

  if (!blk->leaf) {
    for (i = 0; i < blk->n; ++i) {
      ksl_free_blk(ksl, blk->nodes[i].blk);
    }
  }

  ksl_blk_objalloc_del(ksl, blk);
}
#endif /* defined(NOMEMPOOL) */

void dwnx_ksl_free(dwnx_ksl *ksl) {
  if (!ksl || !ksl->root) {
    return;
  }

#ifdef NOMEMPOOL
  ksl_free_blk(ksl, ksl->root);
#endif /* defined(NOMEMPOOL) */

  dwnx_objalloc_free(&ksl->blkalloc);
}

/*
 * ksl_split_blk splits |blk| into 2 dwnx_ksl_blk objects.  The new
 * dwnx_ksl_blk is always the "right" block.
 *
 * It returns the pointer to the dwnx_ksl_blk created which is the
 * located at the right of |blk|, or NULL which indicates out of
 * memory error.
 */
static dwnx_ksl_blk *ksl_split_blk(dwnx_ksl *ksl, dwnx_ksl_blk *blk) {
  dwnx_ksl_blk *rblk;

  rblk = ksl_blk_objalloc_new(ksl);
  if (rblk == NULL) {
    return NULL;
  }

  rblk->next = blk->next;
  blk->next = rblk;

  if (rblk->next) {
    rblk->next->prev = rblk;
  } else if (ksl->back == blk) {
    ksl->back = rblk;
  }

  rblk->prev = blk;
  rblk->leaf = blk->leaf;

  rblk->n = blk->n / 2;
  blk->n -= rblk->n;

  memcpy(rblk->nodes, blk->nodes + blk->n, rblk->n * sizeof(dwnx_ksl_node));

  memcpy(rblk->keys, blk->keys + blk->n * ksl->aligned_keylen,
         rblk->n * ksl->aligned_keylen);

  assert(blk->n >= DWNX_KSL_MIN_NBLK);
  assert(rblk->n >= DWNX_KSL_MIN_NBLK);

  return rblk;
}

/*
 * ksl_split_node splits a node included in |blk| at the position |i|
 * into 2 adjacent nodes.  The new node is always inserted at the
 * position |i+1|.
 *
 * It returns 0 if it succeeds, or one of the following negative error
 * codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
static int ksl_split_node(dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t i) {
  dwnx_ksl_blk *lblk = blk->nodes[i].blk, *rblk;

  rblk = ksl_split_blk(ksl, lblk);
  if (rblk == NULL) {
    return DWNX_ERR_NOMEM;
  }

  memmove(blk->nodes + (i + 2), blk->nodes + (i + 1),
          (blk->n - (i + 1)) * sizeof(dwnx_ksl_node));

  memmove(blk->keys + (i + 1) * ksl->aligned_keylen,
          blk->keys + i * ksl->aligned_keylen,
          (blk->n - i) * ksl->aligned_keylen);

  blk->nodes[i + 1].blk = rblk;
  ++blk->n;

  ksl_set_nth_key(ksl, blk, i, dwnx_ksl_blk_nth_key(lblk, lblk->n - 1));

  return 0;
}

/*
 * ksl_split_root splits a root block.  It increases the height of
 * skip list by 1.
 *
 * It returns 0 if it succeeds, or one of the following negative error
 * codes:
 *
 * DWNX_ERR_NOMEM
 *     Out of memory.
 */
static int ksl_split_root(dwnx_ksl *ksl) {
  dwnx_ksl_blk *rblk, *lblk, *nroot;

  nroot = ksl_blk_objalloc_new(ksl);
  if (nroot == NULL) {
    return DWNX_ERR_NOMEM;
  }

  rblk = ksl_split_blk(ksl, ksl->root);
  if (rblk == NULL) {
    ksl_blk_objalloc_del(ksl, nroot);
    return DWNX_ERR_NOMEM;
  }

  lblk = ksl->root;

  nroot->next = nroot->prev = NULL;
  nroot->n = 2;
  nroot->leaf = 0;

  ksl_set_nth_key(ksl, nroot, 0, dwnx_ksl_blk_nth_key(lblk, lblk->n - 1));
  nroot->nodes[0].blk = lblk;

  ksl_set_nth_key(ksl, nroot, 1, dwnx_ksl_blk_nth_key(rblk, rblk->n - 1));
  nroot->nodes[1].blk = rblk;

  ksl->root = nroot;

  return 0;
}

/*
 * ksl_insert_node inserts a node whose key is |key| with the
 * associated |data| at the index of |i|.  This function assumes that
 * the number of nodes contained by |blk| is strictly less than
 * DWNX_KSL_MAX_NBLK.
 */
static void ksl_insert_node(dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t i,
                            const dwnx_ksl_key *key, void *data) {
  assert(blk->n < DWNX_KSL_MAX_NBLK);

  memmove(blk->nodes + (i + 1), blk->nodes + i,
          (blk->n - i) * sizeof(dwnx_ksl_node));

  memmove(blk->keys + (i + 1) * ksl->aligned_keylen,
          blk->keys + i * ksl->aligned_keylen,
          (blk->n - i) * ksl->aligned_keylen);

  ksl_set_nth_key(ksl, blk, i, key);
  blk->nodes[i].data = data;

  ++blk->n;
}

int dwnx_ksl_insert(dwnx_ksl *ksl, dwnx_ksl_it *it, const dwnx_ksl_key *key,
                    void *data) {
  dwnx_ksl_blk *blk;
  dwnx_ksl_node *node;
  size_t i;
  int rv;

  if (!ksl->root) {
    rv = ksl_root_init(ksl);
    if (rv != 0) {
      return rv;
    }
  }

  if (ksl->root->n == DWNX_KSL_MAX_NBLK) {
    rv = ksl_split_root(ksl);
    if (rv != 0) {
      return rv;
    }
  }

  blk = ksl->root;

  for (;;) {
    i = ksl->search(ksl, blk, key);

    if (blk->leaf) {
      if (i < blk->n && !ksl->compar(key, dwnx_ksl_blk_nth_key(blk, i))) {
        if (it) {
          *it = dwnx_ksl_end(ksl);
        }

        return DWNX_ERR_INVALID_ARGUMENT;
      }

      ksl_insert_node(ksl, blk, i, key, data);
      ++ksl->n;

      if (it) {
        dwnx_ksl_it_init(it, blk, i);
      }

      return 0;
    }

    if (i == blk->n) {
      /* This insertion extends the largest key in this subtree. */
      for (; !blk->leaf;) {
        node = &blk->nodes[blk->n - 1];
        if (node->blk->n == DWNX_KSL_MAX_NBLK) {
          rv = ksl_split_node(ksl, blk, blk->n - 1);
          if (rv != 0) {
            return rv;
          }

          node = &blk->nodes[blk->n - 1];
        }

        ksl_set_nth_key(ksl, blk, blk->n - 1, key);
        blk = node->blk;
      }

      ksl_insert_node(ksl, blk, blk->n, key, data);
      ++ksl->n;

      if (it) {
        dwnx_ksl_it_init(it, blk, blk->n - 1);
      }

      return 0;
    }

    node = &blk->nodes[i];

    if (node->blk->n == DWNX_KSL_MAX_NBLK) {
      rv = ksl_split_node(ksl, blk, i);
      if (rv != 0) {
        return rv;
      }

      if (ksl->compar(dwnx_ksl_blk_nth_key(blk, i), key)) {
        node = &blk->nodes[i + 1];
      }
    }

    blk = node->blk;
  }
}

/*
 * ksl_remove_node removes the node included in |blk| at the index of
 * |i|.
 */
static void ksl_remove_node(dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t i) {
  memmove(blk->nodes + i, blk->nodes + (i + 1),
          (blk->n - (i + 1)) * sizeof(dwnx_ksl_node));

  memmove(blk->keys + i * ksl->aligned_keylen,
          blk->keys + (i + 1) * ksl->aligned_keylen,
          (blk->n - (i + 1)) * ksl->aligned_keylen);

  --blk->n;
}

/*
 * ksl_merge_node merges 2 nodes which are the nodes at the index of
 * |i| and |i + 1|.
 *
 * If |blk| is the root block and it contains just 2 nodes before
 * merging nodes, the merged block becomes root block, which decreases
 * the height of |ksl| by 1.
 *
 * This function returns the pointer to the merged block.
 */
static dwnx_ksl_blk *ksl_merge_node(dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                    size_t i) {
  dwnx_ksl_node *lnode;
  dwnx_ksl_blk *lblk, *rblk;

  assert(i + 1 < blk->n);

  lnode = &blk->nodes[i];

  lblk = lnode->blk;
  rblk = blk->nodes[i + 1].blk;

  assert(lblk->n + rblk->n <= DWNX_KSL_MAX_NBLK);

  memcpy(lblk->nodes + lblk->n, rblk->nodes, rblk->n * sizeof(dwnx_ksl_node));

  memcpy(lblk->keys + lblk->n * ksl->aligned_keylen, rblk->keys,
         rblk->n * ksl->aligned_keylen);

  lblk->n += rblk->n;
  lblk->next = rblk->next;

  if (lblk->next) {
    lblk->next->prev = lblk;
  } else if (ksl->back == rblk) {
    ksl->back = lblk;
  }

  ksl_blk_objalloc_del(ksl, rblk);

  if (ksl->root == blk && blk->n == 2) {
    ksl_blk_objalloc_del(ksl, ksl->root);
    ksl->root = lblk;
  } else {
    ksl_remove_node(ksl, blk, i + 1);
    ksl_set_nth_key(ksl, blk, i, dwnx_ksl_blk_nth_key(lblk, lblk->n - 1));
  }

  return lblk;
}

/*
 * ksl_shift_left moves the first nodes in blk->nodes[i]->blk->nodes
 * to blk->nodes[i - 1]->blk->nodes in a manner that they have the
 * same amount of nodes as much as possible.
 */
static void ksl_shift_left(dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t i) {
  dwnx_ksl_node *lnode, *rnode;
  dwnx_ksl_blk *lblk, *rblk;
  size_t n;

  assert(i > 0);

  lnode = &blk->nodes[i - 1];
  rnode = &blk->nodes[i];

  lblk = lnode->blk;
  rblk = rnode->blk;

  assert(lblk->n < DWNX_KSL_MAX_NBLK);
  assert(rblk->n > DWNX_KSL_MIN_NBLK);

  n = (lblk->n + rblk->n + 1) / 2 - lblk->n;

  assert(n > 0);
  assert(lblk->n <= DWNX_KSL_MAX_NBLK - n);
  assert(rblk->n >= DWNX_KSL_MIN_NBLK + n);

  memcpy(lblk->nodes + lblk->n, rblk->nodes, n * sizeof(dwnx_ksl_node));

  memcpy(lblk->keys + lblk->n * ksl->aligned_keylen, rblk->keys,
         n * ksl->aligned_keylen);

  lblk->n += (uint32_t)n;
  rblk->n -= (uint32_t)n;

  ksl_set_nth_key(ksl, blk, i - 1, dwnx_ksl_blk_nth_key(lblk, lblk->n - 1));

  memmove(rblk->nodes, rblk->nodes + n, rblk->n * sizeof(dwnx_ksl_node));

  memmove(rblk->keys, rblk->keys + n * ksl->aligned_keylen,
          rblk->n * ksl->aligned_keylen);
}

/*
 * ksl_shift_right moves the last nodes in blk->nodes[i]->blk->nodes
 * to blk->nodes[i + 1]->blk->nodes in a manner that they have the
 * same amount of nodes as much as possible.
 */
static void ksl_shift_right(dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t i) {
  dwnx_ksl_node *lnode, *rnode;
  dwnx_ksl_blk *lblk, *rblk;
  size_t n;

  assert(i < blk->n - 1);

  lnode = &blk->nodes[i];
  rnode = &blk->nodes[i + 1];

  lblk = lnode->blk;
  rblk = rnode->blk;

  assert(lblk->n > DWNX_KSL_MIN_NBLK);
  assert(rblk->n < DWNX_KSL_MAX_NBLK);

  n = (lblk->n + rblk->n + 1) / 2 - rblk->n;

  assert(n > 0);
  assert(lblk->n >= DWNX_KSL_MIN_NBLK + n);
  assert(rblk->n <= DWNX_KSL_MAX_NBLK - n);

  memmove(rblk->nodes + n, rblk->nodes, rblk->n * sizeof(dwnx_ksl_node));

  memmove(rblk->keys + n * ksl->aligned_keylen, rblk->keys,
          rblk->n * ksl->aligned_keylen);

  rblk->n += (uint32_t)n;
  lblk->n -= (uint32_t)n;

  memcpy(rblk->nodes, lblk->nodes + lblk->n, n * sizeof(dwnx_ksl_node));

  memcpy(rblk->keys, lblk->keys + lblk->n * ksl->aligned_keylen,
         n * ksl->aligned_keylen);

  ksl_set_nth_key(ksl, blk, i, dwnx_ksl_blk_nth_key(lblk, lblk->n - 1));
}

/*
 * key_equal returns nonzero if |lhs| and |rhs| are equal using the
 * function |compar|.
 */
static int key_equal(dwnx_ksl_compar compar, const dwnx_ksl_key *lhs,
                     const dwnx_ksl_key *rhs) {
  return !compar(lhs, rhs) && !compar(rhs, lhs);
}

int dwnx_ksl_remove_hint(dwnx_ksl *ksl, dwnx_ksl_it *it,
                         const dwnx_ksl_it *hint, const dwnx_ksl_key *key) {
  dwnx_ksl_blk *blk = hint->blk;

  assert(ksl->root);

  if (blk != ksl->root && blk->n == DWNX_KSL_MIN_NBLK) {
    return dwnx_ksl_remove(ksl, it, key);
  }

  ksl_remove_node(ksl, blk, hint->i);

  --ksl->n;

  if (it) {
    if (hint->i == blk->n && blk->next) {
      dwnx_ksl_it_init(it, blk->next, 0);
    } else {
      dwnx_ksl_it_init(it, blk, hint->i);
    }
  }

  return 0;
}

int dwnx_ksl_remove(dwnx_ksl *ksl, dwnx_ksl_it *it, const dwnx_ksl_key *key) {
  dwnx_ksl_blk *blk = ksl->root;
  dwnx_ksl_node *node;
  size_t i;

  if (!blk) {
    return DWNX_ERR_INVALID_ARGUMENT;
  }

  if (!blk->leaf && blk->n == 2 && blk->nodes[0].blk->n == DWNX_KSL_MIN_NBLK &&
      blk->nodes[1].blk->n == DWNX_KSL_MIN_NBLK) {
    blk = ksl_merge_node(ksl, blk, 0);
  }

  for (;;) {
    i = ksl->search(ksl, blk, key);

    if (i == blk->n) {
      if (it) {
        *it = dwnx_ksl_end(ksl);
      }

      return DWNX_ERR_INVALID_ARGUMENT;
    }

    if (blk->leaf) {
      if (ksl->compar(key, dwnx_ksl_blk_nth_key(blk, i))) {
        if (it) {
          *it = dwnx_ksl_end(ksl);
        }

        return DWNX_ERR_INVALID_ARGUMENT;
      }

      ksl_remove_node(ksl, blk, i);
      --ksl->n;

      if (it) {
        if (blk->n == i && blk->next) {
          dwnx_ksl_it_init(it, blk->next, 0);
        } else {
          dwnx_ksl_it_init(it, blk, i);
        }
      }

      return 0;
    }

    node = &blk->nodes[i];

    if (node->blk->n > DWNX_KSL_MIN_NBLK) {
      blk = node->blk;
      continue;
    }

    assert(node->blk->n == DWNX_KSL_MIN_NBLK);

    if (i + 1 < blk->n && blk->nodes[i + 1].blk->n > DWNX_KSL_MIN_NBLK) {
      ksl_shift_left(ksl, blk, i + 1);
      blk = node->blk;

      continue;
    }

    if (i > 0 && blk->nodes[i - 1].blk->n > DWNX_KSL_MIN_NBLK) {
      ksl_shift_right(ksl, blk, i - 1);
      blk = node->blk;

      continue;
    }

    if (i + 1 < blk->n) {
      blk = ksl_merge_node(ksl, blk, i);
      continue;
    }

    assert(i > 0);

    blk = ksl_merge_node(ksl, blk, i - 1);
  }
}

dwnx_ksl_it dwnx_ksl_lower_bound(const dwnx_ksl *ksl, const dwnx_ksl_key *key) {
  return dwnx_ksl_lower_bound_search(ksl, key, ksl->search);
}

dwnx_ksl_it dwnx_ksl_lower_bound_search(const dwnx_ksl *ksl,
                                        const dwnx_ksl_key *key,
                                        dwnx_ksl_search search) {
  dwnx_ksl_blk *blk = ksl->root;
  dwnx_ksl_it it;
  size_t i;

  if (!blk) {
    dwnx_ksl_it_init(&it, &null_blk, 0);
    return it;
  }

  for (;;) {
    i = search(ksl, blk, key);

    if (blk->leaf) {
      if (i == blk->n && blk->next) {
        blk = blk->next;
        i = 0;
      }

      dwnx_ksl_it_init(&it, blk, i);

      return it;
    }

    if (i == blk->n) {
      /* This happens if descendant has smaller key.  Fast forward to
         find last node in this subtree. */
      for (; !blk->leaf; blk = blk->nodes[blk->n - 1].blk)
        ;

      if (blk->next) {
        blk = blk->next;
        i = 0;
      } else {
        i = blk->n;
      }

      dwnx_ksl_it_init(&it, blk, i);

      return it;
    }

    blk = blk->nodes[i].blk;
  }
}

void dwnx_ksl_update_key(dwnx_ksl *ksl, const dwnx_ksl_key *old_key,
                         const dwnx_ksl_key *new_key) {
  dwnx_ksl_blk *blk = ksl->root;
  dwnx_ksl_node *node;
  const dwnx_ksl_key *node_key;
  size_t i;

  assert(ksl->root);

  for (;;) {
    i = ksl->search(ksl, blk, old_key);

    assert(i < blk->n);
    node = &blk->nodes[i];
    node_key = dwnx_ksl_blk_nth_key(blk, i);

    if (blk->leaf) {
      assert(key_equal(ksl->compar, node_key, old_key));
      ksl_set_nth_key(ksl, blk, i, new_key);

      return;
    }

    if (key_equal(ksl->compar, node_key, old_key) ||
        ksl->compar(node_key, new_key)) {
      ksl_set_nth_key(ksl, blk, i, new_key);
    }

    blk = node->blk;
  }
}

size_t dwnx_ksl_len(const dwnx_ksl *ksl) { return ksl->n; }

void dwnx_ksl_clear(dwnx_ksl *ksl) {
  if (!ksl->root) {
    return;
  }

#ifdef NOMEMPOOL
  ksl_free_blk(ksl, ksl->root);
#endif /* defined(NOMEMPOOL) */

  ksl->front = ksl->back = ksl->root = NULL;
  ksl->n = 0;

  dwnx_objalloc_clear(&ksl->blkalloc);
}

#ifndef WIN32
static void ksl_print(const dwnx_ksl *ksl, dwnx_ksl_blk *blk, size_t level) {
  size_t i;

  fprintf(stderr, "LV=%zu n=%u\n", level, blk->n);

  if (blk->leaf) {
    for (i = 0; i < blk->n; ++i) {
      fprintf(stderr, " %" PRId64, *(int64_t *)dwnx_ksl_blk_nth_key(blk, i));
    }

    fprintf(stderr, "\n");

    return;
  }

  for (i = 0; i < blk->n; ++i) {
    ksl_print(ksl, blk->nodes[i].blk, level + 1);
  }
}

void dwnx_ksl_print(const dwnx_ksl *ksl) {
  if (!ksl->root) {
    return;
  }

  ksl_print(ksl, ksl->root, 0);
}
#endif /* !defined(WIN32) */

dwnx_ksl_it dwnx_ksl_begin(const dwnx_ksl *ksl) {
  dwnx_ksl_it it;

  if (ksl->root) {
    dwnx_ksl_it_init(&it, ksl->front, 0);
  } else {
    dwnx_ksl_it_init(&it, &null_blk, 0);
  }

  return it;
}

dwnx_ksl_it dwnx_ksl_end(const dwnx_ksl *ksl) {
  dwnx_ksl_it it;

  if (ksl->root) {
    dwnx_ksl_it_init(&it, ksl->back, ksl->back->n);
  } else {
    dwnx_ksl_it_init(&it, &null_blk, 0);
  }

  return it;
}

void dwnx_ksl_it_init(dwnx_ksl_it *it, dwnx_ksl_blk *blk, size_t i) {
  it->blk = blk;
  it->i = i;
}

void dwnx_ksl_it_prev(dwnx_ksl_it *it) {
  assert(!dwnx_ksl_it_begin(it));

  if (it->i == 0) {
    it->blk = it->blk->prev;
    it->i = it->blk->n - 1;
  } else {
    --it->i;
  }
}

int dwnx_ksl_it_begin(const dwnx_ksl_it *it) {
  return it->i == 0 && it->blk->prev == NULL;
}

dwnx_ksl_search_def(range, dwnx_ksl_range_compar)

size_t dwnx_ksl_range_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                             const dwnx_ksl_key *key) {
  return ksl_range_search(ksl, blk, key);
}

dwnx_ksl_search_def(range_exclusive, dwnx_ksl_range_exclusive_compar)

size_t dwnx_ksl_range_exclusive_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                       const dwnx_ksl_key *key) {
  return ksl_range_exclusive_search(ksl, blk, key);
}

dwnx_ksl_search_def(uint64_less, dwnx_ksl_uint64_less)

size_t dwnx_ksl_uint64_less_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                   const dwnx_ksl_key *key) {
  return ksl_uint64_less_search(ksl, blk, key);
}

dwnx_ksl_search_def(int64_greater, dwnx_ksl_int64_greater)

size_t dwnx_ksl_int64_greater_search(const dwnx_ksl *ksl, dwnx_ksl_blk *blk,
                                     const dwnx_ksl_key *key) {
  return ksl_int64_greater_search(ksl, blk, key);
}
