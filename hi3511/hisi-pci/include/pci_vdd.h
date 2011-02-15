/*
 * include/pci_vdd.h for Linux.
 *
 * History: 
 *      27-Nov-2006 create this file
 */ 
#ifndef _HI3511_PCI_VDD_H_
#define _HI3511_PCI_VDD_H_

#include <linux/errno.h>
#include <linux/interrupt.h>
//#define CONFIG_PCI_TEST 1

//#define CONFIG_PCI_HOST 1
#ifndef OSDRV_MODULE_VERSION_STRING
#define OSDRV_MODULE_VERSION_STRING "HISI_PCI-MDC030007 @Hi3511v110_OSDrv_1_0_0_7 2009-03-18 20:52:49"
#endif
struct Hi3511_pci_data_transfer_head{
	/*the first word*/
	unsigned int target:4;		/* target id */
	unsigned int src:4;			/* source id */
	unsigned int msg:4;		/* msg id */
	unsigned int port:4;		/* port id   */
	unsigned int pri:2;			/* priority  */
	unsigned int reserved:14;	/* reserved */

	/*the second word*/
	unsigned int length:20;		/* the length of the data */
	unsigned int oob:12;
//	unsigned int notused:16;		/* not used yet */
};

#define HI_PCI_HIPRIO	(1<<1)
#define HI_PCI_DMA		(1<<2)
#define HI_PCI_ASYNC 	(1)

#define HI_PCI_OOB(data) (((data)&0x0FFF)<<20)


/* give to recv in opt */
#define Hi3511_pci_head_target(head)  (head->target & 0xF)
#define Hi3511_pci_head_src(head)  (head->src & 0xF)
#define Hi3511_pci_head_msg(head)  (head->msg & 0xF)
#define Hi3511_pci_head_port(head)  (head->port & 0xF)
#define Hi3511_pci_head_length(head)  (head->length & 0xFFFFF)
#define Hi3511_pci_head_oob(head) (head->oob & 0xFFF)

struct pci_vdd_info
{
	unsigned long version;		/* PCI_VDD version */
	unsigned long id;			/* ID of this system, host = 0 */
	unsigned long bus_slot;	/* PCI slot number, host not have this value */
	unsigned long slave_map; 	/* slave map in system. 0x1 means target 1 exist,0x2 means target 1 and 2 exist */
	unsigned long mapped; 	/* slave mapped flage in system */	
};

typedef struct pci_vdd_info pci_vdd_info_t ;

typedef int (*pci_vdd_notifier_sendto)(void *handle, void *buf, int len);

typedef int (*pci_vdd_notifier_recvfrom)(void *handle, void *data);	

struct pci_vdd_opt 
{
	pci_vdd_notifier_sendto      sendto_notify;		/*  发送的回调函数 */
	pci_vdd_notifier_recvfrom  recvfrom_notify;   	/*  接收的回调函数 */
	unsigned long flags;
	unsigned long data;
};

typedef struct pci_vdd_opt pci_vdd_opt_t;




int pci_vdd_init(void);

void pci_vdd_cleanup(void);

void *pci_vdd_open(int target_id, int message_id,int port);

void pci_vdd_close(void *handle);

int pci_vdd_sendto(void *handle, void *buf, int len, int flags);

int pci_vdd_recvfrom(void *handle, void *buf, int len, int flags);

int pci_vdd_getopt(void *handle, pci_vdd_opt_t *opt);

int pci_vdd_setopt(void *handle, pci_vdd_opt_t *opt);

int pci_vdd_getinfo(pci_vdd_info_t *info);
int pci_vdd_target2slot(int target);

#define vether_rcv_get_len(v)  (((struct Hi3511_pci_data_transfer_head *) (v))->length)
#define vether_rcv_get_oob(v)  (((struct Hi3511_pci_data_transfer_head *) (v))->oob)

#ifndef CONFIG_PCI_HOST
extern unsigned long slave_sharebuffer_start;
#define slave_fifo_stuff_physics  (slave_sharebuffer_start + PCI_SLAVE_SHARE_MEM_DEFAULT_SIZE)
#endif


#endif
