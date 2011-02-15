obj-m += pcicom.o
obj-m += hi_hw_adp.o

pcicom-y := drv/data_common.o drv/proto_common.o
hi_hw_adp-y := hal/hisi_pci_adp.o

#CFLAGS += $(INCLUDE_DIR)
export
all:
	make $(KERNEL_MAKE) M=$(PWD) modules
	echo "-------------------------------"
	cp ./hi_hw_adp.ko ./pcicom.ko ../../
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean
