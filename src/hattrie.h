/*
 * This file is part of libhattrie
 *
 * Copyright (c) 2011 by Daniel C. Jones <dcjones@cs.washington.edu>
 *
 *
 * This is an implementation of the HAT-trie data structure described in,
 *
 *    Askitis, N., & Sinha, R. (2007). HAT-trie: a cache-conscious trie-based data
 *    structure for strings. Proceedings of the thirtieth Australasian conference on
 *    Computer science-Volume 62 (pp. 97–105). Australian Computer Society, Inc.
 *
 * The HAT-trie is in essence a hybrid data structure, combining tries and hash
 * tables in a clever way to try to get the best of both worlds.
 *
 */

#ifndef HATTRIE_HATTRIE_H
#define HATTRIE_HATTRIE_H

#include "common.h"
#include <stdlib.h>

typedef struct hattrie_t_ hattrie_t;

hattrie_t* hattrie_create (void);             //< Create an empty hat-trie.
void       hattrie_free   (hattrie_t*);       //< Free all memory used by a trie.
hattrie_t* hattrie_dup    (const hattrie_t*); //< Duplicate an existing trie.
void       hattrie_clear  (hattrie_t*);       //< Remove all entries.


/** Find the given key in the trie, inserting it if it does not exist, and
 * returning a pointer to it's key.
 *
 * This pointer is not guaranteed to be valid after additional calls to
 * hattrie_get, hattrie_del, hattrie_clear, or other functions that modifies the
 * trie.
 */
value_t* hattrie_get (hattrie_t*, const char* key, size_t len);


/** Find a given key in the table, returning a NULL pointer if it does not
 * exist. */
value_t* hattrie_tryget (hattrie_t*, const char* key, size_t len);


#endif


