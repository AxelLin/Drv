#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/termios.h>
#include <linux/tty_driver.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define GPIO_COM_IRQ 6
#define GPIO_VCOM_TTY_NR 1
#define GPIO_VCOM_MAJOR 3

#define GPIO_OUT_PIPE_NUM 2
#define GPIO_IN_PIPE_NUM 1
#define GPIO_GROUP_BASE 0x101e4000

/*config accept char number by every interrupt*/
#define READ_BUF_SIZE 4 

#define GPIO_DATA_IN_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x008)
#define GPIO_DATA_OUT_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x010)
#define GPIO_DIR_IN_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x400)
#define GPIO_DIR_OUT_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x400)
#define GPIO_INT_SET_ADDR1 IO_ADDRESS(GPIO_GROUP_BASE + 0x404)
#define GPIO_INT_SET_ADDR2 IO_ADDRESS(GPIO_GROUP_BASE + 0x408)
#define GPIO_INT_IE_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x410)
#define GPIO_INT_IC_ADDR IO_ADDRESS(GPIO_GROUP_BASE + 0x41c)

#define VCOM_DBUG_LEVEL	0
#define VCOM_DEBUG(level, s, params...) do{ if(level >= VCOM_DBUG_LEVEL)\
	                	printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
	                }while(0)
	        
#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))


struct gpio_vcom_serial{
	int open_cnt;
	struct semaphore sem;
	unsigned char *cir_buff;
	int head;
	int tail;
	
	struct tty_struct *p_tty;	
	spinlock_t	 in_lock;
	unsigned char	 *in_buff;
	int		 in_head;
	int		 in_tail;
};

static struct gpio_vcom_serial *gpio_serial = 0;

struct serial_comm_para {
	int in_baud;
	int out_baud;
	int check_flag;	/* 1--odd,2--even,3-none*/
	int stop_num;		/*stop bits length*/
	int data_num;		/*data bits length*/
};

/*initialize the communication parameter*/
static struct serial_comm_para g_comm_para = {
	19200,
	19200,	
	3,		/*none check bit*/
	1,		/*stop bits length*/
	8		/*data bits length*/
};

void rcv_data_push_tasklet(unsigned long data);

static DECLARE_TASKLET(rcv_data_tasklet, rcv_data_push_tasklet, 0);

void rcv_data_push_tasklet(unsigned long data)
{
	if(!gpio_serial)
		return;
	while(1){
		if(gpio_serial->in_head == gpio_serial->in_tail){
			break;
		}else{
			tty_insert_flip_char(gpio_serial->p_tty, gpio_serial->in_buff[gpio_serial->in_tail], TTY_NORMAL);
			gpio_serial->in_tail = (gpio_serial->in_tail + 1)%4096;
		}
	}	
	
	tty_flip_buffer_push(gpio_serial->p_tty);

}

static irqreturn_t  gpio_com_interrupt(int irq, void *dev_id, struct pt_regs *reg)
{
	local_irq_disable();
	unsigned int rcv_bit;
	unsigned int rcv_data[8] = {0};
	unsigned int check_data = 0;
	int i,k;
	int rcv_cycle = (int)(1000*1000/g_comm_para.in_baud);	/* unit:us */
	udelay(5*rcv_cycle/12);
	
for(k=0; k<READ_BUF_SIZE; k++){
	check_data = 0;
	rcv_bit = *((volatile unsigned int *)GPIO_DATA_IN_ADDR);
	if(rcv_bit == (1<<GPIO_IN_PIPE_NUM)){
		goto out;
	}

	/*get all data bits by timer cycle method*/
	for(i=0; i< g_comm_para.data_num; i++){
		udelay(rcv_cycle);
		rcv_bit = *((volatile unsigned int *)GPIO_DATA_IN_ADDR);
		if (rcv_bit == (1<<GPIO_IN_PIPE_NUM)){
			rcv_bit = (rcv_bit>>GPIO_IN_PIPE_NUM);
			check_data += rcv_bit;
			rcv_data[k] |= (rcv_bit<<i);	/*received data from low bit to high bit*/
		}
	}
	
	/*check the check bit*/
	if(g_comm_para.check_flag != 3){
		udelay(rcv_cycle);
		rcv_bit = *((volatile unsigned int *)GPIO_DATA_IN_ADDR);;
		rcv_bit = (rcv_bit>>GPIO_IN_PIPE_NUM);
		check_data += rcv_bit;
		if(g_comm_para.check_flag == 1){
			if(check_data%2 != 1){
				goto out;
			}
		}
		else{
			if(check_data%2 != 0){
				goto out;
			}
		}
	
	}
	
	/*check the stop bits*/
	udelay(rcv_cycle-2);
	rcv_bit = *((volatile unsigned int *)GPIO_DATA_IN_ADDR);;
	if(rcv_bit != (1<<GPIO_IN_PIPE_NUM)){
		goto out;
	}
	if(g_comm_para.stop_num == 2){
		udelay(rcv_cycle);
		rcv_bit = *((volatile unsigned int *)GPIO_DATA_IN_ADDR);;
		if(rcv_bit != (1<<GPIO_IN_PIPE_NUM)){
			goto out;
		}	
	}
	
	gpio_serial->in_buff[gpio_serial->in_head] = rcv_data[k];
	gpio_serial->in_head = (gpio_serial->in_head + 1)%4096;
	
	if(k!=READ_BUF_SIZE-1)
		udelay(rcv_cycle-5);
}

out:
	tasklet_schedule(&rcv_data_tasklet);
	local_irq_enable();
	return IRQ_HANDLED;
}

static void hi_gpio_vcom_set_termios(struct tty_struct * tty, struct termios *old_termios)
{
	unsigned int cflag;
	cflag = tty->termios->c_cflag;
	/*check para if or not changed*/
	if(old_termios){
		if((cflag == old_termios->c_cflag) && 
			(RELEVANT_IFLAG(tty->termios->c_iflag) == RELEVANT_IFLAG(old_termios->c_iflag))){
			return;
		}else{
			printk("com parameter be changed,%d\n", 1);
		}
	}
	
	/*get the data bits length*/
	switch(cflag & CSIZE){
		case CS5:
			printk("data bits num = %d\n", 5);
			g_comm_para.data_num = 5;
			break;
		case CS6:
			printk("data bits num = %d\n", 6);
			g_comm_para.data_num = 6;
			break;
		case CS7:
			printk("data bits num = %d\n", 7);
			g_comm_para.data_num = 7;
			break;
		case CS8:
			printk("data bits num = %d\n", 8);
			g_comm_para.data_num = 8;
			break;
		default:
			printk("data bits num = %d\n", 8);	
			g_comm_para.data_num = 8;
			break;		
	}
	
	/*get the check bit value*/
	if(cflag & PARENB){
		if(cflag & PARODD){
			printk("Check flag = %d\n", 1);
			g_comm_para.check_flag = 1;
		}
		else{
			printk("Check flag = %d\n", 2);
			g_comm_para.check_flag = 2;		
		}
	}
	else	{
		printk("Check flag = %d\n", 3);
		g_comm_para.check_flag = 3;
	}
	
	/*get the stop bits length*/
	if(cflag & CSTOPB){
		printk("stop bits length  = %d\n", 2);
		g_comm_para.stop_num = 2;
	}
	else{
		printk("stop bits length = %d\n", 1);
		g_comm_para.stop_num = 1;
	}
	
	g_comm_para.in_baud = tty_get_baud_rate(tty);
	g_comm_para.out_baud = tty_get_baud_rate(tty);		
	printk("communication baud = %d\n", g_comm_para.in_baud);
}

static int gpio_send_data(void)
{
	int k = 0;
	/*get the serial communication parameter*/
	unsigned int send_cycle = 1000*1000/g_comm_para.out_baud;	/*unit:us*/
	unsigned int check_bit = 0;
	unsigned int send_bit;
	unsigned int send_data;
	int ret = 0;
	
	if(!gpio_serial){
		return ret;
	}

	while(1){
		down(&gpio_serial->sem);
		if(gpio_serial->head==gpio_serial->tail){/*no data in circle buffer*/
			up(&gpio_serial->sem);
			break;
		}
		else {
			send_data = gpio_serial->cir_buff[gpio_serial->tail];
			ret++;
			gpio_serial->tail = (gpio_serial->tail+1)%512;
			up(&gpio_serial->sem);
		}
			
		local_irq_disable();
		/*send start bit*/
		udelay(send_cycle);
		*((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = 0x00;

		for(k= 0; k < g_comm_para.data_num; k++){
			send_bit = send_data & 0x01;
			udelay(send_cycle);
			if(send_bit == 0x01)
				*((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = (1<<GPIO_OUT_PIPE_NUM);
			else
				*((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = 0x0;
			send_data = send_data >> 1;
		}
		
		/*send the check bit*/
		if(g_comm_para.check_flag != 3){
			if(g_comm_para.check_flag == 1)		/*odd check*/
				check_bit = (check_bit + 1)%2;
			else		/*even check*/
				check_bit = check_bit%2;
				
			udelay(send_cycle);
			if(check_bit == 0x01)
	                        *((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = (1<<GPIO_OUT_PIPE_NUM);
	                else
	                        *((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = 0x0;
		}
		
		/*send the stop bit*/
		udelay(send_cycle);
		*((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = (1<<GPIO_OUT_PIPE_NUM);
		/*send the second stop bit*/
		if(g_comm_para.stop_num == 2){
			udelay(send_cycle);
			*((volatile unsigned int *)GPIO_DATA_OUT_ADDR) = (1<<GPIO_OUT_PIPE_NUM);
		}
		local_irq_enable();
	}

	return ret;
}


static int hi_gpio_vcom_write(struct tty_struct * tty, const unsigned char *buf, int count)
{
	int retval = -EINVAL;
	
	if (gpio_serial == NULL)
		return retval;
	down(&gpio_serial->sem);
	if(gpio_serial->open_cnt <= 0){
		up(&gpio_serial->sem);
		return retval;	
	}
	
	if(gpio_serial->head + count >= 512){
		memcpy(&(gpio_serial->cir_buff[gpio_serial->head]), buf, 512 - gpio_serial->head);
		memcpy(&(gpio_serial->cir_buff[0]), (buf+512-gpio_serial->head), count+gpio_serial->head-512);
		gpio_serial->head = count+gpio_serial->head-512;
	}else{
		memcpy(&(gpio_serial->cir_buff[gpio_serial->head]), buf, count);
		gpio_serial->head += count;
	}
	up(&gpio_serial->sem);
	
	retval = gpio_send_data();
	
	return retval;
}

static int gpio_vcom_write_room(struct tty_struct *tty)
{
        int room = -EINVAL;
        if (gpio_serial == NULL)
	        return -ENODEV;
	down(&gpio_serial->sem);
	if(gpio_serial->open_cnt <= 0)
		goto exit;
	if(gpio_serial->head >= gpio_serial->tail)
		room = 512 - gpio_serial->head + gpio_serial->tail;
	else if(gpio_serial->head < gpio_serial->tail)
                room = gpio_serial->tail - gpio_serial->head;
exit:
	up(&gpio_serial->sem);
	return room;
}

static int hi_gpio_vcom_open(struct tty_struct *tty, struct file *filp)
{
	int ret = 0;

	if(!gpio_serial){
		/*the first time open this device*/
		gpio_serial = (struct gpio_vcom_serial *)kmalloc(sizeof(*gpio_serial), GFP_KERNEL);
		if(!gpio_serial)
			return -ENOMEM;
		init_MUTEX(&gpio_serial->sem);
		gpio_serial->open_cnt = 0;
		gpio_serial->head = 0; 
		gpio_serial->tail = 0;
		gpio_serial->cir_buff = (unsigned char *)vmalloc(512*sizeof(unsigned char));
		gpio_serial->p_tty = tty;
		gpio_serial->in_lock = SPIN_LOCK_UNLOCKED;
		gpio_serial->in_head = 0; 
		gpio_serial->in_tail = 0;
		gpio_serial->in_buff = (unsigned char *)vmalloc(4096*sizeof(unsigned char));
	}
	
	writel(0x00, GPIO_DIR_IN_ADDR);  
	writel(0x01<<GPIO_OUT_PIPE_NUM, GPIO_DIR_OUT_ADDR);
	/* set high value to out line */
	writel(0x01<<GPIO_OUT_PIPE_NUM, GPIO_DATA_OUT_ADDR);  
	
	down(&gpio_serial->sem);

	++gpio_serial->open_cnt;
	if(gpio_serial->open_cnt == 1){
		/*the first time open this device need request IRQ*/
		ret = request_irq(GPIO_COM_IRQ, gpio_com_interrupt, 0, "gpio_com", tty);
		if (ret) 
    	{
        	printk("gpio_com" " can't request irq(%d)\n", GPIO_COM_IRQ);
 		    return ret;
    	}
	}
	up(&gpio_serial->sem); 
	
	return ret;
}

static void hi_gpio_vcom_close(struct tty_struct *tty, struct file *filp)
{
	if (gpio_serial == NULL)
		return;
	down(&gpio_serial->sem);
	if(gpio_serial->open_cnt == 0){
		goto exit;
	}
	
	--gpio_serial->open_cnt;
	if(gpio_serial->open_cnt == 0){
		free_irq(GPIO_COM_IRQ, tty);
		vfree(gpio_serial->cir_buff);
		vfree(gpio_serial->in_buff);
	}
	exit:
		up(&gpio_serial->sem);
}

static int gpio_vcom_chars_in_buffer(struct tty_struct *tty)
{
        int room = -EINVAL;
        if (gpio_serial == NULL)
	        return -ENODEV;
	down(&gpio_serial->sem);
	if(gpio_serial->open_cnt <= 0)
		goto exit;
	room = (512+gpio_serial->head-gpio_serial->tail)%512;
exit:
	up(&gpio_serial->sem);
	return room;
	
}

static struct tty_operations gpio_vcom_ops = {
	.open	= hi_gpio_vcom_open,
	.close	= hi_gpio_vcom_close,
	.write	= hi_gpio_vcom_write,
	.write_room	= gpio_vcom_write_room,
	.set_termios	= hi_gpio_vcom_set_termios,
	.chars_in_buffer = gpio_vcom_chars_in_buffer
};

static struct tty_driver *gpio_vcom_tty_drv = NULL;

static int __init vcom_probe(struct tty_driver *tty_drv)
{
	int ret = 0;
	int i;

	for(i=0; i<GPIO_VCOM_TTY_NR; i++) {
		VCOM_DEBUG(1, "%d", i);
		tty_register_device(tty_drv, i, NULL);
	}

	return ret;
}

static void vcom_remove_ttys(struct tty_driver *tty_drv)
{
	int i;

	for(i=0; i<GPIO_VCOM_TTY_NR; i++) {
		VCOM_DEBUG(1, "%d", i);
		tty_unregister_device(tty_drv, i);
	}
}


static int __init hi_gpio_tty_init(void)
{
	int ret = 0;
	/* create tty driver object */
	gpio_vcom_tty_drv = alloc_tty_driver(GPIO_VCOM_TTY_NR);
	if(!gpio_vcom_tty_drv)
		return -ENOMEM;
	gpio_vcom_tty_drv->magic = TTY_DRIVER_MAGIC;
	gpio_vcom_tty_drv->owner = THIS_MODULE;
	gpio_vcom_tty_drv->driver_name = "gpio_tty";
	gpio_vcom_tty_drv->name = "tty_gpio_vcom";
	gpio_vcom_tty_drv->devfs_name = "ttyGPIO";
	gpio_vcom_tty_drv->major = GPIO_VCOM_MAJOR;
	gpio_vcom_tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	gpio_vcom_tty_drv->subtype = SERIAL_TYPE_NORMAL;
	gpio_vcom_tty_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
	gpio_vcom_tty_drv->init_termios = tty_std_termios;
	gpio_vcom_tty_drv->init_termios.c_cflag = B19200 | CS8 | CREAD | HUPCL |CLOCAL;
	
	tty_set_operations(gpio_vcom_tty_drv, &gpio_vcom_ops);
	
	ret = tty_register_driver(gpio_vcom_tty_drv);

	if(ret == 0) {
		ret = vcom_probe(gpio_vcom_tty_drv);
		if(ret) {
			vcom_remove_ttys(gpio_vcom_tty_drv);
		}
	}

	if(ret) {
		tty_unregister_driver(gpio_vcom_tty_drv);
		put_tty_driver(gpio_vcom_tty_drv);
	}
	/*initialize the in and out port value and */
	writel(0x00, GPIO_DIR_IN_ADDR);  
	writel(0x01<<GPIO_OUT_PIPE_NUM, GPIO_DIR_OUT_ADDR);
	writel(0x01<<GPIO_IN_PIPE_NUM, GPIO_INT_SET_ADDR1);		/*µçÆ½´¥·¢*/
	writel(0x00<<GPIO_IN_PIPE_NUM, GPIO_INT_SET_ADDR2);		
	writel(0x01<<GPIO_IN_PIPE_NUM, GPIO_INT_IE_ADDR);
	
	/* set high value to out line */
	writel(0x04, GPIO_DATA_OUT_ADDR);  
	return ret;
}	

static void __exit hi_gpio_tty_exit(void)
{
	vcom_remove_ttys(gpio_vcom_tty_drv);
	tty_unregister_driver(gpio_vcom_tty_drv);

}

module_init(hi_gpio_tty_init);
module_exit(hi_gpio_tty_exit);

MODULE_AUTHOR("weimaoxi");
MODULE_DESCRIPTION("Hisilicon virtual serial port over gpio");
MODULE_LICENSE("GPL");

