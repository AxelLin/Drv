
obj-m := hi3511_pciv2trans.o
hi3511_pciv2trans-y := pciv2trans.o

obj-m += hi3511_pciv.o
hi3511_pciv-y := pciv.o pciv_ext.o

CFLAGS+=$(FLAGS)

all:
	make -C $(LINUXROOT) M=$(PWD) modules
	cp *.ko ../

clean:
	make -C $(LINUXROOT) M=$(PWD) clean
