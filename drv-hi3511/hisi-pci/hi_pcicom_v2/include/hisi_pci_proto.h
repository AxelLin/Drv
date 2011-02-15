#ifndef _HISI_PROTO_H_ 
#define _HISI_PROTO_H_

/* for 6 bus and 5 slot per bus slot <= 3 * 10 + 5 = 35 */
#define HISI_MAX_PCI_DEV   36
#define HISI_PCI_DEV_BITS  0x3F

#define HISI_PCI_DEV_NEEDBITS  6

/* 10 bits = 2 ^ 10 = 1024 */
#define HISI_MAX_PCI_PORT   1024 
#define HISI_PCI_PORT_BITS  0x3FF
#define HISI_PCI_PORT_NEEDBITS  10


/* 6 bit bus &&  devfn and 10 bit port */
#define HISI_PCI_HANDLE_TABLE_SIZE  (36 * 1024)  

#define HISI_PCI_NORMAL_DATA	  (1 << 0)
#define HISI_PCI_PRIORITY_DATA	  (1 << 1)

#define HISI_PCI_MEM_DATA  (0)
#define HISI_PCI_INITIAL_MEM_DATA (1)

#define HISI_HEAD_MAGIC_INITIAL	(0x333) /* 1100110011b */
#define HISI_HEAD_MAGIC_JUMP	(0x249)	/* 1001001001b*/

#define HISI_HEAD_CHECK_FINISHED (0xA95) /* 101010010101b */

struct hisi_pcidev_map
{
	struct {
		unsigned long 	base_addr_phy;		/* physical addr */
		unsigned long 	base_addr_virt;		/* virtual addr */
		void 		*dev;			/* pcicom dev struct */
	}map[HISI_MAX_PCI_DEV];
	unsigned int num;				/* number of dev in system */
	struct timer_list map_timer;			/* timer list for slave memory map */		
};

struct hisi_pci_transfer_handle{
	unsigned int  target_id;					/*record target*/
	unsigned int  port;						/*record port*/
	unsigned int  priority;						/*record priority*/
	int (*pci_vdd_notifier_recvfrom)(void *handle, void *buf, unsigned int len); 	/* receive func */	
	unsigned long data;
	atomic_t sendmutex;						/* a handle must have send mutex */
};

struct hisi_memory_info
{
	unsigned long base_addr;
	unsigned long end_addr;
	unsigned long buf_len;
	unsigned long rp_addr;
	unsigned long wp_addr;
	atomic_t memmutex;		/* shared memory should be changed one by one */
};

struct hisi_shared_memory_info
{
        struct hisi_memory_info priority;/* priority shared memory */
        struct hisi_memory_info normal;  /* normal shared memory */
};

struct hisi_map_shared_memory_info_host
{
        unsigned long  base_virtual;            /* slave  buffer addr */
        unsigned long  base_physics;            /* send buffer physics address*/
	struct hisi_shared_memory_info send;	/* send shared memory */
	struct hisi_shared_memory_info recv;  	/* recv shared memory */
};

struct hisi_map_shared_memory_info_slave
{
        unsigned long  base_virtual;            /* slave  buffer addr */
        unsigned long  base_physics;            /* send buffer physics address*/
	struct hisi_shared_memory_info recv;  	/* recv shared memory */
	struct hisi_shared_memory_info send;	/* send shared memory */
};

struct hisi_pci_transfer_head{
	unsigned int target_id:6;		/* target_id */
	unsigned int src:6;			/* scr */
	unsigned int port:10;			/* port */
	unsigned int magic:10;			/* magic number for rec */
	unsigned int length:20;			/* data length */
	unsigned int check:12;			/* check number if head tranferd complete */
};
#define HISI_PCI_MAX_MAP_TIME_OUT (20 * HZ)

typedef int (*pci_vdd_notifier_recvfrom)(void *handle, void *buf, unsigned int length);
struct pci_vdd_opt 
{
	pci_vdd_notifier_recvfrom  recvfrom_notify;   	/* call-back receive function in interrupt */
	unsigned long data;
};
#define HISI_PCI_NORMAL_PACK	0
#define HISI_PCI_PRIORITY_PACK	1

typedef struct pci_vdd_opt pci_vdd_opt_t;

void *pci_vdd_open(int target_id, int port, int priority);
void pci_vdd_close(void *handle);
int pci_vdd_sendto(void *handle, const void *buf, unsigned int len, int flage);
int pci_vdd_getopt(void *handle, pci_vdd_opt_t *opt);
int pci_vdd_setopt(void *handle, pci_vdd_opt_t *opt);
int pci_vdd_localid(void);
int pci_vdd_remoteids(int ids[]);
#endif

