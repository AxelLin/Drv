/*
 * s3c2410-485.c
 *
 *  S3C2410 485
 *
 * Author: wb <wbinbuaa@163.com>
 * Date  : $Date: 2005/08/09 $ 
 *
 * $Revision: 1.0.0.1 $
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */
 
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mm.h>

#include <asm/hardware.h>
#include <asm/io.h>


#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif


//#undef DEBUG
#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

#define DEVICE_NAME	"s3c2410-gprs"

int __init gprs_init(void)
{
      ULCON2 &= ~ULCON_IR;	
	  
	DPRINTK (DEVICE_NAME"\tdevice initialized\n");
	DPRINTK ("%x\n", ULCON2);
	return 0;
}

module_init(gprs_init);

#ifdef MODULE
void __exit gprs_exit(void)
{
	ULCON2 |= ULCON_IR;	
	
	DPRINTK (DEVICE_NAME"\tdevice removed\n");
}

module_exit(gprs_exit);
#endif

MODULE_LICENSE("GPL");
