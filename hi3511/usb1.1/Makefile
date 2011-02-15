obj-m:=hi3511_usb_1-1.o
hi3511_usb_1-1-y :=ohci-hcd.o 

KERNEL_MAKE := -C ../../linux-2.6.14

default:
	make  $(KERNEL_MAKE) M=$(PWD) modules

clean:
	make  $(KERNEL_MAKE) M=$(PWD) clean

