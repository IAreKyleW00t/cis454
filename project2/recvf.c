/**
*   Name:       Kyle Colantonio
*   CSUID:      2595744
*   Date:       24-MAR-2016
*   Project 2:  Design and implement the application-layer
*               protocol for transferring files over UDP/IP.
*               (recvf implementation.)
*
*   UPDATED:    02-MAR-2016
**/
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "dllayer.h"

int main(int argc, char **argv) {
    /* Check for valid command-line arguments */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Save command-line input */
    unsigned short port = atoi(argv[1]);
    char *file = argv[2];
    
    /* Open the file we wish to send */
    FILE *fp;
    fp = fopen(file, "wb");
    if (!fp) {
        fprintf(stderr, "fopen: Failed to open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Initialize our data-link */
    if (!dlinit_recv(port)) {
        exit(EXIT_FAILURE);
    }
    
    unsigned char buffer[8192];
    bool eof;
    ssize_t len;
    while (true) {
        /* Wait for data from client */
        len = dlrecv(buffer, sizeof(buffer), &eof);
        
        /* Write data to file */
        fwrite(buffer, 1, len, fp);
        
        /* Terminate if we encountered an error */
        if (ferror(fp)) {
            fprintf(stderr, "fwrite: Failed to write file: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (len == -1) exit(EXIT_FAILURE);
        
        /* Check if we've reached EOF */
        if (eof) break;
    }
    
    fclose(fp);
    
    return EXIT_SUCCESS;
}
