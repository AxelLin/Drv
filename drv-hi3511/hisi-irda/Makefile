#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

KERNEL_MAKE := -C ../../linux-2.6.14
			
#CFLAGS += -I$(MMZ_SRC_BASE)

obj-m := hi_ir.o
hi_ir-y += hiir.o

default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

