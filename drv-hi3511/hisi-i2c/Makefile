#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

KERNEL_MAKE := -C ../../linux-2.6.14
			
#CFLAGS += -I$(MMZ_SRC_BASE)

obj-m := hii2c.o
hii2c-y += hi_i2c.o kcom.o

default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

