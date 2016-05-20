#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "log.h"

#define SERVER_NAME "sangHTTP/1.0.0"
#define BUF_SIZE 8192
#define BACKLOG 10

char *getContentType(char *file) {
    char *type, *dot = strrchr(file, '.');
    if (!dot) {
        asprintf(&type, "content/unknown");
    } else if (strncmp(dot, ".exe", 4) == 0) {
        asprintf(&type, "application/octet-stream");
    } else if (strncmp(dot, ".gif", 4) == 0) {
        asprintf(&type, "image/gif");
    } else if (strncmp(dot, ".htm", 4) == 0 || strncmp(dot, ".html", 5) == 0) {
        asprintf(&type, "text/html");
    } else if (strncmp(dot, ".jpg", 4) == 0) {
        asprintf(&type, "image/jpeg");
    } else if (strncmp(dot, ".pdf", 4) == 0) {
        asprintf(&type, "application/pdf");
    } else if (strncmp(dot, ".ps", 3) == 0) {
        asprintf(&type, "application/postscript");
    } else if (strncmp(dot, ".txt", 4) == 0) {
        asprintf(&type, "text/plain");
    }
    
    return type;
}

int main(int argc, char **argv) {
    struct sockaddr_in server, client;
    socklen_t slen = sizeof(server), clen = sizeof(client);
    char buf[BUF_SIZE];
    int sk;
    
    /* Validate command line arguments. */
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Open (and create) our log file. */
    if (!lopen("httpd.log")) {
        fprintf(stderr, "lopen(): Failed to open file for writing\n");
        return EXIT_FAILURE;
    }
    
    /* Clear all zombie processes. X_x */
    signal(SIGCHLD, SIG_IGN);
    
    /* Create our TCP socket. */
    sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!sk) {
        fprintf(stderr, "socket(): Failed to create TCP socket\n");
        return EXIT_FAILURE;
    }
    
    /* Create our Internet address for the host. */
    memset(&server, 0x00, slen);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = 0; //System-assigned port
    
    /* Attempt to bind our socket to a random port. */
    if (bind(sk, (struct sockaddr *)&server, slen) < 0) {
        close(sk);
        fprintf(stderr, "bind(): Failed to bind socket to port\n");
        return EXIT_FAILURE;
    }
    
    /* Get the port that was assigned to us. */
    getsockname(sk, (struct sockaddr *)&server, &slen);
    
    /* Begin to listen to connections. */
    listen(sk, BACKLOG);
    fprintf(stdout, "Listening on port: %d\n", ntohs(server.sin_port)); //Display port number
    
    /* Continuously accept connections. */
    while (1) {
        int conn = accept(sk, (struct sockaddr *)&client, &clen);
        pid_t pid = fork(); //Create child process
        
        /* Once we establish a connection, fork a child process to handle
            that connection and have the parent listen for another connection. */
        if (pid == 0) { //Child
            /* Close original socket (this is no longer needed.) */
            close(sk);
            
            /* Create a variable that we can use to store
                information so we can log it to a file later. */
            char *lmsg;
            size_t llen = 0;
            
            /* Get the request from the client. Due to the way telnet works,
                we must constantly receive data since it will only send one line
                at a time. Once our header contains "\r\n\r\n", then we know we've
                reached the end and we can stop receiving data. */
            char *header = (char *)malloc(0);
            ssize_t len, header_len = 0;
            do {
                /* Receive data from client. */
                len = recv(conn, buf, sizeof(buf), 0);
                
                /* Reallocate memory to our header based on how much data
                    we got from the client and append the data to our
                    header. */
                header = (char *)realloc(header, header_len + len);
                strncat(header, buf, len);
                header_len += len;
            } while (memmem(header, header_len, "\r\n\r\n", 4) == NULL);
            
            /* Parse the method, path, host, and version of the
                request from the client. */
            char *method, *path, *host, *version;
            int ret = 200; //Default to 200 (OK)
            
            /* Loop through all the lines in our header. */
            int line = 0;
            char *state, *p = strtok_r(header, "\r\n", &state);
            while (p != NULL) {
                /* The first line should always be either GET/HEAD. */
                if (line == 0) {
                    /* We can easily parse the method, path, and version
                        by doing three strtok() calls. */
                    method = strtok(p, " ");
                    path = strtok(NULL, " ");
                    version = strtok(NULL, " ");
                    
                    /* Check if the request method is not GET/HEAD. */
                    if (!method || (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)) ret = 400;
                    
                    /* Check if the path is in the correct format. */
                    if (!path || strncmp(path, "/", 1) != 0) ret = 400;
                    
                    /* Check if the HTTP version number is not 1.1. Our
                        server doesn't support other version, so we'll treat
                        them as invalid. */
                    if (!version && strcmp(version, "HTTP/1.1") != 0) ret = 400;
                } else { //All other lines
                    /* Since we are only concerned about the Host, we only need
                        to parse a single key-value pair. We can safely ignore the
                        others since our server doesn't support those headers. */
                    char *key = strtok(p, " ");
                    char *value = strtok(NULL, " ");
                    
                    /* Check if any of these did not get set. */
                    if (!key || !value) ret = 400;
                    
                    /* Check if we reached the "Host" line. If not then
                        we can simply ignore the line. */
                    if (key && value && strcmp(key, "Host:") == 0) {
                        /* Remove the port from the host if it exists. */
                        char *port = strchr(value, ':');
                        if (port) *port = '\0';
                        
                        /* Save our host */
                        host = value;
                    }
                }
                
                /* Move to the next line. */
                line++;
                p = strtok_r(NULL, "\r\n", &state);
            }
            
            /* Parse the current time for our response. This will always
                be required. */
            char serv_date[32];
            time_t t = time(NULL);
            struct tm *gmt = gmtime(&t);
            strftime(serv_date, sizeof(serv_date), "%a, %d %b %Y %H:%M:%S GMT", gmt);
            
            /* Check if we got a bad request. Not the best way to
                handle this, but it works. */
            if (ret == 400 || !host) {
                char *response;
                ssize_t response_len = 0;
                response_len = asprintf(&response, "HTTP/1.1 400 Bad Request\r\nDate: %s\r\nServer: %s\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n400 Bad Request\r\n", serv_date, SERVER_NAME);
                
                /* Send our response (header) to the client. */
                send(conn, response, response_len, 0);
            
                /* Write our log to the file. */
                lwrite(lmsg, llen);
            }
            
            /* Keep track of our file, file size, last modify time
                and the content type. */
            char file_time[32], *type;
            int fd, size;
            
            /* Check if the host provided matches the ones
                we require. If so, change were our server looks for files 
                based on the host. */
            char *dot = strchr(host, '.');
            if (dot && strcmp(dot, ".eecs.csuohio.edu") == 0) {
                char *user, *file;
                struct passwd *pw;
                
                /* Parse the username and file path from the
                    full path. */
                path++; //Remove the leading /
                file = strchr(path, '/');
                if (file) {
                    /* Check if the user didn't provide a filename. */
                    //if (strrchr(path, '/') == file) ret = 404;
                    
                    *file = '\0';
                    file++;
                } else if (!file) ret = 404;
                user = (path + 1);
                
                /* Get information stored in /etc/passwd for the user
                    that was selected. */
                pw = getpwnam(user);
                if (pw) {
                    /* Get the full path with the users information. */
                    char *full_path;
                    asprintf(&full_path, "%s/public_html/%s", pw->pw_dir, file);
                    type = getContentType(full_path);
                    
                    /* Check if the file exists. */
                    struct stat sf;
                    fd = open(full_path, O_RDONLY);
                    if (fd < 0 && errno == ENOENT) ret = 404;
                    else if (fd < 0 && errno == EACCES) ret = 403;
                    fstat(fd, &sf);
                    size = sf.st_size;
                    
                    /* Get the last modified time. */
                    gmt = gmtime(&sf.st_mtime);
                    strftime(file_time, sizeof(file_time), "%a, %d %b %Y %H:%M:%S GMT", gmt);
                    
                } else ret = 404;
                
                /* Log the access. */
                if (!file) llen = asprintf(&lmsg, "%s - - [%s] \"%s %s %s\" %d\n", inet_ntoa(client.sin_addr), serv_date, method, path - 1, version, ret);
                else llen = asprintf(&lmsg, "%s - - [%s] \"%s %s/%s %s\" %d\n", inet_ntoa(client.sin_addr), serv_date, method, path - 1, file, version, ret);
            } else if (dot && strcmp(dot, ".cis.csuohio.edu") == 0) {
                char *full_path;
                
                path++; //Remove leading /
                asprintf(&full_path, "/home/assignments/cis554s/www/%s", path);
                type = getContentType(full_path);
                
                /* Check if the file exists. */
                struct stat sf;
                fd = open(full_path, O_RDONLY);
                if (fd < 0 && errno == ENOENT) ret = 404;
                else if (fd < 0 && errno == EACCES) ret = 403;
                fstat(fd, &sf);
                size = sf.st_size;
                
                /* Get the last modified time. */
                gmt = gmtime(&sf.st_mtime);
                strftime(file_time, sizeof(file_time), "%a, %d %b %Y %H:%M:%S GMT", gmt);
                
                llen = asprintf(&lmsg, "%s - - [%s] \"%s %s %s\" %d\n", inet_ntoa(client.sin_addr), serv_date, method, path - 1, version, ret);
            }
            
            /* Write our log to the file. */
            lwrite(lmsg, llen);

            
            /* Create our response for the client based on our method, ret #, etc. */
            char *response;
            ssize_t response_len = 0;
            if (ret != 200) { //Not-OK
                switch (ret) {
                    case 403:
                        response_len = asprintf(&response, "HTTP/1.1 %d Permission denied\r\nDate: %s\r\nServer: %s\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n403 Permission Denied\r\n", ret, serv_date, SERVER_NAME);
                        break;
                        
                    case 404:
                        response_len = asprintf(&response, "HTTP/1.1 %d Not Found\r\nDate: %s\r\nServer: %s\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n404 File not Found\r\n", ret, serv_date, SERVER_NAME);
                        break;
                }
            } else { //OK
                response_len = asprintf(&response, "HTTP/1.1 %d OK\r\nDate: %s\r\nServer: %s\r\nLast-Modified: %s\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", ret, serv_date, SERVER_NAME, file_time, size, type);
            }
            
            /* Send our response (header) to the client. */
            send(conn, response, response_len, 0);
            
            /* If everything checks out, then we can send the data from the file
                that was requested if the request method was GET. */ 
            if (ret == 200 && strcmp(method, "GET") == 0) {
                /* Only send up to 8K at a time. */
                do {
                    len = read(fd, buf, sizeof(buf));                    
                    send(conn, buf, len, 0);
                } while (len == sizeof(buf));
            }
                                                
            /* Close connection and kill child process. */
            lclose();
            close(conn);
            _exit(EXIT_SUCCESS);
        } else if (pid > 0) { //Parent
            close(conn);
        } else { //Error
            close(sk); close(conn);
            fprintf(stderr, "main: Failed to fork process: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
    }
    
    return EXIT_SUCCESS;
}