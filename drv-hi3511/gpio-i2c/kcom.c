#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/kcom.h>
#include "gpio_i2c.h"



unsigned char  kcom_gpio_vs6724_read(unsigned char devaddress,unsigned int address)
{
   return  gpio_vs6724_read(devaddress,address);
}

void  kcom_gpio_vs6724_write(unsigned char devaddress,unsigned int address,unsigned char value)
{
    gpio_vs6724_write(devaddress,address,value);
}

unsigned char kcom_gpio_i2c_read(unsigned char devaddress, unsigned char address)
{
    return gpio_i2c_read(devaddress, address);
}

unsigned char kcom_gpio_i2c_read_tw2815(unsigned char devaddress, unsigned char address)
{
    return gpio_i2c_read_tw2815(devaddress, address);
}

unsigned char kcom_gpio_sccb_read(unsigned char devaddress, unsigned char address)
{
    return gpio_sccb_read(devaddress, address);
}
	
void kcom_gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char value)
{
    gpio_i2c_write(devaddress, address, value);
}

struct kcom_gpio_i2c kcom_gpioi2c = {
	.kcom = KCOM_OBJ_INIT(UUID_KCOM_GPIO_I2C_V1_0_0_0, UUID_KCOM_GPIO_I2C_V1_0_0_0, NULL, THIS_MODULE, KCOM_TYPE_OBJECT, NULL),
    .gpio_vs6724_read = kcom_gpio_vs6724_read,
    .gpio_vs6724_write = kcom_gpio_vs6724_write,
    .gpio_i2c_read = kcom_gpio_i2c_read,
    .gpio_i2c_read_tw2815  = kcom_gpio_i2c_read_tw2815,
    .gpio_sccb_read = kcom_gpio_sccb_read,
    .gpio_i2c_write = kcom_gpio_i2c_write

};

int kcom_gpio_i2c_register(void)
{
	return kcom_register(&kcom_gpioi2c.kcom);
}

void kcom_gpio_i2c_unregister(void)
{
	kcom_unregister(&kcom_gpioi2c.kcom);
}

