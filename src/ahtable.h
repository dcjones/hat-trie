/*
 * This file is part of hat-trie.
 *
 * Copyright (c) 2011 by Daniel C. Jones <dcjones@cs.washington.edu>
 *
 *
 * This is an implementation of the 'cache-conscious' hash tables described in,
 *
 *    Askitis, N., & Zobel, J. (2005). Cache-conscious collision resolution in
 *    string hash tables. String Processing and Information Retrieval (pp.
 *    91–102). Springer.
 *
 * Briefly, the idea is, as opposed to separate chaining with linked lists, to
 * store keys contiguously in one big array, thereby improving the caching
 * behavior, and reducing space requirments.
 *
 */

#ifndef HAT_TRIE_ARRAY_HASH_H
#define HAT_TRIE_ARRAY_HASH_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct ahtable_ ahtable;

typedef unsigned long value_t;

ahtable* ahtable_create   (void);         //< Create an empty hash table.
ahtable* ahtable_create_n (size_t n);     //< Create an empty hash table, with
                                          //  n slots reserved.
void     ahtable_free   (ahtable*);       //< Free all memory used by a table.
ahtable* ahtable_dup    (const ahtable*); //< Duplicate an existing table.
void     ahtable_clear  (ahtable*);       //< Remove all entries.


/** Find the given key in the table, inserting it if it does not exist, and
 * returning a pointer to it's key.
 *
 * This pointer is not guaranteed to be valid after additional calls to
 * ahtable_get, ahtable_del, ahtable_clear, or other functions that modify the
 * table.
 */
value_t* ahtable_get (ahtable*, const char* key);


/** Find a given key in the table, returning a NULL pointer if it does not
 * exist. */
value_t* ahtable_tryget (ahtable*, const char* key);


/** Remove the given key from the table if it exists// .
 *
 * Returns 'true' if the key was found, and 'false' otherwise. Note that
 * deletion is not particularity efficient in array hash tables.
 */

// TODO: implement this
//bool ahtable_del(ahtable*, const char* key);



/* Iteration: */
// TODO: document


typedef struct ahtable_iter_t_ ahtable_iter_t;

ahtable_iter_t* ahtable_iter_begin     (const ahtable*);
void            ahtable_iter_next      (ahtable_iter_t*);
bool            ahtable_iter_finished  (ahtable_iter_t*);
void            ahtable_iter_free      (ahtable_iter_t*);
const char*     ahtable_iter_key       (ahtable_iter_t*, size_t* len);
value_t*        ahtable_iter_val       (ahtable_iter_t*);


#endif

