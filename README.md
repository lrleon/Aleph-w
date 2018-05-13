# Aleph-w

Aleph-w is a library of data structures and algorithms implemented in
and for C++.

It contains the most popular data structures: single and double lists,
special arrays (dynamic and of bits), many types of trees: multitrees,
binary search trees, avl, red-black, randomized, treaps, splay and
other interesting types.

Several types of hash tables are implemented: separated chaining, open
addressing with linear probing and double function hash; this last one
has a garbage collector mechanism that allows freeing deleted
entries. Dynamic linear hash tables are implemented too.

The main algorithms on graphs are implemented and with good
performance features. Network Flow (maximum and min cost), cut points,
topological sort, spanning trees, min paths, etc. The graphs are
generic in the sense that they can contain data independent of the
algorithm.

## Building

### Library

The following dependencies are mandatory (the Ubuntu packages names
are given):

1. For having `Imake` you will need `xutils-dev`
2. `GNU` scientific library: `libgsl0-dev`
3. 'GNU MP' bignum library: `libgmp-dev`
4. `GNU MPFR` library: `libmpfr-dev`

We recommend to compile Aleph-w with `clang` compiler. The Imakefiles
are configured for using it, buy you can set another compiler such as
`gcc` or `icc`.

If every dependency is correctly installed, then the following command
sequence should build the library:

    xmkmf
	make depend
	make libAleph.a
	
### Examples

A lot of examples are located in `Examples` directory. They are demos
intended to show the usage of a specific component of Aleph-w. They
are not strictly maintained but their compilation is guaranteed on
each master version. You are welcome to improve and eventually fix
errors if you find them. 

Only with debugging purposes, the examples use `clang` sanitizer. So,
you should have `clang` installed or change the compiler by editing
the Imakefile. In this case, you must remove the the
`-fsanitize=address,undefined` compiler flag.

For building all the examples, execute:

    xmkmf
	make depend
	make all
	
You could build a specific example:

    make test-name

### Test

In order to execute the tests, which are optional and they are located
in the `Tests` directory, you will need:

1. `googletest` <https://github.com/google/googletest>.
2. Ruby with the `concurrent-ruby` gem installed.
3. Version of `llvm` 6 or higher.
4. `Ruby` and the `concurrent-ruby` gem.

In order to execute the tests you must set two environment variables:

1. `ALEPHW` which must point to the directory where Aleph-w is.
2. `GOOGLETEST` which must point to the directory where
   `googletest`. The file `gtest_main.a` must be build which is
   expected to be found at the `$GOOGLETEST/googletest/make/`
   directory. 
   
The file `test-list` contains a list of each test. You can compile and
execute each test by running `all-test` script.

In order to execute a specific test, you could use the `run-test`
script. Some like as:

    ./run-test -f test-name

## Use in projects

In order to use Aleph-w you simply must set your compiler for
searching as include directory the site where Aleph-w is. Some times
you could need to link `libAleph.a`.

## LICENSE

See
[LICENSE](https://github.com/lrleon/Aleph-w/blob/master/LICENSE). Copyright
(c) 2002-2018 Leandro Rabindranath Leon and Alejandro Mujica

