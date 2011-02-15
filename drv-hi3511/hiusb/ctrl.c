#include "hiusb.h"
#include "ctrl.h"

unsigned int usb_poweroff_flag=0;

static inline int _hcd_set_addr(struct hiusb_hcd *hcd, unsigned int addr)
{
	int old;

	old = hiusb_readb(hcd, HIUSB_COMMON_FADDR);

	hiusb_writeb(hcd, addr & HIUSB_COMMON_FADDR_MASK, HIUSB_COMMON_FADDR);

	return old;
}

int hiusb_hcd_set_addr(struct hiusb_hcd *hcd, unsigned int addr)
{
	int old;

	device_lock(hcd);

	old = _hcd_set_addr(hcd, addr);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_get_addr(struct hiusb_hcd *hcd)
{
	int addr;

	device_lock(hcd);

	addr = hiusb_readb(hcd, HIUSB_COMMON_FADDR);

	device_unlock(hcd);

	return addr;
}

int hiusb_hcd_read_power(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_enable_highspeed_mode(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	hiusb_writeb(hcd, old | BIT_COMM_POWER_HIGHSPEED_EN, HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old & BIT_COMM_POWER_HIGHSPEED_EN;
}

int hiusb_hcd_disable_highspeed_mode(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	hiusb_writeb(hcd, old & (~BIT_COMM_POWER_HIGHSPEED_EN), HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old & BIT_COMM_POWER_HIGHSPEED_EN;
}

int hiusb_hcd_read_highspeed_mode(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old & BIT_COMM_POWER_HIGHSPEED_MODE;
}

int hiusb_hcd_send_reset_signal(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	hiusb_writeb(hcd, old | BIT_COMM_POWER_RESET, HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old & BIT_COMM_POWER_RESET;
}

int hiusb_hcd_clear_reset_signal(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	hiusb_writeb(hcd, old & (~BIT_COMM_POWER_RESET), HIUSB_COMMON_POWER);

	device_unlock(hcd);

	return old & BIT_COMM_POWER_RESET;
}

int hiusb_hcd_read_clear_intrtx(struct hiusb_hcd *hcd)
{
	int status;

	device_lock(hcd);

	status = hiusb_readw(hcd, HIUSB_COMMON_INTRTX); 

	status &= hcd->intrtx_mask;

	device_unlock(hcd);

	return status;
}

int hiusb_hcd_read_clear_intrrx(struct hiusb_hcd *hcd)
{
	int status;

	device_lock(hcd);

	status = hiusb_readw(hcd, HIUSB_COMMON_INTRRX); 

	status &= hcd->intrrx_mask;

	device_unlock(hcd);

	return status;
}

int hiusb_hcd_read_clear_intrusb(struct hiusb_hcd *hcd)
{
	int status;

	device_lock(hcd);

	status = hiusb_readb(hcd, HIUSB_COMMON_INTRUSB); 

	status &= hcd->intrusb_mask;

	device_unlock(hcd);

	return status;
}

int hiusb_hcd_read_clear_intrdma(struct hiusb_hcd *hcd)
{
	int status;

	device_lock(hcd);

	status = hiusb_readb(hcd, HIUSB_HCD_DMA_INTR); 

	device_unlock(hcd);

	return status;
}

int hiusb_hcd_enable_intrtx(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_COMMON_INTRTX_EN); 

	hiusb_writew(hcd, old | irqs, HIUSB_COMMON_INTRTX_EN);

	hcd->intrtx_mask |= irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_enable_intrrx(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_COMMON_INTRRX_EN); 

	hiusb_writew(hcd, old | irqs, HIUSB_COMMON_INTRRX_EN);

	hcd->intrrx_mask |= irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_enable_intrusb(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_INTRUSB_EN); 

	hiusb_writeb(hcd, old | irqs, HIUSB_COMMON_INTRUSB_EN); 

	hcd->intrusb_mask |= irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_enable_intrdma(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_HCD_DMA_CNTL); 

	hiusb_writew(hcd, old | BIT_HCD_DMA_CNTL_INTR_EN, HIUSB_HCD_DMA_CNTL); 

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_disable_intrtx(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_COMMON_INTRTX_EN); 

	hiusb_writew(hcd, old & (~irqs), HIUSB_COMMON_INTRTX_EN);

	hcd->intrtx_mask &= ~irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_disable_intrrx(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_COMMON_INTRRX_EN); 

	hiusb_writew(hcd, old & (~irqs), HIUSB_COMMON_INTRRX_EN);

	hcd->intrrx_mask &= ~irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_disable_intrusb(struct hiusb_hcd *hcd, unsigned int irqs)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_INTRUSB_EN); 

	hiusb_writeb(hcd, old & (~irqs), HIUSB_COMMON_INTRUSB_EN);

	hcd->intrusb_mask &= ~irqs;

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_disable_intrdma(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_HCD_DMA_CNTL); 

	hiusb_writew(hcd, old & (~BIT_HCD_DMA_CNTL_INTR_EN), HIUSB_HCD_DMA_CNTL);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_read_frame(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readw(hcd, HIUSB_COMMON_FRAME_NUM) & HIUSB_COMMON_FRAME_NUM_MASK;

	device_unlock(hcd);

	return old;
}

static inline int _hcd_set_reg_fifo_index(struct hiusb_hcd *hcd, unsigned int index)
{
	int old;

	old = hiusb_readb(hcd, HIUSB_COMMON_REG_FIFO_INDEX) & HIUSB_COMMON_REG_FIFO_INDEX_MASK;

	hiusb_writeb(hcd, index & HIUSB_COMMON_REG_FIFO_INDEX_MASK, HIUSB_COMMON_REG_FIFO_INDEX);

	return old;
}

int hiusb_hcd_set_reg_fifo_index(struct hiusb_hcd *hcd, unsigned int index)
{
	int old;

	device_lock(hcd);

	old = _hcd_set_reg_fifo_index(hcd, index);

	device_unlock(hcd);

	return old;
}

int _hcd_get_reg_fifo_index(struct hiusb_hcd *hcd)
{
	int old;

	old = hiusb_readb(hcd, HIUSB_COMMON_REG_FIFO_INDEX) & HIUSB_COMMON_REG_FIFO_INDEX_MASK;

	return old;
}

int hiusb_hcd_get_reg_fifo_index(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = _hcd_get_reg_fifo_index(hcd);

	device_unlock(hcd);

	return old;
}

static inline int _hcd_get_fifo_size_rx(struct hiusb_hcd *hcd, unsigned int index)
{
	int res;

	_hcd_set_reg_fifo_index(hcd, index);
	
	res = hiusb_readb(hcd, HIUSB_HCD_FIFO_SIZE) & BITS_HCD_FIFO_SIZE_RX_MASK;

	return 1<<(res>>4);
}

static inline int _hcd_get_fifo_size_tx(struct hiusb_hcd *hcd, unsigned int index)
{
	int res;

	_hcd_set_reg_fifo_index(hcd, index);

	res = hiusb_readb(hcd, HIUSB_HCD_FIFO_SIZE) & BITS_HCD_FIFO_SIZE_TX_MASK;

	return 1<<res;
}

int hiusb_hcd_get_fifo_size_tx(struct hiusb_hcd *hcd, unsigned int index)
{
	int tx_sz;

	device_lock(hcd);

	tx_sz = _hcd_get_fifo_size_tx(hcd, index);

	device_unlock(hcd);

	return tx_sz;
}

int hiusb_hcd_get_fifo_size_rx(struct hiusb_hcd *hcd, unsigned int index)
{
	int rx_sz;

	device_lock(hcd);

	rx_sz = _hcd_get_fifo_size_rx(hcd, index);

	device_unlock(hcd);

	return rx_sz;
}

int hiusb_hcd_read_devctl(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_set_session(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	hiusb_writeb(hcd, old | BIT_COMM_DEVCTL_SESSION, HIUSB_COMMON_DEVCTL);

	device_unlock(hcd);

	return old & BIT_COMM_DEVCTL_SESSION;
}

int hiusb_hcd_clear_session(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	old = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	hiusb_writeb(hcd, old & (~BIT_COMM_DEVCTL_SESSION), HIUSB_COMMON_DEVCTL);

	device_unlock(hcd);

	return old & BIT_COMM_DEVCTL_SESSION;
}

int hiusb_hcd_get_vbus_level(struct hiusb_hcd *hcd)
{
	/* do not need hold lock */
	int devctl  = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	return (devctl>>OFFSET_BIT_COMM_DEVCTL_VBUS) & MASK_BIT_COMM_DEVCTL_VBUS;
}

int hiusb_hcd_ctrlpipe_set_nak_limit(struct hiusb_hcd *hcd, 
		unsigned int naklimit)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_NAK_LIMIT);

	hiusb_writeb(hcd, naklimit & HIUSB_HCD_NAK_LIMIT_MASK, HIUSB_HCD_NAK_LIMIT);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_ctrlpipe_get_nak_limit(struct hiusb_hcd *hcd)
{
	unsigned int naklimit;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	naklimit = hiusb_readb(hcd, HIUSB_HCD_NAK_LIMIT);

	device_unlock(hcd);

	return naklimit;
}

#define _hcd_write_fifo(hcd, pkt, len)	do{\
	int _fifo_index = hiusb_readb((hcd), HIUSB_COMMON_REG_FIFO_INDEX) & HIUSB_COMMON_REG_FIFO_INDEX_MASK;\
	unsigned char *_fifo_io_addr = (void*)((hcd)->iobase + HIUSB_ENDPOINT_FIFO_BASE + _fifo_index * 4);\
	unsigned char *_p = (void*)pkt;\
	unsigned int _count;\
	for(_count=0; _count<len; _count++)\
		*_fifo_io_addr = *_p++;\
}while(0)

#define _hcd_read_fifo(hcd, pkt, len)	do{\
	int _fifo_index = hiusb_readb((hcd), HIUSB_COMMON_REG_FIFO_INDEX) & HIUSB_COMMON_REG_FIFO_INDEX_MASK;\
	unsigned char *_fifo_io_addr = (void*)((hcd)->iobase + HIUSB_ENDPOINT_FIFO_BASE + _fifo_index * 4);\
	unsigned char *_p = (void*)pkt;\
	unsigned int _count;\
	for(_count=0; _count<len; _count++)\
		*_p++ = *_fifo_io_addr;\
}while(0)

static inline int _hcd_ctrlpipe_checkerror(struct hiusb_hcd *hcd)
{
	int _csr0, _res = 0;

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	_csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

	if(_csr0 & BIT_HCD_CSR0_NAKTIMEOUT){
		hiusb_trace(3, "endpoint 0 NAK timeout, csr0=0x%.8x, HIUSB_HCD_NAK_LIMIT=0x%.8x", _csr0,
				(int)hiusb_readb(hcd, HIUSB_HCD_NAK_LIMIT));
		_res |= HIUSB_HCD_E_NAKTIMEOUT;
	}
	if(_csr0 & BIT_HCD_CSR0_ERROR){
		hiusb_error("device endpoint 0 not respone, csr0=0x%.8x", _csr0);
		_res |= HIUSB_HCD_E_NOTRESPONE;
	}
	if(_csr0 & BIT_HCD_CSR0_RXSTALL){
		hiusb_error("endpoint 0 receive a STALL, csr0=0x%.8x", _csr0);
		_res |= HIUSB_HCD_E_STALL;
	}

	if(_res){
		/* flush fifo */
		hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
		hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);

		/* clear all errors */
		hiusb_writew(hcd, 0, HIUSB_HCD_CSR0);
	}

	return _res;
}

int hiusb_hcd_ctrlpipe_checkerror(struct hiusb_hcd *hcd)
{
	int res;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	res = _hcd_ctrlpipe_checkerror(hcd);

	device_unlock(hcd);

	return res;
}

static inline int _hcd_ctrlpipe_tx_finish(struct hiusb_hcd *hcd)
{
	int csr0, res = 0;

	if(!hcd->rh_port_connect || hcd->rh_port_change)
		return HIUSB_HCD_E_NOTRESPONE;

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	do{
		/* check error:NAKTIMEOUT,NOTRESPONE,STALL */
		res = _hcd_ctrlpipe_checkerror(hcd);
		if(res)
			break;

		csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

		if(!(csr0 & BIT_HCD_CSR0_TXPKTRDY))
			break;
	}while( hcd->rh_port_connect && !hcd->rh_port_change );

	return res;
}

static inline int _hcd_ctrlpipe_rx_ready(struct hiusb_hcd *hcd)
{
	int csr0, res = 0;

	if(!hcd->rh_port_connect || hcd->rh_port_change)
		return HIUSB_HCD_E_NOTRESPONE;

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	do{
		/*check error:NAKTIMEOUT,NOTRESPONE,STALL*/
		res = _hcd_ctrlpipe_checkerror(hcd);
		if(res)
			break;

		csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

		if(csr0 & BIT_HCD_CSR0_RXPKTRDY)
			break;
	}while( hcd->rh_port_connect && !hcd->rh_port_change );

	return res;
}

int hiusb_hcd_ctrlpipe_tx(struct hiusb_hcd *hcd, 
		unsigned int addr,
		void *setup, 
		void *pkt, 
		unsigned int total_len,
		unsigned int *actual_len,
		unsigned int maxpacket,
		int *stage)
{
	int intrtx_mask, res, csr0=0;

	/* disable EP0 intrrupt until tx-transaction complete */
	intrtx_mask = hiusb_hcd_disable_intrtx(hcd, BIT_COMM_INTRTX_CTRLPIPE);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	if(_hcd_get_fifo_size_tx(hcd, HIUSB_HCD_CTRL_FIFO_INDEX) < HIUSB_HCD_SETUP_PACKET_LEN){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_tx(hcd, HIUSB_HCD_CTRL_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set ep0 naklimit  */
	hiusb_writeb(hcd, hcd->ctrlpipe_nak_limit, HIUSB_HCD_NAK_LIMIT);

	if(*stage == HIUSB_CTRLPIEP_STAGE_DATA)
		goto stage_data;
	else if(*stage == HIUSB_CTRLPIEP_STAGE_HANDSHAKE)
		goto stage_handshake;
	
	/********************* send SETUP: start *********************/
	/* flush fifo */
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);

	/* clear all errors */
	hiusb_writew(hcd, 0, HIUSB_HCD_CSR0);

	/* write fifo */
	_hcd_write_fifo(hcd, setup, HIUSB_HCD_SETUP_PACKET_LEN);

	/* use DATA0 */
	csr0 &= ~BIT_HCD_CSR0_STATUS_PKT;

	/* send setup(8) */
	csr0 |= BIT_HCD_CSR0_SETUP_PKT;
	csr0 |= BIT_HCD_CSR0_TXPKTRDY;
	hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

	res = _hcd_ctrlpipe_tx_finish(hcd);
	if(res)
		goto error_exit;
	/********************* send SETUP:   end *********************/
	
	/********************* send DATA : start *********************/
	*stage = HIUSB_CTRLPIEP_STAGE_DATA;
stage_data:

	while(*actual_len < total_len){
		csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

		/* flush fifo */
		hiusb_writew(hcd, csr0 | BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
		hiusb_writew(hcd, csr0 | BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);

		/* write fifo */
		_hcd_write_fifo(hcd, ((char*)pkt) + *actual_len, min(total_len - *actual_len, maxpacket));

		*actual_len += min(total_len - *actual_len, maxpacket);

		/* send OUT packet and DATA */
		csr0 &= ~BIT_HCD_CSR0_SETUP_PKT;
		csr0 |= BIT_HCD_CSR0_TXPKTRDY;
		hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

		res = _hcd_ctrlpipe_tx_finish(hcd);
		if(res)
			goto error_exit;
	}
	/********************* send DATA :   end *********************/

	/********************* handshake : start *********************/
	*stage = HIUSB_CTRLPIEP_STAGE_HANDSHAKE;
stage_handshake:

	csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);
	
	/* use DATA1 */
	csr0 |= BIT_HCD_CSR0_STATUS_PKT;

	/* send IN packet and wait NULL DATA */
	hiusb_writew(hcd, csr0 | BIT_HCD_CSR0_REQPKT, HIUSB_HCD_CSR0);

	res = _hcd_ctrlpipe_rx_ready(hcd);
	if(res)
		goto error_exit;

	if(hiusb_readb(hcd, HIUSB_HCD_COUNT0)){
		hiusb_error("endpoint 0 tx handshake error, pktlen=%lu", hiusb_readb(hcd, HIUSB_HCD_COUNT0));
		res = HIUSB_HCD_E_PROTOCOLERR;
	}

	/* clear RXPKTRDY and DATA-TOGLE */
	csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);
	csr0 &= ~BIT_HCD_CSR0_RXPKTRDY;
	csr0 &= ~BIT_HCD_CSR0_STATUS_PKT;
	hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);
	/********************* handshake :  end *********************/

error_exit:
	/* restore EP0 intrrupt */
	hiusb_hcd_enable_intrtx(hcd, intrtx_mask);

	return res;
}

int hiusb_hcd_ctrlpipe_rx(struct hiusb_hcd *hcd, 
		unsigned int addr,
		void *setup, 
		void *pkt, 
		unsigned int total_len,
		unsigned int *actual_len,
		unsigned int maxpacket,
		int *stage)
{
	int intrtx_mask, res=0, csr0=0;

	/* disable EP0 intrrupt until rx-transaction complete */
	intrtx_mask = hiusb_hcd_disable_intrtx(hcd, BIT_COMM_INTRTX_CTRLPIPE);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_CTRL_FIFO_INDEX);

	if(_hcd_get_fifo_size_rx(hcd, HIUSB_HCD_CTRL_FIFO_INDEX) < HIUSB_HCD_SETUP_PACKET_LEN){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_rx(hcd, HIUSB_HCD_CTRL_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set ep0 naklimit  */
	hiusb_writeb(hcd, hcd->ctrlpipe_nak_limit, HIUSB_HCD_NAK_LIMIT);
	
	if(*stage == HIUSB_CTRLPIEP_STAGE_DATA)
		goto stage_data;
	else if(*stage == HIUSB_CTRLPIEP_STAGE_HANDSHAKE)
		goto stage_handshake;

	/********************* send SETUP: start *********************/
	/* flush fifo */
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);

	/* clear all errors */
	hiusb_writew(hcd, 0, HIUSB_HCD_CSR0);

	/* write fifo */
	_hcd_write_fifo(hcd, setup, HIUSB_HCD_SETUP_PACKET_LEN);

	/* use DATA0 */
	csr0 &= ~BIT_HCD_CSR0_STATUS_PKT;

	/* send setup(8) */
	csr0 |= BIT_HCD_CSR0_SETUP_PKT;
	csr0 |= BIT_HCD_CSR0_TXPKTRDY;
	hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

	res = _hcd_ctrlpipe_tx_finish(hcd);
	if(res)
		goto error_exit;
	/********************* send SETUP:   end *********************/

	/********************* recv DATA : start *********************/
	*stage = HIUSB_CTRLPIEP_STAGE_DATA;
stage_data:

	while(*actual_len < total_len){

		int _len;

		csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

		/* send IN token and wait DATA */
		csr0 &= ~BIT_HCD_CSR0_SETUP_PKT;
		csr0 |= BIT_HCD_CSR0_REQPKT;
		hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

		res = _hcd_ctrlpipe_rx_ready(hcd);
		if(res)
			goto error_exit;

		_len = hiusb_readb(hcd, HIUSB_HCD_COUNT0);

		/* read fifo */
		_hcd_read_fifo(hcd, (((char*)pkt) + (*actual_len)), _len);

		*actual_len += _len;

		/* clear RXPKTRDY */
		csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);
		csr0 &= ~BIT_HCD_CSR0_RXPKTRDY;
		hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

		if(_len < maxpacket)
			break;
	}
	/********************* recv DATA :  end  *********************/

	/********************* handshake : start *********************/
	*stage = HIUSB_CTRLPIEP_STAGE_HANDSHAKE;
stage_handshake:

	csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);

	/* flush fifo */
	hiusb_writew(hcd, csr0 | BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
	hiusb_writew(hcd, csr0 | BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);

	/* use DATA1 */
	csr0 |= BIT_HCD_CSR0_STATUS_PKT;

	/* send OUT packet and NULL DATA */
	csr0 |= BIT_HCD_CSR0_TXPKTRDY;
	hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);

	res = _hcd_ctrlpipe_tx_finish(hcd);
	if(res)
		goto error_exit;

	/* clear DATA-TOGLE */
	csr0 = hiusb_readw(hcd, HIUSB_HCD_CSR0);
	csr0 &= ~BIT_HCD_CSR0_STATUS_PKT;
	hiusb_writew(hcd, csr0, HIUSB_HCD_CSR0);
	/********************* handshake :  end *********************/

error_exit:
	/* restore EP0 intrrupt */
	hiusb_hcd_enable_intrtx(hcd, intrtx_mask);

	return res;
}

int hiusb_hcd_bulkpipe_tx_set_maxpkt(struct hiusb_hcd *hcd, unsigned int txmaxp)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readw(hcd, HIUSB_HCD_TXMAXP);

	hiusb_writew(hcd, txmaxp & BITS_HCD_TXMAXP_PAYLOAD_MASK, HIUSB_HCD_TXMAXP);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_bulkpipe_tx_get_maxpkt(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readw(hcd, HIUSB_HCD_TXMAXP);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_bulkpipe_tx_set_nak_limit(struct hiusb_hcd *hcd, unsigned int naklimit)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_TXINTERVAL);

	hiusb_writeb(hcd, naklimit, HIUSB_HCD_TXINTERVAL);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_bulkpipe_tx_get_nak_limit(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_TXINTERVAL);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_bulkpipe_tx_set_endpoint(struct hiusb_hcd *hcd, unsigned int ep)
{
	int old, val = 0;

	val &= BITS_HCD_TXTYPE_ENDPOINT_MASK;
	val |= PROTOCOL_BULK;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_TXTYPE);

	hiusb_writeb(hcd, val, HIUSB_HCD_TXTYPE);

	device_unlock(hcd);

	return old & BITS_HCD_TXTYPE_ENDPOINT_MASK;
}

int hiusb_hcd_bulkpipe_tx_get_endpoint(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_TXTYPE);

	device_unlock(hcd);

	return old & BITS_HCD_TXTYPE_ENDPOINT_MASK;
}

static inline int _hcd_bulkpipe_tx_check_clear_error(struct hiusb_hcd *hcd)
{
	int _txcsr, _res = 0;

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	_txcsr = hiusb_readw(hcd, HIUSB_HCD_TXCSR);

	if(_txcsr & BIT_HCD_TXCSR_NAKTIMEOUT){

		if(_txcsr & BIT_HCD_TXCSR_FIFONOTEMPTY){

			hiusb_trace(3, "bulkpipe-tx NAK timeout, txcsr=0x%.4x", _txcsr);
			_res |= HIUSB_HCD_E_NAKTIMEOUT;
		}
	}
	if(_txcsr & BIT_HCD_TXCSR_ERROR){
		hiusb_error("bulkpipe-tx device not respone, txcsr=0x%.4x", _txcsr);
		_res |= HIUSB_HCD_E_NOTRESPONE;
	}
	if(_txcsr & BIT_HCD_TXCSR_RX_STALL){
		hiusb_trace(7, "bulkpipe-tx receive a STALL, txcsr=0x%.4x", _txcsr);
		_res |= HIUSB_HCD_E_STALL;

		/*
		 * 5.8.5 Bulk Transfer Data Sequences
		 * Bulk transactions use data toggle bits that are toggled only upon successful transaction completion to
		 * preserve synchronization between transmitter and receiver when transactions are retried due to errors. Bulk
		 * transactions are initialized to DATA0 when the endpoint is configured by an appropriate control transfer.
		 * The host will also start the first bulk transaction with DATA0. If a halt condition is detected on a bulk pipe
		 * due to transmission errors or a STALL handshake being returned from the endpoint, all pending IRPs are
		 * retired. Removal of the halt condition is achieved via software intervention through a separate control pipe.
		 * This recovery will reset the data toggle bit to DATA0 for the endpoint on both the host and the device.
		 * Bulk transactions are retried due to errors detected on the bus that affect a given transaction.
		 */

		/* clear DATA-TOGGLE */
		_txcsr |= BIT_HCD_TXCSR_CLRDATATOG;
		hiusb_writew(hcd, _txcsr, HIUSB_HCD_TXCSR);
	}

	if(_res){

		/* flush fifo twice, ensure fifo is empty */
		hiusb_writew(hcd, _txcsr | BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);
		hiusb_writew(hcd, _txcsr | BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);

		/* clear intrtx */
		hiusb_readw(hcd, HIUSB_COMMON_INTRTX);

		/* clear txcsr */
		hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);
	}

	return _res;
}

int hiusb_hcd_bulkpipe_tx_cpumode_check_clear_error(struct hiusb_hcd *hcd)
{
	int res;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	res = _hcd_bulkpipe_tx_check_clear_error(hcd);

	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_tx_dmamode1(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt_phy, 
		unsigned int len, 
		unsigned int maxpacket)
{
	int res=0;
	int cntl=0, txcsr=0;

	hiusb_assert(pkt_phy);
	hiusb_assert(len >= maxpacket);
	hiusb_assert((len % maxpacket)==0);

	device_lock(hcd);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	/* set fifo index first */
	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	/* clear txcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

	/********************* config-dma : start ********************/
	/* clean dma cntl */
	hiusb_writew(hcd, 0, HIUSB_HCD_DMA_CNTL);

	/* clean dma intr */
	hiusb_readb(hcd, HIUSB_HCD_DMA_INTR);

	/* config dma memory address */
	hiusb_writel(hcd, pkt_phy, HIUSB_HCD_DMA_ADDR);

	/* config count */
	hiusb_writel(hcd, len, HIUSB_HCD_DMA_COUNT);

	/* enable dma function */
	cntl |= BIT_HCD_DMA_CNTL_EN;
	/* set tx direction */
	cntl |= HIUSB_HCD_DMA_CNTL_DIR_TX;
	/* set dma mode */
	cntl |= HIUSB_HCD_DMA_MODE1;
	/* enable dma interrupt */
	cntl |= BIT_HCD_DMA_CNTL_INTR_EN;
	/* set dma fifo index */
	cntl |= (HIUSB_HCD_BULK_TX_FIFO_INDEX << OFFSET_HCD_DMA_CNTL_FIFO_INDEX);
	/* clean bus error */
	cntl &= ~BIT_HCD_DMA_CNTL_BUS_ERROR;
	/* set burst mode */
	cntl |= HIUSB_HCD_DMA_BURST_MODE3;
	hiusb_writew(hcd, cntl, HIUSB_HCD_DMA_CNTL);
	/********************* config-dma : end **********************/

	/***** config-addr,fifo-index,txmxp,txtype,txcsr : start *****/
	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	/* flush fifo twice, ensure fifo is empty */
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);

	/* clear intrtx */
	hiusb_readw(hcd, HIUSB_COMMON_INTRTX);

	/* clear txcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

	if(_hcd_get_fifo_size_tx(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX) < maxpacket){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_tx(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set txmaxp */
	hiusb_writew(hcd, maxpacket & BITS_HCD_TXMAXP_PAYLOAD_MASK, HIUSB_HCD_TXMAXP);

	/* set txtype: protocol and target endpoint */
	hiusb_writeb(hcd, PROTOCOL_BULK | endpoint, HIUSB_HCD_TXTYPE);

	txcsr |= BIT_HCD_TXCSR_AUTOSET;
	txcsr |= BIT_HCD_TXCSR_DIRECTION;
	txcsr |= BIT_HCD_TXCSR_DMAREQEN;
	txcsr |= BIT_HCD_TXCSR_DMAREQMODE;
	//if(!toggle)
	//	txcsr |= BIT_HCD_TXCSR_CLRDATATOG;
	/* in dma mode, the TXPKTRDY is set with dma */
	hiusb_writew(hcd, txcsr, HIUSB_HCD_TXCSR);
	/****** config-addr,fifo-index,txmxp,txtype,txcsr : end ******/

error_exit:
	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_tx_cpumode(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt, 
		unsigned int len, 
		unsigned int maxpacket)
{
	int res=0;
	int txcsr=0;

	hiusb_assert(len <= maxpacket);

	device_lock(hcd);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	/***** config-addr,fifo-index,txmxp,txtype,txcsr : start *****/
	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX);

	/* flush fifo twice, ensure fifo is empty */
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);

	/* clear intrtx */
	hiusb_readw(hcd, HIUSB_COMMON_INTRTX);

	/* clear txcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

	if(_hcd_get_fifo_size_tx(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX) < maxpacket){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_tx(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set nak limit */
	hiusb_writeb(hcd, hcd->bulkpipe_nak_limit, HIUSB_HCD_TXINTERVAL);

	/* set txmaxp */
	hiusb_writew(hcd, maxpacket & BITS_HCD_TXMAXP_PAYLOAD_MASK, HIUSB_HCD_TXMAXP);

	/* set txtype: protocol and target endpoint */
	hiusb_writeb(hcd, PROTOCOL_BULK | endpoint, HIUSB_HCD_TXTYPE);

	/* write fifo */
	_hcd_write_fifo(hcd, pkt, len);

	txcsr |= BIT_HCD_TXCSR_DIRECTION;
	//if(!toggle)
	//	txcsr |= BIT_HCD_TXCSR_CLRDATATOG;
	txcsr |= BIT_HCD_TXCSR_TXPKTRDY;

	hiusb_writew(hcd, txcsr, HIUSB_HCD_TXCSR);
	/****** config-addr,fifo-index,txmxp,txtype,txcsr : end ******/

error_exit:
	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_rx_set_nak_limit(struct hiusb_hcd *hcd, unsigned int naklimit)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_RXINTERVAL);

	hiusb_writeb(hcd, naklimit, HIUSB_HCD_RXINTERVAL);

	device_unlock(hcd);

	return old;
}

int hiusb_hcd_bulkpipe_rx_get_nak_limit(struct hiusb_hcd *hcd)
{
	int old;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	old = hiusb_readb(hcd, HIUSB_HCD_RXINTERVAL);

	device_unlock(hcd);

	return old;
}

static inline int _hcd_bulkpipe_rx_check_clear_error(struct hiusb_hcd *hcd)
{
	int _rxcsr, _res = 0;

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	_rxcsr = hiusb_readw(hcd, HIUSB_HCD_RXCSR);

	if(_rxcsr & BIT_HCD_RXCSR_PIDERROR){
		hiusb_error("bulkpipe-rx PID error, rxcsr=0x%.4x", _rxcsr);
		_rxcsr &= ~BIT_HCD_RXCSR_PIDERROR;
		_res |= HIUSB_HCD_E_PROTOCOLERR;
	}
	if(_rxcsr & BIT_HCD_RXCSR_RX_STALL){
		hiusb_trace(7, "bulkpipe-rx receive a STALL, rxcsr=0x%.4x", _rxcsr);
		_rxcsr &= ~BIT_HCD_RXCSR_RX_STALL;
		_res |= HIUSB_HCD_E_STALL;

		/*
		 * 5.8.5 Bulk Transfer Data Sequences
		 * Bulk transactions use data toggle bits that are toggled only upon successful transaction completion to
		 * preserve synchronization between transmitter and receiver when transactions are retried due to errors. Bulk
		 * transactions are initialized to DATA0 when the endpoint is configured by an appropriate control transfer.
		 * The host will also start the first bulk transaction with DATA0. If a halt condition is detected on a bulk pipe
		 * due to transmission errors or a STALL handshake being returned from the endpoint, all pending IRPs are
		 * retired. Removal of the halt condition is achieved via software intervention through a separate control pipe.
		 * This recovery will reset the data toggle bit to DATA0 for the endpoint on both the host and the device.
		 * Bulk transactions are retried due to errors detected on the bus that affect a given transaction.
		 */

		/* clear DATA-TOGGLE */
		_rxcsr |= BIT_HCD_RXCSR_CLRDATATOG;
		hiusb_writew(hcd, _rxcsr, HIUSB_HCD_RXCSR);
	}
	if(_rxcsr & BIT_HCD_RXCSR_NAKTIMEOUT){
		hiusb_trace(3, "bulkpipe-rx NAK timeout, rxcsr=0x%.4x", _rxcsr);
		_rxcsr &= ~BIT_HCD_RXCSR_NAKTIMEOUT;
		_res |= HIUSB_HCD_E_NAKTIMEOUT;
	}
	if(_rxcsr & BIT_HCD_RXCSR_ERROR){
		hiusb_error("bulkpipe-rx device not respone, rxcsr=0x%.4x", _rxcsr);
		_rxcsr &= ~BIT_HCD_RXCSR_ERROR;
		_res |= HIUSB_HCD_E_NOTRESPONE;
	}

	if(_res){
		/* flush fifo twice, ensure fifo is empty */
		hiusb_writew(hcd, _rxcsr | BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
		hiusb_writew(hcd, _rxcsr | BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);

		/* clean intrrx */
		hiusb_readw(hcd, HIUSB_COMMON_INTRRX);

		/* clear rxcsr */
		hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);
	}

	return _res;
}

int hiusb_hcd_bulkpipe_rx_cpumode_check_clear_error(struct hiusb_hcd *hcd)
{
	int res;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	res = _hcd_bulkpipe_rx_check_clear_error(hcd);

	device_unlock(hcd);

	return res;
}

static inline void _hcd_rx_set_dma_rqpktcount(struct hiusb_hcd *hcd,
		unsigned int rqpktcount)
{
	int fifo_index;

	fifo_index = hiusb_readb(hcd, HIUSB_COMMON_REG_FIFO_INDEX) & HIUSB_COMMON_REG_FIFO_INDEX_MASK;

	/* notes: rqpktcount = 300 + index*4 */
	hiusb_writew( hcd, rqpktcount, HIUSB_HCD_DMA_RQPKT_BASE + (fifo_index<<2) );
}

int hiusb_hcd_bulkpipe_rx_dmamode1(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt_phy, 
		unsigned int buflen,
		unsigned int maxpacket)
{
	int res=0;
	int cntl=0, rxcsr=0;

	hiusb_assert(pkt_phy);
	hiusb_assert(buflen >= maxpacket);
	hiusb_assert((buflen % maxpacket)==0);

	device_lock(hcd);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	/* set fifo index first */
	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	/* clear rxcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);

	/* notes: set fifo direction is rx, clear TXCSR D13 */
	hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

	/********************* config-dma : start ********************/
	/* clear dma cntl */
	hiusb_writew(hcd, 0, HIUSB_HCD_DMA_CNTL);

	/* clean dma intr */
	hiusb_readb(hcd, HIUSB_HCD_DMA_INTR);

	/* config dma memory address */
	hiusb_writel(hcd, pkt_phy, HIUSB_HCD_DMA_ADDR);

	/* config count */
	hiusb_writel(hcd, buflen, HIUSB_HCD_DMA_COUNT);

	/* enable dma function */
	cntl |= BIT_HCD_DMA_CNTL_EN;
	/* set rx direction */
	cntl |= HIUSB_HCD_DMA_CNTL_DIR_RX;
	/* set dma mode */
	cntl |= HIUSB_HCD_DMA_MODE1;
	/* enable dma interrupt */
	cntl |= BIT_HCD_DMA_CNTL_INTR_EN;
	/* set dma fifo index */
	cntl |= (HIUSB_HCD_BULK_RX_FIFO_INDEX<<OFFSET_HCD_DMA_CNTL_FIFO_INDEX); 
	/* clean bus error */
	cntl &= ~BIT_HCD_DMA_CNTL_BUS_ERROR;
	/* set burst mode */
	cntl |= HIUSB_HCD_DMA_BURST_MODE3;
	hiusb_writew(hcd, cntl, HIUSB_HCD_DMA_CNTL);
	/********************* config-dma : end **********************/

	/***** config-addr,fifo-index,rqpktcount,rxmxp,rxtype,rxcsr : start *****/
	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	/* flush fifo twice, ensure fifo is empty */
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);

	/* clean intrrx */
	hiusb_readw(hcd, HIUSB_COMMON_INTRRX);

	/* clear rxcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);

	/* check fifo size */
	if(_hcd_get_fifo_size_rx(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX) < maxpacket){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_rx(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set rqpktcount */
	_hcd_rx_set_dma_rqpktcount(hcd, buflen/maxpacket);

	/* set rxmaxp */
	hiusb_writew(hcd, maxpacket & BITS_HCD_RXMAXP_PAYLOAD_MASK, HIUSB_HCD_RXMAXP);

	/* set rxtype: protocol and target endpoint */
	hiusb_writeb(hcd, PROTOCOL_BULK | endpoint, HIUSB_HCD_RXTYPE);

	rxcsr |= BIT_HCD_RXCSR_AUTOCLEAR;
	rxcsr |= BIT_HCD_RXCSR_AUTOREQ;
	rxcsr |= BIT_HCD_RXCSR_DMAREQEN;
	rxcsr |= BIT_HCD_RXCSR_DMAREQMODE;
	//if(!toggle)
	//	rxcsr |= BIT_HCD_RXCSR_CLRDATATOG;
	rxcsr |= BIT_HCD_RXCSR_REQ_PKT;
	hiusb_writew(hcd, rxcsr, HIUSB_HCD_RXCSR);
	/****** config-addr,fifo-index,rqpktcount,rxmxp,rxtype,rxcsr : end ******/

error_exit:
	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_rx_cpumode_send_in_token(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		unsigned int maxpacket)
{
	int res=0, txcsr=0, rxcsr=0;

	device_lock(hcd);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	_hcd_set_addr(hcd, addr);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	/* flush fifo twice, ensure fifo is empty */
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);

	/* clean intrrx */
	hiusb_readw(hcd, HIUSB_COMMON_INTRRX);

	/* clear rxcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);

	/* notes: set fifo direction is rx, clear TXCSR D13 */
	txcsr &= ~BIT_HCD_TXCSR_DIRECTION;
	hiusb_writew(hcd, txcsr, HIUSB_HCD_TXCSR);

	if(_hcd_get_fifo_size_rx(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX) < maxpacket){
		hiusb_error("fifo size=%d", _hcd_get_fifo_size_rx(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX));
		res = HIUSB_HCD_E_NOTSUPPORT;
		goto error_exit;
	}

	/* set nak limit */
	hiusb_writeb(hcd, hcd->bulkpipe_nak_limit, HIUSB_HCD_RXINTERVAL);

	/* set rxmaxp */
	hiusb_writew(hcd, maxpacket & BITS_HCD_RXMAXP_PAYLOAD_MASK, HIUSB_HCD_RXMAXP);

	/* set rxtype: protocol and target endpoint */
	hiusb_writeb(hcd, PROTOCOL_BULK | endpoint, HIUSB_HCD_RXTYPE);

	//if(!toggle)
	//	rxcsr |= BIT_HCD_RXCSR_CLRDATATOG;
	rxcsr |= BIT_HCD_RXCSR_REQ_PKT;
	hiusb_writew(hcd, rxcsr, HIUSB_HCD_RXCSR);

error_exit:
	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_rx_cpumode_finish(struct hiusb_hcd *hcd, 
		void *pkt,
		unsigned int buflen,
		unsigned int *rxlen)
{
	int res=0;

	device_lock(hcd);

	if(!hcd->rh_port_connect || hcd->rh_port_change){
		res = HIUSB_HCD_E_NOTRESPONE;
		goto error_exit;
	}

	/* set fifo index */
	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	/* get actual rx length */
	*rxlen = hiusb_readw(hcd, HIUSB_HCD_RXCOUNT);
	if( *rxlen > buflen ){
		hiusb_error("rxlen=%d, buflen=%d", *rxlen, buflen);
		res = HIUSB_HCD_E_BUG;
		goto error_exit;
	}

	/* read fifo */
	_hcd_read_fifo(hcd, pkt, *rxlen);

	/* clear rxcsr */
	hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);

error_exit:
	device_unlock(hcd);

	return res;
}

int hiusb_hcd_bulkpipe_dmamode_checkerror(struct hiusb_hcd *hcd)
{
	int res=0, cntl;

	device_lock(hcd);

	_hcd_set_reg_fifo_index(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX);

	cntl = hiusb_readw(hcd, HIUSB_HCD_DMA_CNTL);

	if(cntl & BIT_HCD_DMA_CNTL_BUS_ERROR){
		hiusb_error("dma bus error, cntl=0x%.8x", cntl);
		res = HIUSB_HCD_E_BUSERROR;
	}

	device_unlock(hcd);

	return res;
}

/* caller must hold lock */
void _hcd_clear_pending_task(struct hiusb_hcd *hcd)
{
	/* disable intrtx,intrrx,intrusb,intrdma */
	hiusb_writew(hcd, 0, HIUSB_COMMON_INTRTX_EN);
	hcd->intrtx_mask = 0;

	hiusb_writew(hcd, 0, HIUSB_COMMON_INTRRX_EN);
	hcd->intrrx_mask = 0;

	hiusb_writew(hcd, 0, HIUSB_COMMON_INTRUSB_EN);
	hcd->intrusb_mask = 0;

	hiusb_writew(hcd, 0, HIUSB_HCD_DMA_CNTL);

	/* clear csr0 */
	hiusb_writeb(hcd, 0, HIUSB_COMMON_REG_FIFO_INDEX);
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
	hiusb_writew(hcd, BIT_HCD_CSR0_FLUSH_FIFO, HIUSB_HCD_CSR0);
	hiusb_writew(hcd, 0, HIUSB_HCD_CSR0);

	/* clear txcsr */
	hiusb_writeb(hcd, HIUSB_HCD_BULK_TX_FIFO_INDEX, HIUSB_COMMON_REG_FIFO_INDEX);
	hiusb_writew(hcd, BIT_HCD_TXCSR_CLRDATATOG, HIUSB_HCD_TXCSR);
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);
	hiusb_writew(hcd, BIT_HCD_TXCSR_FLUSH_FIFO, HIUSB_HCD_TXCSR);
	hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

	/* clear rxcsr */
	hiusb_writeb(hcd, HIUSB_HCD_BULK_RX_FIFO_INDEX, HIUSB_COMMON_REG_FIFO_INDEX);
	hiusb_writew(hcd, BIT_HCD_RXCSR_CLRDATATOG, HIUSB_HCD_RXCSR);
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
	hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
	hiusb_writew(hcd, 0, HIUSB_HCD_RXCSR);

	/* clear addr */
	hiusb_writeb(hcd, 0, HIUSB_COMMON_FADDR);

	/* clear index */
	hiusb_writeb(hcd, 0, HIUSB_COMMON_REG_FIFO_INDEX);

	/* clear intrtx,intrrx,intrusb, intrdma */
	hiusb_readw(hcd, HIUSB_COMMON_INTRTX);
	hiusb_readw(hcd, HIUSB_COMMON_INTRRX);
	hiusb_readw(hcd, HIUSB_COMMON_INTRUSB);
	hiusb_readb(hcd, HIUSB_HCD_DMA_INTR);
}

void hiusb_hcd_hw_sw_sync(struct hiusb_hcd *hcd)
{
	device_lock(hcd);

	_hcd_clear_pending_task(hcd);

	/* FIXME: EOF1 */
	/*hiusb_writeb(hcd, 0x60, HIUSB_COMMON_HS_EOF1);*/

	/* enable highspeed mode */
	hiusb_writeb(hcd, BIT_COMM_POWER_HIGHSPEED_EN, HIUSB_COMMON_POWER);

	/* set session to work on host mode */
	hiusb_writeb(hcd, BIT_COMM_DEVCTL_SESSION, HIUSB_COMMON_DEVCTL);

	/* reset roothub */
	hcd->rh_port_change = 1;
	hcd->rh_port_connect = 0;
	hcd->rh_port_speed = 0;
	hcd->rh_port_reset = 0;

	device_unlock(hcd);
}

int hiusb_hcd_hw_status_check(struct hiusb_hcd *hcd)
{
	int power, devctl, res=0;

	power = hiusb_readb(hcd, HIUSB_COMMON_POWER);

	devctl = hiusb_readb(hcd, HIUSB_COMMON_DEVCTL);

	if( (devctl & BIT_COMM_DEVCTL_B_DEVICE) 
			//|| !(devctl & (BIT_COMM_DEVCTL_FULLSPEED | BIT_COMM_DEVCTL_LOWSPEED))
			|| HIUSB_VBUS_LEVEL_VALID != hiusb_hcd_get_vbus_level(hcd)
			//|| !(devctl & BIT_COMM_DEVCTL_HOST_MODE) 
			|| (devctl & BIT_COMM_DEVCTL_HOST_REQ)
			|| !(devctl & BIT_COMM_DEVCTL_SESSION)
			){
	//	hiusb_error("hw abnormal, devctl=0x%.2x", devctl);
		res = HIUSB_HCD_E_HWERROR;
	}
	if((HIUSB_VBUS_LEVEL_VALID !=hiusb_hcd_get_vbus_level(hcd))&&usb_poweroff_flag==0)
	{
		if(HIUSB_VBUS_LEVEL_VALID !=hiusb_hcd_get_vbus_level(hcd))
		{
			usb_poweroff_flag=1;
		}
		printk(KERN_INFO "hiusb poweroff");

		res = HIUSB_HCD_E_HWERROR;
	}
	if(HIUSB_VBUS_LEVEL_VALID == hiusb_hcd_get_vbus_level(hcd))
	{
		usb_poweroff_flag=0;
	}
	return res;
}

extern unsigned int intr_delay_time;
int hiusb_hcd_intpipe_rx(struct hiusb_hcd *hcd, struct urb *urb)
{
    int endp, addr;
    
    endp = usb_pipeendpoint(urb->pipe);
    
    addr = usb_pipedevice(urb->pipe);

    device_lock(hcd);
    _hcd_set_reg_fifo_index(hcd, endp);

    _hcd_set_addr(hcd, addr);
    
    hiusb_writew(hcd, 8, HIUSB_HCD_RXMAXP);
    /* flush fifo twice, ensure fifo is empty */
    hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
    hiusb_writew(hcd, BIT_HCD_RXCSR_FLUSH_FIFO, HIUSB_HCD_RXCSR);
    
    /* clean intrrx */
    hiusb_readw(hcd, HIUSB_COMMON_INTRRX);
    
    /* clear rxcsr */
    hiusb_writew(hcd, 0x20, HIUSB_HCD_RXCSR);
    
    /* notes: set fifo direction is rx, clear TXCSR D13 */
    hiusb_writew(hcd, 0, HIUSB_HCD_TXCSR);

    hiusb_writeb(hcd, intr_delay_time, HIUSB_HCD_RXINTERVAL);

    /* set rxtype: protocol and target endpoint */
    hiusb_writeb(hcd, PROTOCOL_INTERRUPT | endp, HIUSB_HCD_RXTYPE);
    
    device_unlock(hcd);

    hiusb_hcd_enable_intrrx(hcd, 1 << endp);

    return 0;
}
