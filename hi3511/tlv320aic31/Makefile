#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

KERNEL_MAKE := -C ../../../pub/kbuild-LESS_REL
			


obj-m := tlv_320aic31.o
tlv_320aic31-y += tlv320aic31.o

default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

