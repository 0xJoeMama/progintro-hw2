# std.h
A header-only generic implementation of commonly used data structures in pure C.

## Usage
Using this library comes down to just including it as a submodule and then using `#include` with the data structures you want to use.
There are 2 inclusion paradigms used by this library:
1. Header-only implementation based inclusion: In this case you just need to include the file you wanna use and specify whether you want to implement its code or not. An example is shown below:
```c
/* file: main.c */
// the following forces implementation of the string code
#define STRING_IMPL
#include "../include/string.h"

int main(void) {
    String_t s;
    if (!s_init(s, 16)) return 1;
    s_deinit(s);
    foo();
    return 0;
}

/* file: other.c */
// since the implementation has been included in the build already, 
// we don't have to implement the string header and can just include the 
// definitions it provides
#include "../include/string.h"
int foo(void) {
    String_t s;
    if (!s_init(s, 16)) return 1;
    if (!s_push(&s, 'a')) {
        s_deinit(s);
        return 1;
    }
    s_deinit(s);
}

```
2. Generic header-only implementation for containers. In this case you need to 
create the unwrapped version of the code you want to use, for all version of a container:
```c
#include "../include/dynamic_array.h"

// the following macro call, includes the implementation of the code for an integer dynamic array
DA_IMPL(int);
// if another translation unit has already implemented the integer version of 
// dynamic arrays, you need to use the following:
// DA_DECLARE(int);

int main(void) {
    DynamicArray_t(int) da;
    if (!da_init(&da, 16)) return 1;
    
    int x = 12;
    da_push(int)(&da, x);

    da_deinit(&da, NULL);
    return 0;
}
```

## Building and running tests
You need GNU Make, a C compiler and a libc implementation to run the tests.
To build using GNU Make, gcc and glibc, run the following:
```sh
$ make
```

To specify a custom compiler run:
```sh
$ make CC=yourcompiler
```

For custom compilation flags, run:
```sh
$ make CC=yourcompiler CFLAGS=-Os
```

To remove build artifacts, you can use the `clean` target as follows:
```sh
$ make clean
```

