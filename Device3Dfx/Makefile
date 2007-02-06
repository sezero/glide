###############################################################################
# Makefile by Carlo Wood (and others)

# Determine the machine type
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)

# Setup machine dependant compiler flags
ifeq ($(ARCH), i386)
CFLAGS ?= -O2 -mcpu=pentium -fomit-frame-pointer -fno-strength-reduce \
          -falign-loops=2 -falign-jumps=2 -falign-functions=2
endif

ifeq ($(ARCH), alpha)
CFLAGS ?= -O2 -mno-fp-regs -mcpu=ev4 -ffixed-8 -Wa,-mev6 \
          -fomit-frame-pointer -fno-strict-aliasing
endif

KSRC ?= /usr/src/linux
KHEADERS ?= $(KSRC)/include
KVERS ?= $(shell ./kinfo --UTS)
MODULES_DIR = $(DESTDIR)/lib/modules/$(KVERS)

ALL_CFLAGS := -DMODULE -D__KERNEL__ \
              -I$(KHEADERS) -I$(KHEADERS)/asm/mach-default \
              $(CFLAGS)

###############################################################################
# You should never need to change anything below.

NAME := 3dfx

all: modules

# Sanity checks
sanity:
	@( \
	if [ ! -r $(KHEADERS)/linux ]; then \
		echo "Expect readable headers in $(KHEADERS)/linux"; \
		exit -1; \
	fi; \
	if [ ! -r $(KHEADERS)/linux/version.h ]; then \
		echo "Missing $(KHEADERS)/linux/version.h"; \
		echo "Configure and install the kernel first"; \
		exit -1; \
	fi; \
	if [ ! -e /proc/cpuinfo ]; then \
		echo "You need the /proc file system"; \
		echo "Reconfigure kernel and say Yes to CONFIG_PROC_FS"; \
		exit -1; \
	fi; \
	)

ifeq ($(wildcard config),config)

config: sanity

include config

clean_type = clean_$(BUILD_TYPE)
module_type = module_$(BUILD_TYPE)

else

config: sanity kinfo
	@( \
	KVER_MAJOR=`echo $(KVERS) | cut -d. -f1`; \
	KVER_MINOR=`echo $(KVERS) | cut -d. -f2`; \
	if [ $$KVER_MAJOR = 2 -a $$KVER_MINOR -ge 6 ]; then \
	  echo BUILD_TYPE = kbuild; \
	  echo MODULE_TDFX = kbuild/$(NAME).ko; \
	else \
	  echo BUILD_TYPE = legacy; \
	  echo MODULE_TDFX = $(NAME).o; \
	fi; \
	) > config
	@$(MAKE) $(MAKECMDGOALS)

endif

kinfo: kinfo.c
	$(CC) -I$(KHEADERS) -o kinfo kinfo.c

kinfo.h: kinfo
	@echo Generating kernel information header.
	@./kinfo

###############################################################################

modules: config $(module_type)

module_legacy: $(NAME).o

$(NAME).o: 3dfx_driver.c Makefile
	$(CC) $(ALL_CFLAGS) -c -o $@ 3dfx_driver.c

module_kbuild:
	$(MAKE) -C kbuild

###############################################################################

install: config install_modules
	@( \
	if [ -e $(MODULES_DIR)/modules.dep ]; then \
		indep=`grep '$(NAME)/$(MODULE_TDFX):' $(MODULES_DIR)/modules.dep`; \
		if [ -z "$$indep" ]; then \
			echo "$(MODULES_DIR)/$(NAME)/$(MODULE_TDFX):" >> $(MODULES_DIR)/modules.dep; \
			echo "" >> $(MODULES_DIR)/modules.dep; \
		fi; \
	fi; \
	if [ ! -e $(DESTDIR)/dev/.devfsd -a ! -c $(DESTDIR)/dev/$(NAME) ]; then \
		mknod $(DESTDIR)/dev/$(NAME) c 107 0; \
		chmod go+w $(DESTDIR)/dev/$(NAME); \
	fi; \
	if [ "$(RPM_INSTALL)" = "1" ]; then \
		echo "$(MODULES_DIR)/$(NAME)/$(MODULE_TDFX)"; \
	else \
		inconf=`grep 'alias char-major-107 $(NAME)' $(DESTDIR)/etc/modules.conf`; \
		if [ -z "$$inconf" ]; then \
			echo "alias char-major-107 $(NAME)" >> $(DESTDIR)/etc/modules.conf; \
		fi; \
	fi; \
	)

install_modules: modules
	mkdir -p $(MODULES_DIR)/$(NAME)
	cp $(MODULE_TDFX) $(MODULES_DIR)/$(NAME)/

###############################################################################
# This is for debugging purposes by the developers:

clean: config $(clean_type)
	rm -f kinfo kinfo.h
	rm -f config

clean_legacy:
	rm -f *.ko *.o *.mod.* .*.mod.* .*.cmd

clean_kbuild:
	$(MAKE) -C kbuild clean

tar:
	tar czf ../../SOURCES/Dev3Dfx-2.5.tar.gz 3dfx_driver.c Makefile

debug:
	$(MAKE) CFLAGS="-g -Wall -Wstrict-prototypes -DDEBUG"

.PHONY: all sanity modules modules_legacy modules_kbuild
.PHONY: install install_legacy install_kbuild
.PHONY: clean clean_legacy clean_kbuild tar debug

