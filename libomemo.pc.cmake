prefix=@_OMEMO_PKGCONF_PREFIX@
exec_prefix=@_OMEMO_PKGCONF_EXEC_PREFIX@
libdir=@_OMEMO_PKGCONF_LIBDIR@
includedir=@_OMEMO_PKGCONF_INCLUDEDIR@

Name: libomemo
Version: @PROJECT_VERSION@
Description: OMEMO library for C
URL: https://github.com/gkdr/libomemo
Requires: glib-2.0
Requires.private: libgcrypt mxml sqlite3
Cflags: -I${includedir}/libomemo
Libs: -L${libdir} -lomemo
