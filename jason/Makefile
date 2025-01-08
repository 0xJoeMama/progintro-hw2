CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99
BINS=jason json
LDFLAGS=-lcrypto -lssl

all: $(BINS)

vpath %.c src
vpath %.c tests

jason: neurolib.o

clean:
	rm -rf *.o $(BINS)
