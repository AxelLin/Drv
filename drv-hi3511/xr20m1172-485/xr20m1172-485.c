/*
 * I2C_2_GPIO.
 * Copyright (c)  DaoGuang Wu <wdgvip@gmail.com>
 
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
#include <linux/kthread.h>

#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/uaccess.h>

#include <linux/moduleparam.h>

#include <asm/uaccess.h>

#include <asm/arch/gpio.h>
#include <asm/arch/platform.h>
#include <asm/arch/yx_gpiodrv.h>
#include<asm/arch/yx_iomultplex.h>




#include <linux/i2c.h>
#include <linux/i2c-id.h>

#include "xr20m1172-485.h"
#include <linux/debug.h>


// ��������������������
#define I2C_GPIO_INPUT                      0
#define I2C_GPIO_OUTPUT                     1
// �������Ÿߵ͵�ƽ
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
#define GPIOIOC_GETPINVALUE           _IOR(GPIODRV_IOCTL_BASE, 11, int)

#define GPIOIOC_SETBAUDERATE	_IOR(GPIODRV_IOCTL_BASE, 12, int)


#define YX_GPIO_GET_SC_PERCTRL1   (*(volatile unsigned long *)(IO_ADDRESS(REG_BASE_SCTL)+0x20))
#define YX_GPIO_SET_SC_PERCTRL1(v)  do{ *(volatile unsigned long *)(IO_ADDRESS(REG_BASE_SCTL)+0x20)=v;} while(0)
#define YX_GPIO_SC_PERCTRL1(g,b)      YX_GPIO_SET_SC_PERCTRL1 ((YX_GPIO_GET_SC_PERCTRL1 & (~(1<<(g)))) | ((b)<<(g)))




#define I2C_GPIO_DRIVER_NAME        "i2c_485"
#define I2C_GPIO_MAJOR                    230

#define I2C_M_WR     0


#define R_TRG	16
#define T_TRG	8

//#define I2C_DRIVERID_XR20M1172_GPIO    11721

struct workqueue_struct  *xr20m1172_wq ;

struct work_struct   xr20m1172_work;

//static u8 cached_lcr[2];
//static u8 cached_efr[2];
//static u8 cached_mcr[2];
static int i2c_gpio_major = I2C_GPIO_MAJOR;

#define XR20M1172_CLOCK			24000000//14709800
#define PRESCALER 					1
#define I2C_BAUD_RATE			115200
#define SAMPLE_RATE				16


#define IRQ_GPIO_0   			6




#define BUF_HEAD_TX        (i2c_gpio_port->tx_buf[i2c_gpio_port->tx_head])
#define BUF_TAIL_TX         (i2c_gpio_port->tx_buf[i2c_gpio_port->tx_tail])


#define INCBUF(x,mod)   ((++(x)) & ((mod)-1))

#define BUF_HEAD_RX        (i2c_gpio_port->rx_buf[i2c_gpio_port->rx_head])
#define BUF_TAIL_RX         (i2c_gpio_port->rx_buf[i2c_gpio_port->rx_tail])



spinlock_t xr20m1172_lock;
spinlock_t global_lock;


unsigned char read_or_not = 0;

static atomic_t recv_timeout =ATOMIC_INIT(0);

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
* ��������֧��
********************************************************************************
*/ 
#define DEBUG_I2C_GPIO      1

#if DEBUG_I2C_GPIO > 0
	#define PRINTK_I2C_GPIO(x...)  printk(KERN_INFO x)
#else
	#define PRINTK_I2C_GPIO(x...) do{}while(0)
#endif


static const int reg_info[27][3] = {
	{0x0+2, 20, 1},		//RHR
	{0x0+2, 20, 2},		//THR
	{0x0+2, 60, 3},		//DLL
	{0x8+2, 60, 3},		//DLM
	{0x10+2, 50, 3},		//DLD
	{0x8+2, 20, 3},		//IER:bit[4-7] needs efr[4] ==1,but we dont' access them now
	{0x10+2, 20, 1},		//ISR:bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x10+2, 20, 2},		//FCR :bit[4/5] needs efr[4] ==1,but we dont' access them now
	{0x18+2, 10, 3},		//LCR
	{0x20+2, 40, 3},		//MCR :bit[2/5/6] needs efr[4] ==1,but we dont' access them now
	{0x28+2, 40, 1},		//LSR
	{0x30+2, 70, 1},		//MSR
	{0x38+2, 70, 3},		//SPR
	{0x30+2, 80, 3},		//TCR
	{0x38+2, 80, 3},		//TLR
	{0x40+2, 20, 1},		//TXLVL
	{0x48+2, 20, 1},		//RXLVL
	{0x50+2, 20, 3},		//IODir
	{0x58+2, 20, 3},		//IOState
	{0x60+2, 20, 3},		//IOIntEna
	{0x70+2, 20, 3},		//IOControl
	{0x78+2, 20, 3},		//EFCR
	{0x10+2, 30, 3},		//EFR
	{0x20+2, 30, 3},		//Xon1
	{0x28+2, 30, 3},		//Xon2
	{0x30+2, 30, 3},		//Xoff1
	{0x38+2, 30, 3},		//Xoff2
};

static struct class *my_class;


struct xr20m1172_port *i2c_gpio_port;



struct timer_list read_timer;
struct timer_list read_timer1;


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
	//spin_lock(&xr20m1172_lock);
	
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
	//spin_unlock(&xr20m1172_lock);
	
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
		//spin_lock(&xr20m1172_lock);
		
		for(i=0;i<RW_RETRY_TIME;i++)
		{
   	 		msgbuf[0] = addr;
    			msgbuf[1] = value;
    			msg.len   = 2;
    		if (i2c_transfer(save_client->adapter, &msg, 1) < 0)  
    			{
       	 			printk("i2c transfer fail in i2c_gpio  write\n");
       	 			up(&i2c_gpio_mutex);
       	 			//spin_unlock(&xr20m1172_lock);
					
       	 			return 1;
    			}
			
  		  }
		up(&i2c_gpio_mutex);
		//spin_unlock(&xr20m1172_lock);
	
		return 0;	
}



static int isr_thread(void * arg)
{
	unsigned char int_val0;
	unsigned char  i =0;
	DECLARE_WAITQUEUE(wait, current);
	add_wait_queue(&i2c_gpio_port->isr_wait, &wait);
	daemonize("i2c-thread");
	
	//interruptible_sleep_on(&i2c_gpio_port->isr_wait);
	
	while(1)
		{
		retry:
		//printk("I am in isr_thread the %02x time\n",i++);
		//printk("the LCR in thread  is %02x\n", i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0]));

		int_val0 = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_ISR][0]);
		//i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IER][0],0x00);
	//	printk("int_val0 is : %2x\n",int_val0);
	
		switch(int_val0)
		{
			case 0xc4 :

				/*	for(i=0;i< R_TRG;i++)
						{
							BUF_HEAD_RX = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_RHR][0]);
							i2c_gpio_port->rx_head = INCBUF(i2c_gpio_port->rx_head,MAX_BUF);
						
						}
					*/

				while(i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]) & 0x01)
						{
					
						BUF_HEAD_RX = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_RHR][0]);
						i2c_gpio_port->rx_head = INCBUF(i2c_gpio_port->rx_head,MAX_BUF);
						wake_up_interruptible(&i2c_gpio_port->rx_wait);
						}
					if(read_or_not == 1)
						{
							read_timer.expires  = jiffies + HZ/10;
							add_timer(&read_timer);
						}
	//					printk("I am in ktread when int_val0 is  %02x\n",int_val0);
					break;
					
			case 0xcc :
					while(i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]) & 0x01)
						{
					
						BUF_HEAD_RX = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_RHR][0]);
						i2c_gpio_port->rx_head = INCBUF(i2c_gpio_port->rx_head,MAX_BUF);
					
						}
				//	printk("I am in ktread when int_val0 is  %02x\n",int_val0);
							atomic_set(&recv_timeout, 1);
							wake_up_interruptible(&i2c_gpio_port->rx_wait);
					break;
	
			case 0xc1:   
					i = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]);
					break;
			case 0xc2:
					i = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]);
					wake_up_interruptible(&i2c_gpio_port->tx_wait);
				//	printk("I am in ktread when int_val0 is  %02x\n",int_val0);
					break;

			default :  
					//wake_up_interruptible(&i2c_gpio_port->tx_wait);
				//	printk("I am in ktread when int_val0 is  %02x\n",int_val0);
				//	printk("the LSR is %02x\n", i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]));
					
				break;
								
		}


		if(read_or_not == 1)
						{
							read_timer1.expires  = jiffies + HZ/20;
							add_timer(&read_timer1);
						}

//	wake_up_interruptible(&i2c_gpio_port->tx_wait);
	//i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IER][0],0x03);
		interruptible_sleep_on(&i2c_gpio_port->isr_wait); 
		goto retry;
			
		}
	remove_wait_queue(&i2c_gpio_port->isr_wait, &wait);
	return 0;
}


static int i2c_gpio_open(struct inode *inode, struct file *filp)
{
	
	filp->private_data = i2c_gpio_port;
	//DECLARE_WAITQUEUE(wait3, current);
	//add_wait_queue(&i2c_gpio_port->isr_wait, &wait3);
	
	PRINTK_I2C_GPIO(" i2c_gpio_open succed !\n");
	return 0;
}

static int i2c_gpio_release(struct inode *inode, struct file *filp)
{
	PRINTK_I2C_GPIO("i2c_gpio_release \n");

	//remove_wait_queue(&i2c_gpio_port->isr_wait,&wait3);
	return 0;
}

static void read_time_handle(unsigned long arg)
{
		atomic_set(&recv_timeout, 1);
		wake_up_interruptible(&i2c_gpio_port->rx_wait);

}

static void read_time1_handle(unsigned long arg)
{
	wake_up_interruptible(&i2c_gpio_port->isr_wait);

}


static ssize_t i2c_gpio_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	//unsigned char  result;
	unsigned char  temp;
	unsigned char  buf1[1024];
	unsigned int count=0;
	//unsigned int i;
			
	read_or_not =1;
	
	struct xr20m1172_port *gpio_port = filp->private_data;
	
	if(size > MAX_BUF)
		{
			printk("read count > 1024");
			return -EFAULT;
		}
	
	DECLARE_WAITQUEUE(wait1, current);
	add_wait_queue(&i2c_gpio_port->rx_wait, &wait1);
//	interruptible_sleep_on(&i2c_gpio_port->rx_wait);
	
	retry1:
		
	
	//	PRINTK_I2C_GPIO("I am here haha \n");
		if(i2c_gpio_port->rx_head   !=  i2c_gpio_port->rx_tail)
			{
 
				temp = BUF_TAIL_RX;
				i2c_gpio_port->rx_tail = INCBUF(i2c_gpio_port->rx_tail,MAX_BUF);
				*(buf1 + count) = temp;
		//	PRINTK_I2C_GPIO("com read %02x\n",temp);

				count++;
				
				
				if(count == size)
					{
						
						copy_to_user((void *)buf, buf1, count);
						if(1 == atomic_read(&recv_timeout))
							{
							//	i2c_gpio_port->rx_tail = i2c_gpio_port->rx_head;
								atomic_set(&recv_timeout,0);
							}
				      		goto  out;
					}
				 else
				 	{

						goto retry1;
								
				 	}
								
			}
		else
			{
			
			//	printk("read but no data in \n");
				if(1 == atomic_read(&recv_timeout))
					{
					
				//	printk("the timeout count is :%02x\n",atomic_read(&recv_timeout));
					atomic_set(&recv_timeout, 0);
					copy_to_user((void *)buf, buf1, count);
					
				      		goto  out;
					
					}
				
				if(filp->f_flags & O_NONBLOCK)
					{
						remove_wait_queue(&i2c_gpio_port->rx_wait,&wait1);
						return -EAGAIN;
					}
				else
					{
						
						if(read_or_not == 1)
						{
							read_timer1.expires  = jiffies + HZ/50;
							add_timer(&read_timer1);
						}
						if(read_or_not == 1)
						{
							read_timer.expires  = jiffies + HZ/10;
							add_timer(&read_timer);
						}

						interruptible_sleep_on(&i2c_gpio_port->rx_wait);
					
						goto retry1;
					}
			}



//interruptible_sleep_on(&i2c_gpio_port->rx_wait);   ///////////
//goto retry1;  /////////
	out: 
	remove_wait_queue(&i2c_gpio_port->rx_wait,&wait1);
	read_or_not = 0;
	//del_timer(&read_timer);
	
	return count;
}

int  ready_to_write(void)
{
	if(i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]) & 0x20)
	{
	//	printk("22 \n");
		return 1;
	}
	else {	
	//	printk("11 \n");
		mdelay(5);	
		return 0;
	}
}

static ssize_t i2c_gpio_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	//unsigned char  result ;
	unsigned char buf2[MAX_BUF];
	unsigned int count = 0;
	unsigned char temp;
	//unsigned  int i=0;
	
	struct xr20m1172_port *gpio_port = filp->private_data;
	DECLARE_WAITQUEUE(wait2, current);
	add_wait_queue(&i2c_gpio_port->tx_wait, &wait2);
	
         if(size > MAX_BUF)
         	{
			printk("write size to large \n");
			return -EFAULT;
         	}
	copy_from_user(buf2, (void *) buf, size);
retry:

	while( ready_to_write() )
		{
			temp = *(buf2 + count );
		//	PRINTK_I2C_GPIO("com write %02x\n",temp);	
	
			i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_THR][0], temp);
			count += 1;
			if(count == size )
			{
				goto out;
			}
					
		}
	
	interruptible_sleep_on(&i2c_gpio_port->tx_wait);
	goto retry;
	
	out: 
	remove_wait_queue(&i2c_gpio_port->tx_wait,&wait2);

	return count;
}


static int i2c_gpio_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	int result ;
	
	unsigned int value = (unsigned int ) arg;
	unsigned int __user *argp = (unsigned int __user *)arg;
	
	struct xr20m1172_port *gpio_port = filp->private_data;
	//spin_lock(&global_lock);
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
		//	printk(KERN_ERR "i2c driver set value %02x\n",value);
			result = i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0], gpio_port->out_level);
			if (result != 0 )
				{
					printk("i2c_gpio set pin value error !");
				}
			break;
			case GPIOIOC_GETPINVALUE:
			result = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0]);
			//printk(KERN_ERR "driver read value %02x \n",result);
			copy_to_user(argp, &result, sizeof(result));
				break;
			case GPIOIOC_SETBAUDERATE:
				copy_from_user(&result, argp, sizeof(result));
				i2c_set_baudrate(result);
			default: break;


		}
//	spin_unlock(&global_lock);
	
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
	
	unsigned char dld_reg;
	unsigned int baud = baudrate;
	unsigned int temp;

	unsigned long  required_divisor2;
	unsigned short  required_divisor ;
	
	required_divisor2  = (unsigned long) ((XR20M1172_CLOCK * 16)/(PRESCALER * SAMPLE_RATE * baud));
	required_divisor    = required_divisor2 / 16;
	dld_reg	=(char)(required_divisor2   - required_divisor*16);
	dld_reg   &= ~(0x3 << 4);  //16X

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0x83);
	
	printk("dld_reg is :%02x\n",dld_reg);
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLM][0], required_divisor >> 8);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLM][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLM  data is :%02x\n",temp);

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLL][0], required_divisor &0xff);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLL][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLL  data is :%02x\n",temp);

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_DLD][0], dld_reg);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_DLD][0]);
	PRINTK_I2C_GPIO("i2c_gpio read DLD  data is :%02x\n",temp);
	

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0x03);


	return 0;
}


void i2c_gpio_init(void)
{
	
	unsigned char temp;
	//static unsigned char count=0;

/*LCR         0000 0011 */
	//spin_lock(&global_lock);
	temp =0xBF;
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], temp);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0]);
	printk("i2c_gpio read LCR  data is :%02x\n",temp);
	
/*EFR  */
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_EFR][0]);
	temp  |=( 1 << 4) ;
	
	 i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_EFR][0], temp);
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_EFR][0]);
	PRINTK_I2C_GPIO("i2c_gpio read EFR  data is :%02x\n",temp);



	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0x83);
/*MCR        */
	temp = (1 << 3)  ;

	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_MCR][0], temp);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_MCR][0]);
	PRINTK_I2C_GPIO("i2c_gpio read MCR  data is :%02x\n",temp);
/* DLD  */	
	

	i2c_set_baudrate(115200);
//i2c_set_baudrate(4800);




/*LCR         0000 0011 */
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0], 0x03);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LCR][0]);
	PRINTK_I2C_GPIO("i2c_gpio read LCR  data is :%02x\n",temp);

/*IER&*/
	temp =3;
	//temp |= 1 | (1<<1) | (1<<2);
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IER][0], temp);
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IER][0]);
	PRINTK_I2C_GPIO("i2c_gpio read IER  data is :%02x\n",temp);	
	
/*FCR*/
	//i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_FCR][0], 0x01);
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_FCR][0], 0x47);
	printk("The FCR is %02x\n",i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOCONTROL][0]));


/*EFCR*/
	temp = i2c_gpio_byte_read(XR20M1172_ADDR,reg_info[XR20M1170REG_EFCR][0]);
	printk("The EFCR is %02x\n", temp);
	temp |= (1<<4)| (1<<5);
	i2c_gpio_byte_write(XR20M1172_ADDR,reg_info[XR20M1170REG_EFCR][0],temp);

	
/*IOCRL*/
	
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOCONTROL][0]);
	
	PRINTK_I2C_GPIO("i2c_gpio IOCTL data is :%02x\n",temp);

	
	 i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IODIR][0], 0xff);	
	 temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IODIR][0]);
	PRINTK_I2C_GPIO("i2c_gpio IODIR data is :%02x\n",temp);

	
	
	
	i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0], 0xaa);
	//count ++;
	//msleep(1000)
	temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_IOSTATE][0]);
	PRINTK_I2C_GPIO("i2c_gpio read data is  IOSTATE :%02x\n",temp);


	//temp = i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_ISR][0]);
	
	//PRINTK_I2C_GPIO("i2c_gpio ISR data is :%02x\n",temp);
	//i2c_gpio_byte_write(XR20M1172_ADDR, reg_info[XR20M1170REG_IOINTENA][0], 0x00);
	
	//spin_unlock(&global_lock);

}

void i2c_gpio_irq_init(void)
{

	yx_iomul(0, 0, 1);
	//HISILICON_GPIO_SET_BITDIR(0, 0, 0);
	yx_gpio_set_bitdir(0, 0, 0);
	//HISILICON_GPIO_SET_INT_BITIS(0, 0, 0);
	yx_gpio_set_int_bits(0, 0, 0);

	//HISILICON_GPIO_SET_INT_BITIBE(0, 0, 0);
	yx_gpio_set_int_bitibe(0, 0, 0);
	
	//HISILICON_GPIO_SET_INT_BITIEV(0, 0, 0);
	yx_gpio_set_int_bitiev(0, 0, 0);
	//HISILICON_GPIO_SET_INT_BITENABLE(1, 0, 0);	
	yx_gpio_set_int_bitenb(1, 0, 0);
	
}

void xr20m1172_do_work(void *data)
{
	//struct xr20m1172_port  *port = (struct xr20m1172_port * ) data;
	
	PRINTK_I2C_GPIO("I am in work struct  1111111111111!\n");
}

static irqreturn_t i2c_gpio_handler(int irq, void *dev_id, struct pt_regs *reg)
{

//	int i;
//	unsigned char rx_buf,tx_buf;
//	static unsigned int count = 0;
	//PRINTK_I2C_GPIO("I am in interrupt !%03x\n",count++);


	
	 if ((HISILICON_GPIO_GET_INT_MIS(0)& (1 << 0)) == 0) 
	  	{
			
			 return IRQ_NONE;

	         }
//	PRINTK_I2C_GPIO("I am in interrupt !%03x\n",count++);
	
	//printk("The LSR in interrupt is :%02x\n",i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_LSR][0]));
	

	wake_up_interruptible(&i2c_gpio_port->isr_wait);
	
    	HISILICON_GPIO_SET_INT_BITIC(0, 0);
	return IRQ_HANDLED;

}

static int __init xr20m1172_gpio_init(void)
{
	int result ,err;
	int retval ;
	//unsigned char  temp;
//	pid_t pid;
//	unsigned char i=0;
	struct task_struct * i2c_thread = NULL; 
	spin_lock_init(&xr20m1172_lock);
	spin_lock_init(&global_lock);
	
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
		goto fail ;
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
	my_class = class_create(THIS_MODULE, "i2c_gpio");
	if(IS_ERR(my_class))
		{
			printk("Err :failed int creating class !");
			return -1;
		}
	class_device_create(my_class, MKDEV(i2c_gpio_major,0), NULL,"i2c_gpio" );
	devfs_mk_cdev(MKDEV(i2c_gpio_major,0), S_IFCHR | S_IRUSR | S_IWUSR, "i2c_gpio");
	i2c_gpio_port->rx_head = i2c_gpio_port->rx_tail =0;
	i2c_gpio_port->tx_head = i2c_gpio_port->tx_tail = 0;
	init_waitqueue_head(&i2c_gpio_port->rx_wait);
	init_waitqueue_head(&i2c_gpio_port->tx_wait);
	init_waitqueue_head(&i2c_gpio_port->isr_wait);
	
	
	i2c_gpio_init();
	i2c_gpio_irq_init();
	retval = request_irq(IRQ_GPIO_0,i2c_gpio_handler,SA_SHIRQ,"I2C_GPIO_DRV",i2c_gpio_port);
	if(retval)
		{
			printk("request i2c_gpio IRQ failed !");
		}
	else
		{
			printk("request i2c_gpio IRQ success !");
			HISILICON_GPIO_SET_INT_BITENABLE(1, 0, 0);
		}
	//xr20m1172_wq = create_singlethread_workqueue("my wq");
	
	//INIT_WORK(&xr20m1172_work, xr20m1172_do_work,i2c_gpio_port);
	
	i2c_thread = kthread_run(isr_thread, NULL, "i2c-thread");


	init_timer(&read_timer);
	init_timer(&read_timer1);
	
	read_timer.function = &read_time_handle;
	read_timer1.function = & read_time1_handle;
	
printk("the FCR is %02x\n", i2c_gpio_byte_read(XR20M1172_ADDR, reg_info[XR20M1170REG_FCR][0]));
	


	return 0;
       
fail_malloc: 
	kfree(i2c_gpio_port);
fail:	
	unregister_chrdev_region(devno,1);
       return result;
	   
}

static void __exit xr20m1172_gpio_exit(void)
{	i2c_del_driver(&i2c_gpio_driver);
	cdev_del(&i2c_gpio_port->cdev);
 	class_device_destroy(my_class, MKDEV(i2c_gpio_major,0));
	class_destroy(my_class);
	kfree(i2c_gpio_port);
	free_irq(IRQ_GPIO_0, i2c_gpio_port);
	unregister_chrdev_region(MKDEV(i2c_gpio_major,0),1);
	del_timer(&read_timer);
	PRINTK_I2C_GPIO("unregister i2c  gpio driver !");
		
}


module_init(xr20m1172_gpio_init);
module_exit(xr20m1172_gpio_exit);

MODULE_AUTHOR("DaoGuang Wu <wdgvip@gmail.com>");
MODULE_LICENSE("GPL");




















