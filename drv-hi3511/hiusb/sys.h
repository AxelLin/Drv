#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/usb.h>
#include <asm/io.h>

#ifndef __HIUSB_SYS_H
#define __HIUSB_SYS_H

#define HIUSB_SYS_REG_BASE		IO_ADDRESS(0x101e0000)
#define HIUSB_SYS_CLOCK_ENABLE		(HIUSB_SYS_REG_BASE + 0x24)
#define HIUSB_SYS_CLOCK_DISENABLE	(HIUSB_SYS_REG_BASE + 0x28)
#define HIUSB_SYS_RESET			(HIUSB_SYS_REG_BASE + 0x1c)
#define BIT_SYS_RESET			(1U<<21)
#define BIT_SYS_CLOCK			(1U<<20)

#define HIUSB_SYS_GPIO			IO_ADDRESS(0x101f9000)

void sys_hiusb_vbus_power_supply(void);
void sys_hiusb_vbus_power_down(void);

void sys_hiusb_init(void);
void sys_hiusb_exit(void);

#endif
