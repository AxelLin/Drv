#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

KERNEL_MAKE := -C /home/bamboo/work/hi3511/Kernel/linux-2.6.14
			


obj-m := tw_2865.o
tw_2865-y += tw2865.o

default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

