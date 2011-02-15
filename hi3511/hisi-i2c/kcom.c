#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
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
#include "hi_i2c.h"

unsigned char kcom_hi_i2c_read(unsigned char devaddress,unsigned char address)
{
    return hi_i2c_read(devaddress, address);
}

int kcom_hi_i2c_write(unsigned char devaddress,unsigned char address,unsigned char value)
{
    return hi_i2c_write(devaddress, address, value);
}

int kcom_hi_i2c_muti_read(unsigned char devaddress, unsigned int regaddress, int reg_addr_count,
	                   unsigned char* data, unsigned long  count)
{
    return hi_i2c_muti_read(devaddress, regaddress, reg_addr_count, data, count);
}
	                   
int kcom_hi_i2c_muti_write(unsigned char devaddress,unsigned int regaddress,int reg_addr_count, 
	                    unsigned char* data, unsigned long  data_count)
{
    return hi_i2c_muti_write(devaddress, regaddress, reg_addr_count, data, data_count); 	                        
}
	                    
unsigned char kcom_hi_i2c_readspecial(unsigned char devaddress, unsigned char pageindex,
	                                     unsigned char regaddress)
{
    return hi_i2c_readspecial(devaddress, pageindex, regaddress);    
}
	                                     	                    
int kcom_hi_i2c_writespecial(unsigned char devaddress,unsigned char pageindex,
	                            unsigned char regaddress,unsigned char data)
{
    return hi_i2c_writespecial(devaddress, pageindex, regaddress, data);
}
	                           

             

struct kcom_hi_i2c kcom_hii2c = {
	.kcom = KCOM_OBJ_INIT(UUID_KCOM_HI_I2C_V1_0_0_0, UUID_KCOM_HI_I2C_V1_0_0_0, NULL, THIS_MODULE, KCOM_TYPE_OBJECT, NULL),
    .hi_i2c_read = kcom_hi_i2c_read,
    .hi_i2c_write = kcom_hi_i2c_write,
    .hi_i2c_muti_read = kcom_hi_i2c_muti_read,
    .hi_i2c_muti_write = kcom_hi_i2c_muti_write,
    .hi_i2c_readspecial = kcom_hi_i2c_readspecial,
    .hi_i2c_writespecial  = kcom_hi_i2c_writespecial


};

int kcom_hi_i2c_register(void)
{
	return kcom_register(&kcom_hii2c.kcom);
}

void kcom_hi_i2c_unregister(void)
{
	kcom_unregister(&kcom_hii2c.kcom);
}

               
