#pragma once

#include <stdbool.h>
#include <sys/types.h>

#define MAX_RETRY 10

#define FRAME_SIZE(len) ((len) > 125 ? 125 : (len))
#define CHECK_BIT(byte, bit) (byte & (1 << (bit)))
#define GET_BIT(byte, bit) ((byte & (1 << (bit))) >> (bit))
#define TOGGLE_BIT(byte, bit) byte ^= (1 << (bit))
#define SET_BIT(byte, bit) byte |= (1 << (bit))
#define CLEAR_BIT(byte, bit) byte &= ~(1 << (bit))

#ifdef DEBUG
#define _DEBUG(args...) fprintf(stderr, args)
#else
#define _DEBUG(args...)
#endif

bool dlinit_send(char *hostname, unsigned short port);
bool dlinit_recv(unsigned short port);
ssize_t dlsend(unsigned char data[], size_t len, bool eof);
ssize_t dlrecv(unsigned char data[], size_t len, bool *eof);
