include $(PWD)/../Makefile.param

FLAGS+= $(INC_PATH) -I$(PWD)/include 
FLAGS+=-I$(DRV_PATH)/hisi-pci/drv/include -I$(PWD)
FLAGS+=-I$(DRV_PATH)/hisi-pci/pcit
FLAGS+=-I$(DRV_PATH)/hisi-pci/hi_pcicom_v2/include
FLAGS+=-D$(HI_MAKE_DBG)

#pciv compile(cd $(PWD)/kernel; make) will use FLAGS and LINUXROOT
export FLAGS
export LINUXROOT

all:
	@$(CC) $(FLAGS) -lpthread -lm -o pciv_test_host  pciv_test_host.c  mpi_pciv.c pciv_msg.c pciv_stream.c $(LIBS)
	@$(CC) $(FLAGS) -lpthread -lm -o pciv_test_slave pciv_test_slave.c mpi_pciv.c pciv_msg.c pciv_stream.c $(LIBS)
	@cd $(PWD)/kernel; make
	
clean:
	@rm -f pciv_test_host pciv_test_slave 
	@cd $(PWD)/kernel; make clean
