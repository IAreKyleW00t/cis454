#include "crc.h"
unsigned short crc(unsigned char *data, int len) {
    unsigned int crc = 0; //zero by default.

    crc |= (data[0] << 24) + (data[1] << 16); //move initial bytes before looping.
    for (int j = 2; j < len; ++j) {
        crc += (data[j] << 8); //move to into 3rd byte.

        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ POLY; //shl 1 and XOR by CRC-CCITT poly.
            } else {
                crc = (crc << 1); //shl 1 and skip XOR.
            }
        }
    }

    /* Shift 16 bits to the right to get final CRC. */
    return (crc >> 16);
}