
/*
 * Paul Hsieh's SuperFastHash
 * http://www.azillionmonkeys.com/qed/hash.html
 */


#ifndef SUPERFASTHASH_H
#define SUPERFASTHASH_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

uint32_t hash(const char * data, int len);
uint32_t hash_inc(const char * data, int len, uint32_t hash);


#if defined(__cplusplus)
}
#endif

#endif
