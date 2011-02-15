#ifndef __HIUSB_CTRL_H__
#define __HIUSB_CTRL_H__

/* hisub common reg list */
#define HIUSB_COMMON_FADDR		0x0000
#define HIUSB_COMMON_POWER		0x0001
#define HIUSB_COMMON_INTRTX		0x0002 /* read clear */
#define HIUSB_COMMON_INTRRX		0x0004 /* read clear */
#define HIUSB_COMMON_INTRTX_EN		0x0006
#define HIUSB_COMMON_INTRRX_EN		0x0008
#define HIUSB_COMMON_INTRUSB		0x000A /* read clear */
#define HIUSB_COMMON_INTRUSB_EN		0x000B
#define HIUSB_COMMON_FRAME_NUM		0x000C
#define HIUSB_COMMON_REG_FIFO_INDEX	0x000E
#define HIUSB_COMMON_TESTMOD_EN		0x000F

/* host mode reg list */
#define HIUSB_HCD_TXMAXP		0x0010
#define HIUSB_HCD_CSR0			0x0012 /*EP0*/
#define HIUSB_HCD_TXCSR			0x0012 /*EP1-EP15*/
#define HIUSB_HCD_RXMAXP		0x0014
#define HIUSB_HCD_RXCSR			0x0016
#define HIUSB_HCD_COUNT0		0x0018 /*EP0*/
#define HIUSB_HCD_RXCOUNT		0x0018 /*EP1-EP15*/
#define HIUSB_HCD_TXTYPE		0x001A
#define HIUSB_HCD_NAK_LIMIT		0x001B /*EP0*/
#define HIUSB_HCD_TXINTERVAL		0x001B /*EP1-EP15*/
#define HIUSB_HCD_RXTYPE		0x001C
#define HIUSB_HCD_RXINTERVAL		0x001D
#define HIUSB_HCD_CONFIG_DATA		0x001F /*EP0*/
#define HIUSB_HCD_FIFO_SIZE		0x001F /*EP1-EP15*/

/* hiusb endpoint fifo base */
#define HIUSB_ENDPOINT_FIFO_BASE	(0x0020) /* fifo addr = base + index*4 */

/* peripheral mode reg list */
/* FIXME: peripheral mode not support */

/* additional control & configuration reg list */
#define HIUSB_COMMON_DEVCTL		0x0060
#define HIUSB_COMMON_HS_EOF1		0x007C
/* FIXME: other addition control not support */

/* dma controller reg list */
#define HIUSB_HCD_DMA_INTR		0x0200
#define HIUSB_HCD_DMA_CNTL		0x0204
#define HIUSB_HCD_DMA_ADDR		0x0208
#define HIUSB_HCD_DMA_COUNT		0x020C

/* RqPktCount reg list */
#define HIUSB_HCD_DMA_RQPKT_BASE	0x0300

/* bits of HIUSB_COMMON_FADDR */
#define HIUSB_COMMON_FADDR_MASK		((1U<<7)-1)

/* bits of HIUSB_COMMON_POWER */
#define HIUSB_COMMON_POWER_MASK		0xFF
#define BIT_COMM_POWER_ISO_UPDATE	(1U<<7)
#define BIT_COMM_POWER_SOFT_CONNECT	(1U<<6) /*only valid in peripheral mode*/
#define BIT_COMM_POWER_HIGHSPEED_EN	(1U<<5)
#define BIT_COMM_POWER_HIGHSPEED_MODE	(1U<<4)
#define BIT_COMM_POWER_RESET		(1U<<3)
#define BIT_COMM_POWER_RESUME		(1U<<2)
#define BIT_COMM_POWER_SUSPEND_MODE	(1U<<1)
#define BIT_COMM_POWER_SUSPEND_EN	(1U<<0)

/* bits of HIUSB_COMMON_INTRTX HIUSB_COMMON_INTRRX */
#define HIUSB_HCD_CTRLPIPE_INTR_MASK	(0x0001)
#define HIUSB_HCD_TXPIPE_INTR_MASK	(~1)
#define HIUSB_HCD_RXPIPE_INTR_MASK	(~1)
#define BIT_COMM_INTRTX_CTRLPIPE	(1U<<0)

/* bits of HIUSB_COMMON_INTRUSB & HIUSB_COMMON_INTRUSB_EN */
#define HIUSB_COMMON_INTRUSB_MASK	0xFF
#define BIT_COMM_INTRUSB_VBUS_ERROR	(1U<<7)
#define BIT_COMM_INTRUSB_SESSION_REQ	(1U<<6)
#define BIT_COMM_INTRUSB_DISCONNECT	(1U<<5)
#define BIT_COMM_INTRUSB_CONNECT	(1U<<4)
#define BIT_COMM_INTRUSB_SOF		(1U<<3)
#define BIT_COMM_INTRUSB_RESET		(1U<<2) /*only in peripheral mode*/
#define BIT_COMM_INTRUSB_BABBLE		(1U<<2) /*only in host mode*/
#define BIT_COMM_INTRUSB_RESUME		(1U<<1)
#define BIT_COMM_INTRUSB_SUSPEND	(1U<<0)

/* bits of HIUSB_COMMON_FRAME_NUM */
#define HIUSB_COMMON_FRAME_NUM_MASK	((1U<<11)-1)

/* bits of HIUSB_COMMON_REG_FIFO_INDEX */
#define HIUSB_COMMON_REG_FIFO_INDEX_MASK	((1U<<5)-1)

/* bits of HIUSB_COMMON_DEVCTL */
#define HIUSB_COMMON_DEVCTL_MASK	0xFF
#define BIT_COMM_DEVCTL_B_DEVICE	(1U<<7)
#define BIT_COMM_DEVCTL_FULLSPEED	(1U<<6)
#define BIT_COMM_DEVCTL_LOWSPEED	(1U<<5)
#define BITS_COMM_DEVCTL_VBUS		(1U<<3)
#define BIT_COMM_DEVCTL_HOST_MODE	(1U<<2)
#define BIT_COMM_DEVCTL_HOST_REQ	(1U<<1)
#define BIT_COMM_DEVCTL_SESSION		(1U<<0)

#define OFFSET_BIT_COMM_DEVCTL_VBUS	3 /*bit 3*/
#define MASK_BIT_COMM_DEVCTL_VBUS	0x03
#define HIUSB_VBUS_LEVEL_CLEAR		0x00
#define HIUSB_VBUS_LEVEL_VALID		0x03

/* bits of HIUSB_COMMON_HS_EOF1 in host mode */
#define HIUSB_COMMON_HS_EOF1_MASK	0xFF

/* bits of HIUSB_HCD_CSR0 in host mode */
#define HIUSB_HCD_CSR0_MASK		0xFFFF
#define BIT_HCD_CSR0_DISABLE_PING	(1U<<11)
#define BIT_HCD_CSR0_FLUSH_FIFO		(1U<<8)
#define BIT_HCD_CSR0_NAKTIMEOUT		(1U<<7)
#define BIT_HCD_CSR0_STATUS_PKT		(1U<<6)
#define BIT_HCD_CSR0_REQPKT		(1U<<5)
#define BIT_HCD_CSR0_ERROR		(1U<<4)
#define BIT_HCD_CSR0_SETUP_PKT		(1U<<3)
#define BIT_HCD_CSR0_RXSTALL		(1U<<2)
#define BIT_HCD_CSR0_TXPKTRDY		(1U<<1) /*auto clear*/
#define BIT_HCD_CSR0_RXPKTRDY		(1U<<0) /*CPU should clear*/

/* bits of HIUSB_HCD_COUNT0 in host mode */
#define HIUSB_HCD_COUNT0_MASK		((1U<<7)-1)

/* bits of HIUSB_HCD_NAK_LIMIT in host mode */
#define HIUSB_HCD_NAK_LIMIT_MASK	((1U<<5)-1)

/* bits of HIUSB_HCD_TXMAXP in host mode */
#define HIUSB_HCD_TXMAXP_MASK		0xFFFF
#define BITS_HCD_TXMAXP_MULTIPLE	(1U<<11)
#define BITS_HCD_TXMAXP_PAYLOAD		(1U<<0)
#define BITS_HCD_TXMAXP_PAYLOAD_MASK	((1U<<11)-1)

/* bits of HIUSB_HCD_TXCSR in host mode */
#define HIUSB_HCD_TXCSR_MASK		0xFFFF
#define BIT_HCD_TXCSR_AUTOSET		(1U<<15)
#define BIT_HCD_TXCSR_DIRECTION		(1U<<13)
#define BIT_HCD_TXCSR_DMAREQEN		(1U<<12)
#define BIT_HCD_TXCSR_FRCDATATOG	(1U<<11)
#define BIT_HCD_TXCSR_DMAREQMODE	(1U<<10)
#define BIT_HCD_TXCSR_NAKTIMEOUT	(1U<<7) /* bulk endpoint only */
#define BIT_HCD_TXCSR_INCOMPTX		(1U<<7) /* interrupt endpoint only */
#define BIT_HCD_TXCSR_CLRDATATOG	(1U<<6)
#define BIT_HCD_TXCSR_RX_STALL		(1U<<5)
#define BIT_HCD_TXCSR_FLUSH_FIFO	(1U<<3)
#define BIT_HCD_TXCSR_ERROR		(1U<<2)
#define BIT_HCD_TXCSR_FIFONOTEMPTY	(1U<<1)
#define BIT_HCD_TXCSR_TXPKTRDY		(1U<<0)

/* bits of HIUSB_HCD_RXMAXP */
#define HIUSB_HCD_RXMAXP_MASK		0xFFFF
#define BITS_HCD_RXMAXP_MULTIPLE	(1U<<11)
#define BITS_HCD_RXMAXP_PAYLOAD		(1U<<0)
#define BITS_HCD_RXMAXP_PAYLOAD_MASK	((1U<<11)-1)

/* bits of HIUSB_HCD_RXCSR in host mode */
#define HIUSB_HCD_RXCSR_MASK		0xFFFF
#define BIT_HCD_RXCSR_AUTOCLEAR		(1U<<15)
#define BIT_HCD_RXCSR_AUTOREQ		(1U<<14)
#define BIT_HCD_RXCSR_DMAREQEN		(1U<<13)
#define BIT_HCD_RXCSR_PIDERROR		(1U<<12)
#define BIT_HCD_RXCSR_DMAREQMODE	(1U<<11)
#define BIT_HCD_RXCSR_INCOMPRX		(1U<<8) /* only use in isochronous / interrupt endpoint */
#define BIT_HCD_RXCSR_CLRDATATOG	(1U<<7)
#define BIT_HCD_RXCSR_RX_STALL		(1U<<6)
#define BIT_HCD_RXCSR_REQ_PKT		(1U<<5)
#define BIT_HCD_RXCSR_FLUSH_FIFO	(1U<<4)
#define BIT_HCD_RXCSR_DATA_ERROR	(1U<<3) /* in isochronous mode */
#define BIT_HCD_RXCSR_NAKTIMEOUT	(1U<<3) /* in bulk mode */
#define BIT_HCD_RXCSR_ERROR		(1U<<2)
#define BIT_HCD_RXCSR_FIFOFULL		(1U<<1)
#define BIT_HCD_RXCSR_RXPKTRDY		(1U<<0)

/* bits of HIUSB_HCD_RXCOUNT in host mode */
#define HIUSB_HCD_RXCOUNT_MASK		0xFFFF

/* bits of HIUSB_HCD_TXTYPE in host mode */
#define HIUSB_HCD_TXTYPE_MASK		((1U<<6)-1)
#define BITS_HCD_TXTYPE_PROTOCOL	(1U<<4)
#define BITS_HCD_TXTYPE_ENDPOINT	(1U<<0)
#define BITS_HCD_TXTYPE_ENDPOINT_MASK	0x0F

/* TXMAXP-RPOTOCOL value */
#define PROTOCOL_ILLEGAL		0
#define PROTOCOL_ISOCHRONOUS		(1U<<4)
#define PROTOCOL_BULK			(2U<<4)
#define PROTOCOL_INTERRUPT		(3U<<4)

/* bits of HIUSB_HCD_TXINTERVAL in host mode */
#define HIUSB_HCD_TXINTERVAL_MASK	0xFF

/* bits of HIUSB_HCD_RXTYPE in host mode */
#define HIUSB_HCD_RXTYPE_MASK		((1U<<6)-1)
#define BITS_HCD_RXTYPE_PORTOCOL	(1U<<4)
#define BITS_HCD_RXTYPE_ENDPOINT	(1U<<0)

/* bits of HIUSB_HCD_RXINTERVAL in host mode */
#define HIUSB_HCD_RXINTERVAL_MASK	0xFF

/* bits of HIUSB_HCD_FIFO_SIZE in host mode */
#define HIUSB_HCD_FIFO_SIZE_MASK	0xFF
#define BITS_HCD_FIFO_SIZE_RX		(1U<<4)
#define BITS_HCD_FIFO_SIZE_RX_MASK	0xF0
#define BITS_HCD_FIFO_SIZE_TX		(1U<<0)
#define BITS_HCD_FIFO_SIZE_TX_MASK	0x0F

/* bits of HIUSB_HCD_DMA_INTR in host mode */
#define HIUSB_HCD_DMA_INTR_MASK		(0xFFFFFFFF)
#define BIT_HCD_DMA_CHANNEL1_INTR	(1U<<0)

/* bits of HIUSB_HCD_DMA_CNTL in host mode */
#define OFFSET_HCD_DMA_CNTL_FIFO_INDEX	4 /*bit 4*/
#define HIUSB_HCD_DMA_CNTL_MASK		(0xFFFFFFFF)
#define BIT_HCD_DMA_CNTL_EN		(1U<<0)
#define BIT_HCD_DMA_CNTL_DIRECTION	(1U<<1)
#define BIT_HCD_DMA_CNTL_MODE		(1U<<2)
#define BIT_HCD_DMA_CNTL_INTR_EN	(1U<<3)
#define BITS_HCD_DMA_CNTL_FIFO_INDEX	(1U<<4) /*4bits*/
#define BIT_HCD_DMA_CNTL_BUS_ERROR	(1U<<8)
#define BIT_HCD_DMA_CNTL_BURST_MODE	(1U<<9) /*2bits*/

#define HIUSB_HCD_DMA_CNTL_DIR_TX	(1U<<1)
#define HIUSB_HCD_DMA_CNTL_DIR_RX	(0U<<1)

#define HIUSB_HCD_DMA_MODE0		(0U<<2)
#define HIUSB_HCD_DMA_MODE1		(1U<<2)

#define HIUSB_HCD_DMA_BURST_MODE0	(0U<<9)
#define HIUSB_HCD_DMA_BURST_MODE1	(1U<<9)
#define HIUSB_HCD_DMA_BURST_MODE2	(2U<<9)
#define HIUSB_HCD_DMA_BURST_MODE3	(3U<<9)

/* bits of HIUSB_HCD_DMA_RQPKT_BASE in host mode */
#define HIUSB_HCD_DMA_RQPKT_MASK	0xFFFF

/* USB protocol */
#define HIUSB_HCD_SETUP_PACKET_LEN	8

/* hiusb inner defines */
#define BIT_FLAG_BULK_TRANSACTION_RESRT_DATATOG (0x00000001)

#define HIUSB_HCD_CTRL_FIFO_INDEX	0
#define HIUSB_HCD_BULK_TX_FIFO_INDEX	1
#define HIUSB_HCD_BULK_RX_FIFO_INDEX	1

int hiusb_hcd_set_addr(struct hiusb_hcd *hcd, unsigned int addr);
int hiusb_hcd_get_addr(struct hiusb_hcd *hcd);

int hiusb_hcd_read_power(struct hiusb_hcd *hcd);
int hiusb_hcd_enable_highspeed_mode(struct hiusb_hcd *hcd);
int hiusb_hcd_disable_highspeed_mode(struct hiusb_hcd *hcd);
int hiusb_hcd_read_highspeed_mode(struct hiusb_hcd *hcd);
int hiusb_hcd_send_reset_signal(struct hiusb_hcd *hcd);
int hiusb_hcd_clear_reset_signal(struct hiusb_hcd *hcd);
/* FIXME : not support resume, suspend */

int hiusb_hcd_read_clear_intrtx(struct hiusb_hcd *hcd);
int hiusb_hcd_read_clear_intrrx(struct hiusb_hcd *hcd);
int hiusb_hcd_read_clear_intrusb(struct hiusb_hcd *hcd);
int hiusb_hcd_read_clear_intrdma(struct hiusb_hcd *hcd);

int hiusb_hcd_enable_intrtx(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_enable_intrrx(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_enable_intrusb(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_enable_intrdma(struct hiusb_hcd *hcd);

int hiusb_hcd_disable_intrtx(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_disable_intrrx(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_disable_intrusb(struct hiusb_hcd *hcd, unsigned int irq);
int hiusb_hcd_disable_intrdma(struct hiusb_hcd *hcd);

int hiusb_hcd_read_frame(struct hiusb_hcd *hcd);

int hiusb_hcd_set_reg_fifo_index(struct hiusb_hcd *hcd, unsigned int index);
int hiusb_hcd_get_reg_fifo_index(struct hiusb_hcd *hcd);
int hiusb_hcd_get_fifo_size_rx(struct hiusb_hcd *hcd, unsigned int index);
int hiusb_hcd_get_fifo_size_tx(struct hiusb_hcd *hcd, unsigned int index);

/* FIXME : not support test mode */

int hiusb_hcd_read_devctl(struct hiusb_hcd *hcd);
int hiusb_hcd_set_session(struct hiusb_hcd *hcd);
int hiusb_hcd_clear_session(struct hiusb_hcd *hcd);
int hiusb_hcd_get_vbus_level(struct hiusb_hcd *hcd);

/* dma support */
//int hiusb_hcd_set_dma_rqpktcount(struct hiusb_hcd *hcd, unsigned int rqpktcount);
//int hiusb_hcd_get_dma_rqpktcount(struct hiusb_hcd *hcd);
//int hiusb_hcd_set_dma_direction(struct hiusb_hcd *hcd, unsigned int direction);
//int hiusb_hcd_set_dma_mode(struct hiusb_hcd *hcd, unsigned int mode);
//int hiusb_hcd_set_dma_fifo_index(struct hiusb_hcd *hcd, unsigned int fifo_index);
//int hiusb_hcd_set_dma_burst(struct hiusb_hcd *hcd, unsigned int burst);

int hiusb_hcd_ctrlpipe_set_nak_limit(struct hiusb_hcd *hcd, unsigned int naklimit);
int hiusb_hcd_ctrlpipe_get_nak_limit(struct hiusb_hcd *hcd);
int hiusb_hcd_ctrlpipe_checkerror(struct hiusb_hcd *hcd);
int hiusb_hcd_ctrlpipe_tx(struct hiusb_hcd *hcd, 
		unsigned int addr,
		void *setup, 
		void *pkt, 
		unsigned int total_len,
		unsigned int *actual_len,
		unsigned int maxpacket,
		int *stage);
int hiusb_hcd_ctrlpipe_rx(struct hiusb_hcd *hcd, 
		unsigned int addr,
		void *setup, 
		void *pkt, 
		unsigned int total_len,
		unsigned int *actual_len,
		unsigned int maxpacket,
		int *stage);

#define hiusb_hcd_tx_fifo_no_empty(hcd) (hiusb_readw((hcd), HIUSB_HCD_TXCSR) & BIT_HCD_TXCSR_FIFONOTEMPTY)
#define hiusb_hcd_tx_fifo_is_empty(hcd) (!hiusb_hcd_tx_fifo_no_empty((hcd)))

int hiusb_hcd_bulkpipe_tx_set_maxpkt(struct hiusb_hcd *hcd, unsigned int txmaxp);
int hiusb_hcd_bulkpipe_tx_get_maxpkt(struct hiusb_hcd *hcd);
int hiusb_hcd_bulkpipe_tx_set_nak_limit(struct hiusb_hcd *hcd, unsigned int naklimit);
int hiusb_hcd_bulkpipe_tx_get_nak_limit(struct hiusb_hcd *hcd);
int hiusb_hcd_bulkpipe_tx_set_endpoint(struct hiusb_hcd *hcd, unsigned int ep);
int hiusb_hcd_bulkpipe_tx_get_endpoint(struct hiusb_hcd *hcd);
int hiusb_hcd_bulkpipe_tx_cpumode_check_clear_error(struct hiusb_hcd *hcd);

int hiusb_hcd_bulkpipe_tx_dmamode1(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt_phy, 
		unsigned int len, 
		unsigned int maxpacket);

int hiusb_hcd_bulkpipe_tx_cpumode(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt, 
		unsigned int len, 
		unsigned int maxpacket);

int hiusb_hcd_bulkpipe_rx_set_nak_limit(struct hiusb_hcd *hcd, unsigned int naklimit);
int hiusb_hcd_bulkpipe_rx_get_nak_limit(struct hiusb_hcd *hcd);
int hiusb_hcd_bulkpipe_rx_cpumode_check_clear_error(struct hiusb_hcd *hcd);

int hiusb_hcd_bulkpipe_rx_dmamode1(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		void *pkt_phy, 
		unsigned int buflen,
		unsigned int maxpacket);

int hiusb_hcd_bulkpipe_rx_cpumode_send_in_token(struct hiusb_hcd *hcd, 
		unsigned int addr,
		unsigned int endpoint,
		unsigned int maxpacket);
int hiusb_hcd_bulkpipe_rx_cpumode_finish(struct hiusb_hcd *hcd, 
		void *pkt,
		unsigned int buflen,
		unsigned int *rxlen);

int hiusb_hcd_bulkpipe_dmamode_checkerror(struct hiusb_hcd *hcd);

/* FIXME: not support isochronous,interrupt mode */

void _hcd_clear_pending_task(struct hiusb_hcd *hcd);

void hiusb_hcd_hw_sw_sync(struct hiusb_hcd *hcd);

int hiusb_hcd_hw_status_check(struct hiusb_hcd *hcd);

int hiusb_hcd_intpipe_rx(struct hiusb_hcd *hcd, struct urb *urb);
#endif
