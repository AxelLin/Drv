

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





void hi3511_spi_init(void );

void SPISend(unsigned char val,  int channel);

void SPIRecv(int channel);

#endif































