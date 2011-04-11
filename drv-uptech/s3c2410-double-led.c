/*
 * s3c2410-double-led.c
 *
 * genric routine for S3C2410 double led no io
 *
 * Based on s3c2410_gpio_button.c
 *
 * Author: threewater<threewater@up-tech.com>
 * Date  : $Date: 2004/06/19 15:18:00 $
 *
 * $Revision: 1.1.2.12 $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive
 * for more details.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/hardware.h>

#include <asm/delay.h>
#include <asm/uaccess.h>

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK( x... )	printk("s3c2410-led: " ##x)
#else
#define DPRINTK( x... )
#endif

#define LEDGreen		GPIO_G12
#define LEDOrange	GPIO_G13

static unsigned int LED[]={LEDGreen, LEDOrange};
#define NumberOfLed		(sizeof(LED)/sizeof(*LED))

#define DEVICE_NAME	"s3c2410-led"
#define DbLedRAW_MINOR		1

static int DbLedMajor = 0;

static unsigned int ledstatus=3;

static void Updateled(void)
{
	int i;
	for(i=0; i<NumberOfLed; i++){
		if(ledstatus & (1<<i))
			write_gpio_bit(LED[i], 1);
		else
			write_gpio_bit(LED[i], 0);
	}
}

static ssize_t s3c2410_DbLed_write(struct file *file, const char *buffer, size_t count, loff_t * ppos)
{
	copy_from_user(&ledstatus, buffer, sizeof(ledstatus));
	Updateled();
	
	DPRINTK("write: led=0x%x, count=%d\n", ledstatus, count);
	return sizeof(ledstatus);
}

static ssize_t s3c2410_DbLed_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	copy_to_user(buffer, &ledstatus, sizeof(ledstatus));

	DPRINTK("read: led=0x%x\n, count=%d\n", ledstatus, count);

	return sizeof(ledstatus);
}

static int s3c2410_DbLed_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	return 0;
}

static int s3c2410_DbLed_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	open:	s3c2410_DbLed_open,
	read:	s3c2410_DbLed_read,
	write:	s3c2410_DbLed_write,
	release:	s3c2410_DbLed_release,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_DbLed_dir, devfs_DbLedraw;
#endif

static int __init s3c2410_DbLed_init(void)
{
	int ret, i;

	for (i=0; i<NumberOfLed; i++)
		set_gpio_ctrl(GPIO_MODE_OUT | GPIO_PULLUP_DIS | LED[i]);

	Updateled();

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	DbLedMajor = ret;

#ifdef CONFIG_DEVFS_FS
	devfs_DbLed_dir = devfs_mk_dir(NULL, "led", NULL);
	devfs_DbLedraw = devfs_register(devfs_DbLed_dir, "0", DEVFS_FL_DEFAULT,
			DbLedMajor, DbLedRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR,
			&s3c2410_fops, NULL);
#endif

	printk(DEVICE_NAME " initialized\n");

	return 0;
}

static void __exit s3c2410_DbLed_exit(void)
{
#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_DbLedraw);
	devfs_unregister(devfs_DbLed_dir);
#endif	
	unregister_chrdev(DbLedMajor, DEVICE_NAME);
}

module_init(s3c2410_DbLed_init);
module_exit(s3c2410_DbLed_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("threewater<threewater@up-tech.com>");
MODULE_DESCRIPTION("gpio(led) driver for S3C2410");
