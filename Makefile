# (C) LIAB ApS 2009 - 2012
SHELL = /bin/sh
PWD := $(shell pwd)

SERVERROOT = $(PWD)/serverroot
BUILDROOT = $(SERVERROOT)

SUBAPPS = rpserver
SUBLIBS = gsoap

CONFIGURE_EXT =
CONFIGURE_OPTS = --sysconfdir=/etc --bindir=/usr/bin  --libdir=/usr/lib  --includedir=/usr/include --with-rpcheader=$(PWD)/rpserver/include/rpserver.h --with-includes=$(BUILDROOT)/usr/include:/usr/include  --with-libraries=$(BUILDROOT)/usr/lib
INSTALLSTR = install-strip

#rpserver-mk:  DEVICEROOT = $(SERVERROOT)
all: sublibs subapps
# tar

sublibs: $(addsuffix -lib, $(SUBLIBS))
subapps: $(addsuffix -mk, $(SUBAPPS))

.PRECIOUS: %/configure
%/configure: %/configure.ac
	cd $* ; autoreconf --install

.PRECIOUS: %/Makefile
%/Makefile: %/configure
	cd $* ; ./configure $(CONFIGURE_ARM_OPTS) $(CONFIGURE_OPTS) $(CONFIGURE_EXTRA)

%-mk: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -C  $* DESTDIR=$(BUILDROOT) $(INSTALLSTR)

%-lib: %/Makefile
	mkdir -p $(BUILDROOT)
	$(MAKE) -C  $* DESTDIR=$(BUILDROOT) $(INSTALLSTR)

%-install:
	mkdir -p $(DEVICEROOT)
	$(MAKE) -C  $* DESTDIR=$(DEVICEROOT) install

%-build:
	mkdir -p $(DEVICEROOT)
	$(MAKE) -C  $* 

%-clean:
	$(MAKE) -C  $* clean

%-distclean:
	$(MAKE) -C  $* clean
	rm $*/Makefile

clean: $(addsuffix -clean, $(SUBAPPS))
	list='$(ALLDIRS)'; for subdir in $$list; do \
	$(MAKE) -C $$subdir clean ; done

distclean: $(addsuffix -distclean, $(SUBAPPS))
	-rm -r $(BUILDROOT)
	-rm -fr gsoap
	-rm $(COMPONENTNAME).tar.gz
	-find -name config.guess -exec rm {} \;
	-find -name config.log -exec rm {} \;
	-find -name config.status -exec rm {} \;
	-find -name config.sub -exec rm {} \;
	-find -name '*~' -exec rm {} \;
	-find -name '#*#' -exec rm {} \;
	-find -name aclocal.m4 -exec rm {} \;
	-find -name autom4te.cache -exec rm -fr {} \; -a -prune
	-find -name .deps -exec rm -fr {} \; -a -prune
	-find -name depcomp -exec rm {} \;
	-find -name libtool -exec rm {} \;
	-find -name ltmain.sh -exec rm {} \;
	-find -name missing -exec rm {} \;

#gSOAP Special Start
gsoap_2.7.16.zip:
	wget http://downloads.sourceforge.net/project/gsoap2/gSOAP/gSOAP%202.7.16%20stable/gsoap_2.7.16.zip
gsoap: gsoap_2.7.16.zip
	unzip gsoap_2.7.16.zip
	mv gsoap-2.7 gsoap
	rm gsoap/configure
#	cd gsoap; patch -p1 < ../gsoap-patch 
gsoap/Makefile: CONFIGURE_OPTS = --bindir=/usr/remove --libdir=/usr/lib --includedir=/usr/include --datadir=/usr/remove
#LDFLAGS=-L$(PWD)/$(LIBBASE)/lib CPPFLAGS="-I$(PWD)/$(LIBBASE)/include"
#gsoap/Makefile: CONFIGURE_ARM_OPTS = --host=$(CROSS_COMPILE)
gsoap/configure.ac: gsoap
gsoap-lib: INSTALLSTR = install
gsoap-lib: PARALLEL = 1
#gSOAP Special End
