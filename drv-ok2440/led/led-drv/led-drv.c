#include <linux/module.h>
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

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/irq.h>
#include <asm/fiq.h>
#include <asm/mach-types.h>
#include <asm/system.h>
#include <asm/uaccess.h>

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



#define LIGHT_MAJOR	231
#define LIGHT_ON	1
#define LIGHT_OFF	0


struct light_dev
{
	struct cdev cdev;
	unsigned char value;
};

struct light_dev *light_devp;
struct class *my_class;

int    light_major = LIGHT_MAJOR;

MODULE_AUTHOR("Wu DaoGung");
MODULE_LICENSE("Dual BSD/GPL");


void light_gpio_init(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPF(5),S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_setpin(S3C2410_GPF(5),0);
}
void light_on(void)
{
	s3c2410_gpio_setpin(S3C2410_GPF(5),0);
}
void light_off(void)
{
	s3c2410_gpio_setpin(S3C2410_GPF(5),1);
	
}

int light_open(struct inode *inode, struct file *filp)
{
	struct light_dev *dev;
	dev = container_of(inode->i_cdev, struct light_dev,cdev);
	filp->private_data = dev;
	return 0;
}

int light_release(struct inode *inode,struct file *filp)
{
	return 0;
}

ssize_t light_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)

{
	struct light_dev *dev = filp->private_data;
	
	if (copy_to_user(buf,&(dev->value),1))
	{
		return - EFAULT;
	}
	return 1;
}

ssize_t light_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	struct light_dev *dev = filp->private_data;
	if (copy_from_user(&(dev->value),buf ,1))
	{
		return - EFAULT;
	}
	if (dev->value == 1)
	{
		light_on();
	}
	else
	{
		light_off();
	}
	return 1;
}

int light_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
	struct light_dev *dev = filp->private_data;
	switch(cmd)
	{
		case LIGHT_ON:
			dev->value = 1;
			light_on();
			break;
		case LIGHT_OFF:
			dev->value = 0;
			light_off();
			break;
		default:
			return - ENOTTY;
	}
	return 0;
}


struct file_operations light_fops =
{
	.owner = THIS_MODULE,
	.read  = light_read,
	.write = light_write,
	.ioctl = light_ioctl,
	.open  = light_open,
	.release = light_release,

};

static void light_setup_cdev(struct light_dev *dev, int index)
{
	int err, devno = MKDEV(light_major, index);
	cdev_init(&dev->cdev,&light_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &light_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Eorror %d adding LED%d", err, index);
	
}

int light_init(void)
{
	int result;
	dev_t dev = MKDEV(light_major,0);
	if (light_major)
		result = register_chrdev_region(dev, 1, "led");
	else 
	{
		result = alloc_chrdev_region(&dev, 0, 1, "led");
		light_major = MAJOR(dev);
	}
	if (result < 0)
		return result;
	
	light_devp = kmalloc(sizeof(struct light_dev), GFP_KERNEL);
	if (!light_devp)
	{
		result = - ENOMEM;
		goto fail_malloc;
	}
	memset(light_devp, 0, sizeof(struct light_dev));
	light_setup_cdev(light_devp, 0);
	
	my_class = class_create(THIS_MODULE,"led_class");
	if (IS_ERR(my_class))
	  {
		printk("Failed in creating class!");
		return -1;
	  }
	device_create(my_class,NULL,MKDEV(light_major,0),"led","gpio_led%d",0);

	light_gpio_init();
	return 0;
	
	fail_malloc:
		unregister_chrdev_region(dev, 1);
	return result;

}

void light_cleanup(void)
{
	cdev_del(&light_devp->cdev);

	device_destroy(my_class,MKDEV(light_major, 0));
	class_destroy(my_class);

	kfree(light_devp);
	unregister_chrdev_region(MKDEV(light_major, 0), 1);
	
}
module_init(light_init);
module_exit(light_cleanup);
















