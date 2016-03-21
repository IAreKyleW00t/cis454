/**
*   Name:       Kyle Colantonio
*   CSUID:      2595744
*   Date:       24-MAR-2016
*   Project 2:  Design and implement the application-layer
*               protocol for transferring files over UDP/IP.
*
*   UPDATED:    01-MAR-2016
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crc.h"

#define SIZE 256 //buffer size


static int trace = 0; //trace flag

int main(int argc, char **argv) {
    unsigned char buf[SIZE];
    int ch, len = 0;
    memset(buf, 0x00, sizeof(buf)); //zero out buffer (safety precaution)

    /* Check if -trace is set */
    if (argc == 2 && strcmp(argv[1], "-trace") == 0) {
        trace = 1;
    }

    /* Continue to get input from stid until
        we reach EOF. */
    while ((ch = getchar()) != EOF) {
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
