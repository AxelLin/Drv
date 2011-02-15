#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include "hiusb.h"
#include "ctrl.h"
#include "sys.h"

#define GPIO_SPACE_SIZE    0x1000
#define GPIO_DIR           0x0400 /* gpio direction register */
#define DIRECTION_OUTPUT   1
#define DIRECTION_INPUT    0

static void gpio_dir_set_bit(unsigned char bGpioPathNum, unsigned char bBitx, unsigned char bDirBit)
{
	int reg = readl(HIUSB_SYS_GPIO + bGpioPathNum * GPIO_SPACE_SIZE + GPIO_DIR);

	if(bDirBit == DIRECTION_OUTPUT){
		reg |= (DIRECTION_OUTPUT << bBitx);
		writel(reg, (HIUSB_SYS_GPIO + bGpioPathNum * GPIO_SPACE_SIZE + GPIO_DIR));
	}
	else{
		reg &= (!(1 << bBitx));
		writel(reg, (HIUSB_SYS_GPIO+(bGpioPathNum) * GPIO_SPACE_SIZE + GPIO_DIR));
	}
}

static void gpio_write_bit(unsigned char bGpioPathNum ,unsigned char bBitx ,unsigned char bBitValue)
{
	if (bBitValue == 1)
		writel((1<<(bBitx)), (HIUSB_SYS_GPIO + (bGpioPathNum) * GPIO_SPACE_SIZE + (4<<bBitx)));
	else if(bBitValue == 0)
		writel(0, (HIUSB_SYS_GPIO + (bGpioPathNum) * GPIO_SPACE_SIZE + (4<<bBitx)));
}

void sys_hiusb_vbus_power_supply()
{
	gpio_dir_set_bit(0, 1, DIRECTION_OUTPUT);
	gpio_write_bit(0, 1, 0);
}

void sys_hiusb_vbus_power_down()
{
	gpio_dir_set_bit(0, 1, DIRECTION_OUTPUT);
	gpio_write_bit(0, 1, 1);
}

void sys_hiusb_init()
{
	int reg;

	sys_hiusb_exit();
	
	/*improve usb data signal*/
	reg = readl(IO_ADDRESS(0x101e003c));
	reg &= ~0x1E;
	reg |= 0x14;
	writel(reg,IO_ADDRESS(0x101e003c));

	/* enable clock */	
	reg = readl(HIUSB_SYS_CLOCK_ENABLE);
	reg |= BIT_SYS_CLOCK;
	writel(reg, HIUSB_SYS_CLOCK_ENABLE);

	/* release reset */
	reg = readl(HIUSB_SYS_RESET);
	reg |= BIT_SYS_RESET;
	writel(reg, HIUSB_SYS_RESET);

	sys_hiusb_vbus_power_supply();
}

void sys_hiusb_exit()
{
	int reg ;

	sys_hiusb_vbus_power_down();

	/* disable clock */	
	reg = readl(HIUSB_SYS_CLOCK_DISENABLE);
	reg |= BIT_SYS_CLOCK;
	writel(reg, HIUSB_SYS_CLOCK_DISENABLE);

	/* hw reset */
	reg = readl(HIUSB_SYS_RESET);
	reg |= BIT_SYS_RESET;
	writel(reg, HIUSB_SYS_RESET);
}
