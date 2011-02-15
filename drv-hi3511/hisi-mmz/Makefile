ifeq ($(KERNELRELEASE),)
export MMZ_SRC_BASE=$(PWD)
endif

CROSS_COMPILE := arm-hismall-linux-

KERNEL_MAKE := -C ../../linux-2.6.14
			
CFLAGS += -I$(MMZ_SRC_BASE)

obj-m := mmz.o

mmz-y += media-mem.o kcom.o mmz-userdev.o arm926-cacheflush.o

all: 
	make $(KERNEL_MAKE) M=$(PWD) modules 
#	$(CROSS_COMPILE)gcc mmzu-test.c -o userdev-test -Wall -g

clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 
	rm userdev-test -f

