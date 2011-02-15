
#KERNEL_MAKE := -C /home/shared/tmp/SDK_Hi3511v100_OSDrv_V1.0.0.1-20080129/pub/kbuild/
KERNEL_MAKE := -C ../../../pub/kbuild-LESS_REL

ifeq ($(KERNELRELEASE),)
	export SOURCE_BASE=$(PWD)
else
	INCLUDE_DIR := -I$(SOURCE_BASE)/include \
		-I$(SOURCE_BASE)/vdd/include \
		-I$(SOURCE_BASE)/drv/include \
		-I$(SOURCE_BASE)/drv/hal/include \
		-I$(SOURCE_BASE)/hi_pcicom_v2/include 
endif

ifeq ($(HI_PCI_TARGET),slave)
	obj-m := pci_proto_slave.o
	obj-m += pci_hwhal_slave.o
	obj-m += pci_vcom_slave.o
	obj-m += pci_vether_slave.o
	obj-m += pci_preview_slave.o
	obj-m += hi3511_pci_trans.o
	hi3511_pci_trans-y := pcit/pci_trans.o

	pci_proto_slave-y := drv/proto/proto.o
	pci_proto_slave-y += drv/proto/datacomm.o
#	pci_proto_slave-y += drv/proto/hi3511_slave_init.o

	pci_hwhal_slave-y := drv/hal/hi3511_dev.o

	pci_vcom_slave-y := vdd/vcom/vcom.o vdd/common/common.o

	pci_vether_slave-y := vdd/vether/vether.o vdd/common/common.o
	
	pci_preview_slave-y := preview/pci_direct_trans.o vdd/common/common.o
else
	CFLAGS	+= -D CONFIG_PCI_HOST

	obj-m := pci_proto_host.o
	obj-m += pci_hwhal_host.o
	obj-m += pci_vcom_host.o
	obj-m += pci_vether_host.o
	obj-m += pci_slave_boot.o
	obj-m += pci_multi_boot.o
	obj-m += pci_preview_host.o
	obj-m += hi3511_pci_trans.o

	hi3511_pci_trans-y := pcit/pci_trans.o

	pci_proto_host-y := drv/proto/proto.o
	pci_proto_host-y += drv/proto/datacomm.o
	pci_proto_host-y += drv/proto/hi3511_slave_init.o

	pci_hwhal_host-y := drv/hal/hi3511_dev.o

	pci_vcom_host-y := vdd/vcom/vcom.o vdd/common/common.o

	pci_vether_host-y := vdd/vether/vether.o vdd/common/common.o

	pci_slave_boot-y := boot/hi3511_boot.o
	
	pci_multi_boot-y := boot/hi3511_boot_multichip.o
	
	pci_preview_host-y := preview/pci_direct_trans.o vdd/common/common.o
endif
	obj-m += hi_pcicom_v2/

CFLAGS += $(INCLUDE_DIR)

all:
	$(CC) -g -Wall -o device  pcit/pcit_test_device.c
	$(CC) -g -Wall -o host  pcit/pcit_test_host.c
	make $(KERNEL_MAKE) M=$(PWD) modules
clean:
	@ rm -f device host
	make $(KERNEL_MAKE) M=$(PWD) clean

