#ifndef INCLUDED_CRC8_H
#define INCLUDED_CRC8_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculate 8 bit CRC over the supplied string.
 *
 * The crc8 C file defines which polynomial is used.
 */
char crc8(const char *p);

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_CRC8_H
