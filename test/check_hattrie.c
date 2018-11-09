
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "str_map.h"
#include "../src/hat-trie.h"

/* Simple random string generation. */
void randstr(char* x, size_t len)
{
    x[len] = '\0';
    while (len > 0) {
        x[--len] = '\x20' + (rand() % ('\x7e' - '\x20' + 1));
    }
}

const size_t n = 100000;  // how many unique strings
const size_t m_low  = 50;  // minimum length of each string
const size_t m_high = 500; // maximum length of each string
const size_t k = 200000;  // number of insertions
const size_t d = 50000;
const size_t d_low = 1;  // minimal prefix length
const size_t d_high = 4;  // maximal prefix length
const size_t d_delta = 1;  // change between each prefix length test

char** xs;
char** ds;

hattrie_t* T;
str_map* M;


void setup()
{
    fprintf(stderr, "generating %zu keys ... ", n);
    xs = malloc(n * sizeof(char*));
    ds = malloc(d * sizeof(char*));
    size_t i;
    size_t m;
    for (i = 0; i < n; ++i) {
        m = m_low + rand() % (m_high - m_low);
        xs[i] = malloc(m + 1);
        randstr(xs[i], m);
    }
    for (i = 0; i < d; ++i) {
        m = rand()%n;
        ds[i] = xs[m];
    }

    T = hattrie_create();
    M = str_map_create();
    fprintf(stderr, "done.\n");
}


void teardown()
{
    hattrie_free(T);
    str_map_destroy(M);

    size_t i;
    for (i = 0; i < n; ++i) {
        free(xs[i]);
    }
    free(xs);
    free(ds);
}


bool test_hattrie_insert()
{
    fprintf(stderr, "inserting %zu keys ... \n", k);

    bool passed = true;
    size_t i, j;
    value_t* u;
    value_t  v;

    for (j = 0; j < k; ++j) {
        i = rand() % n;


        v = 1 + str_map_get(M, xs[i], strlen(xs[i]));
        str_map_set(M, xs[i], strlen(xs[i]), v);


        u = hattrie_get(T, xs[i], strlen(xs[i]));
        *u += 1;


        if (*u != v) {
            fprintf(stderr, "[error] tally mismatch (reported: %lu, correct: %lu)\n",
                            *u, v);
            passed = false;
        }
    }

    fprintf(stderr, "sizeof: %zu\n", hattrie_sizeof(T));

    fprintf(stderr, "deleting %zu keys ... \n", d);
    for (j = 0; j < d; ++j) {
        str_map_del(M, ds[j], strlen(ds[j]));
        hattrie_del(T, ds[j], strlen(ds[j]));
        u = hattrie_tryget(T, ds[j], strlen(ds[j]));
        if (u) {
            fprintf(stderr, "[error] item %zu still found in trie after delete\n", j);
            passed = false;
        }
    }

    fprintf(stderr, "done.\n");
    return passed;
}


bool test_hattrie_iteration()
{
    fprintf(stderr, "iterating through %zu keys ... \n", k);

    hattrie_iter_t* i = hattrie_iter_begin(T, false);
    bool passed = true;
    size_t count = 0;
    value_t* u;
    value_t  v;

    size_t len;
    const char* key;

    while (!hattrie_iter_finished(i)) {
        ++count;

        key = hattrie_iter_key(i, &len);
        u   = hattrie_iter_val(i);

        v = str_map_get(M, key, len);

        if (*u != v) {
            if (v == 0) {
                fprintf(stderr, "[error] incorrect iteration (%lu, %lu)\n", *u, v);
                passed = false;
            }
            else {
                fprintf(stderr, "[error] incorrect iteration tally (%lu, %lu)\n", *u, v);
                passed = false;
            }
        }

        // this way we will see an error if the same key is iterated through
        // twice
        str_map_set(M, key, len, 0);

        hattrie_iter_next(i);
    }

    if (count != M->m) {
        fprintf(stderr, "[error] iterated through %zu element, expected %zu\n",
                count, M->m);
        passed = false;
    }

    hattrie_iter_free(i);

    fprintf(stderr, "done.\n");
    return passed;
}


int cmpkey(const char* a, size_t ka, const char* b, size_t kb)
{
    int c = memcmp(a, b, ka < kb ? ka : kb);
    return c == 0 ? (int) ka - (int) kb : c;
}


bool test_hattrie_sorted_iteration()
{
    fprintf(stderr, "iterating in order through %zu keys ... \n", k);

    hattrie_iter_t* i = hattrie_iter_begin(T, true);
    bool passed = true;
    size_t count = 0;
    value_t* u;
    value_t  v;

    char* key_copy = malloc(m_high + 1);
    char* prev_key = malloc(m_high + 1);
    memset(prev_key, 0, m_high + 1);
    size_t prev_len = 0;

    const char *key = NULL;
    size_t len = 0;

    while (!hattrie_iter_finished(i)) {
        memcpy(prev_key, key_copy, len);
        prev_key[len] = '\0';
        prev_len = len;
        ++count;

        key = hattrie_iter_key(i, &len);

        /* memory for key may be changed on iter, copy it */
        strncpy(key_copy, key, len);

        if (prev_key != NULL && cmpkey(prev_key, prev_len, key, len) > 0) {
            fprintf(stderr, "[error] iteration is not correctly ordered.\n");
            passed = false;
        }

        u = hattrie_iter_val(i);
        v = str_map_get(M, key, len);

        if (*u != v) {
            if (v == 0) {
                fprintf(stderr, "[error] incorrect iteration (%lu, %lu)\n", *u, v);
                passed = false;
            }
            else {
                fprintf(stderr, "[error] incorrect iteration tally (%lu, %lu)\n", *u, v);
                passed = false;
            }
        }

        // this way we will see an error if the same key is iterated through
        // twice
        str_map_set(M, key, len, 0);

        hattrie_iter_next(i);
    }

    if (count != M->m) {
        fprintf(stderr, "[error] iterated through %zu element, expected %zu\n",
                count, M->m);
        passed = false;
    }

    hattrie_iter_free(i);
    free(prev_key);
    free(key_copy);

    fprintf(stderr, "done.\n");
    return passed;
}


bool test_hattrie_prefix_iteration()
{
    bool passed = true;
    hattrie_t* P;

    hattrie_iter_t* i;
    hattrie_iter_t* prefixes;
    size_t count, compare, found;
    value_t* u;

    size_t fix;
    size_t len;
    size_t size;
    char* p;
    const char* key;
    const char* prefix;

    for (size = d_low; size <= d_high; size += d_delta) {
        fprintf(stderr, "iterating through %zu keys by prefixes of length %ld ... \n",
                k, size);
        count = 0;
        P = hattrie_create();
        p = malloc(size * sizeof(char));
        i = hattrie_iter_begin(T, false);

        while (!hattrie_iter_finished(i)) {
            key = hattrie_iter_key(i, &len);

            if (len >= size) {
                ++count;
                memcpy(p, key, size);
                u = hattrie_get(P, p, size);
                *u += 1;
            }

            hattrie_iter_next(i);
        }

        hattrie_iter_free(i);
        free(p);

        compare = 0;
        prefixes = hattrie_iter_begin(P, false);
        while(!hattrie_iter_finished(prefixes)) {
            prefix = hattrie_iter_key(prefixes, &fix);
            if (size != fix) {
                fprintf(stderr,
                        "[error] iterated over prefix [%.*s] of length %zu, expected length %zu.\n",
                        (int)fix, prefix, fix, size);
                passed = false;
                hattrie_iter_next(prefixes);
                continue;
            }

            found = 0;
            i = hattrie_iter_begin_with_prefix(T, false, prefix, fix);
            while(!hattrie_iter_finished(i)) {
                key = hattrie_iter_key(i, &len);
                ++found;
                if (memcmp(key, prefix, fix) != 0) {
                    fprintf(stderr,
                            "[error] iterated through element [%.*s...] via prefix [%.*s].\n",
                            ((int)fix * 3) / 2, key, (int)fix, prefix);
                    passed = false;
                }
                hattrie_iter_next(i);
            }
            hattrie_iter_free(i);

            u = hattrie_iter_val(prefixes);
            if (u) {
                if (*u != found) {
                    fprintf(stderr,
                            "[error] iterated through %zu elements for prefix [%.*s], expected %zu.\n",
                            found, (int)fix, prefix, *u);
                    passed = false;
                }
            } else {
                fprintf(stderr, "[error] iterated through prefix [%.*s], which shouldn't exist.\n",
                        (int)fix, prefix);
                passed = false;
            }
            compare += found;

            hattrie_iter_next(prefixes);
        }
        hattrie_iter_free(prefixes);

        if (compare != count) {
            fprintf(stderr, "[error] iterated through %zu elements, expected %zu.\n", compare, count);
            passed = false;
        }

        hattrie_free(P);
    }

    fprintf(stderr, "done.\n");
    return passed;
}


bool test_hattrie_non_ascii()
{
    fprintf(stderr, "checking non-ascii... \n");
    bool passed = true;
    value_t* u;
    hattrie_t* T = hattrie_create();
    char* txt = "\x81\x70";

    u = hattrie_get(T, txt, strlen(txt));
    *u = 10;

    u = hattrie_tryget(T, txt, strlen(txt));
    if (*u != 10){
        fprintf(stderr, "[error] can't store non-ascii strings\n");
        passed = false;
    }
    hattrie_free(T);

    fprintf(stderr, "done.\n");
    return passed;
}


bool test_hattrie_odd_keys()
{
    fprintf(stderr, "checking edge-case keys...\n");
    bool passed = true;
    value_t* u;
    hattrie_t* T = hattrie_create();

    char* other = "\x00\x14";
    size_t other_len = 2;
    value_t other_val = 7;
    u = hattrie_get(T, other, other_len);
    *u = other_val;

    char* key = "\x00\x14\x00";
    size_t key_len = 3;
    value_t key_val = 14;
    u = hattrie_get(T, key, key_len);
    *u = key_val;

    u = hattrie_tryget(T, other, other_len);
    if (*u != other_val) {
        fprintf(stderr, "[error] can't store NUL byte keys!\n");
        passed = false;
    }

    u = hattrie_tryget(T, key, key_len);
    if (*u != key_val) {
        fprintf(stderr, "[error] NUL byte keys overwrite each other!\n");
        passed = false;
    }
    hattrie_free(T);

    fprintf(stderr, "done.\n");
    return passed;
}



int main()
{
    bool passed = true;

    setup();
    passed &= test_hattrie_insert();
    passed &= test_hattrie_iteration();
    teardown();

    setup();
    passed &= test_hattrie_insert();
    passed &= test_hattrie_sorted_iteration();
    teardown();

    setup();
    passed &= test_hattrie_insert();
    passed &= test_hattrie_prefix_iteration();
    teardown();

    passed &= test_hattrie_non_ascii();
    passed &= test_hattrie_odd_keys();

    if (passed) return 0;
    return 1;
}
