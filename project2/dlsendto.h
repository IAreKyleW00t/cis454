#pragma once

ssize_t dlsendto(int sk, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen, int err);