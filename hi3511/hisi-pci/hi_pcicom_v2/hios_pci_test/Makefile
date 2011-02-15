
obj-m := priority1.o
obj-m += normal1.o
obj-m += speed_normal.o
obj-m += multi_open.o 

priority1-y := test_priority.o
normal1-y := test_normal.o
speed_normal-y := test_normal_speed.o
multi_open-y := test_multi_open.o

all:
	make $(KERNEL_MAKE) M=$(PWD) modules
clean:
	make $(KERNEL_MAKE) M=$(PWD) clean

