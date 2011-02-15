
obj-m := hiether.o

hiether-y := mdio.o mii-drv.o mac.o ctrl.o glb.o net-drv.o mm.o sys.o

KERNEL_MAKE := -C ../../../pub/kbuild-LESS_REL

all:
	make $(KERNEL_MAKE) M=$(PWD) modules
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean

