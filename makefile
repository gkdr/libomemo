### toolchain
#
CC ?= gcc
AR ?= ar
LIBTOOL ?= libtool
PKG_CONFIG ?= pkg-config
LIBGCRYPT_CONFIG ?= libgcrypt-config
MKDIR = mkdir
MKDIR_P = mkdir -p


SDIR = src
BDIR = build
TDIR = test
LDIR = lib

# according to https://sourceware.org/bugzilla/show_bug.cgi?id=17879
# -fno-builtin-memset is enough to make gcc not optimize it away
FILES =

PKGCFG_C=$(shell $(PKG_CONFIG) --cflags glib-2.0 sqlite3 mxml) \
		 $(shell $(LIBGCRYPT_CONFIG) --cflags)
PKGCFG_L=$(shell $(PKG_CONFIG) --libs glib-2.0 sqlite3 mxml) \
		 $(shell $(LIBGCRYPT_CONFIG) --libs)

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Wstrict-overflow \
		-fno-strict-aliasing -funsigned-char \
		-fno-builtin-memset $(PKGCFG_C)
CPPFLAGS += -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
CFLAGS_CONVERSATIONS=$(CFLAGS) -DOMEMO_XMLNS='"eu.siacs.conversations.axolotl"' -DOMEMO_NS_SEPARATOR='"."' -DOMEMO_NS_NOVERSION
COVFLAGS = --coverage -O0 -g $(CFLAGS)
LDFLAGS += -pthread -ldl -lm $(PKGCFG_L)
TESTFLAGS = -lcmocka $(LDFLAGS)

all: $(BDIR)/libomemo-conversations.a

$(BDIR):
	$(MKDIR_P) $@

libomemo: $(SDIR)/libomemo.c libomemo_crypto libomemo_storage $(BDIR)
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo
	$(LIBTOOL) --mode=link $(CC) -o $(BDIR)/$@.la $(BDIR)/$@.lo $(BDIR)/$@_crypto.lo $(BDIR)/$@_storage.lo

libomemo-conversations: $(SDIR)/libomemo.c libomemo_crypto libomemo_storage $(BDIR)
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/libomemo.c $(CFLAGS_CONVERSATIONS) -o $(BDIR)/libomemo.lo
	$(LIBTOOL) --mode=link $(CC) -o $(BDIR)/libomemo.la $(BDIR)/libomemo.lo $(BDIR)/libomemo_crypto.lo $(BDIR)/libomemo_storage.lo

libomemo_crypto: $(SDIR)/libomemo_crypto.c build
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo

libomemo_storage: $(SDIR)/libomemo_storage.c build
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo

$(BDIR)/libomemo.o: $(BDIR) $(SDIR)/libomemo.c $(SDIR)/libomemo.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(SDIR)/libomemo.c -o $@

$(BDIR)/libomemo-conversations.o: $(SDIR)/libomemo.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo.c $(CFLAGS_CONVERSATIONS) -fPIC -o $@

$(BDIR)/libomemo_crypto.o: $(SDIR)/libomemo_crypto.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo_crypto.c $(CFLAGS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libomemo_storage.o: $(SDIR)/libomemo_storage.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo_storage.c $(CFLAGS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libomemo-conversations.a: $(BDIR)/libomemo-conversations.o $(BDIR)/libomemo_crypto.o $(BDIR)/libomemo_storage.o
	$(AR) rcs $@ $^

.PHONY = test_libomemo.o
test_libomemo: $(TDIR)/test_libomemo.c $(SDIR)/libomemo.c
	$(CC) $(COVFLAGS) $<  $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	mv *.g* $(TDIR)

.PHONY: test_crypto
test_crypto: $(TDIR)/test_crypto.c $(SDIR)/libomemo_crypto.c
	$(CC) $(COVFLAGS) $<  $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	mv *.g* $(TDIR)

.PHONY: test_storage
test_storage: $(TDIR)/test_storage.c $(SDIR)/libomemo_storage.c
	$(CC) $(COVFLAGS) $< $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	mv *.g* $(TDIR)

.PHONY: test
test : test_libomemo test_crypto test_storage

.PHONY: coverage
coverage: test
	gcovr -r . --html --html-details -o $@.html
	gcovr -r . -s
	$(MKDIR_P) $@
	mv $@.* $@

.PHONY: clean
clean:
	rm -rf build coverage
	rm -f $(TDIR)/*.sqlite $(TDIR)/*.o $(TDIR)/*.gcno $(TDIR)/*.gcda
