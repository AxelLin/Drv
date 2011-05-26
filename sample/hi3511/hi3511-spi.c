/*
 * 
 *  File:       	  	hi3511-spi.c 
 *  Description:  	Hi3511 Spi host driver .
 *
 *  Created Date: 	April 21th. 2011
 *  Last Modified:	April 21th. 2011
 *
 *  Author:       		Wu DaoGuang 
 *  Copyright (c)  DaoGuang Wu <wdgvip@gmail.com>
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
 
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>

#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
//#include <linux/platform_device.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/device.h>

#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/uaccess.h>

#include <linux/moduleparam.h>

#include <asm/uaccess.h>

#include <asm/arch/gpio.h>
#include <asm/arch/platform.h>
#include <asm/arch/yx_gpiodrv.h>
#include<asm/arch/yx_iomultplex.h>



#undef PDEBUG 
#define HI3511_SPI_DEBUG   1
#ifdef    HI3511_SPI_DEBUG
	#define PDEBUG(fmt, args...)		printk( KERN_INFO "SPI--" fmt, ##args)
#else
	#define PDEBUG(fmt, args...)    
#endif




#define SC_BASE	0x101E0000
#define SC_PERCTRL0      (IO_ADDRESS(SC_BASE)+ 0x1c)
#define SC_PERCTRL1      (IO_ADDRESS(SC_BASE)+ 0x20)


#define SC_PEREN            (IO_ADDRESS(SC_BASE)+ 0x24)
#define SC_PERDIS          ( IO_ADDRESS(SC_BASE) + 0x28)
#define SC_PERCLKEN     (IO_ADDRESS(SC_BASE) + 0x2c)


#define SSP_BASE	0x101F4000
/* SSP register definition .*/
#define SSP_CR0              (IO_ADDRESS(SSP_BASE) + 0x00)
#define SSP_CR1              (IO_ADDRESS(SSP_BASE )+ 0x04)
#define SSP_DR                ( IO_ADDRESS(SSP_BASE) + 0x08)
#define SSP_SR                ( IO_ADDRESS(SSP_BASE) + 0x0C)
#define SSP_CPSR            ( IO_ADDRESS(SSP_BASE) + 0x10)
#define SSP_IMSC            ( IO_ADDRESS(SSP_BASE )+ 0x14)
#define SSP_RIS               (IO_ADDRESS(SSP_BASE) + 0x18)
#define SSP_MIS              ( IO_ADDRESS(SSP_BASE) + 0x1C)
#define SSP_ICR              ( IO_ADDRESS(SSP_BASE) + 0x20)
#define SSP_DMACR        (  IO_ADDRESS(SSP_BASE) + 0x24)


//#define YX_GPIO_GET_SC_PERCTRL1   (*(volatile unsigned long *)(IO_ADDRESS(SC_BASE)+0x20))
//#define YX_GPIO_SET_SC_PERCTRL1(v)  do{ *(volatile unsigned long *)(IO_ADDRESS(SC_BASE)+0x20)=v;} while(0)
//#define YX_GPIO_SC_PERCTRL1(bit,value)      YX_GPIO_SET_SC_PERCTRL1 ((YX_GPIO_GET_SC_PERCTRL1 & (~(1<<(bit)))) | ((value)<<(bit)))

#define SPIReadReg(address)     		readl(address)
#define SPIWrtieRes(value, address)	writel((value),(address))
#define SPISetRegBit(bit, address)	writel((readl(address) | (1 << (bit))),(address))
#define SPIClrRegBit(bit, address)		writel((readl(address) & (~(1 << (bit)))), (address))
#define SPISetBitValue(bit, value, address)  writel((readl(address) & (~(1 << (bit)))) | ((value) << (bit)), (address))




 void PrintRegs(void)
{
	
	PDEBUG("SC_PERCTRL1:%02x\t\n",SPIReadReg(SC_PERCTRL1));
	PDEBUG("SC_PERCLKEN:%2x\t\n",SPIReadReg(SC_PERCLKEN));
	PDEBUG("SSP_CR0: %8x\t,SSP_CR1:%8x\t,SSP_CPSR:%8x\t\n",SPIReadReg(SSP_CR0),SPIReadReg(SSP_CR1),SPIReadReg(SSP_CPSR));
	PDEBUG("SSP_INTMASK:%8x\t, SSP_SR:%8x\t,SSP_DR:%8x\t\n",SPIReadReg(SSP_IMSC),SPIReadReg(SSP_SR),SPIReadReg(SSP_DR));
	
}
EXPORT_SYMBOL(PrintRegs);
void spi_reset(void )
{
	SPISetBitValue(6, 0, SC_PERCTRL0);
}

void spi_enable(void)
{
	SPISetBitValue(1, 1, SSP_CR1);
}
void spi_desable(void )
{
	SPISetBitValue(1, 0, SSP_CR1);
}
unsigned int spi_busy(void)
{
	int ret = SPIReadReg(SSP_SR);
	return (ret &(1 << 4));   //if busy ,return 1;
	
}

unsigned int spi_txfifo_full(void )
{
	int ret = SPIReadReg(SSP_SR);
	return (ret & (1 << 1));   //if not full return 1
}

unsigned int spi_rxfifo_empty(void)
{
	int ret = SPIReadReg(SSP_SR);
	return (ret & (1 << 2));    //if not empty return 1
}

void spi_chanel_select(int channal)
{
	spi_enable();
	while(spi_busy());
	spi_desable();
	if(channal > 0)
		YX_GPIO_SC_PERCTRL1(12, 1);
	else  YX_GPIO_SC_PERCTRL1(12, 0);
}

 void  SPISendByte(unsigned char val,  int channal)
{
	int value;
	spi_chanel_select(channal);
	spi_enable();
	while(!spi_txfifo_full());
	SPIWrtieRes(val, SSP_DR);


	//value = (char ) SPIReadReg(SSP_DR);
//	PDEBUG(" write and read value  is :%02x\n",value);

	//PrintRegs();
	while(spi_busy());
	spi_desable();
			
}
 EXPORT_SYMBOL(SPISendByte);
 
 unsigned char   SPIRecvByte(int channel)
{
	unsigned int  timeout = 65535;
	 char value;
	
	spi_chanel_select(channel);
	spi_enable();


	
	while((timeout  > 1) &&( !spi_rxfifo_empty()) )
		{
			timeout--;
		}
	
		
	value = (char ) SPIReadReg(SSP_DR);
	PDEBUG("the value in spi read is :%02x\n",value);
	while(spi_busy());
	PrintRegs();
	
	spi_desable();

	return value;
	
}
 EXPORT_SYMBOL(SPIRecvByte);
 
static int hi3511_spi_probe(struct platform_device  *pdev)
{
	int temp = 0;
	spi_reset();
	YX_GPIO_SC_PERCTRL1(10, 1); //set the gpio pin function to spi
	YX_GPIO_SC_PERCTRL1(0, 0);
	YX_GPIO_SC_PERCTRL1(1, 1);
// Enable the SPI clock
	SPISetBitValue(7, 1, SC_PEREN);
//Set the CR0 value   Here SSP is  not Enabled,so we can set the value .
	temp |= (0x7) | (0x0 << 4) | (0x0 << 6) | (0x3 << 8);
	SPIWrtieRes(temp, SSP_CR0);
//Set the SSP_CPSR regs value
	temp = 36;
	SPIWrtieRes(temp, SSP_CPSR);
	spi_enable();
	
	//SPISetBitValue(0, 1, SSP_CR1);
	
	PrintRegs();
	
	
return 0;
}
static int hi3511_spi_remove(struct platform_device *pdev)
{
return 0;
}

static  struct platform_driver hi3511_spi_driver  = {
	.probe    = hi3511_spi_probe,
	.remove = hi3511_spi_remove,
	
	.driver    = {
		.name   = "hi3511-spi",
		.owner = THIS_MODULE,
		},  
};

/*static struct platform_driver Hi3511_spidrv = {
	.probe		= hi3511_spi_probe,
	.remove		= hi3511_spi_remove,
	.suspend	= NULL,
	.resume		= NULL,
	.driver		= {
		.name	= "hi3511-spi",
		.owner	= THIS_MODULE,
	},
};

*/
 static int  hi3511_spi_init(void)
{
	
	platform_driver_register(&hi3511_spi_driver);
	//PrintRegs();
	return 0;
	
}

static void hi3511_spi_exit(void)
{
	platform_driver_unregister(&hi3511_spi_driver);
	PDEBUG("spi module test exit !");
}


module_init(hi3511_spi_init);
module_exit(hi3511_spi_exit);

MODULE_AUTHOR("Wu DaoGuang");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("A spi host  driver !");
MODULE_ALIAS("a simple driver ");
























 
