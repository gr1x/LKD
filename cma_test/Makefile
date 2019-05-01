ISDEBUG = n

ifeq ($(ISDEBUG), y)
	DEBFLAGS = -O -g -DSUPER_DEBUG #"-O" is needed to expand inlines
else
	DEBFLAGS = -O2
endif



EXTRA_CFLAGS += $(DEBFLAGS)
# ./include/linux/kernel.h:6:10: fatal error: stdarg.h: No such file or directory
KBUILD_CFLAGS += -Wno-error=unused-but-set-variable -I /usr/lib/gcc/x86_64-linux-gnu/7.3.0/include

WARN    := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

GARBAGE = *.img *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers *.unsigned 

ifneq ($(KERNELRELEASE),)
	obj-m += cma_test.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
endif

	
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules


clean:
	rm -rf $(GARBAGE)
