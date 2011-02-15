/*
 * datacomm.h for Linux.
 *
 * History: 
 *      27-Nov-2006 create this file
 */ 
#ifndef _PCI_DATACOMM_H_
#define _PCI_DATACOMM_H_
#include "proto.h"
#include "pci_vdd.h"
#define PCI_DEBUG 1

#define HI3511_DBUG_LEVEL1	0
#define HI3511_DBUG_LEVEL2	1
#define HI3511_DBUG_LEVEL3	2
#define HI3511_DBUG_LEVEL4	3
#define HI3511_DBUG_LEVEL5	4

#define HI3511_DBUG_LEVEL       5
#ifdef  PCI_DEBUG

#define hi3511_dbgmsg(level, s, params...) do{ \
	if(level >= HI3511_DBUG_LEVEL)  \
		printk(KERN_INFO "\n[%s, %d]: " s "\n", __FUNCTION__, __LINE__, ##params); \
}while(0)

#else
#define hi3511_dbgmsg(level, s, params...)
#endif

//#define _FOR_TESTONLY_ 1
//#define _FOR_TIME_ 1
//#define _PCI_DMA_ 1
#ifndef CONFIG_PCI_HOST 
//	#define CONFIG_PCI_HOST 1
#endif
//#define CONFIG_PCI_TEST 1

/* 4 slave hi3511 support right now */
#define HI3511_PCI_SLAVE_NUM   4

/* the size of high priority package */
#define PCI_HIGHPRI_PACKAGE_SIZE     (10*1024)


/* read pointer */
#define PCI_READ_POINTER_EXCURSION   4

/* write pointer */
#define PCI_WRITE_POINTER_EXCURSION   8

/* slave memory mapped to host */
#define PCI_SLAVE_UNMAPPED  		0
#define PCI_SLAVE_BEEN_MAPPED		1
#define PCI_SLAVE_MAP_OVERLOAD      2
#define PCI_SLAVE_MAP_NOTENOUGH   3

/*share memory in slave size 128 kbytes by default*/
#define PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE   (512*1024)

/*share memory in slave size 64 kbytes by default*/
#define PCI_HANDLE_MEM_MAXSIZE   (64*1024)

/* set share memory size flag */
#define SHARE_MEMORY_SETSIZEFLAG  0x1


/* PRI FLAG  */
#define PCI_WRITEABLE_PRIPACKAGE_FLAG       0x0

#define PCI_UNWRITEABLE_PRIPACKAGE_FLAG   0x1


/* PRI FLAG  */
#define PCI_READABLE_PRIPACKAGE_FLAG       1
#define PCI_UNREADABLE_PRIPACKAGE_FLAG   0

/* recv data in share memory*/
#define PCI_SHARE_MEMORY_DATA_NOTIN        			0
#define PCI_SHARE_MEMORY_DATA_IN_NORMAL     	       1	
#define PCI_SHARE_MEMORY_DATA_IN_PRIORITY    	       2

/* recv data in share memory*/
#define PCI_SHARE_MEMORY_RECV_SYN        0
#define PCI_SHARE_MEMORY_RECV_ASYN 	  1	

/* add 2 buffer return value */
#define PCI_PRI_SHARE_MEMORY_NOTENOUGH    	-3
#define PCI_NOR_SHARE_MEMORY_FULL       		-4
#define PCI_NOR_SHARE_MEMORY_NOTENOUGH		-5

#define Hi3511_pci_handle_pos(target,msg,port)  (((target & 0xF) << 8) |( (msg & 0xF) << 4) | (port & 0xF ))

#ifdef CONFIG_PCI_HOST

// host
struct mmap_memory_info
{
	unsigned long  baseaddr;				/* host  buffer addr */
	unsigned long  baseaddr_physics;            /*send buffer physics address*/
	unsigned long  pripackageaddr;			/* PRI buffer addr */
	unsigned long  pripackageflagaddr;		/* PRI package write/read flag. writeable when 0,unwriteable when 1 */
	unsigned long  readpointeraddr;		/* recv memory read pointer addr */
	unsigned long  writepointeraddr;		/* recv memory write pointer addr */
	unsigned long  normalpackageaddr; 	/* normal buffer addr */
	int  			 buflength;				/* host  buffer length */
	int 			 normalpackagelength;	/* normal buffer length */
	struct  semaphore  prisemaphore;		/* share memory pri package semaphore */
	struct  semaphore  normalsemaphore;		/* share memory normal package semaphore */	
};

// host send buf means slave revc buf
// host recv buf means slave send buf 
struct  slave2host_mmap_info
{
	struct mmap_memory_info recv;		/* recv memory info */
	struct mmap_memory_info send;		/* send memory info */
	unsigned long  mapped;				/* if info mapped = 1 means slave works,unmapped 0 */
	struct hi3511_dev * dev;
	int 	slot;							/* this mmap slave in pci slot */
};

// host
struct  Hi3511_pci_slave2host_mmap_info
{
	struct slave2host_mmap_info mmapinfo[HI3511_PCI_SLAVE_NUM];
};

#else

// slave
struct mmap_memory_info
{
	unsigned long  baseaddr;				/* slave  buffer addr */
	unsigned long  baseaddr_physics;            /*send buffer physics address*/
	unsigned long  pripackageaddr;			/* PRI buffer addr */
	unsigned long  pripackageflagaddr;		/* PRI package write/read flag. writeable when 0,unwriteable when 1 */
	unsigned long  readpointeraddr;		/* send memory read pointer addr */
	unsigned long  writepointeraddr;		/* send memory write pointer addr */
	unsigned long  normalpackageaddr; 	/* normal buffer addr */
	int			 buflength;				/* slave  buffer length */
	int			 normalpackagelength;	/* normal buffer length */
	struct  semaphore  prisemaphore;		/* share memory pri package semaphore */
	struct  semaphore  normalsemaphore;	/* share memory normal package semaphore */
};

// slave send buf means host revc buf
// slave recv buf means host send buf 
struct  slave_mmap_info
{
	struct mmap_memory_info send;		/* recv memory info */
	struct mmap_memory_info recv;		/* send memory info */
	unsigned long  mapped;				/* if info mapped = 1 means slave works,unmapped 0 */
	struct hi3511_dev * dev;
	int 	slot;							/* this mmap slave in pci slot */
};

// slave
struct  Hi3511_pci_slave_mmap_info
{
	struct slave_mmap_info mmapinfo;
};
#endif

// inital package from host to slave
struct  Hi3511_pci_initial_host2pci
{
	unsigned long recvbuffersize;		/* recv buffer size */
	unsigned long sendbuffersize;		/* send buffer size */
	unsigned long recvbufferaddrhost;		/* recieve buffer physics address on host */
	unsigned long interrupt_flag_addr;      /* interrupt flag physics address on host for each slave */
	int slot;							/* slave slot number */
	int  slaveid;						/* slave id in communicate table */
};

int hi3511_pci_locksend_normal_buffer(int target);

void hi3511_pci_unlocksend_normal_buffer(int target);

int hi3511_pci_locksend_priority_buffer(int target);

void hi3511_pci_unlocksend_priority_buffer(int target);

int hi3511_pci_lockrecv_normal_buffer(int target);

void hi3511_pci_unlockrecv_normal_buffer(int target);

void hi3511_pci_unlockrecv_priority_buffer(int target);

void hi3511_pci_getsend_normal_rwpointer(int target,int *readpointer,int *writepointer);

int hi3511_pci_getsend_priority_rwpointer(int target);

void hi3511_pci_getrecv_normal_rwpointer(int target,int *readpointer,int *writepointer);

int hi3511_pci_getrecv_priority_rwpointer(int target);

int hi3511_pci_add2buffer_priority(struct Hi3511_pci_data_transfer_head *head, char *buffer, int len,int flags);

int hi3511_pci_add2buffer_normal(struct Hi3511_pci_data_transfer_head *head, char *buffer, int len,int flags);

int hi3511_pci_readpack(int target,char *tobuffer, char * frombuffer,int len,int flags);

int hi3511_pci_writepack(int target,char *tobuffer, char * frombuffer,int len,int flags);

int hi3511_pci_readonepackfromsharebuffer_priority(void * handle,char *buffer, int flags);

int hi3511_pci_readonepackfromsharebuffer_normal(void * handle,char *buffer, int flags);

int hi3511_pci_recvhandle_exist(int handle_pos);

void hi3511_pci_getrecv_priority_datain(void * data);

void hi3511_pci_getrecv_normal_datain(void *data);

int hi3511_pci_recvfrom_normalsharebuf2handlebuf(void*mmapinfo,
	struct Hi3511_pci_data_transfer_handle *handle,int info_readpoint, int len);

int hi3511_pci_recvfrom_prioritysharebuf2handlebuf(void *data,void*handle, int len);

int hi3511_pci_get_pointernextpos(void *data,int packlen,int wrpointer, int rpointer);

int hi3511_pci_recvfromhandlebuffer(struct Hi3511_pci_data_transfer_handle *handle,char *buffer);

void hi3511_mapped_info(unsigned long data);

 int hi3511_pci_kernelthread_deamon(void *data);

void hi3511_pci_slave2host_irq(unsigned long data);

#ifdef CONFIG_PCI_HOST
int hi3511_init_sharebuf(int slot,unsigned long sendbufsize,unsigned long recvbufsize, int resetflag);
void hi3511_pci_host2slave_irq(unsigned long target);
#else
int hi3511_init_sharebuf(unsigned long sendbufsize,unsigned long recvbufsize, unsigned long sendbufaddr,int resetflag);
#endif

#endif

