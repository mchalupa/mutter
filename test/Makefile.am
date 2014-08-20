
SUBDIRS = testplugin

inst_tests =				\
	sanity-test			\
	plugin-test			\
	event-test			\
	button-test			\
	bad-buffer-test			\
	client-test

noinst_tests =				\
	display-test

TESTS = $(noinst_tests) $(inst_tests)

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
button_test_SOURCES = button-test.c $(test_runner_src)
bad_buffer_test_SOURCES = bad-buffer-test.c $(test_runner_src)
client_test_SOURCES = client-test.c $(test_runner_src)

display_test_SOURCES = display-test.c $(test_runner_src)
display_test_LDADD = ../src/.libs/libmutter.la
display_test_LDFLAGS = -no-install
display_test_CPPFLAGS =			\
	-I$(top_builddir)/src		\
	-I$(top_builddir)/src/meta	\
	-I$(top_builddir)/src/core	\
	-I$(top_builddir)/src/compositor


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

if ENABLE_INSTALLED_TESTS
insttestdir = $(libexecdir)/installed-tests/$(PACKAGE)
insttest_PROGRAMS = $(inst_tests)

# use this directory for building config files
$(shell mkdir -p '.installed-tests')

# take all inst_tests and transform the name from test_foo to test_foo.test
# which is name of config file for installed tests
insttest_DATA = $(foreach file, $(inst_tests), .installed-tests/$(file).test)

.installed-tests/%.test: % Makefile
	$(AM_V_GEN) (echo "[Test]" > $@;\
	echo "Exec=env gtester -k --verbose $(insttestdir)/$<" >> $@; \
	echo "Type=session-exclusive" >> $@)

BUILT_SOURCES += $(insttest_DATA)

endif
