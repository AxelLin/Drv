
/**
  * This is a driver for xxx device .
  * File:			xxx.h
  * Description:		Install all needed software from ubuntu software center.
  * Created Date:	May 23th. 2011
  * Last Modified:	Mayl 23th. 2011
  * Author:	Wu DaoGuang  <wdgvip@gmail.com>
  * Copywrite (C)	 GPL/GPL2 
  *
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




#include <linux/i2c.h>
#include <linux/i2c-id.h>

#include "xr20m1172.h"
#include <linux/debug.h>







