# future
An implementation of the Held-Karp algorithm to solve the Travelling Salesman Problem in C.

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
After building a `jabbamaps` executable should be available in the top level directory.
If that is not the case, please leave an issue in the [issue tracker](https://github.com/progintro/hw2-0xJoeMama/issues).

To run the program just type the name of the executable followed by the name of an input file.
The input file is a file, where every line is of the form 'A-B: d', where A and B are city names and d is the distance between those cities.
```sh
$ ./jabbamaps
Usage: ./jabbamaps <filename>
```

Upon successful execution the program will show the optimal path to follow to visit all cities exactly once as well as the total distance that will be travelled.


# The Solution
The solution to this problem relies heavily on my [std.h](https://github.com/0xJoeMama/std.h/) header-only library for dynamic memory allocation and data-structures.

## Input File
We begin by mapping the whole input file into memory and creating a string slice out of it.
The we start iterating line by line and creating subslices for every line that point to the first city and the second city.
We then use a linear search to find whether or not we have already visited any of those cities. If not, we add the city to the array of cities.
Finally we copy the distance number into a local buffer, which can be null terminated for safety and passed into the strtol function.
The size of the buffer is 12 because the maximum length of a 32-bit integer is 10 digits. One extra for safety and one more for the null-terminator.

Using the indices of the cities and the distance between them, we construct an array of CityEntry\_t instances, which we will then use to populate the adjacency matrix.

## Memo\_t
The `Memo_t` struct is easily the heaviest part of this whole solution. It is a cache of `int64_t`s of size $n * 2^n$ where $n$ is the number of cities. It contains the distance travelled for all possible combinations of cities, given the last traveled city.
Notice that we **need** to use 64-bit integers. That is because since our distances can be at most $2^31$ and we can have at most $64$ cities, therefore giving us a maximum possible distance travelled equal to $2^31 * 64 = 2^31 * 2^6 = 2^37 > 2^32$.
We use an heap array of heap arrays to achieve this.

## The Held-Karp algorithm
The Held-Karp algorithm is basically a dynamic-programming optimization of the typical brute-force algorithm one would use.

Typically, for brute-force we would use a recursive DFS algortih that tries to find the shortest of all permutations of the nodes of our graph.
However, this leads to computing some subpaths, multiple times. As it usually happens, this is the perfect place to insert memoization.
What do we cache? Well, the shortest distance to travel a subset, given the node we want to visit last.

We could do the above recursively(top-down), however switching to an iterative(bottom-up) approach saves us massively on time.
Therefore, we are lead to the implementation of the following pseudocode which can also be traced back to [here](https://web.archive.org/web/20150208031521/http://www.cs.upc.edu/~mjserna/docencia/algofib/P07/dynprog.pdf).
```txt
function algorithm TSP (G, n) is
    for k := 2 to n do
        g({k}, k) := d(1, k)
    end for

    for s := 2 to n−1 do
        for all S ⊆ {2, ..., n}, |S| = s do
            for all k ∈ S do
                g(S, k) := min m≠k,m∈S [g(S\{k}, m) + d(m, k)]
            end for
        end for
    end for

    opt := mink≠1 [g({2, 3, ..., n}, k) + d(k, 1)]
    return (opt)
end function
```

## Generating subsets of k elements
Basic counting principles indicate that there exist $\choose{n}{k}$ k-element subsets of an n-element set.
We start by counting from $0$ to $2^n - 1$. For all integers, we count the amount of ones in their binary representation and insert them into their respective buffers.

Then we can just iterate over entries of the combinations array to get all subsets of a specific cardinality.

## Tour Reconstruction
To reconstruct the tour, we use a backtracking algorithm, that basically finds successive minima of distances for smaller and smaller subsets, until it finds the empty set.
The result is the inverse of the order we need to visit the cities in. Finally, by adding up the costs of all edges we travel through, we can calculate the total cost.
