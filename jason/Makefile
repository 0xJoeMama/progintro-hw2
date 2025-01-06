CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99
BINS=jason
LDFLAGS=-lcrypto -lssl

all: $(BINS)

vpath %.c src

jason: neurolib.o

clean:
	rm -rf *.o $(BINS)
