
pkglibdir=@MUTTER_PLUGIN_DIR@

INCLUDES=@MUTTER_CFLAGS@ -I $(top_srcdir)/src -DMUTTER_LIBEXECDIR=\"$(libexecdir)\" -DHOST_ALIAS=\"@HOST_ALIAS@\" -DMUTTER_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" -DMUTTER_PKGDATADIR=\"$(pkgdatadir)\" -DMUTTER_DATADIR=\"$(datadir)\" -DG_LOG_DOMAIN=\"mutter\" -DSN_API_NOT_YET_FROZEN=1 -DMUTTER_MAJOR_VERSION=$(MUTTER_MAJOR_VERSION) -DMUTTER_MINOR_VERSION=$(MUTTER_MINOR_VERSION) -DMUTTER_MICRO_VERSION=$(MUTTER_MICRO_VERSION) -DMUTTER_PLUGIN_API_VERSION=$(MUTTER_PLUGIN_API_VERSION) -DMUTTER_PLUGIN_DIR=\"@MUTTER_PLUGIN_DIR@\"

mutter_wayland_test_la_CFLAGS   =	\
	-fPIC				\
	-I$(top_srcdir)/src/wayland	\
	-I$(top_srcdir)/src/core	\
	-I$(top_srcdir)/src/backends	\
	-I$(top_srcdir)/src/compositor	\
	-I$(top_srcdir)/src/meta	\
	-I$(top_srcdir)/src/ui		\
	-I$(top_srcdir)/test
mutter_wayland_test_la_SOURCES  = 	\
	wayland-test.c			\
	wayland-test-implementation.c	\
	../wayland-test-protocol.c
mutter_wayland_test_la_LDFLAGS  =	\
	-module	-avoid-version -no-undefined -lwayland-server
mutter_wayland_test_la_LIBADD   = @CLUTTER_LIBS@


pkglib_LTLIBRARIES = mutter-wayland-test.la

# post-install hook to remove the .la and .a files we are not interested in
# (There is no way to stop libtool generating static libs locally, and we
# cannot do this globally because of libmutter-private.so).
install-exec-hook:
	-rm $(DESTDIR)$(pkglibdir)/*.a
	-rm $(DESTDIR)$(pkglibdir)/*.la

# Since we removed the .la file, 'make uninstall' doesn't work properly,
# since it counts on libtool to remove the .la files, so just kill the
# .so file manually.
uninstall-local:
	-rm -f $(DESTDIR)$(pkglibdir)/mutter-wayland-test.so
