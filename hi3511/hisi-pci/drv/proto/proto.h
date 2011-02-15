/*
 * proto.h for Linux.
 *
 * History: 
 *      27-Nov-2006 create this file
 */ 
#ifndef _HI3511_PCI_PROTOCAL_H_
#define _HI3511_PCI_PROTOCAL_H_
#include <linux/wait.h>

/*handle table size*/
/*target 3bit msg 3bit port 4bit = 2^4  * 2^4 * 2^4 = 2^10 = 1024*/
#define HI3511_PCI_HANDLE_TABLE_SIZE  4096


/* package PRI */
#define HI3511_PCI_PACKAGE_PRI_LEVEL_1  0x0
#define HI3511_PCI_PACKAGE_PRI_LEVEL_2  0x1

/* flags meaning*/
#define HI3511_PCI_PACKAGE_ASYN_FLAG 	 0x1
#define HI3511_PCI_PACKAGE_PRI_FLAG  	 (0x1 << 1)
#define HI3511_PCI_HANDLEBUF_ADDR_TYPE  (0x1<<3)

/* recv flag*/
#define HI3511_RECV_PACKAGE_NORMALMODE        0 
#define HI3511_RECV_PACKAGE_DMAMODE 		      1


/* normal package send timer 10ms */
#define HI3511_PCI_TIMER_DELAY     (1)

/* memory mapped timer */
#define HI3511_PCI_MAPPED_TIMER_DELAY  (20*HZ)

/* recv syn wait 2HZ time out */
#define HI3511_PCI_WAIT_TIMEOUT  (3*HZ)

/* recv syn wait 5ms time out */
#define HI3511_PCI_WAIT_HANDLEBUF_EMPTY  10

#define HI3511_PCI_WAIT_TIMES_HANDLEBUF_EMPTY  5

/*handle buffer size*/
#define HI3511_PCI_HANDLE_BUFFER_SIZE  (512*1024UL)

/*start of sharebuffer on host*/
//#define host_sharebuffer_start 0xe5000000 //delete for dynamic addr alloc
#define host_sharebuffer_size 0x800000


/*exchange buffer address when highlevel provide virtual address*/
//#define exchange_buffer_addr_physics 0xe5900000 //delete for dynamic alloc
#define exchange_buffer_size (512*1024UL)

/*multi-used ram address, used to provide physics address for setting special value of sharebuffer*/
//#define settings_addr_physics 0xe5a00000 //delete for dynamic alloc
#define settings_buffer_size  (512*1024UL)
#define read_pointer_physics 0
#define write_pointer_physics 4
#define pci_writeable_pripackage_flag 8
#define send_buffer_clear  12
#define irq_prepost 16

#define read_pointer_physics_r 20
#define write_pointer_physics_r 24
#define pci_writeable_pripackage_flag_r  28
#define irq_prepost_r 32

#define write_head  36
#define write_head_r 44

#define fifo_stuff_size 0x100

struct HI3511_pci_handle_buffer{
	char *buffer;						/* buffer head */
	int buffersize;					/* buffer size */
	int readpointer;					/* read pointer */
	int writepointer;					/* write pointer */
	struct  semaphore  read;			/* buffer send semaphore */
	struct  semaphore  write;			/* buffer recv semaphore */	
};


struct Hi3511_pci_data_transfer_handle{
	int  target;										/*record target*/
	int  msg;											/*record msg*/
	int  port;											/*record port*/
	
	int (*pci_vdd_notifier_sendto)(void *handle, void *buf, int len);	/* send func */

	int (*pci_vdd_notifier_recvfrom)(void *handle, void *head); /* receive func */	

	int packnum;								/* record the package number in buffer */

	int packsize;								/* pack have bytes */

	int synflage;								/* record the syn or asyn and priority and so on*/

	int recvflage;							/* record if have recv data in share memory */

	unsigned long data;

	struct HI3511_pci_handle_buffer *buffer;	       /* data buffer  FIFO buffer */
	
	wait_queue_head_t  waitqueuehead;	/*work queue will record the thread which sleep on this handle */

	struct  semaphore  sendsemaphore;			/* handle send semaphore */
	
	struct  semaphore  recvsemaphore;			/* handle recv semaphore */
};

struct hi3511_pci_data_transfer_handle_table{
	struct Hi3511_pci_data_transfer_handle *handle[HI3511_PCI_HANDLE_TABLE_SIZE];    /* handle table */
	wait_queue_head_t  dmawaitqueue;	/* work queue will record the DMA thread which sleep on handle */	
	struct timer_list timer;						/*  timer list for send data during time */	
	struct timer_list slavemap_timer;			/*  timer list for send data during time */		
};

extern int hi3511_slave_interrupt_check(void);
extern void slave_int_disable(void);
extern void slave_int_clear(void);
extern void init_hi3511_slave_device(void);
extern void slave_int_enable(void);
extern void hi3511_slave_device_cleanup(void);
#endif

