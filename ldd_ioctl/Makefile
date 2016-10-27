ifneq ($(KERNELPATH),)
	KDIR	:= $(KERNELPATH)
else
	# Default kernel path
	KDIR	:= /lib/modules/$(shell uname -r)/build
endif

PWD	:= $(shell pwd)

obj-m += gchar.o

all:
	make -C $(KDIR)  M=$(PWD) modules
clean:
	make -C $(KDIR)  M=$(PWD) clean
	rm -rf user_io *.o
user_io: user_io.c
	gcc $< -o $@
