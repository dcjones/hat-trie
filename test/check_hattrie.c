
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../src/hat-trie.h"

/* Simple random string generation. */
void randstr(char* x, size_t len)
{
    x[len] = '\0';
    while (len > 0) {
        x[--len] = '\x20' + (rand() % ('\x7e' - '\x20' + 1));
    }
}


const size_t n = 2000000;  // how many uniques strings
const size_t m = 50;       // length of each string
char** xs;
int*   cs;
hattrie_t* T;

void setup()
{
    xs = malloc(n * sizeof(char*));
    size_t i;
    for (i = 0; i < n; ++i) {
        xs[i] = malloc(m + 1);
        randstr(xs[i], m);
    }

    cs = malloc(n * sizeof(int));
    memset(cs, 0, n * sizeof(int));

    T = hattrie_create();
}

void teardown()
{
    hattrie_free(T);

    free(cs);
    size_t i;
    for (i = 0; i < n; ++i) {
        free(xs[i]);
    }
    free(xs);
}


void test_hattrie_insert()
{
    size_t k = 2000000; // number of insertions
    fprintf(stderr, "inserting %zu keys ... \n", k);

    size_t i;
    value_t* val;

    while (k--) {
        i = rand() % n;
        cs[i] += 1;
        val = hattrie_get(T, xs[i], strlen(xs[i]));
        *val += 1;

        if ((size_t) cs[i] != *val) {
            fprintf(stderr, "[error] tally mismatch (reported: %zu, correct: %d)\n",
                            *val, cs[i]);
        }
    }

    fprintf(stderr, "done.\n");
}



int main()
{
    setup();
    test_hattrie_insert();
    teardown();

    return 0;
}





