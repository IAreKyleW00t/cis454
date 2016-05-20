/**
*   Name:       Kyle Colantonio
*   CSUID:      2595744
*   Date:       14-APR-2016
*   Project 3:  Create a program that will send
*               a RAW message to a server in a way
*               that is similar to telnet.
*
*   UPDATED:    24-MAR-2016
**/
#define _GNU_SOURCE
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

static FILE *fp;
static int sk;
static const char *remote;
static struct sockaddr_in host;

bool vsie_init(const char *hostname, const unsigned short port) {
    /* Create a TCP docket. */
    sk = socket(AF_INET, SOCK_STREAM, 0);
    if (!sk) {
        fprintf(stderr, "vsie_open: Failed to create TCP socket: %s\n", strerror(errno));
        return false;
    }
    
    /* Save hostname. */
    struct hostent *hp = gethostbyname(hostname);
    if (!hp) {
        fprintf(stderr, "vsie_open: Failed to get host by name: %s\n", hstrerror(h_errno));
        return false;
    }
    
    /* Create our Internet address for the host. */
    host.sin_family = AF_INET;
    memcpy(&host.sin_addr.s_addr, hp->h_addr, hp->h_length);
    host.sin_port = htons(port);
    
    /* Connect to the remote host. */
    if (connect(sk, (struct sockaddr *)&host, sizeof(host)) < 0) {
        fprintf(stderr, "vsie_open: Failed to connect to host: %s\n", hstrerror(h_errno));
        return false;
    }
    
    /* Save hostname. */
    remote = hostname;
    
    /* Return true if no errors occured when initializing our socket. */
    return true;
}

bool vsie_close(void) {
    /* Check if we have a socket open. */
    if (!sk) {
        fprintf(stderr, "vsie_close: Failed to close socket: Cannot close NULL socket\n");
        return false;
    }
    
    /* Close socket. */
    close(sk);
    return true;
}


ssize_t vsie_req(const char *path, const char *method, const char *filename) {
    char buf[BUFFER_SIZE], *msg;
    ssize_t tlen = 0, len;
    bool content = false;
    
    /* Return -1 if vsie_open has not been called successfully */
    if (!sk) {
        fprintf(stderr, "vsie_req: Failed to receive data: Cannot send request to NULL socket\n");
        return -1;
    }
    
    if (filename != NULL) {
        /* Open file for writing. */
        fp = fopen(filename, "wb");
        if (!fp) {
            fprintf(stderr, "vsie_req: Failed to open file: %s\n", strerror(errno));
            return -1;
        }
    }
    
    /* Send our formatted message to the server. */
    len = asprintf(&msg, "%s /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", method, path, remote);    
    send(sk, msg, len, 0);
    
    /* Continue to read data until we reach the end. (Large files) */
    while (true) {         
        /* Get our response from the server (read data from stream).
            This will continue where we left off if we're looping
            multiple times. */
        len = recv(sk, buf, sizeof(buf), 0);
        
        /* Search for the two "new lines" after our header. */
        char *ptr = (char *) memmem(buf, len, "\r\n\r\n", 4);
        
        /* If we found our End-Of-Header bytes AND we haven't already found our header,
            then we know the content we got is the header. Otherwise, we got normal content. */
        if (ptr && !content) {
            int offset = ptr - buf; //Calculate offset
            
            /* Save the header to a temporary local variable. */
            char header[offset];
            memcpy(header, buf, offset);
            
            /* Display header to screen and content to file. */
            fprintf(stdout, "%s\n", header);
            if (fp) fwrite((buf + offset + 4), 1, len - (offset + 4), fp);
            
            /* Set our content flag to true because we have now reached content. */
            content = true;
        } else if (!content) { //Large header
            fprintf(stdout, "%s", buf);
        } else if (content) { //Only content
            if (fp) fwrite(buf, 1, len, fp);
        }

        /* Add to our total length and check if we're done receiving data. */
        tlen += len;
        if (len == 0) break;
    }
    
    /* Close our file (if needed) and return the total length of the
        data we received from the server. */
    if (fp) fclose(fp);
    return tlen;
}

int main(int argc, char **argv) {
    const unsigned short port = 80;
    const char *url, *hostname, *path, *filename;
    
    /* Check for valid command-line arguments. */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <method> <url>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Save entire URL into local variable. */
    url = argv[2];
    
    /* Check if our URL is valid by checking if a path  
        was provided. (eg: example.com/some/path) */
    char *ptr = strchr(url, '/');
    if (!ptr) {
        fprintf(stderr, "URL does not contain a path\n");
        return EXIT_FAILURE;
    }
    
    /* Replease the '/' with a NULL character. This is so
        we can split the entire URL into a hostname and path. */
    *ptr = '\0';
    hostname = url;
    path = (ptr + 1);
    
    /* Parse the filename from the path. If one does not exist
        then we will assume the path is pointing to a file.
        (eg: example.com/path) */
    ptr = strrchr(path, '/');
    if (ptr) {
        filename = (ptr + 1);
    } else {
        filename = path;
    }
    
    /* Check if a valid file was provided in the path.
        (eg: example.com/path/ is not valid) */
    if (!*filename) {
        fprintf(stderr, "Filename cannot be empty (URL cannot end in a slash)\n");
        return EXIT_FAILURE;
    }
    
    /* Initialize our connection with the input provided
        from the user. */
    if (!vsie_init(hostname, port)) {
        return EXIT_FAILURE;
    }
    
    /* Check if what kind of method we will be using when
        we send our request. */
    ssize_t len;
    if (strcmp(argv[1], "-head") == 0) { //HEAD
        len = vsie_req(path, "HEAD", NULL);
        if (len < 0) return EXIT_FAILURE; //Check for error
    } else if (strcmp(argv[1], "-get") == 0) { //GET
        len = vsie_req(path, "GET", filename);
        if (len < 0) return EXIT_FAILURE; //Check for error
    } else { //Invalid
        fprintf(stderr, "Usage: %s <method> <url>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Close our TCP socket before terminating. */
    vsie_close();
    return EXIT_SUCCESS;
}