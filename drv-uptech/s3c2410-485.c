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


#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

#define MAX485_PIN_R2S		(1<<1)  //GPIO_H1:set mode receive or send
#define GPIO_MAX485_CS		(GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_H1)

#define _485_Mode_Rev()	do {GPHDAT &= ~MAX485_PIN_R2S;udelay(1000);}while(0);
#define _485_Mode_Send()	do {GPHDAT |= MAX485_PIN_R2S;udelay(1000);}while(0);
#define _485_IOCTRL_RE2DE	(0x10)			//send or receive
#define _485_RE			0				//receive
#define _485_DE			1				//send

#define DEVICE_NAME	"s3c2410-485 controller"
#define _485RAW_MINOR	1
static int _485Major = 0;

static int _485_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	DPRINTK( "s3c2410 485 device open!\n");
	return 0;
}

static int _485_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	DPRINTK( "s3c2410 485 device release!\n");
	return 0;
}


//for 485 iocontrol
static int _485_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	switch(cmd){
	case _485_IOCTRL_RE2DE:
		
		if(_485_RE == arg) 
			_485_Mode_Rev()
		else if (_485_DE == arg) 
			_485_Mode_Send()
		break;
	default:
		DPRINTK("Do not have this ioctl methods\n");
	}

	return 0;	
}


static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	open:	_485_open,	
//	read:	_485_read,	
//	write:	_485_write,
	ioctl:	_485_ioctl,
	release:	_485_release,
};


#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_485_dir, devfs_485raw;
#endif

int __init _485_init(void)
{
	int ret;

	set_gpio_ctrl(GPIO_MAX485_CS);
  	DPRINTK ("GPHCON:%x\tGPHUP:%x\tGPHDAT:%x\n",
	  	GPHCON, GPHUP, GPHDAT );

      ULCON2 &= ~ULCON_IR;	

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	_485Major=ret;

#ifdef CONFIG_DEVFS_FS
	devfs_485_dir = devfs_mk_dir(NULL, "485", NULL);
	devfs_485raw = devfs_register(devfs_485_dir, "0raw", DEVFS_FL_DEFAULT,
				_485Major, _485RAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_fops, NULL);
#endif
	DPRINTK (DEVICE_NAME"\tdevice initialized\n");

	return 0;
}

module_init(_485_init);

#ifdef MODULE
void __exit _485_exit(void)
{
	ULCON2 |= ULCON_IR;	
	
#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_485raw);
	devfs_unregister(devfs_485_dir);
#endif
	unregister_chrdev(_485Major, DEVICE_NAME);
}

module_exit(_485_exit);
#endif

MODULE_LICENSE("GPL");
