obj-m += mempool.o

#obj-m += testmempool.o
#obj-m += testmempool_sla.o
#obj-m += speedtest.o

CFLAGS += $(INCLUDE_DIR)
default:
	make $(KERNEL_MAKE) M=$(PWD) modules 
clean: 
	make $(KERNEL_MAKE) M=$(PWD) clean 

