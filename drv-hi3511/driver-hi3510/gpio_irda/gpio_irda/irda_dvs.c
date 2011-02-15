#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>

#include  "hi_gpio.h"
#include  "irda_dvs.h"

static Irkey_Dev irkey_t;

#define BUF_HEAD irkey_t.buf[irkey_t.head]
#define BUF_TAIL irkey_t.buf[irkey_t.tail]
#define INC_BUF(x,len) ((++(x))&(len-1))  

#define IN_MODE   0
#define INT_MODE  1
 

void ir_gpio_read(void)
{
    unsigned int ret ;
    gpio_dirgetbyte(GPIO_2,&ret);
    printk("GPIO_1_DIR=0X%x\n",ret);
    ret=readw(GPIO_2_BASE+GPIO_DIR);
    printk("PIO_2_BASE+GPIO_DIR=0x%x\n",ret);
    ret=readw(GPIO_2_BASE+GPIO_IS);
    printk("GPIO_2_BASE+GPIO_IS=0x%x\n",ret);
    ret=readw(GPIO_2_BASE+GPIO_IE);
    printk("GPIO_2_BASE+GPIO_IE=0x%x\n",ret);
    ret=readw(GPIO_2_BASE+GPIO_IBE);
    printk("GPIO_2_BASE+GPIO_IBE=0x%x\n",ret);

    ret= readw(GPIO_BASE+0x2000+GPIO_DIR);
    printk("PIO_2_BASE+GPIO_DIR=0x%x\n",ret);

}

static int irkey_port_init(unsigned int port_mode)
{
    if(port_mode == IN_MODE)
    {
        gpio_interruptclear(IRKEY_PORT, IRKEY_RX_PIN);
        gpio_interruptdisable(IRKEY_PORT,IRKEY_RX_PIN);
        gpio_dirsetbit(IRKEY_PORT,IRKEY_RX_PIN,DIRECTION_INPUT);
    }
    else if( port_mode == INT_MODE)
    {
       // gpio_dirsetbit(IRKEY_PORT,IRKEY_RX_PIN, DIRECTION_INPUT);
       // gpio_dirsetbit(IRKEY_PORT,3, DIRECTION_OUTPUT);
     //  printk("here\n");
        //gpio_dirsetbyte(GPIO_2,0xdf);       
        writew(0xdf,(GPIO_BASE+0x2000+GPIO_DIR));
    //    writew(0xdf,(GPIO_BASE+0x1000+GPIO_DIR));
        gpio_interruptset(
                              IRKEY_PORT, 
                              IRKEY_RX_PIN, 
                              SENSE_BOTH, 
                              SENSE_EDGE, 
                              EVENT_FALLING_EDGE
                             );  
        gpio_interruptenable(IRKEY_PORT, IRKEY_RX_PIN);
    }
    return 0;
}



static int irkey_start(void)
{
    init_waitqueue_head(&(irkey_t.irkey_wait));
    irkey_port_init(INT_MODE);
    return 0;
}


static int read_irkey(Irkey_Info *irkey_to_user)
{
    irkey_to_user->sys_id_code =BUF_TAIL.sys_id_code;
        //printk("sys_id0x%x\n",irkey_to_user->sys_id_code);
    irkey_to_user->irkey_code =BUF_TAIL.irkey_code;
        //printk("irkey_id0x%x\n",irkey_to_user->irkey_code);
    irkey_to_user->irkey_mask_code =BUF_TAIL.irkey_mask_code;
        //printk("mask_id0x%x\n",irkey_to_user->irkey_mask_code);
    irkey_t.tail=INC_BUF(irkey_t.tail,MAX_BUF);
    return sizeof (Irkey_Info);
}



static   irqreturn_t hi_irkey_interrupt (int irq, void * dev_id, struct pt_regs *regs)
{
     int handled;
     unsigned int key_code;
     unsigned short sys_id;
     unsigned short irkey;
     unsigned short irkey_mask;
     static unsigned int count =0;
     unsigned int i;
     static unsigned int start;
     unsigned int current_irq;
     current_irq=readw(IRKEY_PORT_ADDR+GPIO_MIS);
     if(((current_irq)&(1<<IRKEY_RX_PIN))!=(1<<IRKEY_RX_PIN))goto  no_find_irq;
     spin_lock_irq(& irkey_t.irkey_lock);
     irkey_port_init(IN_MODE);
     key_code=0x00000000;
     if(((readw(IRKEY_RX_ADDR))&(IRKEY_START_MASK))!=IRKEY_START_MASK)
     {  
         for(i=0;i<8;i++)
         {
            mdelay(1);
            if(((readw(IRKEY_RX_ADDR))&(IRKEY_START_MASK))==IRKEY_START_MASK)goto out ;
            start=1;
         }
        while((readw(IRKEY_RX_ADDR)&(IRKEY_START_MASK))!=IRKEY_START_MASK);
        mdelay(5);
        while(count <32)
        {
            while((readw(IRKEY_RX_ADDR)&(IRKEY_START_MASK))!=IRKEY_START_MASK);
            udelay(850);
            key_code=key_code <<1;
            if(((readw(IRKEY_RX_ADDR))&(IRKEY_START_MASK))!=IRKEY_START_MASK)key_code =  key_code & 0xfffffffe;
            else 
            {
                key_code =  key_code |0x00000001;
                while((readw(IRKEY_RX_ADDR)&(IRKEY_START_MASK))==IRKEY_START_MASK); 
            }
            count++ ;
        }
         if(key_code!=0xffffffff)
         {
            while((key_code & 0x80000000)==0x80000000)key_code=(key_code<<1)|0x01;
         }
        sys_id=(key_code )  >> 16 ;
        //printk("sys_id0x%x\n",sys_id);
        irkey =((key_code << 16) & 0xff000000) >> 24;
       // printk("irkey0x%x\n",irkey);
        irkey_mask=(key_code & 0x000000ff);
       // printk("irkey_mask0x%x\n",irkey_mask);
        if(((irkey) & (irkey_mask))==0)
        {
            BUF_HEAD.sys_id_code=sys_id;
            BUF_HEAD.irkey_code=irkey;
            BUF_HEAD.irkey_mask_code=irkey_mask;
            irkey_t.head = INC_BUF(irkey_t.head,MAX_BUF);
        }
out :
        count=0;
        irkey_port_init(INT_MODE);
        spin_unlock_irq(& irkey_t.irkey_lock);
        handled=1;
       return  IRQ_RETVAL(handled);
    }
no_find_irq:
    irkey_port_init(INT_MODE);
    spin_unlock_irq(& irkey_t.irkey_lock);
    handled=1;
    return IRQ_RETVAL(handled);
}



static ssize_t hi_irkey_read(struct file *filp, char *buf ,size_t count,loff_t * ppos)
{   
     Irkey_Info irkey_to_user;
retry :
    if((irkey_t.head)==(irkey_t.tail))
    {
        if((filp->f_flags & O_NONBLOCK) == O_NONBLOCK)return -EAGAIN;
        interruptible_sleep_on(&(irkey_t.irkey_wait));
        if (signal_pending(current))return -ERESTARTSYS;
        goto retry;
    }
    else
    {
        int count ;
        count = read_irkey(&irkey_to_user);
        copy_to_user((struct Irkey_Info *)buf,&irkey_to_user,count);
    }
    return sizeof(Irkey_Info);
    
}




static unsigned int hi_irkey_select(struct file *file, struct poll_table_struct *wait)
{
        printk("poll ok\n");
    if((irkey_t.head)!=(irkey_t.tail)) {
        printk("poll ok\n");
        return 1;
    }
    poll_wait(file,&(irkey_t.irkey_wait),wait);
    return 0;
}




static int hi_irkey_open(struct inode *inode,struct file *filp )
{
    return irkey_start();
    
}



static int hi_irkey_release(struct inode *inode , struct file *filp)
{
    gpio_interruptdisable(IRKEY_PORT, IRKEY_RX_PIN);
    return 0;
}




static struct file_operations  hi_irkey_fops =
{
    owner   : THIS_MODULE,
    open    : hi_irkey_open,
    ioctl   :  NULL,
    poll    : hi_irkey_select,
    read    : hi_irkey_read,
    release : hi_irkey_release,
};


static int __init hi_irkey_init(void)
{
    int ret ;
    gpio_remap();
    ret = register_chrdev(DEVICE_MAJOR,DEVICE_NAME,&hi_irkey_fops);
    if(ret < 0)
    {
        printk(DEVICE_NAME"can't register\n");
        return -EAGAIN;
    }
    ret = request_irq(DEVICE_IRQ_NO,&hi_irkey_interrupt,SA_SHIRQ|SA_INTERRUPT,DEVICE_NAME,&hi_irkey_interrupt);
    if(ret)
    {
        printk(DEVICE_NAME"request IRQ(%d) failed\n",DEVICE_IRQ_NO);
        goto request_irq_failed;
    }
    printk("DVS IR_Remote Device Driver v1.0\n");
    //irkey_start();
        gpio_interruptdisable(IRKEY_PORT, IRKEY_RX_PIN);
    //ir_gpio_read();
    return 0;
    request_irq_failed:

                    unregister_chrdev(DEVICE_MAJOR,DEVICE_NAME);
    return ret;
}


static void __exit hi_irkey_exit(void)
{
    gpio_unmap();
    free_irq(DEVICE_IRQ_NO,&hi_irkey_interrupt);
    unregister_chrdev(DEVICE_MAJOR,DEVICE_NAME);
}

module_init(hi_irkey_init);
module_exit(hi_irkey_exit);
MODULE_LICENSE("GPL");
