###############################################################################
# Makefile by Carlo Wood (and others)

ifeq ($(OPT_CFLAGS),)

# Determine the machine type
ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)

# Setup machine dependant compiler flags
ifeq ($(ARCH), i386)
OPT_CFLAGS = -O2 -mcpu=pentium -fomit-frame-pointer \
                 -fno-strength-reduce \
                 -falign-loops=2 -falign-jumps=2 -falign-functions=2
endif

ifeq ($(ARCH), alpha)
OPT_CFLAGS = -O2 -mno-fp-regs -mcpu=ev4 \
                 -ffixed-8 \
                 -Wa,-mev6 \
                 -fomit-frame-pointer -fno-strict-aliasing
endif

endif	# ifeq ($OPT_CFLAGS),)

CFLAGS := -DMODULE -D__KERNEL__ -I/usr/include/linux $(OPT_CFLAGS)

###############################################################################
# You should never need to change anything below.

all: sanity 3dfx.o

# Sanity checks
sanity:
	@( \
	if [ ! -r /usr/include/linux ]; then \
		echo "Expect readable headers in /usr/include/linux"; \
		exit -1; \
	fi; \
	if [ ! -r /usr/include/linux/version.h ]; then \
		echo "Missing /usr/include/linux/version.h"; \
		echo "Configure and install the kernel first"; \
		exit -1; \
	fi; \
	if [ ! -e /proc/cpuinfo ]; then \
		echo "You need the /proc file system"; \
		echo "Reconfigure kernel and say Yes to CONFIG_PROC_FS"; \
		exit -1; \
	fi; \
	)

kinfo: kinfo.c
	$(CC) -o kinfo kinfo.c

kinfo.h: kinfo
	./kinfo

###############################################################################
# kernel 2.1+

3dfx.o: kinfo.h 3dfx_driver.c Makefile
	$(CC) $(CFLAGS) -c -o $@ 3dfx_driver.c

###############################################################################

install:
	mkdir -p /lib/modules/$(shell ./kinfo --UTS)/misc
	cp 3dfx.o /lib/modules/$(shell ./kinfo --UTS)/misc/3dfx.o
	@( \
	if [ -e /lib/modules/$(shell ./kinfo --UTS)/modules.dep ]; then \
		indep=`grep 'misc/3dfx.o:' /lib/modules/$(shell ./kinfo --UTS)/modules.dep`; \
		if [ -z "$$indep" ]; then \
			echo "/lib/modules/$(shell ./kinfo --UTS)/misc/3dfx.o:" >> /lib/modules/$(shell ./kinfo --UTS)/modules.dep; \
			echo "" >> /lib/modules/$(shell ./kinfo --UTS)/modules.dep; \
		fi; \
	fi; \
	if [ ! -e /dev/.devfsd -a ! -c /dev/3dfx ]; then \
	        mknod /dev/3dfx c 107 0; \
		chmod go+w /dev/3dfx; \
	fi; \
	if [ "$(RPM_INSTALL)" = "1" ]; then \
		echo "/lib/modules/$(shell ./kinfo --UTS)/misc/3dfx.o"; \
	else \
		inconf=`grep 'alias char-major-107 3dfx' /etc/modules.conf`; \
		if [ -z "$$inconf" ]; then \
			echo "alias char-major-107 3dfx" >> /etc/modules.conf; \
		fi; \
	fi; \
	)

###############################################################################
# This is for debugging purposes by the developers:

clean:
	rm -f *.o *.s kinfo kinfo.h

3dfx.s: 3dfx_driver.c Makefile
	$(CC) $(CFLAGS) -S -c 3dfx_driver.c

tar:
	tar czf ../../SOURCES/Dev3Dfx-2.5.tar.gz 3dfx_driver.c Makefile


debug:
	make OPT_CFLAGS="-g -Wall -Wstrict-prototypes -DDEBUG"

