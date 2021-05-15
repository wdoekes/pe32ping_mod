#ifndef INCLUDED_FNV1A_H
#define INCLUDED_FNV1A_H

/* In 2012 user Ian Boyd wrote a very complete reply to the question
 * "which unsecure hash algorithm is the best", at
 * https://softwareengineering.stackexchange.com/questions/49550/\
 * which-hashing-algorithm-is-best-for-uniqueness-and-speed .
 * Boyd selected a few common algorithms and ranked them by speed,
 * number of collisions and distribution (important!).
 *
 * Short summary:
 * - CRC32 had fewer collisions (distribution map was not compared);
 * - Murmur2 (and possibly newer algorithms win in speed);
 * - FNV-1a has a good distribution, is not terribly slow and is super
 *   easy to implement (and doesn't require big lookup tables).
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FNV_PRIME_32 16777619UL
#define FNV_OFFSET_32 2166136261UL

/**
 * FNV-1a inscure hash algorithm.
 * http://www.isthe.com/chongo/tech/comp/fnv/
 */
static inline uint32_t fnv1a_32(const char *s)
{
    uint32_t hash = 2166136261UL; /* FNV_OFFSET_32 */
    char ch;
    while ((ch = *s++)) {
        hash = hash ^ ch;
#if 1
        hash = hash * 16777619UL; /* FNV_PRIME_32; */
#else /* might be faster, is not with gcc-9 */
	hash += (
	    (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) +
	    (hash << 24));
#endif
    }
    return hash;
}

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_FNV1A_H
