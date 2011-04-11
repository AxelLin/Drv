/*
 * s3c2410-kbd-zlg7290.c
 *
 * touchScreen driver for SAMSUNG S3C2410
 *
 * Author: threewater <threewater@up-tech.com>
 * Date  : $Date: 2004/06/15 17:25:00 $ 
 *
 * $Revision: 1.1.2.6 $
 *
 * Based on s3c2410-ts-ads7843.c
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
#include <linux/irq.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/arch/iic.h>

/* debug macros */
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK( x... )	printk("s3c2410-kbd: " ##x)
#else
#define DPRINTK( x... )
#endif

#define KEY_NULL	0
#define KEY_DOWN	0x80
#define MAX_KBD_BUF	16	/* how many do we want to buffer */

#define DEVICE_NAME		"s3c44b0-kbd"
#define KBDRAW_MINOR	1

#define ZLG7289_IICCON		(IICCON_ACKEN |IICCON_CLK512 | IICCON_INTR | IICCON_CLKPRE(0x3))

typedef unsigned char KBD_RET;

typedef struct {
	unsigned int kbdmode;		/*KEYSTATUS_UP KEYSTATUS_DOWN and KEYSTATUS_REPEAT*/
	KBD_RET buf[MAX_KBD_BUF];		/* protect against overrun */
	unsigned int head, tail;		/* head and tail for queued events */
	wait_queue_head_t wq;
	spinlock_t lock;
	unsigned char key;
} KBD_DEV;

static KBD_DEV kbddev;

typedef struct{
	unsigned char key;
	unsigned char RepeatCnt;
	unsigned char Funckey;
}Zlg7290Key;

#define IRQ_KBD		IRQ_EINT4

#define KBD_OPEN_INT()		
#define KBD_CLOSE_INT()		

#define ZLG7290_ADDR			0x70

#define KEYMODE_REPEAT			0
#define KEYMODE_NOREPEAT		1

#define ZLG7290_SysReg			0
#define ZLG7290_Key				1
#define ZLG7290_RepeatCnt		2
#define ZLG7290_FunctionKey		3

#define BUF_HEAD	(kbddev.buf[kbddev.head])
#define BUF_TAIL	(kbddev.buf[kbddev.tail])
#define INCBUF(x,mod) 	((++(x)) & ((mod) - 1))

static int kbdMajor = 0;

static KBD_RET kbdRead(void)
{
	KBD_RET  kbd_ret;
	spin_lock_irq(&(kbddev.lock));
	kbd_ret= BUF_TAIL;
	kbddev.tail = INCBUF(kbddev.tail, MAX_KBD_BUF);
	spin_unlock_irq(&(kbddev.lock));

	return kbd_ret;
}

static ssize_t s3c2410_kbd_write(struct file *file, const char *buffer, size_t count, loff_t * ppos)
{
	int data;

	if(count!=sizeof(data)){
		//error input data size
		DPRINTK("the size of  input data must be %d\n", sizeof(data));
		return 0;
	}

	copy_from_user(&data, buffer, count);
	if(data==0)
		kbddev.kbdmode=KEYMODE_REPEAT;
	else
		kbddev.kbdmode=KEYMODE_NOREPEAT;

	DPRINTK("set kbdmode=%d\n", kbddev.kbdmode);

	return count;
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

static inline unsigned char s3c2410_Get_Key(void)
{
	Zlg7290Key key;
	static unsigned char lastkey=0;
	int oldiiccon;
//	int flags;

//	local_irq_save(flags);
	
	Set_IIC_mode(ZLG7289_IICCON, &oldiiccon);
	IIC_ReadSerial(ZLG7290_ADDR, 1, &key, 3);
	Set_IIC_mode(oldiiccon, NULL);

//	local_irq_restore(flags);

	if(kbddev.kbdmode==KEYMODE_REPEAT){
		if(key.RepeatCnt==0 && key.Funckey!=0xff)
			return 0;
		if(key.key==0xff)
			return 0;

		DPRINTK("key=0x%x, repeat=%d, func=0x%x\n", key.key, key.RepeatCnt, key.Funckey);

		if(key.key!=0){
			lastkey=key.key;
			return key.key|KEY_DOWN;
		}

		return lastkey;
	}
	else{ //KEYMODE_NOREPEAT mode
		if (key.key == 0xff)
			return 0;

		if (key.key==0){
			DPRINTK("key=0x%x up\n", lastkey);
			return lastkey;
		}
		
		if(key.RepeatCnt==0 && key.Funckey==0xff){
			lastkey=key.key;
			DPRINTK("key = 0x%x\n", key.key);
			return key.key|KEY_DOWN;
		}
	}
	return 0;	
}

static inline void kbdEvent_raw(void)
{
	BUF_HEAD = kbddev.key;

	kbddev.head = INCBUF(kbddev.head, MAX_KBD_BUF);
	wake_up_interruptible(&(kbddev.wq));
}


static void s3c2410_isr_kbd(int irq, void *dev_id, struct pt_regs *reg)
{
	//DPRINTK("Zlg7290 keyboard Interrupt occured\n");

	spin_lock_irq(&(kbddev.lock));
	if((kbddev.key=s3c2410_Get_Key())!=0){
		kbdEvent_raw();
	}
	spin_unlock_irq(&(kbddev.lock));
}

static int s3c2410_kbd_open(struct inode *inode, struct file *filp)
{
	kbddev.head = kbddev.tail = 0;
	kbddev.kbdmode= KEY_NULL;
	init_waitqueue_head(&(kbddev.wq));

	KBD_OPEN_INT ();

	MOD_INC_USE_COUNT;
	DPRINTK( "opened\n");
	return 0;
}

static int s3c2410_kbd_release(struct inode *inode, struct file *filp)
{
	KBD_CLOSE_INT();
	MOD_DEC_USE_COUNT;
	DPRINTK("closed\n");
	return 0;
}

static struct file_operations s3c2410_fops = {
  owner:	THIS_MODULE,
  open:	s3c2410_kbd_open,
  read:	s3c2410_kbd_read,	
  write:	s3c2410_kbd_write,	
  release:	s3c2410_kbd_release,
  poll:	s3c2410_kbd_poll,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_kbd_dir, devfs_kbdraw;
#endif

static int __init s3c2410_kbd_init(void)
{
	int ret;
	int oldiiccon;
	int flags;

	ret = set_external_irq(IRQ_KBD, EXT_FALLING_EDGE, GPIO_PULLUP_EN);

	local_irq_save(flags);
	IIC_init();
	
	Set_IIC_mode(ZLG7289_IICCON, &oldiiccon);
	ret=IIC_Read(ZLG7290_ADDR, 0);

	//restore IICCON
	Set_IIC_mode(oldiiccon, NULL);
	local_irq_save(flags);

	DPRINTK("zlg7290 system register=0x%x\n", ret);

	KBD_CLOSE_INT();

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	kbdMajor = ret;

	/* Enable touch interrupt */
	ret = request_irq(IRQ_KBD, s3c2410_isr_kbd,SA_INTERRUPT,
			DEVICE_NAME, s3c2410_isr_kbd);
	if (ret) {
		return ret;
	}
	kbddev.head = kbddev.tail = 0;
	kbddev.kbdmode= KEY_NULL;
	init_waitqueue_head(&(kbddev.wq));

#ifdef CONFIG_DEVFS_FS
	devfs_kbd_dir = devfs_mk_dir(NULL, "keyboard", NULL);
	devfs_kbdraw = devfs_register(devfs_kbd_dir, "0raw", DEVFS_FL_DEFAULT,
				kbdMajor, KBDRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_fops, NULL);
#endif
	printk (DEVICE_NAME"\tinitialized\n");

	return 0;
}

static void __exit s3c2410_kbd_exit(void)
{

#ifdef CONFIG_DEVFS_FS	
	devfs_unregister(devfs_kbdraw);
	devfs_unregister(devfs_kbd_dir);
#endif
	unregister_chrdev(kbdMajor, DEVICE_NAME);

	free_irq(IRQ_KBD, s3c2410_isr_kbd);
}

module_init(s3c2410_kbd_init);
module_exit(s3c2410_kbd_exit);
