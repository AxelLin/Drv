/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   sd_mmc_host.h
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   registers.pdf

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the constants for host driver functions.
	Host driver implements IP specific functionality thereby
	making bus driver independent of IP details.


    REVISION HISTORY:

***************************************************************/

#ifndef SD_MMC_HOST_H
#define SD_MMC_HOST_H

//IP base address.
#ifdef Win98Testing
  //For testing without actual IP,create instance of structure.
  IPRegStruct   tpIPRegInfo;
  #define G_ptBaseAddress  &tpIPRegInfo
#else
  //User needs to define this address here,before compiling 
  //driver.
  DWORD G_ptBaseAddress ;//= map_adr;//EXC_PLD_BLOCK0_BASE;
#endif

//****Use writeb,writel,writew for versions > 2.0
//For lower versions,direct access of memory can be used.


//To use Base address directly,to access bit fields,use macro as
//WriteReg(BYTCNT,Reg,dwByteCntReg)
//Macro will give : G_dwBaseAddress->BYTCNT.Reg=dwByteCntReg;


#ifdef DEBUG_DRIVER
  //x=offset,z=value to be written G_ptBaseAddress should be defined as 0.
  #define WriteReg(x,z)      callback_writereg((int)x,z)
  //x=offset  G_ptBaseAddress should be defined as 0.
  #define ReadReg(x)          callback_readreg((int)x) 
  #define IPregarrelement(x)   ((BYTE*)G_ptBaseAddress+x) 
#else
//weimx add 'volatile' on 20070905
  #define WriteReg(x,z)          *((volatile DWORD*)((volatile BYTE*)map_adr+x))=(DWORD)z
  #define ReadReg(x)             *((volatile DWORD*)((volatile BYTE*)map_adr+x))
  #define IPregarrelement(x)   (map_adr+x) //((IPRegStruct *)(G_ptBaseAddress))->IPRegArr[x]
#endif

//Miscleeneous defines.
#define debg(str)               printk str
#define debg1(str)               


//Constants defined for bitwise access of registers.
#define start_cmd_bit        0x80000000  //bit31
#define HLE_bit              0x00001000  //bit12
#define DataTimeOut_bit      0x00000200  //bit9
#define card_width_bits      0x0000ffff  //bits 0-15
#define enable_OD_pullup_bit 0xfeffffff //bit24
#define fifo_set_bit         0x00000002  //bit1 for ORing
#define dma_set_bit          0x00000004  //bit2 for ORing
#define controller_set_bit   0x00000001  //bit0 for ORing
#define all_set_bits         0x00000006  //bit0 for ANDing

//Structures used. :-

typedef struct SD_MMC_device_st {
	int              irq;                   /* IRQ for device */
} SD_MMC_device;

SD_MMC_device SD_MMC_dev =  {
	5                   /* IRQ for device */
};

#endif
