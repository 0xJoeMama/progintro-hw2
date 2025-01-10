CC=gcc
CFLAGS=-O3 -Wall -Werror -Wextra -pedantic -std=c99 -g3
BINS=string_tests string_slice_tests hash_map_tests dynamic_array_tests bitset_tests allocator_tests
ARTIFACTS=$(BINS) compile_commands.json *.o .cache/

all: $(BINS) gitignore

hash_map_tests: otherfile.o

%.o: tests/%.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm -rf $(ARTIFACTS)

gitignore:
	echo $(ARTIFACTS) | tr ' ' '\n' > .gitignore
