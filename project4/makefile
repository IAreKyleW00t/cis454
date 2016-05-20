CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g -I.
DEPS = log.h
OBJ = main.c log.c
NAME = httpd

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run:
	./$(NAME)

.PHONY: clean
clean:
	rm -f *.o *~ $(NAME) httpd.log
