/*
 * s3c2410-adc.c
 *
 * S3C2410 ADC 
 *  exclusive with s3c2410-ts.c
 *
 * Author: SeonKon Choi <bushi@mizi.com>
 * Date  : $Date: 2003/01/20 14:24:49 $ 
 *
 * $Revision: 1.1.2.6 $
 *
	2004-6-14 add a device by threewater<threewater@up-tech.com>

   Fri Dec 03 2002 SeonKon Choi <bushi@mizi.com>
   - initial

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
#include <linux/irq.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>

#include "s3c2410-adc.h"

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

#define DEVICE_NAME	"s3c2410-adc"
#define ADCRAW_MINOR	1

static int adcMajor = 0;

typedef struct {
	struct semaphore lock;
	wait_queue_head_t wait;
	int channel;
	int prescale;
}ADC_DEV;

static ADC_DEV adcdev;

#define START_ADC_AIN(ch, prescale) \
	do{ \
		ADCCON = PRESCALE_EN | PRSCVL(prescale) | ADC_INPUT((ch)) ; \
		ADCCON |= ADC_START; \
	}while(0)

static void adcdone_int_handler(int irq, void *dev_id, struct pt_regs *reg)
{
	wake_up(&adcdev.wait);
}

static ssize_t s3c2410_adc_write(struct file *file, const char *buffer, size_t count, loff_t * ppos)
{
	int data;

	if(count!=sizeof(data)){
		//error input data size
		DPRINTK("the size of  input data must be %d\n", sizeof(data));
		return 0;
	}

	copy_from_user(&data, buffer, count);
	adcdev.channel=ADC_WRITE_GETCH(data);
	adcdev.prescale=ADC_WRITE_GETPRE(data);

	DPRINTK("set adc channel=%d, prescale=0x%x\n", adcdev.channel, adcdev.prescale);

	return count;
}

static ssize_t s3c2410_adc_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	int ret = 0;

	if (down_interruptible(&adcdev.lock))
		return -ERESTARTSYS;

	START_ADC_AIN(adcdev.channel, adcdev.prescale);
	interruptible_sleep_on(&adcdev.wait);

	ret = ADCDAT0;
	ret &= 0x3ff;
	DPRINTK("AIN[%d] = 0x%04x, %d\n", adcdev.channel, ret, ADCCON & 0x80 ? 1:0);

	copy_to_user(buffer, (char *)&ret, sizeof(ret));

	up(&adcdev.lock);

	return sizeof(ret);
}

static int s3c2410_adc_open(struct inode *inode, struct file *filp)
{
	init_MUTEX(&adcdev.lock);
	init_waitqueue_head(&(adcdev.wait));

	adcdev.channel=0;
	adcdev.prescale=0xff;

	MOD_INC_USE_COUNT;
	DPRINTK( "adc opened\n");
	return 0;
}

static int s3c2410_adc_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	DPRINTK( "adc closed\n");
	return 0;
}


static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	open:	s3c2410_adc_open,
	read:	s3c2410_adc_read,	
	write:	s3c2410_adc_write,
	release:	s3c2410_adc_release,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_adc_dir, devfs_adcraw;
#endif

int __init s3c2410_adc_init(void)
{
	int ret;

	/* normal ADC */
	ADCTSC = 0; //XP_PST(NOP_MODE);

	ret = request_irq(IRQ_ADC_DONE, adcdone_int_handler, SA_INTERRUPT, DEVICE_NAME, NULL);
	if (ret) {
		return ret;
	}

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	adcMajor=ret;

#ifdef CONFIG_DEVFS_FS
	devfs_adc_dir = devfs_mk_dir(NULL, "adc", NULL);
	devfs_adcraw = devfs_register(devfs_adc_dir, "0raw", DEVFS_FL_DEFAULT,
				adcMajor, ADCRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_fops, NULL);
#endif
	printk (DEVICE_NAME"\tinitialized\n");

	return 0;
}

module_init(s3c2410_adc_init);

#ifdef MODULE
void __exit s3c2410_adc_exit(void)
{
#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_adcraw);
	devfs_unregister(devfs_adc_dir);
#endif
	unregister_chrdev(adcMajor, DEVICE_NAME);

	free_irq(IRQ_ADC_DONE, NULL);
}

module_exit(s3c2410_adc_exit);
MODULE_LICENSE("GPL");
#endif
