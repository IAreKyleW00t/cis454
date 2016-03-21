/**
*   Name:       Kyle Colantonio
*   CSUID:      2595744
*   Date:       24-MAR-2016
*   Project 2:  Design and implement the application-layer
*               protocol for transferring files over UDP/IP.
*               (sendf implementation.)
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
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <hostname> <port> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Save command-line input */
    char *hostname = argv[1];
    unsigned short port = atoi(argv[2]);
    char *file = argv[3];
    
    /* Open the file we wish to send */
    FILE *fp;
    fp = fopen(file, "rb");
    if (!fp) {
        fprintf(stderr, "fopen: Failed to open file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    /* Initialize our data-link */
    if (!dlinit_send(hostname, port)) exit(EXIT_FAILURE);
    
    unsigned char buffer[8192];
    size_t len;
    ssize_t ret;
    while (true) {
        /* Read up to 8192 bytes from file */
        len = fread(buffer, 1, sizeof(buffer), fp);
        
        /* Send data and check if we've reached EOF */
        if (len < sizeof(buffer)) {
            ret = dlsend(buffer, len, true); //Reached EOF
            
            /* Terminate if we encountered an error */
            if (ferror(fp)) {
                fprintf(stderr, "fread: Failed to read file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            break;
        } else {
            ret = dlsend(buffer, len, false); //Did not reach EOF
        }
        
        if (ret == -1) exit(EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS;
}
