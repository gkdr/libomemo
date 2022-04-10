# libomemo 0.8.1
Implements [OMEMO](https://conversations.im/omemo/) ([XEP-0384 v0.3.0](https://xmpp.org/extensions/attic/xep-0384-0.3.0.html)) in C.

Input and output are XML strings, so it does not force you to use a certain XML lib.
While the actual protocol functions do not depend on any kind of storage, it comes with a basic implementation in SQLite.

It deals with devicelists and bundles as well as encrypting the payload, but does not handle the double ratchet sessions for encrypting the key to the payload.
However, you can use my [axc](https://github.com/gkdr/axc) lib for that and easily combine it with this one (or write all the libsignal client code yourself if that is better suited to your needs).

## Dependencies
* CMake (`cmake`)
* pkg-config (`pkg-config`) or pkgconf (`pkgconf`)
* [Mini-XML](http://www.msweet.org/projects.php?Z3) (`libmxml-dev`)
* gcrypt (`libgcrypt20-dev`)
* glib (`libglib2.0-dev`)
* SQLite (`libsqlite3-dev`)
* GNU make (`make`) or Ninja (`ninja-build`)

Optional: 
* [cmocka](https://cmocka.org/) (`libcmocka-dev`) for testing (`make test`)
* [gcovr](http://gcovr.com/) (`gcovr`) for a coverage report (`make coverage`)

## Installation
libomemo uses CMake as a build system.  It can be used with either GNU make or Ninja.  For example:

```
mkdir build
cd build

cmake -G Ninja ..  # for options see below

ninja -v all
ninja -v test  # potentially with CTEST_OUTPUT_ON_FAILURE=1 in the environment
ninja -v install
```

The following configuration options are supported:

```console
# rm -f CMakeCache.txt ; cmake -D_OMEMO_HELP=ON -LH . | grep -B1 ':.*=' | sed 's,^--$,,'
// Build shared libraries (rather than static ones)
BUILD_SHARED_LIBS:BOOL=ON

// Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ...
CMAKE_BUILD_TYPE:STRING=

// Install path prefix, prepended onto install directories.
CMAKE_INSTALL_PREFIX:PATH=/usr/local

// Install build artifacts
OMEMO_INSTALL:BOOL=ON

// Build test suite (depends on cmocka)
OMEMO_WITH_TESTS:BOOL=ON
```

They can be passed to CMake as `-D<key>=<value>`, e.g. `-DBUILD_SHARED_LIBS=OFF`.

## Usage
Basically, there are three data types: messages, devicelists, and bundles.
You can import received the received XML data to work with it, or create them empty. When done with them, they can be exported back to XML for displaying or sending.

The test cases should demonstrate the general usage.

Many errors have their own specific error code. This is only for debugging purposes and they are not intended to be stable.
Some error cases which do not have their own error code print a message with more details to `stderr` if the environment variable `LIBOMEMO_DEBUG` is set to any value.
Additionally, `mxml` also prints some errors to `stderr`, which can help with debugging.


If a different namespace than the one specified in the XEP is to be used, you can use specify it at compile time. See the makefile for an example.
