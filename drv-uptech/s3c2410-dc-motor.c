/*
 * s3c2410-dcmotor.c
 *
 * DC motor driver for UP-NETARM2410-S DA
 *
 * Author: Wang bin <wbinbuaa@163.com>
 * Date  : $Date: 2005/7/26 15:50:00 $ 
 * Description : UP-NETARM2410-S
 * $Revision: 0.1.0 $
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * History:
 *
 * 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
//#include <linux/kdev_t.h> MAJOR MKDEV
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mm.h>
//#include <linux/poll.h>
//#include <linux/spinlock.h>
//#include <linux/irq.h>
#include <asm/uaccess.h>				/* copy_from_user */
//#include <asm/sizes.h>
#include <asm/arch/S3C2410.h>
//#include <asm/hardware.h>

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif

/* debug macros */
//#undef DEBUG
#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

#define DEVICE_NAME				"s3c2410-dc-motor"
#define DCMRAW_MINOR	1		//direct current motor

#define DCM_IOCTRL_SETPWM 			(0x10)
#define DCM_TCNTB0					(16384)
#define DCM_TCFG0					(2)
static int dcmMajor = 0;


#define tout01_enable() \
	({ 	GPBCON &=~ 0xf;		\
		GPBCON |= 0xa;	 })

#define tout01_disable() \
	({ 	GPBCON &=~ 0xf;		\
		GPBCON |= 0x5; 		\
		GPBUP &=~0x3; 	})
	
/* deafault divider value=1/2		*/
/* deafault prescaler = 0;			*/
/* Timer input clock Frequency = PCLK / {prescaler value+1} / {divider value} */
#define dcm_stop_timer()	({ TCON &= ~0x1; })
#define dcm_start_timer() \
	({ TCFG0 &= ~(0x00ff0000); 						          \
	    TCFG0 |= (DCM_TCFG0); 						          \
	    TCFG1 &= ~(0xf);  						          		   \
	    TCNTB0 = DCM_TCNTB0; 	/* less than 10ms */               \
	    TCMPB0 = DCM_TCNTB0/2;				    		           \
	    TCON &=~(0xf);            	    							    \
	    TCON |= (0x2);            	  							    \
	    TCON &=~(0xf);            	    							    \
	    TCON |= (0x19); })

#define DPRINTREG() \
	({  													\
	DPRINTK("GPBCON %x\n", GPBCON);					\
	DPRINTK("TCFG0 %x\n", TCFG0); 						\
	DPRINTK("TCFG1 %x\n", TCFG1); 						\
	DPRINTK("TCNTB0 %x\n", TCNTB0); 						\
	DPRINTK("TCMPB0 %x\n", TCMPB0); 						\
	DPRINTK("TCON %x\n", TCON); 							\
	DPRINTK("GPBCON %x\n", GPBCON);					\
	DPRINTK("\n");										\
})


static int s3c2410_dcm_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	DPRINTK( "S3c2410 DC Motor device open!\n");
	tout01_enable();
	dcm_start_timer();
	return 0;
}

static int s3c2410_dcm_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	DPRINTK( "S3c2410 DC Motor device release!\n");
	tout01_disable();
	dcm_stop_timer();
	return 0;
}

static int dcm_setpwm(int v)
{
	return (TCMPB0 =DCM_TCNTB0/2 + v);
}
static int s3c2410_dcm_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd){
		
	/*********write da 0 with (*arg) ************/		
	case DCM_IOCTRL_SETPWM:
		return dcm_setpwm((int)arg);	
	}
	return 0;	
}

static struct file_operations s3c2410_dcm_fops = {
	owner:	THIS_MODULE,
	open:	s3c2410_dcm_open,
	ioctl:	s3c2410_dcm_ioctl,
	release:	s3c2410_dcm_release,
};


#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_dcm_dir, devfs_dcm0;
#endif
int __init s3c2410_dcm_init(void)
{
	int ret;
	
//	DPRINTREG();
	
	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_dcm_fops);
	if (ret < 0) {
		DPRINTK(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	dcmMajor=ret;

#ifdef CONFIG_DEVFS_FS
	devfs_dcm_dir = devfs_mk_dir(NULL, "dcm", NULL);
	devfs_dcm0 = devfs_register(devfs_dcm_dir, "0raw", DEVFS_FL_DEFAULT,
				dcmMajor, DCMRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_dcm_fops, NULL);
#endif

	DPRINTK (DEVICE_NAME"\tdevice initialized\n");
	return 0;
	
}

module_init(s3c2410_dcm_init);

#ifdef MODULE
void __exit s3c2410_dcm_exit(void)
{
#ifdef CONFIG_DEVFS_FS
	devfs_unregister(devfs_dcm0);
	devfs_unregister(devfs_dcm_dir);
#endif

	unregister_chrdev(dcmMajor, DEVICE_NAME);
}

module_exit(s3c2410_dcm_exit);
#endif

MODULE_LICENSE("GPL"); 

