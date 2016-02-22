/**
*   Name:       Kyle Colantonio
*   CSUID:      2595744
*   Date:       23-FEB-2016
*   Project 1:  CRC-CCITT implementation in C
*
*   UPDATED:    21-FEB-2016
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 256 //buffer size
#define POLY 0x10210000 //x^16 + x^12 + x^5 + 1 (with padding)

static int trace = 0; //trace flag

/**
    Method that will calculate the CRC for a given input
    of data. If this is method is called as a sender, the remainder
    will be returned and need to appended to the end of a message.
    If the reciever calls this method with that data (including CRC),
    it should give a remainder of 0, indicating no errors.
**/
unsigned short crc(unsigned char msg[], int len) {
    unsigned int crc = 0; //zero by default

    crc |= msg[0] << 24; // move to MSB
    crc |= msg[1] << 16; // move to 2nd MSB

    for (int j = 2; j < len; ++j) {
        crc += msg[j] << 8; //move to 3rd MSB

        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ POLY; //Shift to left by 1 and XOR by CRC-CCITT poly
            } else {
                crc = crc << 1; //Shift to left by 1
            }

            /* Display trace if set */
            if (trace) {
                fprintf(stdout, "%04x\n", crc >> 16);
            }
        }
    }

    /* Shift 16 bits to the right to get CRC */
    return crc >> 16;
}

int main(int argc, char **argv) {
    unsigned char buf[SIZE];
    unsigned int ch, len = 0;
    memset(buf, 0x00, sizeof(buf)); //zero out buffer (safety precaution)

    /* Check if -trace is set */
    if (argc == 2 && strcmp(argv[1], "-trace") == 0) {
        trace = 1;
    }

    /* Continue to get input from stid until
        we reach EOF. */
    while ((ch = getchar()) != (unsigned int)EOF) {
        buf[len++] = ch; //save char
    }
    len += 2; //add 2 zero bytes to end of message

    /* Calculate and display sender CRC */
    unsigned short crc_sender = crc(buf, len);
    fprintf(stdout, "%04hx\n\n", crc_sender);

    /* Add CRC to end of message */
    buf[len-2] = (crc_sender >> 8) & 0xFF; //MSB
    buf[len-1] = (crc_sender >> 0) & 0xFF; //LSB

    /* Calculate and display reciever CRC */
    unsigned short crc_reciever = crc(buf, len);
    fprintf(stdout, "%04hx\n", crc_reciever);

    return EXIT_SUCCESS;
}
