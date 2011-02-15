
DRV_HEADERS := 
CROSS_COMPILE := 

CFLAGS := $(DRV_HEADERS) 

default:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c hi_daes_common.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c hi_des_api.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c hi_aes_api.c -Wall  -O2
	$(CROSS_COMPILE)ar -rv libhi_daes.a hi_aes_api.o hi_des_api.o hi_daes_common.o
clean:
	rm -rf *.a *.o
