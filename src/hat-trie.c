
#include "hat-trie.h"
#include "ahtable.h"
#include "misc.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

/* maximum number of keys that may be stored in a bucket */
static const size_t MAX_BUCKET_SIZE = 8192;

static const uint8_t NODE_TYPE_TRIE          = 0x1;
static const uint8_t NODE_TYPE_PURE_BUCKET   = 0x2;
static const uint8_t NODE_TYPE_HYBRID_BUCKET = 0x4;


struct trie_node_t_;

/* Node's may be trie nodes or buckets. This union allows us to keep
 * non-specific pointer. */
typedef union node_ptr_
{
    ahtable_t*           b;
    struct trie_node_t_* t;
    uint8_t*             flag;
} node_ptr;


typedef struct trie_node_t_
{
    uint8_t flag;

    /* the value for the key that is consumed on a trie node */
    value_t val;
    bool    has_val;

    /* Map a character to either a trie_node_t or a ahtable_t. The first byte
     * must be examined to determine which. */
    node_ptr xs[256];

} trie_node_t;


/* Create a new trie node with all pointer pointing to the given child (which
 * can be NULL). */
static trie_node_t* alloc_trie_node(node_ptr child)
{
    trie_node_t* node = malloc_or_die(sizeof(trie_node_t));
    node->flag = NODE_TYPE_TRIE;
    node->val     = 0;
    node->has_val = false;

    size_t i;
    for (i = 0; i < 256; ++i) node->xs[i] = child;
    return node;
}


struct hattrie_t_
{
    node_ptr root; // root node
    size_t m;      // number of stored keys
};



hattrie_t* hattrie_create()
{
    hattrie_t* T = malloc_or_die(sizeof(hattrie_t));
    T->m = 0;

    node_ptr node;
    node.b = ahtable_create();
    node.b->flag = NODE_TYPE_HYBRID_BUCKET;
    node.b->c0 = 0x00;
    node.b->c1 = 0xff;
    T->root.t = alloc_trie_node(node);

    return T;
}


static void hattrie_free_node(node_ptr node)
{
    if (*node.flag & NODE_TYPE_TRIE) {
        size_t i;
        for (i = 0; i < 256; ++i) {
            if (i > 0 && node.t->xs[i].t == node.t->xs[i - 1].t) continue;

            /* XXX: recursion might not be the best choice here. It is possible
             * to build a very deep trie. */
            if (node.t->xs[i].t) hattrie_free_node(node.t->xs[i]);
        }
        free(node.t);
    }
    else {
        ahtable_free(node.b);
    }
}


void hattrie_free(hattrie_t* T)
{
    hattrie_free_node(T->root);
    free(T);
}

/* Perform one split operation on the given node with the given parent.
 */
static void hattrie_split(node_ptr parent, node_ptr node)
{
    /* only buckets may be split */
    assert(*node.flag & NODE_TYPE_PURE_BUCKET ||
           *node.flag & NODE_TYPE_HYBRID_BUCKET);

    assert(*parent.flag & NODE_TYPE_TRIE);

    if (*node.flag & NODE_TYPE_PURE_BUCKET) {
        /* turn the pure bucket into a hybrid bucket */
        parent.t->xs[node.b->c0].t = alloc_trie_node(node);
        node.b->c0   = 0x00;
        node.b->c1   = 0xff;
        node.b->flag = NODE_TYPE_HYBRID_BUCKET;

        /* if the bucket had an empty key, move it to the new trie node */
        value_t* val = ahtable_tryget(node.b, NULL, 0);
        if (val) {
            parent.t->xs[node.b->c0].t->val     = *val;
            parent.t->xs[node.b->c0].t->has_val = true;
            *val = 0;
        }


        return;
    }

    /* This is a hybrid bucket. Perform a proper split. */


    /* count the number of occourances of every leading character */
    unsigned int cs[256]; // occurance count for leading chars
    memset(cs, 0, 256 * sizeof(unsigned int));
    size_t len;
    const char* key;

    ahtable_iter_t* i = ahtable_iter_begin(node.b);
    while (!ahtable_iter_finished(i)) {
        key = ahtable_iter_key(i, &len);
        assert(len > 0);
        cs[(size_t) key[0]] += 1;
        ahtable_iter_next(i);
    }
    ahtable_iter_free(i);


    /* choose a split point */
    unsigned int left_m, right_m;
    size_t j = node.b->c0;
    left_m  = cs[j];
    right_m = ahtable_size(node.b) - left_m;
    int d;

    while (j + 1 < node.b->c1) {
        d = abs((int) (left_m + cs[j + 1]) - (int) (right_m - cs[j + 1]));
        if (d <= abs(left_m - right_m)) {
            j += 1;
            left_m  += cs[j];
            right_m -= cs[j];
        }
        else break;
    }


    /* now split into two node cooresponding to ranges [0, j] and
     * [j + 1, 255], respectively. */


    /* create new left and right nodes */

    node_ptr left, right;
    left.b  = ahtable_create();
    left.b->c0   = node.b->c0;
    left.b->c1   = (uint8_t) j;
    left.b->flag = left.b->c0 == left.b->c1 ?
                      NODE_TYPE_PURE_BUCKET : NODE_TYPE_HYBRID_BUCKET;

    right.b = ahtable_create();
    right.b->c0   = (uint8_t) j + 1;
    right.b->c1   = node.b->c1;
    right.b->flag = right.b->c0 == right.b->c1 ?
                      NODE_TYPE_PURE_BUCKET : NODE_TYPE_HYBRID_BUCKET;


    /* update the parent's pointer */

    size_t c;
    for (c = (size_t) node.b->c0; c <= j; ++c) parent.t->xs[c] = left;
    for (; c <= (size_t) node.b->c1; ++c)      parent.t->xs[c] = right;



    /* distribute keys to the new left or right node */
    value_t* u;
    value_t* v;
    i = ahtable_iter_begin(node.b);
    while (!ahtable_iter_finished(i)) {
        key = ahtable_iter_key(i, &len);
        u   = ahtable_iter_val(i);
        assert(len > 0);

        /* left */
        if ((size_t) key[0] <= j) {
            if (*left.flag & NODE_TYPE_PURE_BUCKET) {
                v = ahtable_get(left.b, key + 1, len - 1);
            }
            else {
                v = ahtable_get(left.b, key, len);
            }
            *v = *u;
        }

        /* right */
        else {
            if (*right.flag & NODE_TYPE_PURE_BUCKET) {
                v = ahtable_get(right.b, key + 1, len - 1);
            }
            else {
                v = ahtable_get(right.b, key, len);
            }
            *v = *u;
        }

        ahtable_iter_next(i);
    }

    ahtable_iter_free(i);
    ahtable_free(node.b);
}


value_t* hattrie_get(hattrie_t* T, const char* key, size_t len)
{
    node_ptr parent = T->root;
    assert(*parent.flag & NODE_TYPE_TRIE);

    if (len == 0) return &parent.t->val;
    node_ptr node = parent.t->xs[(size_t) *key];

    while (*node.flag & NODE_TYPE_TRIE && len > 1) {
        ++key;
        --len;
        parent = node;
        node   = node.t->xs[(size_t) *key];
    }


    /* if the key has been consumed on a trie node, use its value */
    if (*node.flag & NODE_TYPE_TRIE) {
        if (!node.t->has_val) {
            node.t->has_val = true;
            ++T->m;
        }
        return &node.t->val;
    }


    /* preemptively split the bucket if it is full */
    while (ahtable_size(node.b) >= MAX_BUCKET_SIZE) {
        hattrie_split(parent, node);

        /* after the split, the node pointer is invalidated, so we search from
         * the parent again. */
        node = parent.t->xs[(size_t) *key];
        while (*node.flag & NODE_TYPE_TRIE && len > 1) {
            ++key;
            --len;
            parent = node;
            node   = node.t->xs[(size_t) *key];
        }

        if (*node.flag & NODE_TYPE_TRIE) {
            assert(len == 1);

            if (!node.t->has_val) {
                node.t->has_val = true;
                ++T->m;
            }
            return &node.t->val;
        }
    }

    size_t m_old = node.b->m;
    value_t* val;
    if (*node.flag & NODE_TYPE_PURE_BUCKET) {
        val = ahtable_get(node.b, key + 1, len - 1);
    }
    else {
        val = ahtable_get(node.b, key, len);
    }
    T->m += (node.b->m - m_old);

    return val;
}


value_t* hattrie_tryget(hattrie_t* T, const char* key, size_t len)
{
    node_ptr parent = T->root;
    assert(*parent.flag & NODE_TYPE_TRIE);

    if (len == 0) return &parent.t->val;
    node_ptr node = parent.t->xs[(size_t) *key];

    while (*node.flag & NODE_TYPE_TRIE && len > 1) {
        ++key;
        --len;
        parent = node;
        node   = node.t->xs[(size_t) *key];
    }


    /* if the key has been consumed on a trie node, use its value */
    if (*node.flag & NODE_TYPE_TRIE) {
        if (!node.t->has_val) {
            node.t->has_val = true;
            ++T->m;
        }
        return &node.t->val;
    }
    else if (*node.flag & NODE_TYPE_PURE_BUCKET) {
        return ahtable_tryget(node.b, key + 1, len - 1);
    }
    else {
        return ahtable_tryget(node.b, key, len);
    }
}


