/*
 * I2C_2_GPIO.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
 
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>

#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
//#include <linux/platform_device.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/errno.h>


#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/uaccess.h>

#include <linux/moduleparam.h>







#include <linux/i2c.h>
#include <linux/i2c-id.h>

#include "xr20m1172.h"
#include <linux/debug.h>


// 定义引脚输入输出方向
#define I2C_GPIO_INPUT                      0
#define I2C_GPIO_OUTPUT                     1
// 定义引脚高低电平
#define I2C_GPIO_PIN_LOW                    0
#define I2C_GPIO_PIN_HIGH                   1

#define	GPIODRV_IOCTL_BASE	'G'

#define GPIOIOC_SETDIRECTION    _IOW(GPIODRV_IOCTL_BASE, 0, int)
#define GPIOIOC_SETPINVALUE     _IOW(GPIODRV_IOCTL_BASE, 1, int)
#define GPIOIOC_SETTRIGGERMODE  _IOW(GPIODRV_IOCTL_BASE, 2, int)
#define GPIOIOC_SETTRIGGERIBE   _IOW(GPIODRV_IOCTL_BASE, 3, int)
#define GPIOIOC_SETTRIGGERIEV   _IOW(GPIODRV_IOCTL_BASE, 4, int)
#define GPIOIOC_SETIRQALL       _IOW(GPIODRV_IOCTL_BASE, 5, int)
#define GPIOIOC_SETCALLBACK     _IOW(GPIODRV_IOCTL_BASE, 6, int)
#define GPIOIOC_ENABLEIRQ       _IO(GPIODRV_IOCTL_BASE,  7)
#define GPIOIOC_DISABLEIRQ      _IO(GPIODRV_IOCTL_BASE,  8)
#define GPIOIOC_GETPININFO      _IOR(GPIODRV_IOCTL_BASE, 9, GPIO_INFO_T)
#define GPIOIOC_GETCALLBACK     _IOR(GPIODRV_IOCTL_BASE, 10, int)



#define I2C_GPIO_DRIVER_NAME        "i2c_gpio"
#define I2C_GPIO_MAJOR                    231

#define I2C_M_WR     0


//#define I2C_DRIVERID_XR20M1172_GPIO    11721


static u8 cached_lcr[2];
static u8 cached_efr[2];
static u8 cached_mcr[2];
static int i2c_gpio_major = I2C_GPIO_MAJOR;

#define XR20M1172_CLOCK			14.7098
#define PRESCALER 					1
#define I2C_BAUD_RATE			115200
#define SAMPLE_RATE				16






//static spinlock_t xr20m1172_lock = SPIN_LOCK_UNLOCKED;


static DECLARE_MUTEX(i2c_gpio_mutex);
static struct i2c_driver i2c_gpio_driver;
static struct i2c_client *save_client;
static unsigned short ignore[] = {I2C_CLIENT_END };
static unsigned short normal_i2c[]= {
	XR20M1172_ADDR,I2C_CLIENT_END
};

static struct i2c_client_address_data addr_data = {
	.normal_i2c	=normal_i2c,
	.probe		=ignore,
	.ignore		=ignore,
//	.forces		=ignore,
};

/*
********************************************************************************
* 定义调试支持
********************************************************************************
*/ 
#define DEBUG_I2C_GPIO      1

#if DEBUG_I2C_GPIO > 0
	#define PRINTK_I2C_GPIO(x...)  printk(x)
#else
	#define PRINTK_I2C_GPIO(x...) do{}while(0)
#endif



/* 
****************************************************************************
 * meaning of the pair:
 * first: the subaddress (physical offset<<3) of the register
 * second: the access constraint:
 * 10: no constraint
 * 20: lcr[7] == 0
 * 30: lcr == 0xbf
 * 40: lcr != 0xbf
 * 50: lcr[7] == 1 , lcr != 0xbf, efr[4] = 1
 * 60: lcr[7] == 1 , lcr != 0xbf,
 * 70: lcr != 0xbf,  and (efr[4] == 0 or efr[4] =1, mcr[2] = 0)
 * 80: lcr != 0xbf,  and (efr[4] = 1, mcr[2] = 1)
 * 90: lcr[7] == 0, efr[4] =1 
 * 100: lcr!= 0xbf, efr[4] =1
 * third:  1: readonly
 * 2: writeonly
 * 3: read/write
 ****************************************************************************
 */
/* static const int reg_info[27][3] = {
	{0x0, 20, 1},		//RHR
	{0x0, 20, 2},		//THR
	{0x0, 60, 3},		//DLL
	{0x1, 60, 3},		//DLM
	{0x2, 50, 3},		//DLD
	{0x1, 20, 3},		//IER:bit[4-7] needs efr[4] ==1,but we dont' access them now
	{0x2, 20, 1},		//ISR:bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x2, 20, 2},		//FCR :bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x03, 10, 3},		//LCR
	{0x04, 40, 3},		//MCR :bit[2/5/6] needs efr[4] ==1,but we dont' access them now
	{0x05, 40, 1},		//LSR
	{0x06, 70, 1},		//MSR
	{0x07, 70, 3},		//SPR
	{0x06, 80, 3},		//TCR
	{0x07, 80, 3},		//TLR
	{0x08, 20, 1},		//TXLVL
	{0x09, 20, 1},		//RXLVL
	{0x0A, 20, 3},		//IODir
	{0x0B, 20, 3},		//IOState
	{0x0C, 20, 3},		//IOIntEna
	{0x0E, 20, 3},		//IOControl
	{0x0F, 20, 3},		//EFCR
	{0x02, 30, 3},		//EFR
	{0x04, 30, 3},		//Xon1
	{0x05, 30, 3},		//Xon2
	{0x06, 30, 3},		//Xoff1
	{0x07, 30, 3},		//Xoff2
};
*/
static const int reg_info[27][3] = {
	{0x0, 20, 1},		//RHR
	{0x0, 20, 2},		//THR
	{0x0, 60, 3},		//DLL
	{0x8, 60, 3},		//DLM
	{0x10, 50, 3},		//DLD
	{0x8, 20, 3},		//IER:bit[4-7] needs efr[4] ==1,but we dont' access them now
	{0x10, 20, 1},		//ISR:bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x10, 20, 2},		//FCR :bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x18, 10, 3},		//LCR
	{0x20, 40, 3},		//MCR :bit[2/5/6] needs efr[4] ==1,but we dont' access them now
	{0x28, 40, 1},		//LSR
	{0x30, 70, 1},		//MSR
	{0x38, 70, 3},		//SPR
	{0x30, 80, 3},		//TCR
	{0x38, 80, 3},		//TLR
	{0x40, 20, 1},		//TXLVL
	{0x48, 20, 1},		//RXLVL
	{0x50, 20, 3},		//IODir
	{0x58, 20, 3},		//IOState
	{0x60, 20, 3},		//IOIntEna
	{0x70, 20, 3},		//IOControl
	{0x78, 20, 3},		//EFCR
	{0x10, 30, 3},		//EFR
	{0x20, 30, 3},		//Xon1
	{0x28, 30, 3},		//Xon2
	{0x30, 30, 3},		//Xoff1
	{0x38, 30, 3},		//Xoff2
};

//static struct class *i2c_class;


struct xr20m1172_port *i2c_gpio_port;


unsigned int i2c_set_baudrate(unsigned  int baudrate);

unsigned char i2c_gpio_byte_read(unsigned char chip_addr, unsigned char addr)
{
  int i;
  unsigned int have_retried = 0;
  unsigned char msgbuf0[1];
  unsigned char msgbuf1[1];
  struct i2c_msg msg[2] = {{chip_addr >> 1, 0, 1, msgbuf0 }, 
	                         {chip_addr >>1, I2C_M_RD   | I2C_M_NOSTART , 1, msgbuf1 }
	                        };
	                        
	down(&i2c_gpio_mutex);
	for(i=0;i<RW_RETRY_TIME;i++)
	{
        msgbuf0[0] = addr;       
        msg[0].len = 1;
        msg[1].len = 1;
        if (i2c_transfer(save_client->adapter, msg, 2) < 0) {
            printk("i2c transfer fail in i2c_gpio read\n");
        } else {   

		   have_retried = 1;
		if (have_retried) { 
				break;
		    }
		
		}
	}
	up(&i2c_gpio_mutex);
	if (i == RW_RETRY_TIME) {
		printk("read i2c_gpio fail!\n");
		return -EAGAIN;
	}
	return msgbuf1[0];
	
}




unsigned int i2c_gpio_byte_write(unsigned char chip_addr, unsigned char addr, unsigned char value)
{

    int i;
    unsigned char msgbuf[2];	
    struct i2c_msg msg = {chip_addr >> 1, 0,1, msgbuf};
		down(&i2c_gpio_mutex);

		for(i=0;i<RW_RETRY_TIME;i++)
		{
   	 		msgbuf[0] = addr;
    			msgbuf[1] = value;
    			msg.len   = 2;
    		if (i2c_transfer(save_client->adapter, &msg, 1) < 0)  
    			{
       	 			printk("i2c transfer fail in i2c_gpio  write\n");
       	 			up(&i2c_gpio_mutex);
       	 			return 1;
    			}
			
  		  }
		up(&i2c_gpio_mutex);
	
		return 0;	
}




static int i2c_gpio_open(struct inode *inode, struct file *filp)
{
	filp->private_data = i2c_gpio_port;
	
	PRINTK_I2C_GPIO(" i2c_gpio_open \n");
	return 0;
}

static int i2c_gpio_release(struct inode *inode, struct file *filp)
{
	PRINTK_I2C_GPIO("i2c_gpio_release \n");
	return 0;
}

static ssize_t i2c_gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned char  result;
	
	struct xr20m1172_port *gpio_port = filp->private_data;
	
	result = i2c_gpio_byte_read(XR20M1172_ADDR,reg_info[XR20M1170REG_IOSTATE][0]);
	
	PRINTK_I2C_GPIO("i2c gpio read %d/n",result);
	
	if(copy_to_user(buf,&result,1))
		{
			return -EFAULT;
		}
	
	return 1;
}

static ssize_t i2c_gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned char  result ;
	
	
	struct xr20m1172_port *gpio_port = filp->private_data;
	if(copy_from_user(&(gpio_port->out_level),buf,1))
		{
			return -EFAULT;
		}
	PRINTK_I2C_GPIO("i2c_gpio_write : %d\n",gpio_port->out_level);
	
	result = i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0], gpio_port->out_level);
	
	return 1;
}


static int i2c_gpio_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	int result ;
	unsigned int value = (unsigned int ) arg;
	
	struct xr20m1172_port *gpio_port = filp->private_data;

	switch(cmd)
		{
			case GPIOIOC_SETDIRECTION :
				
			result = i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IODIR][0], value);	
			if(result != 0)
				{
					printk("i2c_gpio set direction error !");
				}

			break;
			
			case GPIOIOC_SETPINVALUE   :
			gpio_port->out_level = value;
			
			result = i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0], gpio_port->out_level);
			if (result != 0 )
				{
					printk("i2c_gpio set pin value error !");
				}
			default: break;


		}
	
	return 0;
}



static struct file_operations i2c_gpio_fops ={
	.owner	= THIS_MODULE,
	.open	= i2c_gpio_open,
	.read	= i2c_gpio_read,
	.write	= i2c_gpio_write,
	.ioctl	= i2c_gpio_ioctl,
	.release	= i2c_gpio_release,
};





static int i2c_gpio_probe(struct i2c_adapter *adapter,int address, int kind)
{
	static struct i2c_client  *i2c_gpio_client;
	int ret;

	i2c_gpio_client = kmalloc(sizeof(struct i2c_client),GFP_KERNEL);
	
	if(!i2c_gpio_client)
		{
			return -ENOMEM;
		}

	i2c_gpio_client->driver = &i2c_gpio_driver;
	i2c_gpio_client->addr   = address;
	i2c_gpio_client->adapter = adapter;
	i2c_gpio_client->flags     = I2C_DF_NOTIFY;
	
	strncpy(i2c_gpio_client->name,I2C_GPIO_DRIVER_NAME,I2C_NAME_SIZE);
	DEBUG_PRINTK("I am here1 !");
	if((ret = i2c_attach_client(i2c_gpio_client)) != 0)
		{
		DEBUG_PRINTK("I am here3 !");
			kfree(i2c_gpio_client);
		}
	else
		{
			DEBUG_PRINTK("I am here2 !");
			save_client  = i2c_gpio_client;
		}
	return ret;
	
}
static int xr20m1172_gpio_attach(struct i2c_adapter *adapter)
{
	
	return i2c_probe(adapter,&addr_data,i2c_gpio_probe);
}

static int xr20m1172_gpio_detach(struct i2c_client *client)
{
	int ret=0;
	if(client->adapter)
		{
			ret = i2c_detach_client(client);
			if(!ret)
				{
					kfree(client);
					client->adapter =NULL;
				}
		}
	return ret;
}


static struct i2c_driver  i2c_gpio_driver =
{
	.owner	= THIS_MODULE,
	.name	= I2C_GPIO_DRIVER_NAME,
	.id		= I2C_DRIVERID_XR20M1172_GPIO,
	.flags	= I2C_DF_NOTIFY,
	.attach_adapter	= xr20m1172_gpio_attach,
	.detach_client		= xr20m1172_gpio_detach,
};

unsigned int i2c_set_baudrate(unsigned  int baudrate)
{
	unsigned char prescale;
	unsigned char samplemode;
	unsigned char dld_reg;
	unsigned int baud = baudrate;
	unsigned int temp;

	unsigned int  required_divisor2;
	unsigned int  required_divisor ;
	
	required_divisor2  = (XR20M1172_CLOCK * 16)/(PRESCALER * SAMPLE_RATE * baud);
	required_divisor    = required_divisor2 / 16;
	dld_reg	= required_divisor2   - required_divisor*16;
	dld_reg   &= ~(0x3 << 4);  //16X

	
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLM][0], required_divisor >> 8);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLM][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLM  data is :%02x\n",temp);

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLL][0], required_divisor &0xff);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLL][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLL  data is :%02x\n",temp);

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLD][0], dld_reg);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLD][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLD  data is :%02x\n",temp);

	

	return 0;
}


void i2c_gpio_init(void)
{
	unsigned int err;
	
	unsigned char temp;

	
/*LCR         0000 0011 */
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0xBF);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0]);
	printk("i2c_gpio read LCR  data is :%02x\n",temp);
	
/*EFR  */
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_EFR][0]);
	temp  |=( 1 << 4) ;
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_EFR][0], temp);

/*MCR        */
	temp = (1 << 3) | (0 << 7)  | (1 << 4) ;

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_MCR][0], temp);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_MCR][0]);
	printk("i2c_gpio read MCR  data is :%02x\n",temp);
/* DLD  */	
	

	i2c_set_baudrate(115200);


	
	
/*LCR         0000 0011 */
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0x03);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0]);
	PRINTK_I2C_GPIO("i2c_gpio read LCR  data is :%02x\n",temp);





	
	 i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IODIR][0], 0xff);
	
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IODIR][0]);
	printk("i2c_gpio write data is :%02x\n",temp);
		
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE], 0x55);
	printk("i2c_gpio write 0x66 \n");
	
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0]);
	 printk("i2c_gpio read data is :%02x\n",temp);
		
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE], 0x55);
	printk("i2c_gpio write 0x66 \n");
	
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0]);
	 printk("i2c_gpio read data is :%02x\n",temp);

	

}
static int __init xr20m1172_gpio_init(void)
{
	int result ,err;
	unsigned char  temp;
	
	dev_t devno = MKDEV(i2c_gpio_major,0);
	i2c_gpio_port = kmalloc(sizeof(struct xr20m1172_port ),GFP_KERNEL);
	
	if(!i2c_gpio_port)
		{
			result = -ENOMEM;
			goto fail_malloc;
		}
	memset(i2c_gpio_port,0,sizeof(struct xr20m1172_port));
	
	if(i2c_gpio_major)
		result = register_chrdev_region(devno,1,I2C_GPIO_DRIVER_NAME);
	else
		{
			result = alloc_chrdev_region(&devno,0,1,I2C_GPIO_DRIVER_NAME);
			i2c_gpio_major = MAJOR(devno);
		}
	if(result < 0)
		return result ;
	cdev_init(&i2c_gpio_port->cdev,&i2c_gpio_fops);
	i2c_gpio_port->cdev.owner = THIS_MODULE;
	i2c_gpio_port->cdev.ops   = &i2c_gpio_fops;
	err = cdev_add(&i2c_gpio_port->cdev,devno,1);
	if(err)
		printk(KERN_NOTICE "i2c gpio cdev add failed");
	
	err = i2c_add_driver(&i2c_gpio_driver);
	
	if(err)
		{
			printk("Registering I2C driver failed !");
			
			return err;
		}

	i2c_gpio_init();
	
		
	return 0;

fail_malloc: unregister_chrdev_region(devno,1);
       return result;
	   
}

static void __exit xr20m1172_gpio_exit(void)
{	i2c_del_driver(&i2c_gpio_driver);
	cdev_del(&i2c_gpio_port->cdev);
 	
	kfree(i2c_gpio_port);
	unregister_chrdev_region(MKDEV(i2c_gpio_major,0),1);
	PRINTK_I2C_GPIO("unregister i2c  gpio driver !");
		
}


module_init(xr20m1172_gpio_init);
module_exit(xr20m1172_gpio_exit);

MODULE_AUTHOR("DaoGuang Wu <wdgvip@gmail.com>");
MODULE_LICENSE("GPL");




















