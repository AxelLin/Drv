

/*
*	Key driver for linux-2.6.32.
*	Modified by Wu DaoGuang.
*
*/

#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/sysdev.h>
#include <linux/ioctl.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/irq.h>


#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <asm/fiq.h>
#include <asm/mach-types.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/delay.h>

#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>
#include <mach/leds-gpio.h>
#include <mach/idle.h>
#include <mach/fb.h>

#include <plat/iic.h>
#include <plat/s3c2410.h>
#include <plat/s3c2440.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/nand.h>
#include <plat/pm.h>
#include <plat/mci.h>


#define DEVICE_NAME	"irq-key"

static int  key_major = 201;
#define MAX_KEY_BUF	16

#define KEY_NUM			2
#define KEY_STATUS_UP		0
#define KEY_STATUS_DOWNX	1
#define KEY_STATUS_DOWN		2

#define KEY_TIMER_DELAY		(HZ/10)
#define KEY_TIMER_DELAY1	(HZ/10)


typedef struct 
{
	struct cdev cdev;
	wait_queue_head_t wq;
	struct timer_list key_timer;
	
}KEY_DEV;

KEY_DEV keydev;

static int s3c2410_key_open(struct inode *inode, struct file *filp)
{
	
	printk("open success !");
	return 0;
}

static int s3c2410_key_release(struct inode *inode, struct file *filp)
{
	printk("key-irq released");
	return 0;
}


static ssize_t s3c2410_key_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
{
	return 0;
}



static struct file_operations key_fops =
{
	.owner = THIS_MODULE,
	.open  = s3c2410_key_open,
	.release = s3c2410_key_release,
	.read  = s3c2410_key_read,
};


static void key_setup_cdev(KEY_DEV *dev, int minor)
{
	int err, devno = MKDEV(key_major, minor);
	cdev_init(&dev->cdev, &key_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops   = &key_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk("Error adding key driver !");
	
}

static void init_gpio(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPF(0),S3C2410_GPF0_EINT0);
	s3c2410_gpio_cfgpin(S3C2410_GPF(2),S3C2410_GPF2_EINT2);
	
}

static  irqreturn_t  s3c2410_eint_key(int irq, void * dev_id)
{

	disable_irq_nosync(IRQ_EINT0);
	printk("in irq !");
	mod_timer(&keydev.key_timer,jiffies + KEY_TIMER_DELAY1);	
	//enable_irq(IRQ_EINT0);
	return IRQ_RETVAL(IRQ_HANDLED);
}


static int request_irqs(void)
{
	int i;
	
		set_irq_type(IRQ_EINT0, IRQ_TYPE_EDGE_FALLING);
	//	set_irq_type(k->irq_no, IRQT_FALLING);
		if (request_irq(IRQ_EINT0, s3c2410_eint_key, IRQF_DISABLED, DEVICE_NAME,NULL))
		{
		printk("request isq failed !\n");	
		return -1;
		}	
		printk("request irq success !\n");	

	return 0;
}

static void free_irqs(void)
{	
		free_irq(IRQ_EINT0, s3c2410_eint_key);	
}

static void key_timer_handler(unsigned long data)
{
	int key = data;
    		
	printk("keydown");
	
	//mod_timer(&keydev.key_timer,jiffies + KEY_TIMER_DELAY1);	
			
	enable_irq(IRQ_EINT0);

}

static int __init s3c2410_key_init(void)
{
	int i, result;
	dev_t dev = MKDEV(key_major, 0);
	
	if(key_major)
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	else {
		result = alloc_chrdev_region(&dev, 0, 1,DEVICE_NAME);
		key_major = MAJOR(dev);
	}

	if (result < 0) {
		printk("key_device: unable to get major %d\n",key_major);
		return result;
	}

	key_setup_cdev(&keydev, 0);
	
	init_gpio();
	setup_timer(&keydev.key_timer,key_timer_handler, i);
	//mod_timer(&keydev.key_timer,jiffies + KEY_TIMER_DELAY1);	
	request_irqs();
	
	return 0;
}

static void __exit s3c2410_key_exit(void)
{
//	int i;
	free_irqs();
	del_timer(&keydev.key_timer);
	cdev_del(&keydev.cdev);
	unregister_chrdev_region(MKDEV(key_major, 0), 1);
}




module_init(s3c2410_key_init);
module_exit(s3c2410_key_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wu DaoGuang");
MODULE_DESCRIPTION("The usefull key deviece driver");









