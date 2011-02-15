export SOURCE_BASE=$(PWD)

INCLUDE_DIR := -I$(SOURCE_BASE)/include 
export INCLUDE_DIR

HIOS_MCC_TEST := hios_mcc_test
obj-m := hios_pci/
obj-m += hios_mcc/
obj-m += hios_pci_test/
obj-m += mempool/
CFLAGS += $(INCLUDE_DIR)

all:
	make $(KERNEL_MAKE) M=$(PWD) modules
	arm-hismall-linux-gcc $(INCLUDE_DIR)  $(HIOS_MCC_TEST)/mccu-hostsend.c -o $(HIOS_MCC_TEST)/send-host -Wall -g 
	arm-hismall-linux-gcc $(INCLUDE_DIR)  $(HIOS_MCC_TEST)/mccu-hostread.c -o $(HIOS_MCC_TEST)/recv-host -Wall -g 
	arm-hismall-linux-gcc $(INCLUDE_DIR)  $(HIOS_MCC_TEST)/mccu-slavesend.c -o $(HIOS_MCC_TEST)/send-slave -Wall -g 
	arm-hismall-linux-gcc $(INCLUDE_DIR)  $(HIOS_MCC_TEST)/mccu-slaveread.c -o $(HIOS_MCC_TEST)/recv-slave -Wall -g 
	arm-hismall-linux-gcc $(INCLUDE_DIR)  $(HIOS_MCC_TEST)/mccu-test.c -o $(HIOS_MCC_TEST)/userdev-test -Wall -g 
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean
	rm $(HIOS_MCC_TEST)/send-* $(HIOS_MCC_TEST)/recv-*  -rf

