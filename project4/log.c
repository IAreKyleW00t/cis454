#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include "log.h"

static int fd;

/* Opens a file so we can log to it. This proces will
    open the file as write-only and create it if it does
    not exist with permissions 0600. Any data that is
    written to it will be appended to the end. */
bool lopen(char *file) {
    fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0600);
    return (fd ? true : false);
}

/* Closes the file we originally opened for logging.
    Any changes that need flushed to the file will happen
    at this point before being closed. */
bool lclose(void) {
    return (close(fd) == 0 ? true : false);
}

/* Attempts to write data to the log file. This will append
    the data to the end of the file without overwritting
    what was already there. */
ssize_t lwrite(const void *msg, size_t len) {
    return write(fd, msg, len);
}