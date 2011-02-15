
/*
 *    This file has definition data structure and interfaces for 
 *    hisilicon multicpu communication.
 *
 *    Copyright (C) 2008 hisilicon , chanjinn@hauwei.com
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *    MCC is short for Multy-Cpu-Communication
 *
 *    create by chanjinn, 2008.11.20
 */

#include <linux/ioctl.h>

#ifndef __HI_MCC_USERDEV_H__
#define __HI_MCC_USERDEV_H__

/* HI_MCC_PORT_NR : max ports supported 
 * HI_MCC_RESERVE_PORT_NR : count of reserve ports 
 */
#define HI_MCC_PORT_NR 1024
#define HI_MCC_RESERVE_PORT_NR 32

/* HI_MCC_PORT_NR : max target id supported
* 					it depends on hardware pci device number
*/
#define HI_MCC_TARGET_ID_NR  36

typedef unsigned long hi_mcc_handle_t;

struct hi_mcc_handle_attr {
    unsigned int target_id;
    unsigned int port;
    unsigned int priority;
};

#define	HI_IOC_MCC_BASE  'M'

/* Create a new mcc handle. A file descriptor is only used once for one mcc handle. */
#define HI_MCC_IOC_CONNECT  _IOW(HI_IOC_MCC_BASE, 1, struct hi_mcc_handle_attr)
#define HI_MCC_IOC_DISCONNECT  _IOW(HI_IOC_MCC_BASE, 2, unsigned long)

/* Bind a handle to a port */
#define HI_MCC_IOC_SETOPTION   _IOW(HI_IOC_MCC_BASE, 3, unsigned int)

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/ioctl.h>
#include <linux/wait.h>
#define HI_MCC_DBG_LEVEL 0x0
#define hi_mcc_trace(level, s, params...) do{ if(level & HI_MCC_DBG_LEVEL)\
                printk(KERN_INFO "[%s, %d]: " s "\n", __FUNCTION__, __LINE__, params);\
                }while(0)


#define MCC_PORT_ISVALID(s) (s < HI_MCC_PORT_NR)
struct hios_mcc_handle
{
	unsigned long handle;		/* pci handle */
	struct list_head mem_list;	/* mem list for */
	wait_queue_head_t wait;
};

typedef struct hios_mcc_handle  hios_mcc_handle_t;

/* recvfrom_notify sample  
*int myrecv_notify(void *handle ,void *buf, unsigned int data_len)
*{		        ~~~~~~~
*	struct hios_mcc_handle hios_handle;
*	hios_mcc_handle_opt opt;
*	unsigned long cus_data;
*	hios_handle.handle = (unsigned long) handle
*					     ~~~~~~
*	hios_mcc_getopt(&hios_handle, &opt);
*	cus_data = opt.data;
*	...
*}
*/

struct hios_mcc_handle_opt{
	/* Remaind:input parameter "void *handle" is not the hios_mcc_open return value */
	/* see recvfrom_notify the sample if use want to get the value hide in opt->data */
	int (*recvfrom_notify)(void *handle, void *buf, unsigned int data_len);
        unsigned long data;
};

typedef struct hios_mcc_handle_opt hios_mcc_handle_opt_t;

hios_mcc_handle_t *hios_mcc_open(struct hi_mcc_handle_attr *attr);

int hios_mcc_sendto(hios_mcc_handle_t *handle, const void *buf, unsigned int len);

int hios_mcc_close(hios_mcc_handle_t *handle);

int hios_mcc_getopt(hios_mcc_handle_t *handle, hios_mcc_handle_opt_t *opt);

int hios_mcc_setopt(hios_mcc_handle_t *handle, const hios_mcc_handle_opt_t *opt);

int hios_mcc_getlocalid(void);

int hios_mcc_getremoteids(int ids[]);

#endif  /* __KERNEL__ */

#endif  /* __HI_MCC_H__ */
