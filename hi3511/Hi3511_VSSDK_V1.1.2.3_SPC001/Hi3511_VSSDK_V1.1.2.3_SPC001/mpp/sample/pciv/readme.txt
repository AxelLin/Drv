  
How to run this sample:

1. Put this directory to Hi3511_VSSDK_V1.x.x.x/mpp/sample/pciv.
   Besure that the Hi3511_VSSDK_V1.x.x.x SDK has setup successfully.
   Check the following directory:
     Hi3511_VSSDK_V1.x.x.x/pub/tarball, the rootfs tar package should be OK.
     Hi3511_VSSDK_V1.x.x.x/pub/images, the kernel and rootfs files should be OK.
     Hi3511_VSSDK_V1.x.x.x/source/drv/hisi-pci/, the source code should be OK.

2. Goto ./run directory and execute ./rebuild.sh(sudo password maybe needed)
   to bulid hisi-pci and pciv source code, and gather the drivers needed. 
   Strongly recommended to read rebuild.sh script carefully. The resource file
   name or directory can be modifyed if necessary,

   The default version of those driver is full-release. But the kernel root in 
   Makefile of hisi-pci driver may be less-release version, modify the kernel 
   root to full-release if necessary.  
   
3. Prepare two Hi3511Demo board which's PCI is OK.

4. Download the u-boot and kernel image to host board. 

5. Add "pcimod=host pciclksel=16" options to the bootarg of host board.

6. Goto "pciv/run" directory and execute ./host.sh after host board start.

7. After host.sh complete successfully, the slave board will be boot success.
   In 10 seconds, goto /root directory and execute ./slave.sh on slave board.

8. Goto pciv/run/test and execute ./runhost.sh, then the command line will ready.
   Enter the command "pre 1 0 6". Then enjoy the PCI preview.

The file list as follow   
�� sample\pciv
�� Makefile          : The makefile to build PCIV KO and test program.
�� mpi_pciv.c        : The adapter for IOCTRL
�� pciv_msg.c        : The adapter for message 
�� pciv_stream.c       
�� pciv_stream.h     : Recive and send VENC stream
�� pciv_test_comm.h
�� pciv_test_host.c  : The test pragram for host board.
�� pciv_test_slave.c : The test pragram for slave board.
�� readme.txt        
�ǡ� include
���� hi_comm_pciv.h
���� mkp_pciv.h
���� mpi_pciv.h
���� pciv_msg.h
�ǡ� kernel
���� Makefile
���� pciv.c          : Build the hi3511_pciv.ko. The video frame is got from VI, 
����                   and scaled by DSU, then send to the memory of host board
����                   through PCI.
���� pciv.h
���� pciv2trans.c    : Build the hi3511_pciv2trans.ko.
����                   It just an adapter layer of PCI driver.
����                   
���� pciv_ext.c
���� run               : The bin files can run on Hi3511Demo board.
  �� host.sh          : The script to boot the slave board and load the drivers.
  �� rebuild.sh       : The script to gather the kernel image, driver files
  ��                    and make the rootfs image needed by slave.  
  ��                    This script can tell you how to compile PCI and PCIV driver,
  ��                    how to make initrd images for slave, and where to gather
  ��                    the meadia driver files. But you should be sure that the
  ��                    version of those driver is consistent. If you need to 
  ��                    rebuild the hisi-pci driver or pciv, check the Makefile 
  ��                    to be sure the kernel root is consistent.
  ��                    
  �� slave.sh         : The script to load drivers on slave board.
  �ǡ� boot            : Image files for slave board.
  ���� boot.sh        : The script to boot the slave board.
  ���� mkimg.rootfs   : The script to make the initrd image file needed by slave
  ���� u-boot.bin     : The u-boot for slave board. You should replace this file
  ��                    with your own u-boot. This file has some difference with
  ��                    the u-boot file of host board.
  �ǡ� meadia_driver   : The meadia driver files gathered from mpp. 
  �ǡ� pci_driver      : The PCI driver files gathered from hisi-pci.
  ��                    See Hi3511_VSSDK_V1.x.x.x/source/drv/hisi-pci/.
  ���� test            : 
    �� runhost.sh     : The script to run host pragram
    �� runslave.sh    : The script to run slave pragram

