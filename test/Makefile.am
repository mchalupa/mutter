
SUBDIRS = testplugin

TESTS =					\
	sanity-test			\
	plugin-test			\
	event-test

check_PROGRAMS = $(TESTS)

protocol_sources = 					\
	../src/xdg-shell-protocol.c			\
	../src/gtk-shell-protocol.c			\
	wayland-test-protocol.c
helper_sources =					\
	weston-test-client-helper.c			\
	weston-test-client-helper.h
test_runner_src = 			\
	mutter-test-runner.c		\
	mutter-test-runner.h		\
	$(protocol_sources)		\
	$(helper_sources)

sanity_test_SOURCES = sanity-test.c $(test_runner_src)
plugin_test_SOURCES = plugin-test.c $(test_runner_src)
event_test_SOURCES = event-test.c $(test_runner_src)


AM_CPPFLAGS = -I$(top_builddir)/src/wayland
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
