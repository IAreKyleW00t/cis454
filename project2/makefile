CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -I.
DEPS = dllayer.c crc.c dlsendto.o
NAME = proj2

all: sendf recvf

sendf: sendf.c $(DEPS) 
	$(CC) -o $@ $^ $(CFLAGS)

recvf: recvf.c $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

debug: CFLAGS += -g -DDEBUG
debug: sendf recvf

.PHONY: clean
clean:
	rm -f *~ sendf recvf
