

/*
 * 
 *  File:       	  	hi3511-spi.h
 *  Description:  	Hi3511 Spi host driver .
 *
 *  Created Date: 	April 21th. 2011
 *  Last Modified:	May 12th. 2011
 *
 *  Author:       		Wu DaoGuang 
 *  Copyright (c)  DaoGuang Wu <wdgvip@gmail.com>
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */


 
#ifndef  __HI3511_SPI_H__
#define  __HI3511_SPI_H__
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




#define SC_BASE	0x101E0000
#define SC_PERCTRL0      (IO_ADDRESS(SC_BASE)+ 0x1c)
#define SC_PERCTRL1      (IO_ADDRESS(SC_BASE)+ 0x20)


#define SC_PEREN            (IO_ADDRESS(SC_BASE)+ 0x24)
#define SC_PERDIS          ( IO_ADDRESS(SC_BASE) + 0x28)


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

//#define SCTL_SSP_DEMUX     (  IO_ADDRESS(0x101e0020) )






/*

#define  ssp_writew(value, addr)    do { ((*(volatile unsigned int *)(addr)) = (value)) ;} while(0)
#define  ssp_readw(addr)           (*(volatile unsigned int *)(addr))

#define CR0_DEFAULT (Hi3511_CR0_DSS_8BIT |  Hi3511_CR0_FRF_SPI | Hi3511_CR0_SPO_HIGH | Hi3511_CR0_SPH_HIGH)
#define CR1_DEFAULT (Hi3511_CR1_MS_MASTER |  Hi3511_CR1_LBM_NORMAL & (~Hi3511_CR1_SSE_ENABLE))
#define INTMASK_DEFAULT (Hi3511_INTMASK_RORIM | Hi3511_INTMASK_RTIM | Hi3511_INTMASK_RXIM | Hi3511_INTMASK_TXIM)

#define YX_GPIO_GET_SC_PERCTRL1   (*(volatile unsigned long *)(IO_ADDRESS(REG_BASE_SCTL)+0x20))
#define YX_GPIO_SET_SC_PERCTRL1(v)  do{ *(volatile unsigned long *)(IO_ADDRESS(REG_BASE_SCTL)+0x20)=v;} while(0)
#define YX_GPIO_SC_PERCTRL1(g,b)      YX_GPIO_SET_SC_PERCTRL1 ((YX_GPIO_GET_SC_PERCTRL1 & (~(1<<(g)))) | ((b)<<(g)))
*/



void hi3511_spi_init(void );

void SPISend(unsigned char val,  int channel);

void SPIRecv(int channel);

#endif































