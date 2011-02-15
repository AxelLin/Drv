
/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   pal.h
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the paltform/kernel version specific code.

    REVISION HISTORY:

***************************************************************/
#ifndef _PAL_H_
#define _PAL_H_

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/bitops.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/interrupt.h>	/* for in_interrupt() */
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/vmalloc.h>
//#include <linux/wrapper.h>

#ifdef CONFIG_USB_DEBUG
# define DEBUG
#else
# undef DEBUG
#endif

#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/io.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)	/* ===> Linux < 2.2 */
# error Kernel versions lower than 2.2 not supported by this driver
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)	/* ===> Linux 2.2 */

# define MODULE_OWNER_ENTRY	/**/
# define __devinit		/**/
# define __devexit		/**/
# define list_del_init(e)	do { \
					__list_del((e)->prev, (e)->next); \
					INIT_LIST_HEAD(e); \
				} while (0)
#else						/* ===> Linux 2.3 - 2.5 */

# define MODULE_OWNER_ENTRY	THIS_MODULE,

#endif	/* Linux 2.2 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* ===> Linux 2.2 - 2.4 */

# define NULL_MODULE_OWNER	/**/
# define minor(x)		MINOR(x)
# ifndef min_t
#  define min_t(type,x,y) ({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#  define min_t(type,x,y) ({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })
# endif

#else						/* ===> Linux >= 2.5 */

# define NULL_MODULE_OWNER	NULL,

#endif	/* Linux 2.2 - 2.4 */

/************************************************************************/

/* Base address and size of the USB-OTG controller registers */
#ifndef EXC_PLD_BLOCK0_BASE
# define EXC_PLD_BLOCK0_BASE	0x0f000000 // 0x80000000 //0x0f000000
# define EXC_PLD_BLOCK0_SIZE	0x00080000 // 0x00004000 //0x8000
# define EXC_PLD_BLOCK1_BASE    0x0f080000 // 0x80004000 //0x0f008000
# define EXC_PLD_BLOCK1_SIZE	0x00080000 // 0x00004000 //0x8000
#endif

#endif

