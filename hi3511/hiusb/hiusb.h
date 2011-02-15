#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#ifndef __HISILICON_USB_H
#define __HISILICON_USB_H

/* hiusb device name, such as platform device name */
#define HIUSB_HCD_DEV_NAME	"hiusb-hcd"
#define REG_BASE_HIUSB		0x80090000
#define REG_HIUSB_IOSIZE	0x1000
#define INTNR_HIUSB		23

#define HIUSB_DEBUG		1

#ifdef HIUSB_DEBUG	
#define hiusb_dump_buf(buf, len) do{\
	int i;\
	char *p = (void*)(buf);\
	printk("%s->%d:\n", __FUNCTION__, __LINE__); \
	for(i=0;i<(len);i++){\
		printk("0x%.2x ", *(p+i));\
		if( !((i+1) & 0x0F) )\
			printk("\n");\
	}\
	printk("\n");\
}while(0)

#define HIUSB_TRACE_LEVEL 18
#define HIUSB_TRACE_FMT KERN_INFO
//#define HIUSB_TRACE_FMT KERN_DEBUG
#define hiusb_trace(level, msg...) do { \
	if((level) >= HIUSB_TRACE_LEVEL) { \
		printk(HIUSB_TRACE_FMT "hiusb_trace:%s:%d: ", __FUNCTION__, __LINE__); \
		printk(msg); \
		printk("\n"); \
	} \
}while(0)

#define hiusb_assert(cond) do{ \
	if(!(cond)) {\
		printk("Assert:hiusb:%s:%d\n", \
				__FUNCTION__, \
				__LINE__); \
		BUG(); \
	}\
}while(0)
#else
#define hiusb_dump_buf(buf, len)
#define hiusb_trace(level, msg...) 
#define hiusb_assert(cond) 
#endif

#define hiusb_error(s...) do{ \
		printk(KERN_ERR "hiusb:%s:%d: ", __FUNCTION__, __LINE__); \
		printk(s); \
		printk("\n"); \
	}while(0)

/* Error number */
#define HIUSB_HCD_E_BUG		(0x00000001)
#define HIUSB_HCD_E_NOTSUPPORT	(0x00000002)
#define HIUSB_HCD_E_PROTOCOLERR	(0x00000004)
#define HIUSB_HCD_E_BUSY	(0x00000008)
#define HIUSB_HCD_E_NOTRESPONE	(0x00000010)
#define HIUSB_HCD_E_STALL	(0x00000020)
#define HIUSB_HCD_E_NAKTIMEOUT	(0x00000040) /*restoreable error*/
#define HIUSB_HCD_E_BUSERROR	(0x00000080) /*restoreable error*/
#define HIUSB_HCD_E_HWERROR	(0x00000100) /*restoreable error*/

struct hiusb_qtd {
	struct urb			*urb;
	unsigned int			actual;
	struct list_head		qtd_list;

#define HIUSB_CTRLPIEP_STAGE_SETUP	0
#define HIUSB_CTRLPIEP_STAGE_DATA	1
#define HIUSB_CTRLPIEP_STAGE_HANDSHAKE	2
	int				stage;
};

struct hiusb_hcd {
	spinlock_t			lock;
	unsigned long			lockflags;

	unsigned long			iobase;		/* virtual io addr */
	unsigned long			iobase_phys;	/* physical io addr */

	int				rh_port_change;	/* roothub port change flag */
	int				rh_port_connect;/* roothub port connect flag */
	int				rh_port_speed;	/* roothub port speed */
	unsigned long			rh_port_reset;	/* roothub port reset flag */

#define HIUSB_HCD_URB_PENDING		0
	unsigned long			pending;

#define MAX_QH_LIST			((16<<2) - 1)
	struct list_head		qh_list[MAX_QH_LIST];
	int				qh_index;

	struct tasklet_struct		bh_isr;

	struct workqueue_struct		*wq_scheduler;
	struct work_struct		wks_scheduler;

	struct timer_list		watch_dog_timer;
	int				watch_dog_timeval;
	int				watch_dog_reset;

	int				ctrlpipe_nak_limit;
	int				bulkpipe_nak_limit;

	int				intrtx;
	int				intrrx;
	int				intrusb;

	int				intrtx_mask;
	int				intrrx_mask;
	int				intrusb_mask;
};

/* read/write IO */
#define hiusb_readb(ld, ofs) ({ unsigned long reg=readb((ld)->iobase + (ofs)); \
				hiusb_trace(2, "readb(0x%04X) = 0x%08lX", (ofs), reg); \
				reg; })
#define hiusb_writeb(ld, v, ofs) do{ writeb((v), (ld)->iobase + (ofs)); \
				hiusb_trace(2, "writeb(0x%04X) = 0x%08lX", (ofs), (unsigned long)(v)); \
			}while(0)

#define hiusb_readw(ld, ofs) ({ unsigned long reg=readw((ld)->iobase + (ofs)); \
				hiusb_trace(2, "readw(0x%04X) = 0x%08lX", (ofs), reg); \
				reg; })
#define hiusb_writew(ld, v, ofs) do{ writew((v), (ld)->iobase + (ofs)); \
				hiusb_trace(2, "writew(0x%04X) = 0x%08lX", (ofs), (unsigned long)(v)); \
			}while(0)

#define hiusb_readl(ld, ofs) ({ unsigned long reg=readl((ld)->iobase + (ofs)); \
				hiusb_trace(2, "readl(0x%04X) = 0x%08lX", (ofs), reg); \
				reg; })
#define hiusb_writel(ld, v, ofs) do{ writel(v, (ld)->iobase + (ofs)); \
				hiusb_trace(2, "writel(0x%04X) = 0x%08lX", (ofs), (unsigned long)(v)); \
			}while(0)

#define device_lock_init(ld)	spin_lock_init(&(ld)->lock)
#define device_lock_exit(ld)	
#define device_lock(ld)		spin_lock_irqsave(&(ld)->lock, (ld)->lockflags)
#define device_unlock(ld)	spin_unlock_irqrestore(&(ld)->lock, (ld)->lockflags)

#define machine_lock_init(ld)
#define machine_lock_exit(ld)	
#define machine_lock(ld)	spin_lock_irqsave(&(ld)->lock, (ld)->lockflags)
#define machine_unlock(ld)	spin_unlock_irqrestore(&(ld)->lock, (ld)->lockflags)

#endif
