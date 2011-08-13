
#include <stdlib.h>
#include <check.h>
#include "../src/ahtable.h"

/* Simple random string generation. */
void randstr(char* x, size_t len)
{
    while (len > 0) {
        x[--len] = '\x20' + (rand() % ('\x7e' - '\x20' + 1));
    }
    x[len] = '\0';
}


const size_t n = 100000;  // how many uniques strings
const size_t m = 50;      // length of each string
char** xs;
int*   cs;
ahtable* T;

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

    T = ahtable_create();
}

void teardown()
{
    ahtable_free(T);

    free(cs);
    size_t i;
    for (i = 0; i < n; ++i) {
        xs[i] = malloc(m);
        randstr(xs[i], m);
    }
}


START_TEST (test_ahtable_insert)
{
    size_t k = 1000000; // number of insertions
    size_t i;
    value_t* val;

    while (k--) {
        i = rand() % n;
        cs[i] += 1;
        val = ahtable_get(T, xs[i]);
        *val += 1;
    }
}
END_TEST



Suite* ahtable_suite()
{
    Suite* s = suite_create("ahtable");
    TCase* tc = tcase_create("core");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_ahtable_insert);
    suite_add_tcase(s, tc);

    return s;
}


int main()
{
    int number_failed;
    Suite* s = ahtable_suite();
    SRunner* sr = srunner_create (s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}




