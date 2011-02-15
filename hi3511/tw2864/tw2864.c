/* 
 *
 * Copyright (c) 2006 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 * History:
 *      10-April-2006 create this file
 *      2006-04-29  add record path half d1 mod
 *      2006-05-13  set the playpath default output mod to full
 *      2006-05-24  add record mod 2cif
 *      2006-06-15  support mod changing between every record mod
 *      2006-08-12  change the filters when record mod change
 */

#include <linux/config.h>
#include <linux/kernel.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <linux/string.h>
#include <linux/list.h>
#include <asm/semaphore.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/kcom.h>
#include <kcom/gpio_i2c.h>

#include "tw2864_def.h"
#include "tw2864.h"

#define DEBUG_2864 1


static unsigned int tw2864a_dev_open_cnt =0;
static unsigned int tw2864b_dev_open_cnt =0;
static unsigned int  cascad_judge = 0;


static unsigned char tw2864_byte_write(unsigned char chip_addr,unsigned char addr,unsigned char data) {
    gpio_i2c_write(chip_addr,addr,data);   
    return 0;
}

static unsigned char tw2864_byte_read(unsigned char chip_addr,unsigned char addr)
{
    return gpio_i2c_read(chip_addr,addr);   
}



static void tw2864_write_table(unsigned char chip_addr,unsigned char addr,unsigned char *tbl_ptr,unsigned char tbl_cnt)
{
    unsigned char i = 0;
    for(i = 0;i<tbl_cnt;i++)
    {
        gpio_i2c_write(chip_addr,(addr+i),*(tbl_ptr+i));
    }
}



static int tw2864_device_video_init(unsigned char chip_addr,unsigned char video_mode)
{
	unsigned char tw2864_id =0;
    
    tw2864_id = tw2864_byte_read(chip_addr,TW2864_ID);
    
    /* 0x6a or 0x6b for different version of tw2864, 0x6a is fit for the version of tw2864 0816; 
    0x6b is fit for the version of tw2864 0826*/
    if(tw2864_id != 0x6a && tw2864_id != 0x6b)
    {
	    printk("id err, tw2864_id =%x\n",tw2864_id);
        return -1;
    }
    
    //tw2864_write_table(chip_addr,0x80,tbl_tw2864_common_0x80,16);
    tw2864_write_table(chip_addr,0x80,tbl_tw2864_common_0x80_88,9);
    tw2864_write_table(chip_addr,0x8a,tbl_tw2864_common_0x8a_8f,6);
    
    tw2864_write_table(chip_addr,0x90,tbl_tw2864_common_0x90,16);
    tw2864_write_table(chip_addr,0xa4,tbl_tw2864_common_0xa4,12);
    tw2864_write_table(chip_addr,0xb0,tbl_tw2864_common_0xb0,1);
    tw2864_write_table(chip_addr,0xc4,tbl_tw2864_common_0xc4,12);
    tw2864_write_table(chip_addr,0xd0,tbl_tw2864_common_0xd0,16);
    tw2864_write_table(chip_addr,0xe0,tbl_tw2864_common_0xe0,16);
    tw2864_write_table(chip_addr,0xf0,tbl_tw2864_common_0xf0,16);

    tw2864_write_table(chip_addr,0x00,tw2864_pal_channel,16);
    tw2864_write_table(chip_addr,0x10,tw2864_pal_channel,16);
    tw2864_write_table(chip_addr,0x20,tw2864_pal_channel,16);
    tw2864_write_table(chip_addr,0x30,tw2864_pal_channel,16);
    
	return 0;
}


ssize_t tw2864a_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

/*
 * tw2864a write routine.
 * do nothing.
 */
ssize_t tw2864a_write(struct file * file, const char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}

ssize_t tw2864b_read(struct file * file, char __user * buf, size_t count, loff_t * offset)
{
return 0;
}

/*
 * tw2864a write routine.
 * do nothing.
 */
ssize_t tw2864b_write(struct file * file, const char __user * buf, size_t count, loff_t * offset)
{
    return 0;
}






int tw2864b_open(struct inode * inode, struct file * file)
{
    return 0;
#if 0
    if(tw2864a_dev_open_cnt)
   {
   	printk("tw2864a_dev is still on");
   	return -EFAULT;
   }	
    tw2864a_dev_open_cnt++;	
    if(DEBUG_2864)
    {
    	printk("tw2864a_dev open ok\n");
    }	
    return 0;
#endif    
}

/*
 * tw2864a close routine.
 * do nothing.
 */
int tw2864b_close(struct inode * inode, struct file * file)
{
   tw2864a_dev_open_cnt --;	
    return 0;
}



int tw2864a_open(struct inode * inode, struct file * file)
{
#if 0
   if(tw2864a_dev_open_cnt)
   {
   	printk("error tw2864adev had open\n");
        return -EFAULT;        
   }	
   tw2864a_dev_open_cnt++;
    if(DEBUG_2864)
    {
    	printk("tw2864a_dev open ok\n");
    }	
#endif    
    return 0;
} 
/*
 * tw2864a close routine.
 * do nothing.
 */
int tw2864a_close(struct inode * inode, struct file * file)
{
    tw2864a_dev_open_cnt--;	
    return 0;
}



/*
 *      The various file operations we support.
 */

static struct file_operations tw2864a_fops = {
    .owner      = THIS_MODULE,
    .read       = tw2864a_read,
    .write      = tw2864a_write,
    
    .open       = tw2864a_open,
    .release    = tw2864a_close
};



static struct miscdevice tw2864a_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "tw2864adev",
   .fops  = &tw2864a_fops,
};



/*
 *      The various file operations we support.
 */
 

static struct file_operations tw2864b_fops = {
    .owner      = THIS_MODULE,
    .read       = tw2864a_read,
    .write      = tw2864a_write,
    
    .open       = tw2864a_open,
    .release    = tw2864a_close
};



static struct miscdevice tw2864b_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "tw2864bdev",
   .fops  = &tw2864b_fops,
};



DECLARE_KCOM_GPIO_I2C();

static int __init tw2864_init(void)
{
    int ret = 0;
    
    ret = KCOM_GPIO_I2C_INIT();
    if(ret)
    {
        printk("GPIO I2C module is not load.\n");
        return -1;
    }

#if 1
    //register tw2864a_dev
    ret = misc_register(&tw2864a_dev);
    printk("TW2864a driver init start ... \n");
    if (ret)
    {
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2864a_ERROR: could not register tw2864a devices. \n");
        return -1;
    }
    if (tw2864_device_video_init(TW2864a_I2C_ADDR,AUTOMATICALLY) < 0)
    {
        misc_deregister(&tw2864a_dev);
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2864a_ERROR: tw2864a driver init fail for device init error!\n");
        return -1;
    }

    printk("tw2864a driver init successful!\n");

#endif


#if 0

    //register tw2864b_dev
    ret = misc_register(&tw2864b_dev);
    printk("TW2864b driver init start ... \n");
    if (ret)
    {
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2864b_ERROR: could not register tw2864b devices. \n");
        return -1;
    }
    if (tw2864_device_video_init(TW2864b_I2C_ADDR,AUTOMATICALLY) < 0)
    {
        misc_deregister(&tw2864b_dev);
    	KCOM_GPIO_I2C_EXIT();
        printk("\ttw2864b_ERROR: tw2864b driver init fail for device init error!\n");
        return -1;
    }
    printk("tw2864b driver init successful!\n");
#endif


    return ret;
}



static void __exit tw2864_exit(void)
{
    misc_deregister(&tw2864a_dev);
//    misc_deregister(&tw2864b_dev);
    KCOM_GPIO_I2C_EXIT();
}

module_init(tw2864_init);
module_exit(tw2864_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");

