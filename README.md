Advent of Code 2020
===================

This repository details my solutions to [Advent of Code
2020](https://adventofcode.com/2020). Solutions are written in pure
POSIX-compliant C99 as the [Suckless](http://suckless.org/) cult recommends.
Coding style is mostly based on Suckless, but some freedoms were taken.

Building
--------

The `Makefile` takes care of everything.
```bash
make
```

Running
-------

All subprograms assume your puzzle input is given through standard input.

**If you have a file (assuming it's saved as `input`), run this command:**
```bash
./advent N < input
```
where `N` is the number of the day.

**If you have a one-liner, run `./advent N`, enter your input and press
Enter.**

If the program gives you a bad input format error, check:
* that the puzzle input corresponds to the day you chose in the parameter;
* that when you built the program, the input-dependent constants are compatible
with your input (see debugging section).

Debugging
---------

Each file contains all the code required for a given day with the exception of
`advent.c` which serves as a hub. That way, each day holds in a near standalone
translation unit and may have its own global variable.

In order to keep the code relatively simple, the programs were written assuming
my puzzle input format, but there's no guarantee yours will be the same. To
make these changes easy to apply, preprocessor constants were defined, and most
tweaking should be as simple as changing those constants and some integer
types. If for example your puzzle input for day 3 is not 31 characters wide,
you may change the macro `PATTERN_WIDTH`. Then, keep an eye on data types to
make sure your puzzle input can be sensically stored (for day 3, lines are
stored in 32-bit integers).

All subprograms were confirmed leak-free with valgrind, at least for my
correctly-passed puzzle input. If you manage to find leaks or any other error
detected by valgrind, feel free to send me how to reproduce it.

It has been observed that warnings pop up when lowering the optimization
levels. This is due to the compiler not being able to figure out some safety
properties of the code anymore. The file `combat.c` compiled in GCC with `-Og`
is such an example.

License
-------

![](http://www.wtfpl.net/wp-content/uploads/2012/12/wtfpl-badge-2.png)

This code is released under the [Do What the Fuck You Want to Public
License](http://www.wtfpl.net). I don't care what you do with the code or your
build of it.
