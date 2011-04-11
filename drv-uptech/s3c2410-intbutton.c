/*
 * s3c2410-zlg7289.c
 *
 * keyboard driver for SAMSUNG S3C2410 on interrupt 4-7
 *
 * Author: threewater <threewater@up-tech.com>
 * Date  : $Date: 2004/06/02 02:02:00 $ 
 *
 * $Revision: 1.1.0.0 $
 *
 * Based on s3c44b0-zlg7289.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * History:
 *
 * 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/arch/irqs.h>

/* debug macros */
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK( x... )	printk("s3c2410-kbd: " ##x)
#else
#define DPRINTK( x... )
#endif

//key board satus
#define KEYSTATUS_UP		0
#define KEYSTATUS_DOWN	1
#define KEYSTATUS_DOWNX	0xff	//maybe first down

#define KBD_TIMER_DELAY  (HZ/10) /* 100 ms */
#define KBD_TIMER_DELAY1  (HZ/100) /*the first key down delay 10 ms */
static struct timer_list kbd_timer;

#define KEY_NULL	0
#define KEY_DOWN	0x80
#define MAX_KBD_BUF	16	/* how many do we want to buffer */

#define DEVICE_NAME	"s3c2410-kbd"
#define KBDRAW_MINOR	1

typedef unsigned char KBD_RET;

typedef struct {
	unsigned int kbdStatus;		/*KEYSTATUS_UP KEYSTATUS_DOWN and KEYSTATUS_REPEAT*/
	KBD_RET buf[MAX_KBD_BUF];		/* protect against overrun */
	unsigned int head, tail;	/* head and tail for queued events */
	wait_queue_head_t wq;
	spinlock_t lock;
} KBD_DEV;

static KBD_DEV kbddev;

#define IRQ_KBD(n)		IRQ_EINT##n

#define KBD_OPEN_INT()	do{set_external_irq(IRQ_KBD(4), EXT_LOWLEVEL, GPIO_PULLUP_EN);\
							set_external_irq(IRQ_KBD(5), EXT_LOWLEVEL, GPIO_PULLUP_EN);\
							set_external_irq(IRQ_KBD(6), EXT_LOWLEVEL, GPIO_PULLUP_EN);\
							set_external_irq(IRQ_KBD(7), EXT_LOWLEVEL, GPIO_PULLUP_EN);}while(0)

#define KBD_CLOSE_INT()	do{set_gpio_ctrl(GPIO_MODE_IN | GPIO_PULLUP_EN | GPIO_F4);\
							set_gpio_ctrl(GPIO_MODE_IN | GPIO_PULLUP_EN | GPIO_F5);\
							set_gpio_ctrl(GPIO_MODE_IN | GPIO_PULLUP_EN | GPIO_F6);\
							set_gpio_ctrl(GPIO_MODE_IN | GPIO_PULLUP_EN | GPIO_F7);}while(0)
#define ISKBD_DOWN()		((GPFDAT & (0xf<<4)) != (0xf<<4))
#define Read_ExINT_Key()	((~GPFDAT)>>4) & 0xf

#define BUF_HEAD	(kbddev.buf[kbddev.head])
#define BUF_TAIL		(kbddev.buf[kbddev.tail])
#define INCBUF(x,mod) 	((++(x)) & ((mod) - 1))

static int kbdMajor = 0;

static void (*kbdEvent)(void);

static int key;	/*keyboard scan code*/

static inline void kbdEvent_raw(void)
{
	if (kbddev.kbdStatus == KEYSTATUS_DOWN ) {
		BUF_HEAD = key|KEY_DOWN;
	}
	else {
		BUF_HEAD = key;
	}

	kbddev.head = INCBUF(kbddev.head, MAX_KBD_BUF);
	wake_up_interruptible(&(kbddev.wq));
}

static KBD_RET kbdRead(void)
{
	KBD_RET  kbd_ret;
	spin_lock_irq(&(kbddev.lock));
	kbd_ret= BUF_TAIL;
	kbddev.tail = INCBUF(kbddev.tail, MAX_KBD_BUF);
	spin_unlock_irq(&(kbddev.lock));

	return kbd_ret;
}

static ssize_t s3c2410_kbd_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	KBD_RET kbd_ret;

retry: 
	if (kbddev.head != kbddev.tail) {
		kbd_ret = kbdRead();
		copy_to_user(buffer, (char *)&kbd_ret, sizeof(KBD_RET));
		return sizeof(KBD_RET);
	} else {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		interruptible_sleep_on(&(kbddev.wq));
		if (signal_pending(current))
			return -ERESTARTSYS;
		goto retry;
	}

	return sizeof(KBD_RET);
}

static unsigned int s3c2410_kbd_poll(struct file *filp, struct poll_table_struct *wait)
{
	poll_wait(filp, &(kbddev.wq), wait);
	return (kbddev.head == kbddev.tail) ? 0 : (POLLIN | POLLRDNORM); 
}

static  void s3c2410_get_kbd(void)
{

	key=Read_ExINT_Key();

#ifdef DEBUG
	if(kbddev.kbdStatus == KEYSTATUS_DOWN)
		DPRINTK("KEY DOWN, code=%x\n", key);
	else 
		DPRINTK("KEY REPEAT, code=%x\n", key);
#endif
	kbdEvent();
}

static void s3c2410_isr_kbd(int irq, void *dev_id, struct pt_regs *reg)
{

	spin_lock_irq(&(kbddev.lock));

	DPRINTK("Occured key board Interrupt, irq=%d\n", irq);

	if ((kbddev.kbdStatus == KEYSTATUS_UP)) {
		KBD_CLOSE_INT();
		udelay(1);
		if (ISKBD_DOWN()) {
			kbddev.kbdStatus = KEYSTATUS_DOWNX;
			kbd_timer.expires = jiffies + KBD_TIMER_DELAY1;
			add_timer(&kbd_timer);
		}
		else{
			KBD_OPEN_INT();
		}
	}
	spin_unlock_irq(&(kbddev.lock));
}

static void kbd_timer_handler(unsigned long data)
{
	spin_lock_irq(&(kbddev.lock));
	if (ISKBD_DOWN()) {
		if(kbddev.kbdStatus == KEYSTATUS_DOWNX){	//the key down
			kbddev.kbdStatus = KEYSTATUS_DOWN;
			kbd_timer.expires = jiffies + KBD_TIMER_DELAY;
			s3c2410_get_kbd();
		}
		else{	//kbddev.kbdStatus == KEYSTATUS_DOWN
			kbd_timer.expires = jiffies + KBD_TIMER_DELAY;
		}
		add_timer(&kbd_timer);
	}
	else{
		kbddev.kbdStatus = KEYSTATUS_UP;
		del_timer(&kbd_timer);

		DPRINTK("KEY UP, code=%x\n", key);
		kbdEvent();
		KBD_OPEN_INT();
	}
	spin_unlock_irq(&(kbddev.lock));
}

static int s3c2410_kbd_open(struct inode *inode, struct file *filp)
{
	kbddev.head = kbddev.tail = 0;
//	kbddev.kbdStatus = KEYSTATUS_UP;

#ifndef DEBUG
//	init_timer(&kbd_timer);
//	kbd_timer.function = kbd_timer_handler;
#endif
	kbdEvent = kbdEvent_raw;
//	init_waitqueue_head(&(kbddev.wq));

//	KBD_OPEN_INT();

	MOD_INC_USE_COUNT;
	return 0;
}

void kbdEvent_dummy(void) {}

static int s3c2410_kbd_release(struct inode *inode, struct file *filp)
{
//	del_timer(&kbd_timer);

//	KBD_CLOSE_INT();
	kbdEvent = kbdEvent_dummy;

	MOD_DEC_USE_COUNT;
	return 0;
}

static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	open:	s3c2410_kbd_open,
	read:	s3c2410_kbd_read,	
	release:	s3c2410_kbd_release,
	poll:	s3c2410_kbd_poll,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_kbd_dir, devfs_kbdraw;
#endif

static int __init s3c2410_kbd_init(void)
{
	int ret;

	set_external_irq(IRQ_KBD(4), EXT_LOWLEVEL, GPIO_PULLUP_EN);
	set_external_irq(IRQ_KBD(5), EXT_LOWLEVEL, GPIO_PULLUP_EN);
	set_external_irq(IRQ_KBD(6), EXT_LOWLEVEL, GPIO_PULLUP_EN);
	set_external_irq(IRQ_KBD(7), EXT_LOWLEVEL, GPIO_PULLUP_EN);

	kbdEvent = kbdEvent_dummy;

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	kbdMajor = ret;

        kbddev.head = kbddev.tail = 0;
        kbddev.kbdStatus = KEYSTATUS_UP;
        init_waitqueue_head(&(kbddev.wq));

        init_timer(&kbd_timer);
        kbd_timer.function = kbd_timer_handler;

//	KBD_CLOSE_INT();

	// Enable interrupt
	ret = request_irq(IRQ_KBD(4), s3c2410_isr_kbd, SA_INTERRUPT, DEVICE_NAME, s3c2410_isr_kbd);
	if (ret) 	return ret;
	ret = request_irq(IRQ_KBD(5), s3c2410_isr_kbd, SA_INTERRUPT, DEVICE_NAME, s3c2410_isr_kbd);
	if (ret) 	return ret;
	ret = request_irq(IRQ_KBD(6), s3c2410_isr_kbd, SA_INTERRUPT, DEVICE_NAME, s3c2410_isr_kbd);
	if (ret) 	return ret;
	ret = request_irq(IRQ_KBD(7), s3c2410_isr_kbd, SA_INTERRUPT, DEVICE_NAME, s3c2410_isr_kbd);
	if (ret) 	return ret;

	udelay(10);

#ifdef CONFIG_DEVFS_FS
	devfs_kbd_dir = devfs_mk_dir(NULL, "keyboard", NULL);
	devfs_kbdraw = devfs_register(devfs_kbd_dir, "0raw", DEVFS_FL_DEFAULT,
			kbdMajor, KBDRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR,
			&s3c2410_fops, NULL);
#endif

#ifdef DEBUG
	s3c2410_kbd_open(NULL, NULL);
#endif
	printk(DEVICE_NAME " initialized\n");

	return 0;
}

static void __exit s3c2410_kbd_exit(void)
{

	del_timer(&kbd_timer);

#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_kbdraw);
	devfs_unregister(devfs_kbd_dir);
#endif	
	unregister_chrdev(kbdMajor, DEVICE_NAME);

	free_irq(IRQ_KBD(4), s3c2410_isr_kbd);
	free_irq(IRQ_KBD(5), s3c2410_isr_kbd);
	free_irq(IRQ_KBD(6), s3c2410_isr_kbd);
	free_irq(IRQ_KBD(7), s3c2410_isr_kbd);
}

module_init(s3c2410_kbd_init);
module_exit(s3c2410_kbd_exit);
