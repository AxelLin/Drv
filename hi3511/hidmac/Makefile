#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

KERNEL_MAKE := -C ../../../pub/kbuild-LESS_REL
			
#CFLAGS += -I$(MMZ_SRC_BASE)

obj-m := hidmac.o
hidmac-y += hi_dmac.o kcom.o

default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

