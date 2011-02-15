/*
 * Hisilicon HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 2008 Jiang Zhonglin <istone@huawei.com>
 *
 * This file is licenced under the GPL.
 */

#include <linux/module.h>
#include <linux/smp_lock.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/dmapool.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include "hcd.h"
#include "hub.h"

#include "hiusb.h"
#include "ctrl.h"
#include "sys.h"

#define hiusb_trace_urb(level, urb, msg...) do{\
	hiusb_trace((level), msg \
			":urb(0x%.8x):%s:addr(%d):ep(%d):(%d/%d):status(%d):hcpriv(0x%.8x)", \
			(int)(urb), \
			!usb_pipeout((urb)->pipe)? "in" : "out", \
			usb_pipedevice((urb)->pipe), \
			usb_pipeendpoint((urb)->pipe), \
			(urb)->actual_length, \
			(urb)->transfer_buffer_length, \
			(urb)->status, \
			(int)(urb)->hcpriv ); \
}while(0)

/* convert between an HCD pointer and the corresponding HIUSB_HCD */ 
static inline struct hiusb_hcd *hcd_to_hiusb (struct usb_hcd *usb_hcd)
{
	return (struct hiusb_hcd*) (usb_hcd->hcd_priv);
}
static inline struct usb_hcd *hiusb_to_hcd (struct hiusb_hcd *hiusb_hcd)
{
	return container_of ((void *) hiusb_hcd, struct usb_hcd, hcd_priv);
}

static int ctrlpipe_nak_limit = 3;
module_param_named(ctrlpipe_nak_limit, ctrlpipe_nak_limit, int, 0600);

static int bulkpipe_nak_limit = 3;
module_param_named(bulkpipe_nak_limit, bulkpipe_nak_limit, int, 0600);

static int watch_dog_timeval = HZ;
module_param_named(watch_dog_timeval, watch_dog_timeval, int, 0600);

static int hiusb_hcd_start (struct usb_hcd *usb_hcd);
static void hiusb_hcd_stop (struct usb_hcd *usb_hcd);
static int hiusb_hcd_reset (struct usb_hcd *usb_hcd);

static void hiusb_hcd_scheduler(void *data);

static void hiusb_hcd_urb_done(struct hiusb_hcd *hcd, struct urb *urb, struct pt_regs *regs)
{
	struct hiusb_qtd	*qtd;

	/* caller must hold urb pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));

	hiusb_trace_urb(7, urb, "urb done");

	device_lock(hcd);

	/* gain qtd */
	qtd = urb->hcpriv;

	/* set urb status */
	spin_lock(&urb->lock);
	urb->hcpriv = NULL;

	switch (urb->status) {
	case -EINPROGRESS:		/* success */
		urb->status = 0;
	default:			/* fault */
		break;
	case -EREMOTEIO:		/* fault or normal */
		if (!(urb->transfer_flags & URB_SHORT_NOT_OK))
			urb->status = 0;
		break;
	case -ECONNRESET:		/* canceled */
	case -ENOENT:
		break;
	}
	spin_unlock(&urb->lock);

	/* free list and qtd */
	list_del (&qtd->qtd_list);
	kfree (qtd);

	device_unlock(hcd);

	/* give back urb */
	usb_hcd_giveback_urb (hiusb_to_hcd(hcd), urb, regs);

	/* clear urb pending */
	clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

	/* kick the scheduler */
	if(hcd->rh_port_connect && !hcd->rh_port_change 
			&& list_empty(&hcd->wks_scheduler.entry)){

		PREPARE_WORK(&hcd->wks_scheduler, hiusb_hcd_scheduler, hcd);
		queue_work(hcd->wq_scheduler, &hcd->wks_scheduler);
	}
}

static void hiusb_hcd_handle_error(struct hiusb_hcd *hcd, struct urb *urb, int err)
{
	/* caller must hold pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));

	hiusb_trace_urb(7, urb, "urb error");

	if(err & HIUSB_HCD_E_NAKTIMEOUT){

		/* clear urb pending */
		clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

		/* kick the scheduler */
		if(hcd->rh_port_connect && !hcd->rh_port_change 
				&& list_empty(&hcd->wks_scheduler.entry)){

			PREPARE_WORK(&hcd->wks_scheduler, hiusb_hcd_scheduler, hcd);
			queue_work(hcd->wq_scheduler, &hcd->wks_scheduler);
		}
	}
	else if(err & HIUSB_HCD_E_BUG){

		hiusb_error("BUG");
		BUG();
	}
	else if(err & HIUSB_HCD_E_STALL){

		spin_lock(&urb->lock);
		urb->status = -EPIPE;
		spin_unlock(&urb->lock);
	
		hiusb_hcd_urb_done(hcd, urb, NULL);
	}
	else{
		//HIUSB_HCD_E_BUSERROR:
		//HIUSB_HCD_E_NOTRESPONE:
		//HIUSB_HCD_E_PROTOCOLERR:
		//HIUSB_HCD_E_NOTSUPPORT:
		//HIUSB_HCD_E_BUSERROR:
		//HIUSB_HCD_E_BUSY:

		spin_lock(&urb->lock);
		urb->status = -EPROTO;
		spin_unlock(&urb->lock);
	
		hiusb_hcd_urb_done(hcd, urb, NULL);
	}
}

static void hiusb_hcd_ctrlpipe_proc(struct hiusb_hcd *hcd, struct urb *urb)
{
	int			ret;
	unsigned int		maxpacket;
	struct hiusb_qtd	*qtd;

	/* caller must hold pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));
	hiusb_assert(usb_pipecontrol(urb->pipe));

	qtd = urb->hcpriv;

	hiusb_trace_urb(7, urb, "start ctrlpipe urb");

	maxpacket = (unsigned int)usb_maxpacket(urb->dev, 
			urb->pipe, usb_pipeout(urb->pipe));

	if(usb_pipeout(urb->pipe))
		ret = hiusb_hcd_ctrlpipe_tx(hcd,
				usb_pipedevice(urb->pipe),
				urb->setup_packet,
				urb->transfer_buffer,
				urb->transfer_buffer_length,
				(void*)&urb->actual_length,
				maxpacket,
				&qtd->stage);
	else
		ret = hiusb_hcd_ctrlpipe_rx(hcd,
				usb_pipedevice(urb->pipe),
				urb->setup_packet,
				urb->transfer_buffer,
				urb->transfer_buffer_length,
				(void*)&urb->actual_length,
				maxpacket,
				&qtd->stage);

	if(likely(!ret)){

		//hiusb_dump_buf(urb->transfer_buffer, urb->actual_length);

		hiusb_hcd_urb_done(hcd, urb, NULL);
	}
	else
		hiusb_hcd_handle_error(hcd, urb, ret);
}

static void hiusb_hcd_bulkpipe_proc(struct hiusb_hcd *hcd, struct urb *urb)
{
	int			ret;
	unsigned int		maxpacket, remain;
	struct hiusb_qtd	*qtd;

	/* caller must hold pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));
	hiusb_assert(usb_pipebulk(urb->pipe));

	qtd = urb->hcpriv;

	hiusb_trace_urb(7, urb, "start bulkpipe urb");

	maxpacket = (unsigned int)usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe));

	hiusb_assert(urb->transfer_buffer_length >= urb->actual_length);
	remain = (unsigned int)(urb->transfer_buffer_length - urb->actual_length);

	if( likely(remain >= maxpacket) )
		qtd->actual = maxpacket;
	else
		qtd->actual = remain;

	if( unlikely(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP) ){

		if(!urb->transfer_buffer)
			urb->transfer_buffer = phys_to_virt(urb->transfer_dma);

		/* invalidate dma buf */
//		consistent_sync(urb->transfer_buffer, urb->transfer_buffer_length, DMA_FROM_DEVICE);
	}

	if(usb_pipeout(urb->pipe)){

		ret = hiusb_hcd_bulkpipe_tx_cpumode( hcd,
				usb_pipedevice(urb->pipe),
				usb_pipeendpoint(urb->pipe),
				(char*)urb->transfer_buffer + urb->actual_length,
				qtd->actual,
				maxpacket);
		if(!ret)
			hiusb_hcd_enable_intrtx(hcd, 1<<HIUSB_HCD_BULK_TX_FIFO_INDEX);
	}
	else{
		ret = hiusb_hcd_bulkpipe_rx_cpumode_send_in_token( hcd,
				usb_pipedevice(urb->pipe),
				usb_pipeendpoint(urb->pipe),
				maxpacket);
		if(!ret)
			hiusb_hcd_enable_intrrx(hcd, 1<<HIUSB_HCD_BULK_RX_FIFO_INDEX);
	}

	if(unlikely(ret))
		hiusb_hcd_handle_error(hcd, urb, ret);

}

static int hiusb_hcd_intpipe_proc(struct hiusb_hcd *hcd, struct urb *urb)
{
    hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));
    hiusb_assert(usb_pipeint(urb->pipe));
    
    hiusb_trace_urb(8, urb, "intpipe_submit_urb");
    
    return hiusb_hcd_intpipe_rx(hcd, urb);
}

static void hiusb_hcd_scheduler(void *data)
{
	struct hiusb_hcd	*hcd = data;
	struct hiusb_qtd	*qtd;
	struct urb		*urb;
	int			i;

	device_lock(hcd);

	/* make sure no urb in pending */
	if(test_and_set_bit(HIUSB_HCD_URB_PENDING, &hcd->pending))
		goto out_hold_lock;

rescan:
	urb = NULL;
	qtd = NULL;

	for(i=0; i<MAX_QH_LIST; i++){

		hcd->qh_index ++;
		if(hcd->qh_index >= MAX_QH_LIST)
			hcd->qh_index = 0;

		if(!list_empty(&hcd->qh_list[hcd->qh_index])){

			qtd = list_entry(hcd->qh_list[hcd->qh_index].next, struct hiusb_qtd, qtd_list);

			break;
		}
	}

	if(i>=MAX_QH_LIST){
		hiusb_trace(6, "no urb");
		goto out_hold_pending;
	}
	else{
		urb = qtd->urb;
		hiusb_assert(urb);

		if (!hcd->rh_port_connect || hcd->rh_port_change){

			spin_lock(&urb->lock);
			urb->status = -ECONNRESET;
			spin_unlock(&urb->lock);
		}

		if (urb->status != -EINPROGRESS) {
			/* likely it was just unlinked */

			spin_lock(&urb->lock);
			urb->hcpriv = NULL;
			spin_unlock(&urb->lock);

			list_del (&qtd->qtd_list);
			kfree (qtd);

			device_unlock(hcd);
			usb_hcd_giveback_urb (hiusb_to_hcd(hcd), urb, NULL);
			device_lock(hcd);

			goto rescan;
		}
	}

	device_unlock(hcd);

	/* here: we find urb and hold urb pending */
	hiusb_trace_urb(6, urb, "find urb");

	switch(usb_pipetype(urb->pipe)) {
		case PIPE_CONTROL:
			hiusb_hcd_ctrlpipe_proc(hcd, urb);
			break;
		case PIPE_BULK:
			hiusb_hcd_bulkpipe_proc(hcd, urb);
			break;
		case PIPE_INTERRUPT:
			hiusb_hcd_intpipe_proc(hcd, urb);
			break;
		case PIPE_ISOCHRONOUS:
			hiusb_error("not support isochronous.");
			break;
		default:
			hiusb_error("pipetype error.");
			break;
	}

	return;

out_hold_pending:
	clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

out_hold_lock:
	device_unlock(hcd);

	return;
}

static int hiusb_hcd_urb_enqueue (
	struct usb_hcd		*usb_hcd,
	struct usb_host_endpoint *ep,
	struct urb		*urb,
	unsigned		mem_flags)
{
	int			res = 0;
	struct hiusb_qtd	*qtd;
	struct hiusb_hcd	*hcd = hcd_to_hiusb(usb_hcd);
	int			ep_num = usb_pipeendpoint(urb->pipe);
	int			qh_index;

	if (!hcd->rh_port_connect || hcd->rh_port_change){
		res = -ECONNRESET;
		goto error_exit;
	}

	hiusb_trace_urb(7, urb, "urb enqueue");

	urb->actual_length = 0;

	/* ralink client call urb qnqueue in tasklet (in atomic) */
	/*qtd = kzalloc (sizeof *qtd, mem_flags);*/
	qtd = kzalloc (sizeof *qtd, GFP_ATOMIC);
	if (!qtd){
		hiusb_error("out of memory");
		res = -ENOMEM;
		goto error_exit;
	}
	qtd->urb = urb;

	if(usb_pipecontrol(urb->pipe)){

		hiusb_assert(ep_num == 0);
		qh_index = ep_num;
	}
	else if(usb_pipebulk(urb->pipe)){

		hiusb_assert(ep_num != 0);
		if(!usb_pipeout(urb->pipe))
			qh_index = (ep_num<<1) - 1;
		else
			qh_index = ep_num<<1;
	}
	else if(usb_pipeint(urb->pipe)){
		qh_index = ep_num;
	}
	else{
		hiusb_error("not support interrupt/isochronous pipe");
		res = -ENOMEM;
		goto error_exit;
	}

	device_lock(hcd);

	list_add_tail (&qtd->qtd_list, &hcd->qh_list[qh_index]);

	spin_lock(&urb->lock);
	urb->hcpriv = qtd;
	spin_unlock(&urb->lock);
	
	/* kick the scheduler */
	if(hcd->rh_port_connect && !hcd->rh_port_change 
			&& list_empty(&hcd->wks_scheduler.entry)){

		PREPARE_WORK(&hcd->wks_scheduler, hiusb_hcd_scheduler, hcd);
		queue_work(hcd->wq_scheduler, &hcd->wks_scheduler);
	}

	device_unlock(hcd);

error_exit:
	return res;
}

static int hiusb_hcd_urb_dequeue (struct usb_hcd *usb_hcd, struct urb *urb)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);

	hiusb_trace_urb(7, urb, "urb dequeue");

	device_lock(hcd);

	/* usb core mark urb->status, when it unlink urb */

	/* giveback happens automatically in scheduler,
	 * so make sure the wq_scheduler happens */
	if(list_empty(&hcd->wks_scheduler.entry)){

		PREPARE_WORK(&hcd->wks_scheduler, hiusb_hcd_scheduler, hcd);
		queue_work(hcd->wq_scheduler, &hcd->wks_scheduler);
	}

	device_unlock(hcd);

	return 0;
}

static void hiusb_hcd_handle_ctrlpipe_intr(struct hiusb_hcd *hcd)
{
	BUG();
}

static void hiusb_hcd_handle_txpipe_intr(struct hiusb_hcd *hcd, int intrtx)
{
	int			ret;
	unsigned int		maxpacket, fifo_size;
	struct hiusb_qtd	*qtd;
	struct urb		*urb;

	/* caller must hold pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));

	qtd = list_entry (hcd->qh_list[hcd->qh_index].next, struct hiusb_qtd, qtd_list);
	urb = qtd->urb;

	hiusb_assert(intrtx & (1<<HIUSB_HCD_BULK_TX_FIFO_INDEX));
	hiusb_assert(usb_pipebulk(urb->pipe));
	hiusb_assert(usb_pipeout(urb->pipe));

	hiusb_trace_urb(7, urb, "handle txpipe intr");

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		ret = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	ret = hiusb_hcd_bulkpipe_tx_cpumode_check_clear_error(hcd);
	if(unlikely(ret))
		goto error_exit;

	maxpacket = (unsigned int)usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe));

	/* notes: wait special double fifo intrtx
	 * all fifo is double fifo, so if txmaxp*2<=fifo-size we ought to handle intrtx towice */
	fifo_size = (unsigned int)hiusb_hcd_get_fifo_size_tx(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	if( likely((maxpacket<<1) <= fifo_size) ){

		if( hiusb_hcd_tx_fifo_no_empty(hcd) ){

			hiusb_trace(5, "wait for special double fifo intrtx");
			hiusb_hcd_enable_intrtx(hcd, 1<<HIUSB_HCD_BULK_TX_FIFO_INDEX);

			goto out;
		}
	}

	/* update actual_length */
	urb->actual_length += qtd->actual;

	if(urb->actual_length >= urb->transfer_buffer_length){

		/* refer USB2.0 spec */
		if (usb_pipebulk (urb->pipe)
			&& (urb->transfer_flags & URB_ZERO_PACKET)
			&& !(urb->transfer_buffer_length % maxpacket)
			&& (qtd->actual == maxpacket) ) {

			hiusb_trace(7, "send zero packet");

			/* send zero packet */
			qtd->actual = 0;
			ret = hiusb_hcd_bulkpipe_tx_cpumode( hcd,
					usb_pipedevice(urb->pipe),
					usb_pipeendpoint(urb->pipe),
					NULL,
					qtd->actual,
					maxpacket);
			if(!ret)
				hiusb_hcd_enable_intrtx(hcd, 1<<HIUSB_HCD_BULK_TX_FIFO_INDEX);
		}
		else
			hiusb_hcd_urb_done(hcd, urb, NULL);
	}
	else{
		qtd->actual = min(urb->transfer_buffer_length - urb->actual_length, 
				(int)maxpacket);

		ret = hiusb_hcd_bulkpipe_tx_cpumode( hcd,
				usb_pipedevice(urb->pipe),
				usb_pipeendpoint(urb->pipe),
				(char*)urb->transfer_buffer + urb->actual_length,
				qtd->actual,
				maxpacket);
		if(!ret)
			hiusb_hcd_enable_intrtx(hcd, 1<<HIUSB_HCD_BULK_TX_FIFO_INDEX);
	}

error_exit:
	if(unlikely(ret))
		hiusb_hcd_handle_error(hcd, urb, ret);
out:
	return;
}

static void hiusb_hcd_handle_rxpipe_intr(struct hiusb_hcd *hcd, int intrrx)
{
	int			ret;
	unsigned int		maxpacket;
	struct hiusb_qtd	*qtd;
	struct urb		*urb;

	/* caller must hold pending */
	hiusb_assert(test_bit(HIUSB_HCD_URB_PENDING, &hcd->pending));

	qtd = list_entry (hcd->qh_list[hcd->qh_index].next, struct hiusb_qtd, qtd_list);
	urb = qtd->urb;

	hiusb_assert(intrrx & (1<<HIUSB_HCD_BULK_RX_FIFO_INDEX));
	hiusb_assert(usb_pipebulk(urb->pipe)||usb_pipeint(urb->pipe));
	hiusb_assert(usb_pipein(urb->pipe));

	hiusb_trace_urb(7, urb, "handle rxpipe intr");

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		ret = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	ret = hiusb_hcd_bulkpipe_rx_cpumode_check_clear_error(hcd);
	if(unlikely(ret))
		goto error_exit;

	maxpacket = (unsigned int)usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe));

	if( likely(urb->actual_length < urb->transfer_buffer_length) ){

		/* packet is ready, copy to buf */
		hiusb_hcd_bulkpipe_rx_cpumode_finish( hcd,
				((char*)urb->transfer_buffer) + urb->actual_length,
				urb->transfer_buffer_length - urb->actual_length,
				&qtd->actual);

		if(unlikely(ret)){
			hiusb_error("rx cpumode error");
			goto error_exit;
		}

		/* update actual_length */
		urb->actual_length += qtd->actual;
	}

	if( likely(urb->actual_length >= urb->transfer_buffer_length) ){

		hiusb_hcd_urb_done(hcd, urb, NULL);
	}
	else if(qtd->actual < maxpacket){

		/* recv a short pkt */
		spin_lock(&urb->lock);
		urb->status = -EREMOTEIO;
		spin_unlock(&urb->lock);

		hiusb_hcd_urb_done(hcd, urb, NULL);
	}
	else{
		/* send in token and wait next packet */
		ret = hiusb_hcd_bulkpipe_rx_cpumode_send_in_token( hcd,
				usb_pipedevice(urb->pipe),
				usb_pipeendpoint(urb->pipe),
				maxpacket);
		if(!ret)
			hiusb_hcd_enable_intrrx(hcd, 1<<HIUSB_HCD_BULK_RX_FIFO_INDEX);
	}

error_exit:
	if(unlikely(ret))
		hiusb_hcd_handle_error(hcd, urb, ret);

	return;
}

static int hiusb_hcd_get_speed_mode(struct hiusb_hcd *hcd)
{
	int res = 0;
	int devctl = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);
	int power = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	if(devctl & BIT_COMM_DEVCTL_LOWSPEED){
		hcd->rh_port_speed = 1<<USB_PORT_FEAT_LOWSPEED;
	}
	else if(devctl & BIT_COMM_DEVCTL_FULLSPEED){
		if(power & BIT_COMM_POWER_HIGHSPEED_MODE)
			hcd->rh_port_speed = 1<<USB_PORT_FEAT_HIGHSPEED;
		else
			hcd->rh_port_speed = 0;
	}
	else{
		hiusb_error("hw abnormal");
		res = -1;
	}

	hiusb_trace(4, "speed mode (0x%.8x)", hcd->rh_port_speed);
	return res;
}

static int hiusb_hcd_connect(struct hiusb_hcd *hcd)
{
	int res = 0;
	int devctl;

	device_lock(hcd);

	devctl = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	if(!(devctl & BIT_COMM_DEVCTL_HOST_MODE)){
		hiusb_error("not host mode");
		res = -1;
		goto error_exit;
	}

	res = hiusb_hcd_get_speed_mode(hcd);

	hcd->rh_port_change = USB_PORT_STAT_C_CONNECTION;
	hcd->rh_port_connect = 1;

error_exit:

	device_unlock(hcd);

	return res;
}

static void hiusb_hcd_handle_status_intr(struct hiusb_hcd *hcd, int intrusb)
{
	int res;

	if(intrusb & BIT_COMM_INTRUSB_VBUS_ERROR){
		hiusb_error("vbus error has detected.");
		goto error_exit;
	}

	if(intrusb & BIT_COMM_INTRUSB_SESSION_REQ){
		hiusb_error("session request signaling has detected.");
	}

	if(intrusb & BIT_COMM_INTRUSB_DISCONNECT){
		hiusb_trace(8, "device disconnect detected.");
		goto error_exit;
	}

	if(intrusb & BIT_COMM_INTRUSB_CONNECT){
		hiusb_trace(8, "device connect detected.");

		if(hcd->rh_port_connect){
			//hiusb_error("error connect intr");
			goto error_exit;
		}

		res = hiusb_hcd_connect(hcd);
		if(res)
			goto error_exit;
	}

	if(intrusb & BIT_COMM_INTRUSB_SOF){
		hiusb_trace(1, "SOF detected.");
	}

	if(intrusb & BIT_COMM_INTRUSB_BABBLE){
		hiusb_error("babble is detected.");
		goto error_exit;
	}

	if(intrusb & BIT_COMM_INTRUSB_RESUME){
		hiusb_error("resume signaling detected.");
		goto error_exit;
	}

	if(intrusb & BIT_COMM_INTRUSB_SUSPEND){
		hiusb_error("suspend signaling detected.");
		goto error_exit;
	}

	return;

error_exit:

	device_lock(hcd);
	/* reset roothub */
	hcd->rh_port_change = 1;
	hcd->rh_port_connect = 0;
	hcd->rh_port_speed = 0;
	hcd->rh_port_reset = 0;
	device_unlock(hcd);

	return;
}

static void hiusb_hcd_bh_proc(unsigned long data)
{
	struct hiusb_hcd *hcd = (void*)data;

	if(unlikely(hcd->intrusb))
		hiusb_hcd_handle_status_intr(hcd, hcd->intrusb);

	if(unlikely(hcd->intrtx & HIUSB_HCD_CTRLPIPE_INTR_MASK))
		hiusb_hcd_handle_ctrlpipe_intr(hcd);

	if(likely(hcd->intrtx & HIUSB_HCD_TXPIPE_INTR_MASK))
		hiusb_hcd_handle_txpipe_intr(hcd, hcd->intrtx);

	if(likely(hcd->intrrx & HIUSB_HCD_RXPIPE_INTR_MASK))
		hiusb_hcd_handle_rxpipe_intr(hcd, hcd->intrrx);

	/* always enable intrusb */
	hiusb_hcd_enable_intrusb(hcd, BIT_COMM_INTRUSB_VBUS_ERROR
			| BIT_COMM_INTRUSB_DISCONNECT
			| BIT_COMM_INTRUSB_CONNECT
			| BIT_COMM_INTRUSB_BABBLE);
}

static irqreturn_t hiusb_hcd_isr(struct usb_hcd *usb_hcd, struct pt_regs *ptregs)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);

	hcd->intrtx = hiusb_hcd_read_clear_intrtx(hcd);
	hcd->intrrx = hiusb_hcd_read_clear_intrrx(hcd);
	hcd->intrusb = hiusb_hcd_read_clear_intrusb(hcd);

	hiusb_trace(3, "intrtx=0x%.4x, intrrx=0x%.4x, intrusb=0x%.4x", 
			hcd->intrtx, hcd->intrrx, hcd->intrusb);

	if(hcd->intrusb && !(hcd->intrtx & HIUSB_HCD_TXPIPE_INTR_MASK)
			&& !(hcd->intrrx & HIUSB_HCD_RXPIPE_INTR_MASK)
			&& (hcd->intrtx_mask || hcd->intrrx_mask)){
		if(hcd->intrtx_mask){
			hiusb_writeb(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX, HIUSB_COMMON_REG_FIFO_INDEX);
			hcd->intrtx |= HIUSB_HCD_TXPIPE_INTR_MASK;
			//hiusb_error("T 0x%x", hiusb_readw(hcd, HIUSB_HCD_TXCSR));
		}
		if(hcd->intrrx_mask){
			hiusb_writeb(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX, HIUSB_COMMON_REG_FIFO_INDEX);
			hcd->intrrx |= HIUSB_HCD_RXPIPE_INTR_MASK;
			//hiusb_error("R 0x%x", hiusb_readw(hcd, HIUSB_HCD_RXCSR));
		}
	}

	hiusb_hcd_disable_intrtx(hcd, ~0);
	hiusb_hcd_disable_intrrx(hcd, ~0);
	hiusb_hcd_disable_intrusb(hcd, ~0);

	tasklet_schedule(&hcd->bh_isr);

	return IRQ_HANDLED;
}

static void hiusb_hcd_endpoint_disable(struct usb_hcd *usb_hcd, struct usb_host_endpoint *ep)
{
	hiusb_trace(6, "endpoint(%p) disable", ep);

	return;
}

static int hiusb_hcd_get_frame (struct usb_hcd *usb_hcd)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);

	return hiusb_hcd_read_frame(hcd);
}

static int hiusb_hcd_hub_status_data (struct usb_hcd *usb_hcd, char *buf)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);
	unsigned int status = 0;
	int ports, i, retval = 1;

	/* if !USB_SUSPEND, root hub timers won't get shut down ... */
	if (!HC_IS_RUNNING(usb_hcd->state))
		return 0;

	/* init status to no-changes */
	buf [0] = 0;
	ports = 1;
	if (ports > 7) {
		buf [1] = 0;
		retval++;
	}
	
	/* no hub change reports (bit 0) for now (power, ...) */

	/* port N changes (bit N)? */
	device_lock(hcd);

	if(hcd->rh_port_change){

		for (i = 0; i < ports; i++) {

			if (i < 7)
				buf [0] |= 1 << (i + 1);
			else
				buf [1] |= 1 << (i - 7);
			status = 1;
		}
	}

	device_unlock(hcd);

	hiusb_trace(6, "hub status data 0x%.8x", status ? retval : 0);

	return status ? retval : 0;
}

static void hiusb_hcd_hub_descriptor (struct hiusb_hcd *hcd,
		struct usb_hub_descriptor *desc)
{
	int ports = 1;
	unsigned short temp;

	desc->bDescriptorType = 0x29;
	desc->bPwrOn2PwrGood = 50;	/* refer vitual root hub */
	desc->bHubContrCurrent = 0;

	desc->bNbrPorts = ports;
	temp = 1 + (ports / 8);
	desc->bDescLength = 7 + 2 * temp;

	/* two bitmaps:  ports removable, and usb 1.0 legacy PortPwrCtrlMask */
	memset (&desc->bitmap [0], 0, temp);
	memset (&desc->bitmap [temp], 0xff, temp);

	temp = 0x0008;		/* per-port overcurrent reporting */
	temp |= 0x0001;		/* per-port power control */
	//temp |= 0x0002;	/* no power switching */

	desc->wHubCharacteristics = (__force __u16)cpu_to_le16 (temp);
}

static int hiusb_hcd_hub_control(struct usb_hcd	*usb_hcd, 
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);
	int		ports = 1;
	unsigned int	power, devctl, status;
	int		retval = 0;

	device_lock(hcd);

	power = hiusb_readb(hcd, HIUSB_COMMON_POWER);
	devctl = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	switch (typeReq) {
	case ClearHubFeature:
		switch (wValue) {
		case C_HUB_LOCAL_POWER:
		case C_HUB_OVER_CURRENT:
			/* no hub-wide feature/status flags */
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_ENABLE");
			break;
		case USB_PORT_FEAT_C_ENABLE:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_C_ENABLE");
			break;
		case USB_PORT_FEAT_SUSPEND:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_SUSPEND");
			break;
		case USB_PORT_FEAT_C_SUSPEND:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_C_SUSPEND");
			break;
		case USB_PORT_FEAT_POWER:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_POWER");
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_C_CONNECTION");
			hcd->rh_port_change = 0;
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_C_OVER_CURRENT");
			break;
		case USB_PORT_FEAT_C_RESET:
			hiusb_trace(4, "ClearPortFeature : USB_PORT_FEAT_C_RESET");
			break;
		default:
			goto error;
		}
		break;
	case GetHubDescriptor:
		hiusb_hcd_hub_descriptor(hcd, (struct usb_hub_descriptor *)buf);
		break;
	case GetHubStatus:
		/* no hub-wide feature/status flags */
		memset (buf, 0, 4);
		break;
	case GetPortStatus:
		if (!wIndex || wIndex > ports)
			goto error;
		status = 0;

		// wPortChange bits

		/* we must update speed mode first */
		if(power & BIT_COMM_POWER_RESET){

			if(time_after(jiffies, hcd->rh_port_reset)){
				/* reset done */

				hiusb_writeb(hcd, power & (~BIT_COMM_POWER_RESET), HIUSB_COMMON_POWER);

				/* we must update speed mode first, 
				 * speed mode maybe change after reset */
				hiusb_hcd_get_speed_mode(hcd);

				hcd->rh_port_reset = 0;

				status |= 1 << USB_PORT_FEAT_C_RESET;
			}
			else{
				/* reset in progress */
				status |= 1 << USB_PORT_FEAT_RESET;
			}
		}

		if(hcd->rh_port_change)
			status |= 1 << USB_PORT_FEAT_C_CONNECTION;

		if(hcd->rh_port_connect){
			status |= 1 << USB_PORT_FEAT_CONNECTION;

			status |= hcd->rh_port_speed;

			status |= 1 << USB_PORT_FEAT_ENABLE;
		}

		//status |= 1 << USB_PORT_FEAT_C_ENABLE;
		//status |= 1 << USB_PORT_FEAT_ENABLE;
		status |= 1 << USB_PORT_FEAT_POWER;

		// we "know" this alignment is good, caller used kmalloc()...
		*((__le32 *) buf) = cpu_to_le32 (status);

		hiusb_trace(4, "GetPortStatus (0x%.8x)", status);
		break;
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_LOCAL_POWER:
		case C_HUB_OVER_CURRENT:
			/* no hub-wide feature/status flags */
			break;
		default:
			goto error;
		}
		break;
	case SetPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;

		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			hiusb_trace(4, "SetPortFeature : USB_PORT_FEAT_SUSPEND");
			break;
		case USB_PORT_FEAT_POWER:
			hiusb_trace(4, "SetPortFeature : USB_PORT_FEAT_POWER");
			break;
		case USB_PORT_FEAT_RESET:
			hiusb_trace(4, "SetPortFeature : USB_PORT_FEAT_RESET");
			hiusb_writeb(hcd, power | BIT_COMM_POWER_RESET, HIUSB_COMMON_POWER);
			hcd->rh_port_reset = jiffies + msecs_to_jiffies (50);
			break;
		default:
			goto error;
		}
		break;

	default:
error:
		/* "stall" on error */
		retval = -EPIPE;
	}

	device_unlock(hcd);

	return retval;
}

/* timer callback */
static void watch_dog_timer_func(unsigned long data)
{
	struct hiusb_hcd* hcd = (void*)data;

	if(!hcd->watch_dog_reset){

			if(!hiusb_hcd_hw_status_check(hcd))
				goto done;
			else
				goto clear_power;
	}
	else if(hiusb_hcd_get_vbus_level(hcd) == HIUSB_VBUS_LEVEL_CLEAR){

		/* vbus power supply */
		sys_hiusb_vbus_power_supply();

		/* hw-sw sync */
		hiusb_hcd_hw_sw_sync(hcd);

		/* enable intrusb, wait for connect event */
		hiusb_hcd_enable_intrusb(hcd, BIT_COMM_INTRUSB_VBUS_ERROR
				| BIT_COMM_INTRUSB_DISCONNECT
				| BIT_COMM_INTRUSB_CONNECT
				| BIT_COMM_INTRUSB_BABBLE);

		/* clear pending */
		clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

		goto done;
	}

clear_power:
	/* vbus power down */
	sys_hiusb_vbus_power_down();

	hcd->watch_dog_reset = 1;
	mod_timer(&hcd->watch_dog_timer, jiffies 
			+ msecs_to_jiffies(hcd->watch_dog_timeval));

	return;

done:
	/* hw status is ok */
	hcd->watch_dog_reset = 0;
	mod_timer(&hcd->watch_dog_timer, jiffies 
			+ msecs_to_jiffies(hcd->watch_dog_timeval));

	return;
}

static int hiusb_hcd_init(struct usb_hcd* usb_hcd)
{
	int i, ret = 0;
	struct hiusb_hcd* hcd = hcd_to_hiusb(usb_hcd);

	memset(hcd, 0, sizeof(*hcd));
	
	/* init device lock */
	device_lock_init(hcd);

	/* init io base */
	hcd->iobase = (unsigned long)usb_hcd->regs;
	hcd->iobase_phys = (unsigned long)usb_hcd->rsrc_start;

	/* init urb pending */
	clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

	/* init all qtd_list */
	for(i=0; i<MAX_QH_LIST; i++){
		INIT_LIST_HEAD (&hcd->qh_list[i]);
	}

	/* init port attribute */
	hcd->rh_port_change = 0;
	hcd->rh_port_connect = 0;
	hcd->rh_port_speed = 0;
	hcd->rh_port_reset = 0;

	/* init tasklet */
	hcd->bh_isr.next = NULL;
	hcd->bh_isr.state = 0;
	hcd->bh_isr.func = hiusb_hcd_bh_proc;
	hcd->bh_isr.data = (unsigned long)hcd;
	atomic_set(&hcd->bh_isr.count, 0);

	/* init workqueue */
	hcd->wq_scheduler = create_singlethread_workqueue("hiusb_scheduler");
	if(hcd->wq_scheduler ==NULL) {
		hiusb_error("create_singlethread_workqueue failed");
		ret = -1;
		goto create_wq_error_exit;
	}
	INIT_WORK(&hcd->wks_scheduler, hiusb_hcd_scheduler, hcd);

	/* init watch dog timer */
	init_timer(&hcd->watch_dog_timer);
	hcd->watch_dog_timer.function = watch_dog_timer_func;
	hcd->watch_dog_timer.data = (unsigned long) hcd;
	hcd->watch_dog_timeval = watch_dog_timeval;
	hcd->watch_dog_reset = 0;

	/* init default intrmask */
	hcd->intrtx_mask = 0;
	hcd->intrrx_mask = 0;
	hcd->intrusb_mask = BIT_COMM_INTRUSB_VBUS_ERROR
		| BIT_COMM_INTRUSB_DISCONNECT
		| BIT_COMM_INTRUSB_CONNECT
		| BIT_COMM_INTRUSB_BABBLE;

	/* module paramers */
	hcd->ctrlpipe_nak_limit = ctrlpipe_nak_limit;
	hcd->bulkpipe_nak_limit = bulkpipe_nak_limit;

	return 0;

create_wq_error_exit:
	/* exit device lock */
	device_lock_exit(&hiusb_hcd);

	return ret;
}

static void hiusb_hcd_exit(struct hiusb_hcd *hcd)
{
	sys_hiusb_vbus_power_down();

	/* hw halt */
	sys_hiusb_exit();

	flush_workqueue(hcd->wq_scheduler);
	destroy_workqueue(hcd->wq_scheduler);

	device_lock_exit(&hiusb_hcd);
}

static void hiusb_hcd_start_hc(struct platform_device *dev)
{
	/* enable host controller clock */
	sys_hiusb_init();
}

static void hiusb_hcd_stop_hc(struct platform_device *dev)
{
	/* disable clock */
	sys_hiusb_exit();
}

static int hiusb_hcd_probe(const struct hc_driver *driver, struct platform_device *dev)
{
	int retval;
	struct usb_hcd *usb_hcd;

	if(dev->resource[1].flags != IORESOURCE_IRQ) {
		hiusb_error("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}

	usb_hcd = usb_create_hcd(driver, &dev->dev, HIUSB_HCD_DEV_NAME);
	if (!usb_hcd){
		hiusb_error("usb_create_hcd fail");
		return -ENOMEM;
	}
	usb_hcd->rsrc_start = dev->resource[0].start;
	usb_hcd->rsrc_len = dev->resource[0].end - dev->resource[0].start + 1;

	if (!request_mem_region(usb_hcd->rsrc_start, usb_hcd->rsrc_len, HIUSB_HCD_DEV_NAME)) {
		hiusb_error("request_mem_region failed");
		retval = -EBUSY;
		goto err1;
	}

	usb_hcd->regs = ioremap(usb_hcd->rsrc_start, usb_hcd->rsrc_len);
	if (!usb_hcd->regs) {
		hiusb_error("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}

	/* enable clock and reset */
	hiusb_hcd_start_hc(dev);

	/* init hiusb priv resource */
	retval = hiusb_hcd_init(usb_hcd);
	if(retval){
		hiusb_error("hiusb_hcd_init error");
		goto err3;
	}

	retval = usb_add_hcd(usb_hcd, dev->resource[1].start, SA_INTERRUPT);
	if (retval == 0)
		return retval;
	else
		hiusb_error("usb_add_hcd fail");

err3:
	/* disable clock */
	hiusb_hcd_stop_hc(dev);
	iounmap(usb_hcd->regs);
err2:
	release_mem_region(usb_hcd->rsrc_start, usb_hcd->rsrc_len);
err1:
	usb_put_hcd(usb_hcd);

	return retval;
}

static int hiusb_hcd_remove(struct usb_hcd *usb_hcd, struct platform_device *dev)
{
	usb_remove_hcd(usb_hcd);

	/* disable clock */
	hiusb_hcd_stop_hc(dev);

	iounmap(usb_hcd->regs);

	release_mem_region(usb_hcd->rsrc_start, usb_hcd->rsrc_len);

	usb_put_hcd(usb_hcd);

	return 0;
}

static int hiusb_hcd_start (struct usb_hcd *usb_hcd)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);

	hiusb_trace(6, "hcd start");

	/* hw enable */
	sys_hiusb_init();

	/* hw-sw sync */
	hiusb_hcd_hw_sw_sync(hcd);

	/* enable intrusb, wait for connect event */
	hiusb_hcd_enable_intrusb(hcd, BIT_COMM_INTRUSB_VBUS_ERROR
			| BIT_COMM_INTRUSB_DISCONNECT
			| BIT_COMM_INTRUSB_CONNECT
			| BIT_COMM_INTRUSB_BABBLE);

	/* tell kernel hcd is running */
	usb_hcd->state = HC_STATE_RUNNING;

	/* clear pending */
	clear_bit(HIUSB_HCD_URB_PENDING, &hcd->pending);

	/* start watch dog timer */
	hcd->watch_dog_reset = 0;
	mod_timer(&hcd->watch_dog_timer, jiffies 
			+ msecs_to_jiffies(hcd->watch_dog_timeval));

	return 0;
}

static void hiusb_hcd_stop (struct usb_hcd *usb_hcd)
{
	struct hiusb_hcd *hcd = hcd_to_hiusb(usb_hcd);

	hiusb_trace(6, "hcd stop");

	/* tell kernel hcd is stop */
	usb_hcd->state = HC_STATE_HALT;

	/* del watch dog timer */
	del_timer_sync(&hcd->watch_dog_timer);
	
	/* release all hisub priv resource */
	hiusb_hcd_exit(hcd);
}

static int hiusb_hcd_reset (struct usb_hcd *usb_hcd)
{
	hiusb_trace(6, "hcd reset");

	return hiusb_hcd_start(usb_hcd);
}

static const struct hc_driver hiusb_hcd_hc_driver = {
	.description =		HIUSB_HCD_DEV_NAME,
	.product_desc =		"Hisilicon USB host controller",
	.hcd_priv_size =	sizeof(struct hiusb_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq =			hiusb_hcd_isr,
	.flags =		HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.start =		hiusb_hcd_start,
	.stop =			hiusb_hcd_stop,
	.reset =		hiusb_hcd_reset,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		hiusb_hcd_urb_enqueue,
	.urb_dequeue =		hiusb_hcd_urb_dequeue,
	.endpoint_disable =	hiusb_hcd_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number =	hiusb_hcd_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data =	hiusb_hcd_hub_status_data,
	.hub_control =		hiusb_hcd_hub_control,
};

static int hiusb_hcd_drv_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	if (usb_disabled())
		return -ENODEV;

	return hiusb_hcd_probe(&hiusb_hcd_hc_driver, pdev);
}

static int hiusb_hcd_drv_remove(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct usb_hcd *usb_hcd = dev_get_drvdata(dev);

	hiusb_hcd_remove(usb_hcd, pdev);
	return 0;
}

static struct device_driver hiusb_hcd_driver = {
	.owner		= THIS_MODULE,
	.name		= HIUSB_HCD_DEV_NAME,
	.bus		= &platform_bus_type,
	.probe		= hiusb_hcd_drv_probe,
	.remove		= hiusb_hcd_drv_remove,

	/*.suspend	= hiusb_hcd_drv_suspend, */
	/*.resume	= hiusb_hcd_drv_resume, */
};

/*-------------------------------------------------------------------------*/

static struct resource hiusb_res[] = {
	[0] = {
		.start = REG_BASE_HIUSB,
		.end   = REG_BASE_HIUSB + REG_HIUSB_IOSIZE,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INTNR_HIUSB,
		.end   = INTNR_HIUSB,
		.flags = IORESOURCE_IRQ,
	},
};

static void hiusb_hcd_platdev_release (struct device *dev)
{
	/* These don't need to do anything because the pdev structures are
	 * statically allocated. */
}

static struct platform_device hiusb_platdev= {
	.name	= HIUSB_HCD_DEV_NAME,
	.id	= 0,
	.dev	= {
		.platform_data	= NULL,
		.dma_mask = (u64*)~0,
		.coherent_dma_mask = (u64)~0,
		.release = hiusb_hcd_platdev_release,
	},
	.num_resources = ARRAY_SIZE(hiusb_res),
	.resource	= hiusb_res,
};

unsigned int intr_delay_time=10;
module_param(intr_delay_time,int,0444);
MODULE_PARM_DESC(intr_delay_time,
	                "Set the number of ticks to ignore suspend bounces");

static int __init hiusb_hcd_drv_init(void)
{
	int	retval;

	if (usb_disabled ())
		return -ENODEV;

	retval = driver_register(&hiusb_hcd_driver);
	if (retval)
		goto error_exit;

	retval = platform_device_register(&hiusb_platdev);
	if (!retval)
		return retval;

	driver_unregister(&hiusb_hcd_driver);

error_exit:
	return retval;
}

static void __exit hiusb_hcd_drv_exit(void)
{
	driver_unregister(&hiusb_hcd_driver);

	platform_device_unregister(&hiusb_platdev);
}

module_init(hiusb_hcd_drv_init);
module_exit(hiusb_hcd_drv_exit);

MODULE_DESCRIPTION("Hisilicon USB host controller driver");
MODULE_AUTHOR("Zhonglin Jiang");
MODULE_LICENSE("GPL");
