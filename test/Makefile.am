
SUBDIRS = testplugin

BUILT_SOURCES =							\
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
