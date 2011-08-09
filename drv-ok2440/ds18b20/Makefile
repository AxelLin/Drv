# A simple Makefile to build modules
# if a module called module.ko is genrated from two source files the correct
# incantation would be:
# obj-m      :=module.o
# module-objs:=file1.o file2.o

obj-m := ds18b20.o

#KERNELDIR ?=/lib/modules/$(shell uname -r)/build
KERNELDIR :=/home/bamboo/work/ok2440/Kernel/linux-2.6

PWD       :=$(shell pwd)

all:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux- -C $(KERNELDIR) M=$(PWD) modules


#install:
#	install -d $(INSTALLDIR) 
#	install -c $(TARGET).O $(INSTALLDIR)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .temp_versions *.order \
	       *.markers *symvers





