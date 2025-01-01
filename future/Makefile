CC=gcc
CFLAGS=-Os -Wall -Wextra -Werror -pedantic -std=c99
BINS=future

all: $(BINS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf *.o $(BINS)
