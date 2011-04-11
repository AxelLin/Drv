/*
 * s3c2410-ts-ads7843.c
 *
 * touchScreen driver for SAMSUNG S3C2410
 *
 * Author: threewater <threewater@up-tech.com>
 * Date  : $Date: 2004/06/10 17:25:00 $ 
 *
 * $Revision: 1.1.2.6 $
 *
 * Based on s3c2410-ts.c
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
#include <asm/arch/spi.h>

/* debug macros */
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK( x... )	printk("s3c2410-ts: " ##x)
#else
#define DPRINTK( x... )
#endif

#define PEN_UP	        0		
#define PEN_DOWN	1
#define PEN_FLEETING	2
#define MAX_TS_BUF	16	/* how many do we want to buffer */

#define DEVICE_NAME	"s3c2410-ts"
#define TSRAW_MINOR	1

typedef struct {
  unsigned int penStatus;		/* PEN_UP, PEN_DOWN, PEN_SAMPLE */
  TS_RET buf[MAX_TS_BUF];		/* protect against overrun */
  unsigned int head, tail;	/* head and tail for queued events */
  wait_queue_head_t wq;
  spinlock_t lock;
} TS_DEV;

static TS_DEV tsdev;

#undef	IRQ_TC
#define IRQ_TC				IRQ_EINT5	//IRQ_EINT5
#define ADS7843_PIN_CS		(1<<12)  //GPG12
#define ADS7843_PIN_PEN		(1<<5)  //GPF5

#define TS_OPEN_INT()		set_external_irq(IRQ_TC, EXT_LOWLEVEL, GPIO_PULLUP_EN)
#define TS_CLOSE_INT()		set_gpio_ctrl(GPIO_MODE_IN | GPIO_PULLUP_EN | GPIO_F5)

#define GPIO_ADS7843_CS		(GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_G12)

#define ADS7843_CTRL_START              0x80
#define ADS7843_GET_X                   0x50
#define ADS7843_GET_Y                   0x10
#define ADS7843_CTRL_12MODE    		0x0
#define ADS7843_CTRL_8MODE      	0x8
#define ADS7843_CTRL_SER                0x4
#define ADS7843_CTRL_DFR                0x0
#define ADS7843_CTRL_DISPWD     	0x3     // Disable power down
#define ADS7843_CTRL_ENPWD      	0x0     // enable power down

#define ADS7843_CMD_X   (ADS7843_CTRL_START|ADS7843_GET_X|ADS7843_CTRL_12MODE|ADS7843_CTRL_DFR|ADS7843_CTRL_ENPWD)
#define ADS7843_CMD_Y   (ADS7843_CTRL_START|ADS7843_GET_Y|ADS7843_CTRL_12MODE|ADS7843_CTRL_DFR|ADS7843_CTRL_ENPWD)
#define ADS7843_CMD_START ADS7843_CMD_X

#define BUF_HEAD	(tsdev.buf[tsdev.head])
#define BUF_TAIL	(tsdev.buf[tsdev.tail])
#define INCBUF(x,mod) 	((++(x)) & ((mod) - 1))

#if 0
#define CONVERT_X(x)  (((x)-200) / 5)
#define CONVERT_Y(y)  ((1800-(y)) /7 )
#else
#define CONVERT_X(x)  (x)
#define CONVERT_Y(y)  (y)
#endif

#define HOOK_FOR_DRAG
#ifdef HOOK_FOR_DRAG
#define TS_TIMER_DELAY			(HZ/10) /* 20 ms */
#define	TS_TIMER_DELAY1		(HZ/20) /* 50 ms*/
static struct timer_list ts_timer;
#endif

static int x, y;	/* touch screen coorinates */
static int tsMajor = 0;
static void (*tsEvent)(void);

inline static void enable_7843(void)
{
	GPGDAT &= ~ADS7843_PIN_CS;
	udelay(1);
}

inline static void disable_7843(void)
{
	GPGDAT |= ADS7843_PIN_CS;
}

inline static void set7843toIRQmode(void)
{
  int flags;

  local_irq_save(flags);

  enable_7843 ();
  SPISend (ADS7843_CMD_X, 0);
  SPIRecv (0);
  SPIRecv (0);
  disable_7843 ();

  local_irq_restore(flags);
}

inline static void setSPImode (void)
{
	int delay;

	Set_SIO_mode(0, SPCON_SMOD_POLL | SPCON_ENSCK | SPCON_MSTR |SPCON_CPOL_HIGH | 
		SPCON_CPHA_FMTA, 33, 2, NULL, NULL, NULL);

	//enable_7843();
	//for (delay = 0; delay < 1; delay++){
	//	SPISend (0xff, 0);
		//SPIRecv (0);
		//SPIRecv (0);
	//}
	//disable_7843();
}

static void tsEvent_raw(void)
{
  if (tsdev.penStatus == PEN_DOWN) {
    BUF_HEAD.x = CONVERT_X(x);
    BUF_HEAD.y = CONVERT_Y(y);
    BUF_HEAD.pressure = PEN_DOWN;
  } else if (tsdev.penStatus == PEN_UP ){
    BUF_HEAD.x = CONVERT_X(x);
    BUF_HEAD.y = CONVERT_Y(y);
    BUF_HEAD.pressure = PEN_UP;
  } else {
    BUF_HEAD.x = CONVERT_X(x);
    BUF_HEAD.y = CONVERT_Y(y);
    BUF_HEAD.pressure = PEN_FLEETING;
  }

  tsdev.head = INCBUF(tsdev.head, MAX_TS_BUF);
  wake_up_interruptible(&(tsdev.wq));
}

static int tsRead(TS_RET * ts_ret)
{
  spin_lock_irq(&(tsdev.lock));
  ts_ret->x = BUF_TAIL.x;
  ts_ret->y = BUF_TAIL.y;
  ts_ret->pressure = BUF_TAIL.pressure;
  tsdev.tail = INCBUF(tsdev.tail, MAX_TS_BUF);
  spin_unlock_irq(&(tsdev.lock));

  return sizeof(TS_RET);
}


static ssize_t s3c2410_ts_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
  TS_RET ts_ret;

 retry: 
  if (tsdev.head != tsdev.tail) {
    int count;
    count = tsRead(&ts_ret);
    if (count) copy_to_user(buffer, (char *)&ts_ret, count);
    return count;
  } else {
    if (filp->f_flags & O_NONBLOCK)
      return -EAGAIN;
    interruptible_sleep_on(&(tsdev.wq));
    if (signal_pending(current))
      return -ERESTARTSYS;
    goto retry;
  }

  return sizeof(TS_RET);
}

static unsigned int s3c2410_ts_poll(struct file *filp, struct poll_table_struct *wait)
{
  poll_wait(filp, &(tsdev.wq), wait);
  return (tsdev.head == tsdev.tail) ? 0 : (POLLIN | POLLRDNORM); 
}


static inline void s3c2410_get_XY(void)
{
  unsigned int temp;
  int flags;
  int i;

  local_irq_save(flags);
                                                                                
  enable_7843();
  SPISend(ADS7843_CMD_X, 0);
  SPISend(0, 0);
  temp=SPIRecv(0);
  SPISend(ADS7843_CMD_X, 0);
  temp<<=8;
  temp|=SPIRecv(0);
  x=(temp>>3);

  x=0;                       
  for(i=0; i<16; i++){
	SPISend(0, 0);
	temp=SPIRecv(0);
	SPISend(ADS7843_CMD_X, 0);
	temp<<=8;
	temp|=SPIRecv(0);
	x+=(temp>>3);
  }
  x>>=4;          //x=x/16;
  
  SPISend(ADS7843_CMD_Y, 0);
  SPISend(0, 0);
  temp=SPIRecv(0);
  SPISend(ADS7843_CMD_Y, 0);
  temp<<=8;
  temp|=SPIRecv(0);
  y=(temp>>3);

  y=0;
  for(i=0; i<16; i++){
        SPISend(0, 0);
        temp=SPIRecv(0);
        SPISend(ADS7843_CMD_Y, 0);
        temp<<=8;
        temp|=SPIRecv(0);
        y+=(temp>>3);
  }
  y>>=4;          //y=y/16;
                                                                                
  disable_7843 ();
  local_irq_restore(flags);
                                                                                
  //tsdev.penStatus = PEN_DOWN;
  if(tsdev.penStatus == PEN_DOWN)
    DPRINTK("PEN DOWN: x: %d, y: %d\n", x, y);
  else
    DPRINTK("PEN FLEETING: x: %d, y: %d\n", x, y);
  tsEvent();
}

static void s3c2410_isr_tc(int irq, void *dev_id, struct pt_regs *reg)
{
	DPRINTK("Touch Screen Interrupt occured\n");

  spin_lock_irq(&(tsdev.lock));
  if (tsdev.penStatus == PEN_UP) {
    TS_CLOSE_INT();
    udelay(1);
	DPRINTK("Touch Screen close int\n");
    if (( GPFDAT & ADS7843_PIN_PEN) == 0 ) {
      tsdev.penStatus = PEN_DOWN;
#ifdef	HOOK_FOR_DRAG
      ts_timer.expires = jiffies + TS_TIMER_DELAY1;
      add_timer (&ts_timer);
#endif
    }else{
      TS_OPEN_INT();
	DPRINTK("Touch Screen open int\n");
    }	
  }
  spin_unlock_irq(&(tsdev.lock));
}

#ifdef HOOK_FOR_DRAG
static void ts_timer_handler(unsigned long data)
{
  spin_lock_irq(&(tsdev.lock));
  if((GPFDAT & ADS7843_PIN_PEN) == 0 ){
    s3c2410_get_XY ();
    tsdev.penStatus = PEN_FLEETING;
#ifdef HOOK_FOR_DRAG
    ts_timer.expires = jiffies + TS_TIMER_DELAY;
    add_timer(&ts_timer);
#endif
  }else{
    tsdev.penStatus = PEN_UP;
    del_timer (&ts_timer);
    DPRINTK ("PEN UP:x:%d,y:%d\n", x, y);
    tsEvent ();
    TS_OPEN_INT ();
  }
  spin_unlock_irq(&(tsdev.lock));
}
#endif

static int s3c2410_ts_open(struct inode *inode, struct file *filp)
{
	tsdev.head = tsdev.tail = 0;
	tsdev.penStatus = PEN_UP;
#ifdef HOOK_FOR_DRAG 
	init_timer(&ts_timer);
	ts_timer.function = ts_timer_handler;
#endif
	tsEvent = tsEvent_raw;
	init_waitqueue_head(&(tsdev.wq));

	TS_OPEN_INT ();

	MOD_INC_USE_COUNT;
	DPRINTK( "ts opened\n");
	return 0;
}

static int s3c2410_ts_release(struct inode *inode, struct file *filp)
{
#ifdef HOOK_FOR_DRAG
  del_timer(&ts_timer);
#endif
  TS_CLOSE_INT ();
  MOD_DEC_USE_COUNT;
  //printk ( "ts closed\n");
  return 0;
}

static struct file_operations s3c2410_fops = {
  owner:	THIS_MODULE,
  open:	s3c2410_ts_open,
  read:	s3c2410_ts_read,	
  release:	s3c2410_ts_release,
  poll:	s3c2410_ts_poll,
};

void tsEvent_dummy(void) {}

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_ts_dir, devfs_tsraw;
#endif

static int __init s3c2410_ts_init(void)
{
	int ret;

	set_gpio_ctrl(GPIO_ADS7843_CS);
	SPI_initIO(0);
	setSPImode();
	set7843toIRQmode();

	ret = TS_OPEN_INT();//set_external_irq( IRQ_TC, EXT_FALLING_EDGE, GPIO_PULLUP_DIS);
	if (ret)
		return ret;
	TS_CLOSE_INT();

	tsEvent = tsEvent_dummy;

	tsdev.head = tsdev.tail = 0;
	tsdev.penStatus = PEN_UP;
        init_waitqueue_head(&(tsdev.wq));
#ifdef HOOK_FOR_DRAG
        init_timer(&ts_timer);
        ts_timer.function = ts_timer_handler;
#endif

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	tsMajor = ret;

	/* Enable touch interrupt */
	ret = request_irq(IRQ_TC, s3c2410_isr_tc,SA_INTERRUPT,
			DEVICE_NAME, s3c2410_isr_tc);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_DEVFS_FS
	devfs_ts_dir = devfs_mk_dir(NULL, "touchscreen", NULL);
	devfs_tsraw = devfs_register(devfs_ts_dir, "0raw", DEVFS_FL_DEFAULT,
				tsMajor, TSRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR, &s3c2410_fops, NULL);
#endif
	printk (DEVICE_NAME"\tinitialized\n");

#ifdef DEBUG
	s3c2410_ts_open(NULL, NULL);
#endif
	return 0;
}

static void __exit s3c2410_ts_exit(void)
{

#ifdef CONFIG_DEVFS_FS	
  devfs_unregister(devfs_tsraw);
  devfs_unregister(devfs_ts_dir);
#endif
  unregister_chrdev(tsMajor, DEVICE_NAME);

  free_irq(IRQ_TC, s3c2410_isr_tc);
}

module_init(s3c2410_ts_init);
module_exit(s3c2410_ts_exit);
