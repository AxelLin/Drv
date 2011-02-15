#!/bin/sh
insmod pci_multi_boot.ko path="./" slotuse=1 ddrsize=128
rmmod pci_multi_boot.ko

