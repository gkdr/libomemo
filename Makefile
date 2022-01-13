### toolchain
#
CC ?= gcc
AR ?= ar
LIBTOOL ?= libtool
MKDIR = mkdir
MKDIR_P = mkdir -p

ARCH = $(shell $(CC) -print-multiarch)
VER_MAJ = 0
VERSION = 0.7.1

PKG_CONFIG ?= pkg-config
GLIB_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags glib-2.0)
GLIB_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs glib-2.0)

SQLITE3_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags sqlite3)
SQLITE3_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs sqlite3)

MXML_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags mxml)
MXML_LDFLAGS ?= $(shell $(PKG_CONFIG) --libs mxml)

LIBGCRYPT_CONFIG ?= libgcrypt-config
LIBGCRYPT_LDFLAGS ?= $(shell $(LIBGCRYPT_CONFIG) --libs)

SDIR = src
BDIR = build
TDIR = test
LDIR = lib

# according to https://sourceware.org/bugzilla/show_bug.cgi?id=17879
# -fno-builtin-memset is enough to make gcc not optimize it away
FILES =

PKGCFG_C=$(GLIB_CFLAGS) \
	 $(MXML_CFLAGS) \
	 $(SQLITE3_CFLAGS) \
	 $(LIBGCRYPT_CFLAGS)

PKGCFG_L=$(GLIB_LDFLAGS) \
	 $(MXML_LDFLAGS) \
	 $(SQLITE3_LDFLAGS) \
	 $(LIBGCRYPT_LDFLAGS)

CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Wstrict-overflow \
		-fno-strict-aliasing -funsigned-char \
		-fno-builtin-memset -g $(PKGCFG_C)
CPPFLAGS += -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
CFLAGS_CONVERSATIONS=$(CFLAGS) -DOMEMO_XMLNS='"eu.siacs.conversations.axolotl"' -DOMEMO_NS_SEPARATOR='"."' -DOMEMO_NS_NOVERSION
COVFLAGS = --coverage -O0 -g $(CFLAGS)
LDFLAGS += -pthread -ldl -lm $(PKGCFG_L)
TESTFLAGS = -lcmocka $(LDFLAGS)

all: $(BDIR)/libomemo-conversations.a shared

$(BDIR):
	$(MKDIR_P) $@

libomemo: $(SDIR)/libomemo.c libomemo_crypto libomemo_storage $(BDIR)
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo
	$(LIBTOOL) --mode=link $(CC) -o $(BDIR)/$@.la $(BDIR)/$@.lo $(BDIR)/$@_crypto.lo $(BDIR)/$@_storage.lo

libomemo-conversations: $(SDIR)/libomemo.c libomemo_crypto libomemo_storage $(BDIR)
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/libomemo.c $(CFLAGS_CONVERSATIONS) -o $(BDIR)/libomemo.lo $(CPPFLAGS)
	$(LIBTOOL) --mode=link $(CC) -o $(BDIR)/libomemo.la $(BDIR)/libomemo.lo $(BDIR)/libomemo_crypto.lo $(BDIR)/libomemo_storage.lo

libomemo_crypto: $(SDIR)/libomemo_crypto.c build
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo

libomemo_storage: $(SDIR)/libomemo_storage.c build
	$(LIBTOOL) --mode=compile $(CC) -c $(SDIR)/$@.c $(CFLAGS) $(CPPFLAGS) -o $(BDIR)/$@.lo

$(BDIR)/libomemo.o: $(BDIR) $(SDIR)/libomemo.c $(SDIR)/libomemo.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(SDIR)/libomemo.c -o $@

$(BDIR)/libomemo-conversations.o: $(SDIR)/libomemo.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo.c $(CFLAGS_CONVERSATIONS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libomemo_crypto.o: $(SDIR)/libomemo_crypto.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo_crypto.c $(CFLAGS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libomemo_storage.o: $(SDIR)/libomemo_storage.c $(BDIR)
	$(CC) -c $(SDIR)/libomemo_storage.c $(CFLAGS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libomemo-conversations.a: $(BDIR)/libomemo-conversations.o $(BDIR)/libomemo_crypto.o $(BDIR)/libomemo_storage.o
	$(AR) rcs $@ $^

$(BDIR)/libomemo.so: $(BDIR)
	$(CC) -shared $(SDIR)/libomemo.c -Wl,-soname,libomemo.so.$(VER_MAJ) \
		$(SDIR)/libomemo_crypto.c $(SDIR)/libomemo_storage.c \
		$(LDFLAGS) $(CPPFLAGS) -fPIC -o $@ $(CFLAGS_CONVERSATIONS)

$(BDIR)/libomemo.pc: $(BDIR)
	echo 'prefix='$(PREFIX) > $@
	echo 'exec_prefix=$${prefix}' >> $@
	echo 'libdir=$${prefix}/lib/$(ARCH)' >> $@
	echo 'includedir=$${prefix}/include' >> $@
	echo 'Name: libomemo' >> $@
	echo 'Version: ${VERSION}' >> $@
	echo 'Description: OMEMO library for C' >> $@
	echo 'Requires.private: glib-2.0' >> $@
	echo 'Cflags: -I$${includedir}/libomemo' >> $@
	echo 'Libs: -L$${libdir} -lomemo' >> $@

shared: $(BDIR)/libomemo.so $(BDIR)/libomemo.pc

install: $(BDIR)
	install -d $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/pkgconfig/
	install -m 644 $(BDIR)/libomemo-conversations.a  $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/libomemo.a
	install -m 644 $(BDIR)/libomemo.so $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/libomemo.so.$(VERSION)
	ln -s libomemo.so.$(VERSION) $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/libomemo.so.$(VER_MAJ)
	ln -s libomemo.so.$(VERSION) $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/libomemo.so
	install -m 644 $(BDIR)/libomemo.pc $(DESTDIR)/$(PREFIX)/lib/$(ARCH)/pkgconfig/
	install -d $(DESTDIR)/$(PREFIX)/include/libomemo/
	install -m 644 $(SDIR)/libomemo_crypto.h $(DESTDIR)/$(PREFIX)/include/libomemo/
	install -m 644 $(SDIR)/libomemo_storage.h $(DESTDIR)/$(PREFIX)/include/libomemo/
	install -m 644 $(SDIR)/libomemo.h $(DESTDIR)/$(PREFIX)/include/libomemo/

.PHONY = test_libomemo.o
test_libomemo: $(TDIR)/test_libomemo.c $(SDIR)/libomemo.c
	$(CC) $(COVFLAGS) $<  $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	find .  -maxdepth 1 -iname 'test*.g*' -exec mv {} $(TDIR) \;

.PHONY: test_crypto
test_crypto: $(TDIR)/test_crypto.c $(SDIR)/libomemo_crypto.c
	$(CC) $(COVFLAGS) $<  $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	find .  -maxdepth 1 -iname 'test*.g*' -exec mv {} $(TDIR) \;

.PHONY: test_storage
test_storage: $(TDIR)/test_storage.c $(SDIR)/libomemo_storage.c
	$(CC) $(COVFLAGS) $< $(FILES) -o $(TDIR)/$@.o $(TESTFLAGS)
	-$(TDIR)/$@.o
	find .  -maxdepth 1 -iname 'test*.g*' -exec mv {} $(TDIR) \;

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
