
DRV_HEADERS := 
CROSS_COMPILE := 

CFLAGS := $(DRV_HEADERS) 

default:
	$(CROSS_COMPILE)gcc $(CFLAGS) -c ../app/hi_daes_common.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c ../app/hi_des_api.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c ../app/hi_aes_api.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c des_set_key.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c hi_des.c -Wall  -O2
	$(CROSS_COMPILE)gcc $(CFLAGS) -c hi_aes.c -Wall  -O2
	$(CROSS_COMPILE)ar -rv libhi_openssl.a hi_aes_api.o hi_des_api.o hi_daes_common.o des_set_key.o hi_des.o hi_aes.o
clean:
	rm -rf *.a *.o
