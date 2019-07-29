ddPKGNAME = cfginterfaces
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
	model/802.1/Qcw/ieee802-dot1q-preemption.yin \
	model/802.1/Qcw/ieee802-dot1q-sched.yin \
	model/ieee802-dot1q-types.yin \
	model/ietf-interfaces-config.rng \
	model/ietf-interfaces-gdefs-config.rng \
	model/ietf-interfaces-schematron.xsl

SRCS = $(TARGET).c

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

.PHONY: validate
validate:
	lnctool --model model/ietf-interfaces@2014-05-08.yang \
		--augment-model model/802.1/Qcw/ieee802-dot1q-preemption.yang \
		--augment-model model/802.1/Qcw/ieee802-dot1q-sched.yang \
		--augment-model model/ieee802-dot1q-types.yang \
		--search-path model/ \
		validation

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
			--augment $(NETOPEER_DIR)/ietf-interfaces/ieee802-dot1q-preemption.yin \
			--features frame-preemption; \
		$(NETOPEER_MANAGER) add --name ietf-interfaces \
			--augment $(NETOPEER_DIR)/ietf-interfaces/ieee802-dot1q-sched.yin \
			--features scheduled-traffic; \
		$(NETOPEER_MANAGER) add --name ietf-interfaces \
			--import $(NETOPEER_DIR)/ietf-interfaces/ieee802-dot1q-types.yin; \
	fi
	./$(TARGET)-init $(NETOPEER_DIR)/ietf-interfaces/datastore.xml

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
