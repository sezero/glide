KSRC = /usr/src/linux

obj-m += ../3dfx_driver.o

modules:
	$(MAKE) -C $(KSRC) SUBDIRS=$(shell pwd) modules

clean:
	cd ..; rm -f *.ko *.o *.mod.* .*.mod.* .*.cmd

