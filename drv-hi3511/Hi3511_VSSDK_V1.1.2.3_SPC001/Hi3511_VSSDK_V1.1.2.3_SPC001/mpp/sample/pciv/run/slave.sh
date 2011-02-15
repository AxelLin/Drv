#!/bin/sh
ROOT_DIR=$PWD

# If you want to run the pciv sample, you should set mmz as follow
rmmod mmz
modprobe mmz mmz=anonymous,0,0xE3000000,80M:window,0,0xE2100000,15M anony=1

# Insmod all the pci drivers
cd $ROOT_DIR/pci_driver
insmod pci_hwhal_slave.ko shm_phys_addr=0xe2000000 shm_size=0x1000000
insmod hi_hw_adp.ko
insmod pcicom.ko
insmod mcc_usrdev.ko
insmod hi3511_pci_trans.ko

# Load all the media drivers
cd $ROOT_DIR/meadia_driver/
./load3511

cd $ROOT_DIR/test
./runslave.sh
