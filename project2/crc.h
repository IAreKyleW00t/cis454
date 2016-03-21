#pragma once

#define POLY 0x10210000 //x^16 + x^12 + x^5 + 1 (with padding)

unsigned short crc(unsigned char msg[], int len);