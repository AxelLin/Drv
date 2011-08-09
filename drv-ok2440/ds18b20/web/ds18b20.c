/************************* s3c2440_ds18b20.c文件开始 **************************/
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

typedef unsigned char BYTE;

#define DS18B20_PIN   S3C2410_GPB1
#define DS18B20_PIN_OUTP S3C2410_GPB1_OUTP
#define DS18B20_PIN_INP   S3C2410_GPB1_INP
#define HIGH 1
#define LOW 0
#define DEV_NAME "DS18B20"
#define DEV_MAJOR 232
static BYTE data[2];

// DS18B20复位函数
BYTE DS18b20_reset (void)
{
    // 配置GPIOB0输出模式
    s3c2410_gpio_cfgpin(DS18B20_PIN, DS18B20_PIN_OUTP);
    
    // 向18B20发送一个上升沿，并保持高电平状态约100微秒
    s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
    udelay(100);
    
    // 向18B20发送一个下降沿，并保持低电平状态约600微秒
    s3c2410_gpio_setpin(DS18B20_PIN, LOW);
    udelay(600);
    
    // 向18B20发送一个上升沿，此时可释放DS18B20总线
    s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
    udelay(100);
    
    // 以上动作是给DS18B20一个复位脉冲
    // 通过再次配置GPIOB1引脚成输入状态，可以检测到DS18B20是否复位成功
    s3c2410_gpio_cfgpin(DS18B20_PIN, DS18B20_PIN_INP);
    
    // 若总线在释放后总线状态为高电平，则复位失败
    if(s3c2410_gpio_getpin(DS18B20_PIN)){ printk("DS18b20 reset failed.\r\n"); return 1;}

    return 0;
}


void DS18b20_write_byte (BYTE byte)
{
    BYTE i;
    // 配置GPIOB1为输出模式
    s3c2410_gpio_cfgpin(DS18B20_PIN, DS18B20_PIN_OUTP);

    // 写“1”时隙：
    //     保持总线在低电平1微秒到15微秒之间
    //     然后再保持总线在高电平15微秒到60微秒之间
    //     理想状态: 1微秒的低电平然后跳变再保持60微秒的高电平
    //
    // 写“0”时隙：
    //     保持总线在低电平15微秒到60微秒之间
    //     然后再保持总线在高电平1微秒到15微秒之间
    //     理想状态: 60微秒的低电平然后跳变再保持1微秒的高电平
    for (i = 0; i < 8; i++)
    {
        s3c2410_gpio_setpin(DS18B20_PIN, LOW); udelay(1);
        if(byte & HIGH)
        {
             // 若byte变量的D0位是1，则需向总线上写“1”
             // 根据写“1”时隙规则，电平在此处翻转为高
             s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
        }
        else 
        {
             // 若byte变量的D0位是0，则需向总线上写“0”
             // 根据写“0”时隙规则，电平在保持为低
             // s3c2410_gpio_setpin(DS18B20_PIN, LOW);
        }
        // 电平状态保持60微秒
        udelay(60);

        s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
        udelay(15);

        byte >>= 1;
    }
    s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
}

BYTE DS18b20_read_byte (void)
{
    BYTE i = 0;
    BYTE byte = 0;
    // 读“1”时隙：
    //     若总线状态保持在低电平状态1微秒到15微秒之间
    //     然后跳变到高电平状态且保持在15微秒到60微秒之间
    //      就认为从DS18B20读到一个“1”信号
    //     理想情况: 1微秒的低电平然后跳变再保持60微秒的高电平
    //
    // 读“0”时隙：
    //     若总线状态保持在低电平状态15微秒到30微秒之间
    //     然后跳变到高电平状态且保持在15微秒到60微秒之间
    //     就认为从DS18B20读到一个“0”信号
    //     理想情况: 15微秒的低电平然后跳变再保持46微秒的高电平
    for (i = 0; i < 8; i++)
    {
        s3c2410_gpio_cfgpin(DS18B20_PIN, DS18B20_PIN_OUTP); 
        s3c2410_gpio_setpin(DS18B20_PIN, LOW);

        udelay(1);
        byte >>= 1;

        s3c2410_gpio_setpin(DS18B20_PIN, HIGH);
        s3c2410_gpio_cfgpin(DS18B20_PIN, DS18B20_PIN_INP);

        // 若总线在我们设它为低电平之后若1微秒之内变为高
        // 则认为从DS18B20处收到一个“1”信号
        // 因此把byte的D7为置“1”
        if (s3c2410_gpio_getpin(DS18B20_PIN)) byte |= 0x80;
        udelay(60);
    }
    return byte;       
}

void DS18b20_proc(void)         
{
    while(DS18b20_reset());
    
    udelay(120);
    
    DS18b20_write_byte(0xcc);
    DS18b20_write_byte(0x44);
    
    udelay(5);
    
    while(DS18b20_reset());
    udelay(200);
    
    DS18b20_write_byte(0xcc);
    DS18b20_write_byte(0xbe);
    
    data[0] = DS18b20_read_byte();
    data[1] = DS18b20_read_byte();
}

static ssize_t s3c2440_18b20_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    DS18b20_proc();

    buf[0] = data[0];
    buf[1] = data[1];
    
    return 1;
}

static struct file_operations s3c2440_18b20_fops = 
{
    .owner = THIS_MODULE,
    .read = s3c2440_18b20_read,
};

static int __init s3c2440_18b20_init(void)
{
    if (register_chrdev(DEV_MAJOR, DEV_NAME, &s3c2440_18b20_fops) < 0)
    {
        printk(DEV_NAME ": Register major failed.\r\n");
        return -1;
    }
    
    devfs_mk_cdev(MKDEV(DEV_MAJOR, 0),S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP, DEV_NAME);
    
    while(DS18b20_reset());   
}

static void __exit s3c2440_18b20_exit(void)
{
    devfs_remove(DEV_NAME);
    unregister_chrdev(DEV_MAJOR, DEV_NAME);
}
module_init(s3c2440_18b20_init);
module_exit(s3c2440_18b20_exit);

/************************* s3c2440_ds18b20.c文件结束 **************************/
