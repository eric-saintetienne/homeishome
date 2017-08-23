`$HOME` is Home
===============

Some applications query the password database without checking "HOME" to
determine where to save data, and if these paths are not configurable within
the application itself, it becomes impossible to control where the data is
stored. This library can be used with dynamically linked binaries on Linux via
"LD_PRELOAD" to ensure that the value of the environment variable "HOME" is
always used as the current user's home directory even if the password database
contains something else. This is achieved by replacing GNU libc's default
implementations of _getpwent(3)_, _getpwent_r(3)_, _getpwnam(3)_,
_getpwnam_r(3)_, _getpwuid(3)_ and _getpwuid_r(3)_ with thin wrappers that
change `passwd->pw_dir` to `getenv("HOME")` when `passwd->pw_uid` is the same
as the effective UID. For example:

    # Normal behavior:
    $ HOME="EXAMPLE" getent passwd ericpruitt
    ericpruitt:x:1000:1000:Eric Pruitt,,,:/home/ericpruitt:/bin/bash

    # Modified behavior:
    $ LD_PRELOAD="$PWD/homeishome.so" HOME="EXAMPLE" getent passwd ericpruitt
    ericpruitt:x:1000:1000:Eric Pruitt,,,:EXAMPLE:/bin/bash

This project is licensed under the [2-clause BSD license][bsd-2-clause].

  [bsd-2-clause]: http://opensource.org/licenses/BSD-2-Clause

Build Targets
-------------

- **all**: Build the library. This is the default target.
- **run-tests:** Run behavior verification test suite.
- **install**: Copy the library to `$(INSTALLDIR)` which defaults to
  "/usr/local/lib". The test suite will automatically be run after the library
  is installed, but log messages for passing and ignored tests are suppressed.
- **uninstall**: Remove the library from `$(INSTALLDIR)`. This recipe will fail
  if the library is not installed.
- **sanity**: Compile the library and test suite using Clang with various
  sanitizers enabled and "-Weverything". This is intended to be use for
  development purposes. At the beginning and end of the recipe, `make clean` is
  implicitly executed.
- **clean**: Delete files generated by the build system.
