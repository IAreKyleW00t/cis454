CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g -I.
DEPS =
OBJ = main.c
NAME = proj1

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

run:
	./$(NAME)

.PHONY: clean
clean:
	rm -f *.o *~ $(NAME)
