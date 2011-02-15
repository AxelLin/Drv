
obj-m := mcc_usrdev.o
mcc_usrdev-y := hios_mcc_usrdev.o hios_mcc.o 

CFLAGS += $(INCLUDE_DIR)
all:
	make $(KERNEL_MAKE) M=$(PWD) modules
	cp ./mcc_usrdev.ko ../../
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean
	rm userdev-host userdev-slave userdev-test -rf

