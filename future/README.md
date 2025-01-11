# future
An implementation of a Simple Moving Average to predict the next term of a time series.

# Building
To build the project you need GNU Make, a C compiler and an implementation of libc available on your system.

To build using GNU Make and gcc use the following:
```sh
$ make
```

To build using a different compiler, you can use:
```sh
$ make CC=yourcompiler
```

To change the flags passed to the compiler, you need to specify a CFLAGS section, as shown below:
```sh
$ make CFLAGS="-Wall -Wextra -fno-frame-pointer" # I don't know why you would want this but anyways
```

*NOTE: to delete build artifacts, you need to use the `clean` rule as shown below:*
```sh
$ make clean
```

# Running
After building a `future` executable should be available in the top level directory.
If that is not the case, please leave an issue in the [issue tracker](https://github.com/progintro/hw2-0xJoeMama/issues).

To run the program just type the name of the executable followed by the name of an input file.
An input file is a whitespace separated list of decimal numbers.
Optionally you can provide a `--window` argument followed by an integer to specify the amount of elements to use for the simple moving average.
Default window size is 50.
```sh
$ ./future
Usage: ./future <filename> [--window N (default: 50)] 

$ cat input
1 2 3 4 56 2132.3 2034 32   42

$ ./future input
Window too large!

$ ./future --window 3 input 
702.67
```

Upon successful execution the program will show the average of the N last elements in the input file.


# The Solution
Other than file handling, the only real challenge on this exercise was to only read the input file once.
To do that, we can use a circular buffer.

## Circular Buffer
Instead of creating a dynamic array, reading all input and using the last N elements(which could waste a big amount of memory),
we use a circular buffer of size N.
We simply index it using the amount of elements currently read mod N.
Notice that this buffer will always hold the last N elements(though their order is not correct).
Since we don't case about order, that does not constitute an issue for us.

