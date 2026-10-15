#include "sw_crc32.h"

/* Table-free CRC32/ISO-HDLC: identical image contract, bounded RAM/ROM cost.
 * Kept callable with a far pointer for both Flash and staged RAM metadata. */
unsigned long F13_Crc32Bytes(const unsigned char __far *data, unsigned long size)
{
    unsigned long crc;
    unsigned char bit;
    crc = 0xFFFFFFFFUL;
    while (size != 0UL)
    {
        crc ^= *data++;
        for (bit = 0U; bit < 8U; bit++)
        {
            crc = (crc >> 1) ^ ((crc & 1UL) ? 0xEDB88320UL : 0UL);
        }
        size--;
    }
    return crc ^ 0xFFFFFFFFUL;
}

#ifndef F13_CRC_HOST_TEST
unsigned long caculate_crc32_checksum(unsigned long start, unsigned long size)
{
    return F13_Crc32Bytes((const unsigned char __far *)start, size);
}
#endif
