
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "str_map.h"
#include "../src/ahtable.h"

/* Simple random string generation. */
void randstr(char* x, size_t len)
{
    x[len] = '\0';
    while (len > 0) {
        x[--len] = '\x20' + (rand() % ('\x7e' - '\x20' + 1));
    }
}


const size_t n = 2000000;  // how many uniques strings
const size_t m_low  = 50;  // minimum length of each string
const size_t m_high = 500; // maximum length of each string
const size_t k = 2000000;  // number of insertions
char** xs;

ahtable_t* T;
str_map* M;


void setup()
{
    fprintf(stderr, "generating %zu keys ... ", n);
    xs = malloc(n * sizeof(char*));
    size_t i;
    size_t m;
    for (i = 0; i < n; ++i) {
        m = m_low + rand() % (m_high - m_low);
        xs[i] = malloc(m + 1);
        randstr(xs[i], m);
    }

    T = ahtable_create();
    M = str_map_create();
    fprintf(stderr, "done.\n");
}


void teardown()
{
    ahtable_free(T);
    str_map_destroy(M);

    size_t i;
    for (i = 0; i < n; ++i) {
        free(xs[i]);
    }
    free(xs);
}


void test_ahtable_insert()
{
    fprintf(stderr, "inserting %zu keys ... \n", k);

    size_t i, j;
    value_t* u;
    value_t  v;

    for (j = 0; j < k; ++j) {
        i = rand() % n;


        v = 1 + str_map_get(M, xs[i], strlen(xs[i]));
        str_map_set(M, xs[i], strlen(xs[i]), v);


        u = ahtable_get(T, xs[i], strlen(xs[i]));
        *u += 1;


        if (*u != v) {
            fprintf(stderr, "[error] tally mismatch (reported: %lu, correct: %lu)\n",
                            *u, v);
        }
    }

    fprintf(stderr, "done.\n");
}



void test_ahtable_iteration()
{
    fprintf(stderr, "iterating through %zu keys ... \n", k);

    ahtable_iter_t* i = ahtable_iter_begin(T);

    size_t count = 0;
    value_t* u;
    value_t  v;

    size_t len;
    const char* key;

    while (!ahtable_iter_finished(i)) {
        ++count;

        key = ahtable_iter_key(i, &len);
        u   = ahtable_iter_val(i);

        v = str_map_get(M, key, len);

        if (*u != v) {
            if (v == 0) {
                fprintf(stderr, "[error] incorrect iteration (%lu, %lu)\n", *u, v);
            }
            else {
                fprintf(stderr, "[error] incorrect iteration tally (%lu, %lu)\n", *u, v);
            }
        }

        // this way we will see an error if the same key is iterated through
        // twice
        str_map_set(M, key, len, 0);

        ahtable_iter_next(i);
    }

    if (count != M->m) {
        fprintf(stderr, "[error] iterated through %zu element, expected %zu\n",
                count, M->m);
    }

    ahtable_iter_free(i);

    fprintf(stderr, "done.\n");
}




int main()
{
    setup();
    test_ahtable_insert();
    test_ahtable_iteration();
    teardown();

    return 0;
}





