#include "crc8.h"

/**
 * On this page
 * https://reveng.sourceforge.io/crc-catalogue/1-15.htm#crc.cat-bits.8
 * there is a list of CRC checksums, like:
 *
 * - CRC-8/DARC (x8+x5+x4+x3+1)
 *   width=8 poly=0x39 init=0x00 refin=true refout=true xorout=0x00
 *   check=0x15 residue=0x00 name="CRC-8/DARC"
 *
 * Those properties can be broken down as:
 * - width=8:       8 bits storage and return value
 * - poly=0x39:     0b0011'1001 -> x^8 (implied) x^5 x^4 x^3 x^0
 *                  (Koopman notation implies the x^0 instead, so that
 *                  would be 0b1001'1100 which is 0x9C.)
 * - init=0x00:     initial value 0x0
 * - refin=true:    XXX?
 * - refin=true:    XXX?
 * - xorout=0x00:   do XOR with this value before returning
 * - check=0x15:    XXX?
 * - residue=0x00:  XXX?
 *
 * A few others:
 *
 * - CRC-8 (x8+x2+x1+1)
 *   width=8 poly=0x07 init=0x00 refin=false refout=false xorout=0x00
 *   check=0xf4 residue=0x00 name="CRC-8/SMBUS" (Koopman 0x83.)
 *
 * - CRC-8/SAE-J1850
 *   width=8 poly=0x1d init=0xff refin=false refout=false xorout=0xff
 *   check=0x4b residue=0xc4 name="CRC-8/SAE-J1850"
 *
 * According to "Checksum and CRC Tutorial"
 * https://www.youtube.com/watch?v=4y_rowNNVUQ
 * are "All CRCs Not The Same" you should start by declaring what
 * Hamming Distance you find acceptable. And then check what the message
 * length is. And then look up the appropriate bit size and the right
 * polynomial.
 *
 * The Hamming Distance (HD) is the "the minimum number of bit errors
 * that a CRC code would detect". So HD=2 will detect (at least) up to
 * two flipped bits in a message. If there are more flipped bits,
 * collisions (= false negatives) are possible.
 *
 * Apparently, the different polynomials have varying results.
 *
 * For instance, for up to 9 bits, the CRC-8/DARC polynomial 0x39
 * (Koopman 0x9C) has HD=5. But higher than 9 bits, the HD quickly drops
 * to 2 for this polynomial.
 *
 * If we instead decide on an 8 bits CRC, we can then choose the
 * polynomial with the longest bit length for an acceptable hemming
 * distance:
 * - 0x(1)39 (Koopman 0x9C) allows 9 bits for HD=5
 * - 0x(1)2F (Koopman 0x97) allows 119 bits for HD=4
 * - 0x(1)4D (Koopman 0xA6) allows 247 bits for HD=3
 * - 0x(1)4D (Koopman 0xA6) allows 2048+ bits for HD=2
 *
 * Exhaustive list:
 * https://users.ece.cmu.edu/~koopman/crc/crc8.html
 *
 *        Explicit+1 (9bits)         bits-for-HD3,HD4,HD5-etc..
 *  Koopman-implicit+1 (8bits)  Bit-reversed (works the same)
 *  |     |              +------|    |
 * (0xe7; 0x1cf) <=> (0xf3; 0x1e7) {247,19,1,1,1} | CRC-8F/3 ("717p")
 * (0xa6; 0x14d) <=> (0xb2; 0x165) {247,15,6}     | CRC-8K/3
 * (0x8e; 0x11d) <=> (0xb8; 0x171) {247,13,6}     | SAE J-1850; FP-8
 * (0xb1; 0x163) <=> (0xc6; 0x18d) {247,12,4}     | CCITT-8
 *
 * (0xbf; 0x17f) <=> (0xfe; 0x1fd) {119,119,1,1,1,1} | CRC-8F/8 ("577")
 * (0x97; 0x12f) <=> (0xf4; 0x1e9) {119,119,3,3}  | CRC-8-AUTOSAR; CRC-8F/4.2
 * (0xd3; 0x1a7) <=> (0xe5; 0x1cb) {119,119,3,3}  | CRC-8-Bluetooth
 * (0xcd; 0x19b) <=> (0xd9; 0x1b3) {119,119,2,2}  | WCDMA-8
 * (0x83; 0x107) <=> (0xe0; 0x1c1) {119,119}      | FOP-8; ATM-8; CRC-8P
 * (0x98; 0x131) <=> (0x8c; 0x119) {119,119}      | DOWCRC
 * (0x9b; 0x137) <=> (0xec; 0x1d9) {118,118,4,4}  | CRC-8F/6 ("467")
 *
 * (0xa4; 0x149) <=> (0x92; 0x125) {97,97}        | CRC-8-GSM-B
 * (0xea; 0x1d5) <=> (0xab; 0x157) {85,85,2,2}    | (*o) CRC-8
 * (0x8d; 0x11b) <=> (0xd8; 0x1b1) {43,26,5}      | CRC-8F/4.1 ("433")
 * (0x9c; 0x139) <=> (0x9c; 0x139) {9,9,9}        | DARC-8
 * (0xeb; 0x1d7) <=> (0xeb; 0x1d7) {9,9,9,2,1}    | CRC-8F/5 ("727s")
 * (0x80; 0x101) <=> (0x80; 0x101) {}
 *
 * Let us decide on a POLYNOMIAL 0x107 which has HD=4 for 119 bits, and
 * happens to be pretty standard.
 */
#define INXOR 0x00
#define OUTXOR 0x00
#define POLYNOMIAL 0x107 /* x8 + x2 + x1 + 1 */

#include <stdint.h>

#if 1
/**
 * This one is up to 25% slower than the one using a uint16_t, on a x86_64.
 * But it may run faster on an 8-bit processor like the AVR.
 */
char crc8(const char *p)
{
    uint8_t crc = INXOR;
    while (*p != '\0') {
        crc ^= (uint8_t)*p;
        for (int bit = 8; bit > 0; --bit) {
            int msb = crc & 0x80;
            crc = (crc << 1);
            if (msb) {
                crc ^= (POLYNOMIAL & 0xff);
            }
        }
        ++p;
    }
    return crc ^ OUTXOR;
}
#else
/**
 * This one is up to 25% faster than the one using a uint8_t, on a x86_64.
 */
char crc8(const char *p)
{
    uint16_t crc = INXOR;
    while (*p != '\0') {
        crc ^= ((uint8_t)*p << 8);
        for (int bit = 8; bit > 0; --bit) {
            if ((crc & 0x8000)) {
                crc ^= (POLYNOMIAL << 7);
            }
            crc <<= 1;
        }
        ++p;
    }
    return (uint8_t)(crc >> 8) ^ OUTXOR;
}
#endif

#if 0 /* Don't forget to enable optimizations -O3 when testing/comparing! */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "fnv1a.h"

double timeit(void (*f)(), int iterations) {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; ++i)
        (*f)();
    clock_gettime(CLOCK_MONOTONIC, &end);

    return ((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) /
            1000000000.0);
}

const char *the_data;

void crc8_the_data() {
    /* With gcc-9 optimizations turned on crc8 runs _slightly_ faster
     * than fnv1a_32. But only just. */
    crc8(the_data);
}

void fnv1a_the_data() {
    /* With gcc-9 optimizations turned on fnv1a_32 runs _slightly_ slower
     * than crc8. But only just. Note that _without_ optimizations,
     * fnv1a_32 wins the race by a whopping 5 times. */
    fnv1a_32(the_data);
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        the_data = argv[1];
    } else {
        the_data = "http://ifconfig.co/";
    }

    printf("CRC8: 0x%hhx (%hhu)\n", crc8(the_data), crc8(the_data));
    printf("FNV-1a: 0x%x (%u)\n", fnv1a_32(the_data), fnv1a_32(the_data));
    printf("[crc8 : %f]\n", timeit(crc8_the_data, 100000000));
    printf("[fnv1a: %f]\n", timeit(fnv1a_the_data, 100000000));
    return 0;
}
#endif
