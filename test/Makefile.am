
SUBDIRS = testplugin

TESTS =					\
	sanity-test

check_PROGRAMS = $(TESTS)

test_runner_src = 			\
	mutter-test-runner.c		\
	mutter-test-runner.h

sanity_test_SOURCES = sanity-test.c $(test_runner_src)


AM_CFLAGS = $(MUTTER_CFLAGS)
AM_LDFLAGS = $(MUTTER_LIBS)

BUILT_SOURCES =				\
	wayland-test-protocol.c		\
	wayland-test-server-protocol.h	\
	wayland-test-client-protocol.h	\
	xdg-shell-client-protocol.h

CLEANFILES = $(BUILT_SOURCES)

%-protocol.c : protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) code < $< > $@
%-server-protocol.h : protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) server-header < $< > $@
%-client-protocol.h : protocol/%.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@

xdg-shell-client-protocol.h : ../src/wayland/protocol/xdg-shell.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@
