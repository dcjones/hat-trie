
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
const size_t d_low = 0;  // minimal prefix length
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
    hattrie_t* prefix_counts;
    hattrie_t* prefix_index;

    hattrie_iter_t* i;
    hattrie_iter_t* prefixes;
    size_t count, compare, found;
    value_t* index;
    value_t* u;

    size_t fix;
    size_t len;
    size_t size;
    char* p;
    const char* key;
    value_t* val;
    const char* prefix;

    for (size = d_low; size <= d_high; size += d_delta) {
        count = 0;
        prefix_counts = hattrie_create();
        prefix_index = hattrie_create();
        p = malloc(size * sizeof(char));
        i = hattrie_iter_begin(T, false);

        while (!hattrie_iter_finished(i)) {
            key = hattrie_iter_key(i, &len);

            if (len >= size) {
                ++count;
                memcpy(p, key, size);

                u = hattrie_get(prefix_index, p, size);
                if (!u)
                    *u = hattrie_size(prefix_index) + 1;
                val = hattrie_iter_val(i);
                *val = *u;

                u = hattrie_get(prefix_counts, p, size);
                *u += 1;
            }

            hattrie_iter_next(i);
        }

        hattrie_iter_free(i);
        free(p);

        fprintf(stderr, "iterating through %zu keys by prefixes of length %ld ... \n",
                count, size);
        compare = 0;
        prefixes = hattrie_iter_begin(prefix_counts, false);
        while(!hattrie_iter_finished(prefixes)) {
            prefix = hattrie_iter_key(prefixes, &fix);
            index = hattrie_get(prefix_index, prefix, fix);
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
                val = hattrie_iter_val(i);
                ++found;
                if (*index != *val) {
                    fprintf(stderr,
                            "[error] given prefix id #%zu, iterated over element with id #%zu\n"
                            "        prefix string [%.*s], iterated over element string [%.*s].\n",
                            *index, *val, (int)fix, prefix, (int)len, key);
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

        hattrie_free(prefix_counts);
        hattrie_free(prefix_index);
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


typedef struct {
    const char* test;
    size_t length;
    value_t value;
    bool seen;
    bool valid;
    bool skip;
    const char* name;
} edge_case_test;


bool test_hattrie_odd_keys()
{
    fprintf(stderr, "checking edge-case keys...\n");
    bool passed = true;
    value_t* u;
    hattrie_t* T = hattrie_create();

    size_t test_count = 5;
    edge_case_test tests[] = {
        { .test = "", .length = 0, .value = 0,
          .seen = false, .valid = false, .skip = false,
          .name = "empty" },
        { .test = "\x00", .length = 1, .value = 1,
          .seen = false, .valid = false, .skip = false,
          .name = "empty terminated" },
        { .test = "\x00\x14", .length = 2, .value = 2,
          .seen = false, .valid = false, .skip = false,
          .name = "NUL byte initialized" },
        { .test = "\x14\x00", .length = 3, .value = 3,
          .seen = false, .valid = false, .skip = false,
          .name = "NUL byte terminated" },
        { .test = "\x00\x14\x00", .length = 4, .value = 4,
          .seen = false, .valid = false, .skip = false,
          .name = "NUL byte surrounded" }
    };

    for (size_t test_index = 0; test_index < test_count; test_index++) {
        u = hattrie_get(T, tests[test_index].test, tests[test_index].length);
        *u = tests[test_index].value;
    }

    for (size_t test_index = 0; test_index < test_count; test_index++) {
        u = hattrie_tryget(T, tests[test_index].test, tests[test_index].length);
        if (*u != tests[test_index].value) {
            fprintf(stderr,
                    "[error] can't store %s string key with value %zu!\n",
                    tests[test_index].name, tests[test_index].value);
            tests[test_index].skip = true;
            passed = false;
        }
    }

    hattrie_iter_t* i = hattrie_iter_begin(T, false);
    size_t verify_len;
    value_t* verify_ptr;
    value_t verify_val;
    const char* verify_key;
    while (!hattrie_iter_finished(i)) {
        verify_key = hattrie_iter_key(i, &verify_len);
        verify_ptr = hattrie_iter_val(i);
        verify_val = *verify_ptr;

        edge_case_test* matched_test = NULL;
        for (size_t test_index = 0; test_index < test_count; test_index++) {
            if (verify_len == tests[test_index].length &&
                    memcmp(verify_key, tests[test_index].test, tests[test_index].length) == 0) {
                matched_test = &tests[test_index];
                break;
            }
        }
        if (matched_test == NULL) {
            fprintf(stderr, "[error] iterated over unknown edge-case key [ ");
            for (size_t i = 0; i < verify_len; i++) {
                fprintf(stderr, "0x%X ", verify_key[i]);
            }
            fprintf(stderr, "].\n");
            passed = false;
        } else if (matched_test->seen) {
            fprintf(stderr, "[error] iterated over key [ ");
            for (size_t i = 0; i < verify_len; i++) {
                fprintf(stderr, "0x%X ", verify_key[i]);
            }
            fprintf(stderr, "] more than once!\n");
            passed = false;
        } else {
            matched_test->seen = true;
            if (verify_val == matched_test->value) {
                matched_test->valid = true;
            } else {
                fprintf(stderr, "[error] value stored for key [ ");
                for (size_t i = 0; i < verify_len; i++) {
                    fprintf(stderr, "0x%X ", verify_key[i]);
                }
                fprintf(stderr,
                        "] was incorrect. (expected %zu, got %zu)\n",
                        matched_test->value, verify_val);
                passed = false;
            }
        }
        hattrie_iter_next(i);
    }

    for (uint8_t test_index = 0; test_index < test_count; test_index++) {
        if (tests[test_index].skip)
            continue;
        if (!tests[test_index].seen) {
            fprintf(stderr, "[error] key [ ");
            for (size_t i = 0; i < tests[test_index].length; i++) {
                fprintf(stderr, "0x%X ", tests[test_index].test[i]);
            }
            fprintf(stderr, "] was never iterated over.\n");
            passed = false;
        } else if (!tests[test_index].valid) {
            fprintf(stderr, "[error] key [ ");
            for (size_t i = 0; i < tests[test_index].length; i++) {
                fprintf(stderr, "0x%X ", tests[test_index].test[i]);
            }
            fprintf(stderr, "] was iterated, but with the wrong value.\n");
            passed = false;
        }
    }

    hattrie_iter_free(i);
    hattrie_free(T);

    fprintf(stderr, "done.\n");
    return passed;
}


int main()
{
    bool passed = true;

    if (passed)
        passed &= test_hattrie_non_ascii();
    if (passed)
        passed &= test_hattrie_odd_keys();

    if (passed) {
        setup();
        passed &= test_hattrie_insert();
        passed &= test_hattrie_iteration();
        teardown();
    }

    if (passed) {
        setup();
        passed &= test_hattrie_insert();
        passed &= test_hattrie_sorted_iteration();
        teardown();
    }

    if (passed) {
        setup();
        passed &= test_hattrie_insert();
        passed &= test_hattrie_prefix_iteration();
        teardown();
    }

    if (passed) return 0;
    return 1;
}
