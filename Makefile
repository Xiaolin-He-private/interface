PKGNAME = cfginterfaces
TARGET = cfginterfaces
MODULE = $(TARGET).la

# Various configurable paths (remember to edit Makefile.in, not Makefile)
prefix = /usr/local
exec_prefix = ${prefix}
datarootdir = ${prefix}/share
datadir = ${datarootdir}
bindir = ${exec_prefix}/bin
includedir = ${prefix}/include
libdir =  ${exec_prefix}/lib
mandir = ${datarootdir}/man
libtool = ./libtool
sysconfdir = ${prefix}/etc
NETOPEER_DIR = ${prefix}/etc/netopeer/

CC = gcc
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
LIBS = -lxml2 -lnetconf 
CFLAGS = -Wall -I/usr/include/libxml2  -O3 -Wno-unused-result
CPPFLAGS = 
LIBTOOL = $(libtool) --tag=CC --quiet
NETOPEER_MANAGER = /usr/local/bin/netopeer-manager

MODEL = model/ietf-interfaces.yin \
	model/ietf-ip.yin \
	model/iana-if-type.yin \
	model/ietf-interfaces-config.rng \
	model/ietf-interfaces-gdefs-config.rng \
	model/ietf-interfaces-schematron.xsl

SRCS = $(TARGET).c \
	iface_if.c

OBJDIR = .obj
LOBJS = $(SRCS:%.c=$(OBJDIR)/%.lo)

all: $(MODULE) $(TARGET)-init

$(TARGET)-init: $(SRCS) $(TARGET)-init.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(MODULE): $(LOBJS)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(CPPFLAGS) $(LIBS) -avoid-version -module -shared -export-dynamic --mode=link -o $@ $^ -rpath $(libdir)

$(OBJDIR)/%.lo: %.c
	@[ -d $$(dirname $@) ] || \
		(mkdir -p $$(dirname $@))
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) $(CPPFLAGS) -fPIC -shared -c $< -o $@

.PHONY: install
install: $(MODULE) $(TARGET)-init
	$(INSTALL) -m 775 -d $(DESTDIR)/$(libdir)
	$(LIBTOOL) --mode=install cp $(MODULE) $(DESTDIR)/$(libdir)/;
	$(INSTALL) -d $(NETOPEER_DIR)/ietf-interfaces/
	@for i in $(MODEL); do \
		$(INSTALL_DATA) -m 600 $$i $(NETOPEER_DIR)/ietf-interfaces/; \
	done
	if test -n "$(NETOPEER_MANAGER)"; then \
		if test -n "`$(NETOPEER_MANAGER) list | grep "^ietf-interfaces ("`"; then \
			$(NETOPEER_MANAGER) rm --name ietf-interfaces; \
		fi; \
		$(NETOPEER_MANAGER) add --name ietf-interfaces \
			--model $(NETOPEER_DIR)/ietf-interfaces/ietf-interfaces.yin \
			--transapi $(DESTDIR)/$(libdir)/cfginterfaces.so \
			--datastore $(NETOPEER_DIR)/ietf-interfaces/datastore.xml; \
		$(NETOPEER_MANAGER) add --name ietf-interfaces \
			--augment $(NETOPEER_DIR)/ietf-interfaces/ietf-ip.yin \
			--features ipv4-non-contiguous-netmasks ipv6-privacy-autoconf; \
		$(NETOPEER_MANAGER) add --name ietf-interfaces \
			--import $(NETOPEER_DIR)/ietf-interfaces/iana-if-type.yin; \
	fi
	./$(TARGET)-init $(NETOPEER_DIR)/ietf-interfaces/datastore.xml ipv4-non-contiguous-netmasks ipv6-privacy-autoconf

.PHONY: uninstall
uninstall:
	$(LIBTOOL) --mode=uninstall rm -rf $(DESTDIR)/$(libdir)/$(MODULE);
	rm -rf $(NETOPEER_DIR)/ietf-interfaces/
	if test -n "$(NETOPEER_MANAGER)"; then \
		if test -n "`$(NETOPEER_MANAGER) list | grep "^ietf-interfaces ("`"; then \
			$(NETOPEER_MANAGER) rm --name ietf-interfaces; \
		fi; \
	fi

.PHONY: clean
clean:
	$(LIBTOOL) --mode clean rm -f $(LOBJS)
	$(LIBTOOL) --mode clean rm -f $(MODULE)
	rm -rf $(MODULE) $(TARGET)-init $(OBJDIR)
