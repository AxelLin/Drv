
obj-m := hiusb.o

hiusb-y := hiusb-hcd.o ctrl.o sys.o

KERNEL_MAKE := -C ../../linux-2.6.14

all:
	make $(KERNEL_MAKE) M=$(PWD) modules
	rm -rf ./.*.cmd ./.tmp_versions
	rm -rf *.o
	rm -rf *.mod.c
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean
	rm -rf ./.*.cmd
	rm -rf *.o
	rm -rf *.mod.c

