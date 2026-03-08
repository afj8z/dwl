.POSIX:
.SUFFIXES:

include config.mk

# flags for compiling
DWLCPPFLAGS = -I. -DWLR_USE_UNSTABLE -D_POSIX_C_SOURCE=200809L \
	-DVERSION=\"$(VERSION)\" $(XWAYLAND)
DWLDEVCFLAGS = -g -Wpedantic -Wall -Wextra -Wdeclaration-after-statement \
	-Wno-unused-parameter -Wshadow -Wunused-macros -Werror=strict-prototypes \
	-Werror=implicit -Werror=return-type -Werror=incompatible-pointer-types \
	-Wfloat-conversion

# CFLAGS / LDFLAGS
PKGS      = wlroots-0.18 wayland-server xkbcommon libinput $(XLIBS)
DWLCFLAGS = `$(PKG_CONFIG) --cflags $(PKGS)` $(WLR_INCS) $(DWLCPPFLAGS) $(DWLDEVCFLAGS) $(CFLAGS)
LDLIBS    = `$(PKG_CONFIG) --libs $(PKGS)` $(WLR_LIBS) -lm $(LIBS)

all: dwl
dwl: dwl.o util.o dwl-ipc-unstable-v2-protocol.o
	$(CC) dwl.o util.o dwl-ipc-unstable-v2-protocol.o $(DWLCFLAGS) $(LDFLAGS) $(LDLIBS) -o $@
dwl.o: dwl.c client.h config.h config.mk cursor-shape-v1-protocol.h \
	pointer-constraints-unstable-v1-protocol.h wlr-layer-shell-unstable-v1-protocol.h \
	wlr-output-power-management-unstable-v1-protocol.h \
	xdg-shell-protocol.h \
	river-control-unstable-v1-protocol.h river-control-unstable-v1-private-protocol.c river-control.h \
	dwlctl dwl-ipc-unstable-v2-protocol.h
util.o: util.c util.h
#if there is a cleaner way of doing this please inform me this looks a little ugly
dwlctl: river-control-unstable-v1-client-protocol.h river-control-unstable-v1-private-protocol.c river-control-unstable-v1-private-protocol.o dwlctl.c
	$(CC) -c -o $@.o dwlctl.c 
	$(CC) -lwayland-client -o $@ dwlctl.o river-control-unstable-v1-private-protocol.o
dwl-ipc-unstable-v2-protocol.o: dwl-ipc-unstable-v2-protocol.c dwl-ipc-unstable-v2-protocol.h

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
WAYLAND_SCANNER   = `$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner`
WAYLAND_PROTOCOLS = `$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols`

river-control-unstable-v1-client-protocol.h:
	$(WAYLAND_SCANNER) client-header \
		protocols/river-control-unstable-v1.xml  $@
river-control-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/river-control-unstable-v1.xml  $@
river-control-unstable-v1-private-protocol.c:
	$(WAYLAND_SCANNER) private-code \
		protocols/river-control-unstable-v1.xml  $@
river-control-unstable-v1-private-protocol.o:
	$(CC) -c -o $@ river-control-unstable-v1-private-protocol.c
cursor-shape-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/staging/cursor-shape/cursor-shape-v1.xml $@
pointer-constraints-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) enum-header \
		protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-output-power-management-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/wlr-output-power-management-unstable-v1.xml $@
xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@
dwl-ipc-unstable-v2-protocol.h:
	$(WAYLAND_SCANNER) server-header \
		protocols/dwl-ipc-unstable-v2.xml $@
dwl-ipc-unstable-v2-protocol.c:
	$(WAYLAND_SCANNER) private-code \
		protocols/dwl-ipc-unstable-v2.xml $@
config.h:
	cp config.def.h $@
clean:
	rm -f dwl *.o *-protocol.h *-protocol.c

dist: clean
	mkdir -p dwl-$(VERSION)
	cp -R LICENSE* Makefile CHANGELOG.md README.md client.h config.def.h \
		config.mk protocols dwl.1 dwl.c util.c util.h dwl.desktop \
		dwl-$(VERSION)
	tar -caf dwl-$(VERSION).tar.gz dwl-$(VERSION)
	rm -rf dwl-$(VERSION)

install: dwl
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/dwl
	cp -f dwl $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dwl
	cp -f dwlctl $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dwlctl
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f dwl.1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/dwl.1
	mkdir -p $(DESTDIR)$(DATADIR)/wayland-sessions
	cp -f dwl.desktop $(DESTDIR)$(DATADIR)/wayland-sessions/dwl.desktop
	chmod 644 $(DESTDIR)$(DATADIR)/wayland-sessions/dwl.desktop
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwl $(DESTDIR)$(MANDIR)/man1/dwl.1 \
		$(DESTDIR)$(DATADIR)/wayland-sessions/dwl.desktop

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CPPFLAGS) $(DWLCFLAGS) -o $@ -c $<
