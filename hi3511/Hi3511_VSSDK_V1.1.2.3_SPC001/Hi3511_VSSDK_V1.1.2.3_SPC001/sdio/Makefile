#
# Makefile for the Linux kernel device drivers.
#
# 2006-04-03, designed by huawei corp.
# written as the first level Makefile.
#

include $(PWD)/Makefile.files

KERNEL_MAKE := -C /work/hi3511/linux-2.6.14

default:
	make $(KERNEL_MAKE) M=$(PWD) modules

clean:
	make $(KERNEL_MAKE) M=$(PWD) clean

