/*
 * s3c2410-can-mcp2510.c
 *
 * can driver for SAMSUNG UP-NETARM2410
 *
 * Author: ZOU JIAN GUO <zounix@126.com>
 * Updated : WANG BIN <wbinbuaa@163.com>
 * based on threewater <threewater@up-tech.com>
 * Date  : $Date: 2004/09/29 13:22:00 $ 
 *
 * $Revision: 1.1.0.0 $
 *
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
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/arch/spi.h>
#include "mcp2510.h"
#include "up-can.h"

#include <asm/arch/S3C2410.h>


/* debug macros */
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK( x... )	printk("s3c2410-mcp2510: " ##x)
#else
#define DPRINTK( x... )
#endif


/********************** MCP2510 Pin *********************************/
#define MCP2510_IRQ		IRQ_EINT4	//IRQ_EINT6
#define MCP2510_PIN_CS		(1)  //GPIO_H0
#define GPIO_MCP2510_CS		(GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_H0)

#define MCP2510_Enable()	do {GPHDAT &= ~MCP2510_PIN_CS;udelay(1000);}while(0);
#define MCP2510_Disable()	do {GPHDAT |= MCP2510_PIN_CS;}while(0);
#define MCP2510_OPEN_INT()		enable_irq(MCP2510_IRQ)		//added by wb
#define MCP2510_CLOSE_INT()		disable_irq(MCP2510_IRQ)

////////////////////////////////////////////////////////////////////////////
// Start the transmission from one of the tx buffers.
#define MCP2510_transmit(address)		do{MCP2510_WriteBits(address, TXB_TXREQ_M, TXB_TXREQ_M);}while(0)
#define MCP2510_CanRevBuffer	128	//CAN接收缓冲区大小


/********************** MCP2510 Candata *********************************/
typedef struct {
	CanData MCP2510_Candata[MCP2510_CanRevBuffer];	//recieve data buffer
	int nCanRevpos;	//recieve buffer pos for queued events
	int nCanReadpos;	//read buffer pos for queued events
	int loopbackmode;
	wait_queue_head_t wq;
	spinlock_t lock;
}Mcp2510_DEV;

static Mcp2510_DEV mcp2510dev;

#define NextCanDataPos(pos)		do{(pos)=((pos)+1>=MCP2510_CanRevBuffer? 0: (pos)+1);}while(0)


#define DEVICE_NAME		"s3c2410-mcp2510"
#define SPIRAW_MINOR	1

static int Major = 0;
static int opencount=0;

#define TRUE 1
#define FALSE 0

//#define SendSIOData(data) SPISend(data,0)
//#define ReadSIOData()	SPIRecv(0)


///////////////////////////////////////////////////////////////////
static void printRegisters(void)
{
  DPRINTK ("GPFCON:%x\tGPFUP:%x\tGPFDAT:%x\n",
	  GPFCON, GPFUP, GPFDAT );
  DPRINTK ("GPGCON:%x\tGPGUP:%x\tGPGDAT:%x\n",
	  GPGCON, GPGUP, GPGDAT );
  DPRINTK ("INTMOD:%x\tINTMSK:%x\tINTPND:%x\tSRCPND:%x\n",
	  INTMOD, INTMSK, INTPND, SRCPND);
  DPRINTK ("EXTINT0:%x\tEINTMASK:%x\tEINTPEND:%x\n",
	  EXTINT0, EINTMASK, EINTPEND );
}
static void printGPE (void)
{
  DPRINTK ("GPECON:%x\tGPEUP:%x\tGPEDAT:%x\n",
	  GPECON, GPEUP, GPEDAT );
}
static void printSPI (void)
{
  DPRINTK ("SPPRE0:%d\tSPCON0:%x\tSPSTA0:%x\n", SPPRE0, SPCON0, SPSTA0 );
  DPRINTK ("SPTDAT0:%x\tSPRDAT0:%x\n", SPTDAT0, SPRDAT0 );
  DPRINTK ("SPPIN0:%x\n", SPPIN0);
}
///////////////////////////////////////////////////////////////////////


static void  SendSIOData(unsigned int data)
{
	SPISend(data,0);
}	
static unsigned int ReadSIOData()
{
	return SPIRecv(0);
	
}

static inline void MCP2510_Reset(void)
{
	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_RESET);

	MCP2510_Disable();
}

static void MCP2510_Write(int address, int value)
{
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_WRITE);
	SendSIOData((unsigned char)address);
	SendSIOData((unsigned char)value);

	MCP2510_Disable();

	local_irq_restore(flags);
}

static unsigned char MCP2510_Read(int address)
{
	unsigned char result;
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_READ);
	SendSIOData((unsigned char)address);

	SendSIOData(0);
	result=ReadSIOData();
	MCP2510_Disable();

	local_irq_restore(flags);

	return result;
}

static unsigned char MCP2510_ReadStatus(void)
{
	unsigned char result;
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_RDSTAT);

	SendSIOData(0);
	result=ReadSIOData();

	MCP2510_Disable();

	local_irq_restore(flags);
	return result;
}

static void MCP2510_WriteBits( int address, int data, int mask )
{
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_BITMDFY);
	SendSIOData((unsigned char)address);
	SendSIOData((unsigned char)mask);
	SendSIOData((unsigned char)data);

	MCP2510_Disable();

	local_irq_restore(flags);
}

/*******************************************\
*	序列读取MCP2510数据				*
\*******************************************/
static void MCP2510_SRead( int address, unsigned char* pdata, int nlength )
{
	int i;
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();
	SendSIOData(MCP2510INSTR_READ);
	SendSIOData((unsigned char)address);

	for (i=0; i<nlength; i++) {
		SendSIOData(0);
		*pdata=ReadSIOData();
		pdata++;
	}
	MCP2510_Disable();

	local_irq_restore(flags);
}


/*******************************************\
*	序列写入MCP2510数据				*
\*******************************************/
static void MCP2510_Swrite(int address, unsigned char* pdata, int nlength)
{
	int i;
	int flags;

	local_irq_save(flags);

	MCP2510_Enable();

	SendSIOData(MCP2510INSTR_WRITE);
	SendSIOData((unsigned char)address);

	for (i=0; i < nlength; i++) {
		SendSIOData((unsigned char)*pdata);
		pdata++;
	}
	MCP2510_Disable();

	local_irq_restore(flags);
}

/************************************************************\
*	设置MCP2510 CAN总线波特率						*
*	参数: bandrate为所设置的波特率				*
*			IsBackNormal为是否要返回Normal模式		*
\************************************************************/

static void MCP2510_SetBandRate(CanBandRate bandrate, int IsBackNormal)
{
	//
	// Bit rate calculations.
	//
	//Input clock fre=16MHz
	// In this case, we'll use a speed of 125 kbit/s, 250 kbit/s, 500 kbit/s.
	// If we set the length of the propagation segment to 7 bit time quanta,
	// and we set both the phase segments to 4 quanta each,
	// one bit will be 1+7+4+4 = 16 quanta in length.
	//
	// setting the prescaler (BRP) to 0 => 500 kbit/s.
	// setting the prescaler (BRP) to 1 => 250 kbit/s.
	// setting the prescaler (BRP) to 3 => 125 kbit/s.
	//
	// If we set the length of the propagation segment to 3 bit time quanta,
	// and we set both the phase segments to 1 quanta each,
	// one bit will be 1+3+2+2 = 8 quanta in length.
	// setting the prescaler (BRP) to 0 => 1 Mbit/s.

	// Go into configuration mode
	MCP2510_Write(MCP2510REG_CANCTRL, MODE_CONFIG);

	switch(bandrate){
	case BandRate_125kbps:
		MCP2510_Write(CNF1, SJW1|BRP4);	//Synchronization Jump Width Length =1 TQ
		MCP2510_Write(CNF2, BTLMODE_CNF3|(SEG4<<3)|SEG7); // Phase Seg 1 = 4, Prop Seg = 7
		MCP2510_Write(CNF3, SEG4);// Phase Seg 2 = 4
		break;
	case BandRate_250kbps:
		MCP2510_Write(CNF1, SJW1|BRP2);	//Synchronization Jump Width Length =1 TQ
		MCP2510_Write(CNF2, BTLMODE_CNF3|(SEG4<<3)|SEG7); // Phase Seg 1 = 4, Prop Seg = 7
		MCP2510_Write(CNF3, SEG4);// Phase Seg 2 = 4
		break;
	case BandRate_500kbps:
		MCP2510_Write(CNF1, SJW1|BRP1);	//Synchronization Jump Width Length =1 TQ
		MCP2510_Write(CNF2, BTLMODE_CNF3|(SEG4<<3)|SEG7); // Phase Seg 1 = 4, Prop Seg = 7
		MCP2510_Write(CNF3, SEG4);// Phase Seg 2 = 4
		break;
	case BandRate_1Mbps:
		MCP2510_Write(CNF1, SJW1|BRP1);	//Synchronization Jump Width Length =1 TQ
		MCP2510_Write(CNF2, BTLMODE_CNF3|(SEG3<<3)|SEG2); // Phase Seg 1 = 2, Prop Seg = 3
		MCP2510_Write(CNF3, SEG2);// Phase Seg 2 = 1
		break;
	}

	if(IsBackNormal){
		//Enable clock output
		MCP2510_Write(CLKCTRL, MODE_NORMAL | CLKEN | CLK1);
	}
}


/*******************************************\
*	读取MCP2510 CAN总线ID				*
*	参数: address为MCP2510寄存器地址*
*			can_id为返回的ID值			*
*	返回值								*
*	TRUE，表示是扩展ID(29位)			*
*	FALSE，表示非扩展ID(11位)		*
\*******************************************/
static int MCP2510_Read_Can_ID( int address, __u32* can_id)
{
	__u32 tbufdata;
	unsigned char* p=(unsigned char*)&tbufdata;

	MCP2510_SRead(address, p, 4);
	*can_id = (tbufdata<<3)|((tbufdata>>13)&0x7);
	*can_id &= 0x7ff;

	if ( (p[MCP2510LREG_SIDL] & TXB_EXIDE_M) ==  TXB_EXIDE_M ) {
		*can_id = (*can_id<<2) | (p[MCP2510LREG_SIDL] & 0x03);
		*can_id <<= 16;
		*can_id |= tbufdata>>16;
		return TRUE;
	}
	return FALSE;
}

/***********************************************************\
*	读取MCP2510 接收的数据							*
*	参数: 													*
*		nbuffer为第几个缓冲区可以为3或者4		*
*		CanData为CAN数据结构							*
\***********************************************************/
static void MCP2510_Read_Can(unsigned char nbuffer, PCanData candata)
{

	unsigned char mcp_addr = (nbuffer<<4) + 0x31, ctrl;
	int IsExt;
	char dlc;

	IsExt=MCP2510_Read_Can_ID( mcp_addr, &(candata->id));

	ctrl=MCP2510_Read(mcp_addr-1);
	dlc=MCP2510_Read( mcp_addr+4);
	if ((ctrl & 0x08)) {
		candata->rxRTR = TRUE;
	}
	else{
		candata->rxRTR = FALSE;
	}
	dlc &= DLC_MASK;
	MCP2510_SRead(mcp_addr+5, candata->data, dlc);

	candata->dlc=dlc;
}
/*******************************************\
*	设置MCP2510 CAN总线ID				*
*	参数: address为MCP2510寄存器地址*
*			can_id为设置的ID值			*
*			IsExt表示是否为扩展ID	*
\*******************************************/
static void MCP2510_Write_Can_ID(int address, __u32 can_id, int IsExt)
{
	__u32 tbufdata;

	if (IsExt) {
		can_id &= 0x1fffffff;	//29位
		tbufdata=can_id &0xffff;
		tbufdata<<=16;
		tbufdata|=((can_id>>(18-5))&(~0x1f));
		tbufdata |= TXB_EXIDE_M;
	}
	else{
		can_id&=0x7ff;	//11位
		tbufdata= (can_id>>3)|((can_id&0x7)<<13);
	}
	MCP2510_Swrite(address, (unsigned char*)&tbufdata, 4);
	MCP2510_Read_Can_ID(address, &tbufdata);
	DPRINTK("write can id=%x, result id=%x\n",can_id, tbufdata);
}

/***********************************************************\
*	写入MCP2510 发送的数据							*
*	参数: 													*
*		nbuffer为第几个缓冲区可以为0、1、2		*
*		CanData为CAN数据结构							*
\***********************************************************/
static void MCP2510_Write_Can( unsigned char nbuffer, PCanData candata)
{
	unsigned char dlc;
	unsigned char mcp_addr = (nbuffer<<4) + 0x31;

	dlc=candata->dlc;
	MCP2510_Swrite(mcp_addr+5, candata->data, dlc);  // write data bytes
	MCP2510_Write_Can_ID( mcp_addr, candata->id,candata->IsExt);  // write CAN id
	if (candata->rxRTR)
		dlc |= RTR_MASK;  // if RTR set bit in byte
	MCP2510_Write((mcp_addr+4), dlc);            // write the RTR and DLC
}

/***********************************************************\
*	write and send Can data									*
*	we must set can id first.										*
*	parament:												*
*		nbuffer: which buffer, should be: 0, 1, 2					*
*		pbuffer: send data 										*
*		nbuffer: size of data 									*
\***********************************************************/
static void MCP2510_Write_CanData( unsigned char nbuffer, char *pbuffer, int nsize)
{
	unsigned char dlc;
	unsigned char mcp_addr = (nbuffer<<4) + 0x31;

	dlc=nsize&DLC_MASK;	//nbuffer must <= 8
	MCP2510_Swrite(mcp_addr+5, pbuffer, dlc);  // write data bytes
	MCP2510_Write((mcp_addr+4), dlc);            // write the RTR and DLC
}

/***********************************************************\
*	write and send Remote Can Frame							*
*	we must set can id first.										*
*	parament:												*
*		nbuffer: which buffer, should be: 0, 1, 2					*
\***********************************************************/
static void MCP2510_Write_CanRTR( unsigned char nbuffer)
{
	unsigned char dlc=0;
	unsigned char mcp_addr = (nbuffer<<4) + 0x31;

	dlc |= RTR_MASK;  // if RTR set bit in byte
	MCP2510_Write((mcp_addr+4), dlc);            // write the RTR and DLC
}


// Setup the CAN buffers used by the application.
// We currently use only one for reception and one for transmission.
// It is possible to use several to get a simple form of queue.
//
// We setup the unit to receive all CAN messages.
// As we only have at most 4 different messages to receive, we could use the
// filters to select them for us.
//
// mcp_init() should already have been called.
static void MCP2510_Setup(PCanFilter pfilter)
{
    // As no filters are active, all messages will be stored in RXB0 only if
    // no roll-over is active. We want to recieve all CAN messages (standard and extended)
    // (RXM<1:0> = 11).
    //SPI_mcp_write_bits(RXB0CTRL, RXB_RX_ANY, 0xFF);
    //SPI_mcp_write_bits(RXB1CTRL, RXB_RX_ANY, 0xFF);

    // But there is a bug in the chip, so we have to activate roll-over.
	if(pfilter){	//有过滤器
		MCP2510_WriteBits(RXB0CTRL, (RXB_BUKT|RXB_RX_STDEXT|RXB_RXRTR|RXB_RXF0), 0xFF);
		MCP2510_WriteBits(RXB1CTRL, RXB_RX_STDEXT, 0xFF);
	}
	else{
		MCP2510_WriteBits(RXB0CTRL, (RXB_BUKT|RXB_RX_ANY|RXB_RXRTR), 0xFF);
		MCP2510_WriteBits(RXB1CTRL, RXB_RX_ANY, 0xFF);
	}
}

/***********************************************************************************\
								发送数据
	参数:
		data，发送数据

	Note: 使用三个缓冲区循环发送，没有做缓冲区有效检测
\***********************************************************************************/
static int ntxbuffer=0;

static inline void MCP2510_canTxBuffer(void)
{
	switch(ntxbuffer){
	case 0:
		MCP2510_transmit(TXB0CTRL);
		ntxbuffer=1;
		break;
	case 1:
		MCP2510_transmit(TXB1CTRL);
		ntxbuffer=2;
		break;
	case 2:
		MCP2510_transmit(TXB2CTRL);
		ntxbuffer=0;
		break;
	}
}

static inline void MCP2510_canWrite(PCanData data)
{
	MCP2510_Write_Can(ntxbuffer, data);
	MCP2510_canTxBuffer();
}

static inline void MCP2510_canWriteData(char *pbuffer, int nbuffer)
{
	MCP2510_Write_CanData(ntxbuffer, pbuffer, nbuffer);
	MCP2510_canTxBuffer();
}

static inline void MCP2510_canWriteRTR(void)
{
	MCP2510_Write_CanRTR(ntxbuffer);
	MCP2510_canTxBuffer();
}


/***********************************************************************************\
								中断服务程序									
\***********************************************************************************/

static void s3c2410_isr_mcp2510(int irq, void *dev_id, struct pt_regs *reg)
{
	unsigned char byte;

	DPRINTK("enter interrupt!\n");

	byte=MCP2510_Read(CANINTF);
	
	if(byte & RX0INT){
		MCP2510_Read_Can(3,&(mcp2510dev.MCP2510_Candata[mcp2510dev.nCanRevpos]));
		MCP2510_WriteBits(CANINTF, ~RX0INT, RX0INT); // Clear interrupt
		NextCanDataPos(mcp2510dev.nCanRevpos);
		DPRINTK("mcp2510dev.nCanRevpos= %d\n", mcp2510dev.nCanRevpos);
		DPRINTK("mcp2510dev.nCanReadpos= %d\n", mcp2510dev.nCanReadpos);

	}

	if(byte & RX1INT){
		MCP2510_Read_Can(4,&(mcp2510dev.MCP2510_Candata[mcp2510dev.nCanRevpos]));
		MCP2510_WriteBits(CANINTF, ~RX1INT, RX1INT); // Clear interrupt
		NextCanDataPos(mcp2510dev.nCanRevpos);
	}

	if(byte & (RX0INT|RX1INT)){
		wake_up_interruptible(&(mcp2510dev.wq));
	}

}

/*********************************************************************\
	CAN设备初始化函数
	参数:	bandrate，CAN波特率
\*********************************************************************/
static int init_MCP2510(CanBandRate bandrate)
{
	unsigned char i,j,a;
	MCP2510_Reset();

	MCP2510_SetBandRate(bandrate,FALSE);

	// Disable interrups.
	MCP2510_Write(CANINTE, NO_IE);

	// Mark all filter bits as don't care:
	MCP2510_Write_Can_ID(RXM0SIDH, 0, TRUE);
	MCP2510_Write_Can_ID(RXM1SIDH, 0, TRUE);
	// Anyway, set all filters to 0:
	MCP2510_Write_Can_ID(RXF0SIDH, 0, 0);
	MCP2510_Write_Can_ID(RXF1SIDH, 0, 0);
	MCP2510_Write_Can_ID(RXF2SIDH, 0, 0);
	MCP2510_Write_Can_ID(RXF3SIDH, 0, 0);
	MCP2510_Write_Can_ID(RXF4SIDH, 0, 0);
	MCP2510_Write_Can_ID(RXF5SIDH, 0, 0);

	MCP2510_Write_Can_ID(TXB0SIDH, 123, 0);
	MCP2510_Write_Can_ID(TXB1SIDH, 100, 0);
	MCP2510_Write_Can_ID(TXB2SIDH, 111, 0);

	//Enable clock output
	MCP2510_Write(CLKCTRL, MODE_LOOPBACK| CLKEN | CLK1);

	// Clear, deactivate the three transmit buffers
	a = TXB0CTRL;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 14; j++) {
			MCP2510_Write(a, 0);
			a++;
	        }
       	a += 2; // We did not clear CANSTAT or CANCTRL
	}
	// and the two receive buffers.
	MCP2510_Write(RXB0CTRL, 0);
	MCP2510_Write(RXB1CTRL, 0);

	// The two pins RX0BF and RX1BF are used to control two LEDs; set them as outputs and set them as 00.
	MCP2510_Write(BFPCTRL, 0x3C);
	
	return 0;
}

static void MCP2510_SetFilter(PCanFilter pfilter)
{
	MCP2510_Write(MCP2510REG_CANCTRL, MODE_CONFIG);
	// Disable interrups.
	MCP2510_Write(CANINTE, NO_IE);

	if(!pfilter){
		// Mark all filter bits as don't care:
		MCP2510_Write_Can_ID(RXM0SIDH, 0, TRUE);
		MCP2510_Write_Can_ID(RXM1SIDH, 0, TRUE);
		// Anyway, set all filters to 0:
		MCP2510_Write_Can_ID(RXF0SIDH, 0, 0);
		MCP2510_Write_Can_ID(RXF1SIDH, 0, 0);
		MCP2510_Write_Can_ID(RXF2SIDH, 0, 0);
		MCP2510_Write_Can_ID(RXF3SIDH, 0, 0);
		MCP2510_Write_Can_ID(RXF4SIDH, 0, 0);
		MCP2510_Write_Can_ID(RXF5SIDH, 0, 0);
	}
	else{
		// Mark
		MCP2510_Write_Can_ID(RXM0SIDH, pfilter->Mask, TRUE);
		MCP2510_Write_Can_ID(RXM1SIDH, pfilter->Mask, TRUE);

		// set all filters to same = pfilter->Filter:
		MCP2510_Write_Can_ID(RXF0SIDH, pfilter->Filter, pfilter->IsExt);
		MCP2510_Write_Can_ID(RXF1SIDH, pfilter->Filter, pfilter->IsExt);
		MCP2510_Write_Can_ID(RXF2SIDH, pfilter->Filter, pfilter->IsExt);
		MCP2510_Write_Can_ID(RXF3SIDH, pfilter->Filter, pfilter->IsExt);
		MCP2510_Write_Can_ID(RXF4SIDH, pfilter->Filter, pfilter->IsExt);
		MCP2510_Write_Can_ID(RXF5SIDH, pfilter->Filter, pfilter->IsExt);
	}
	//Enable clock output
	if(mcp2510dev.loopbackmode)
		MCP2510_Write(CLKCTRL, MODE_LOOPBACK| CLKEN | CLK1);
	else
		MCP2510_Write(CLKCTRL, MODE_NORMAL| CLKEN | CLK1);

	// and the two receive buffers.
	MCP2510_Write(RXB0CTRL, 0);
	MCP2510_Write(RXB1CTRL, 0);

	MCP2510_Setup(pfilter);
}

static int s3c2410_mcp2510_ioctl(struct inode *inode, struct file *file, 
                                unsigned int cmd, unsigned long arg)
{
	int flags;

local_irq_save(flags);

	switch (cmd){
	case UPCAN_IOCTRL_SETBAND:	//set can bus band rate
		MCP2510_SetBandRate((CanBandRate)arg ,TRUE);
		mdelay(10);
		break;
	case UPCAN_IOCTRL_SETID:	//set can frame id data
		MCP2510_Write_Can_ID(TXB0SIDH, arg, arg&UPCAN_EXCAN);
		MCP2510_Write_Can_ID(TXB1SIDH, arg, arg&UPCAN_EXCAN);
		MCP2510_Write_Can_ID(TXB2SIDH, arg, arg&UPCAN_EXCAN);
		break;
	case UPCAN_IOCTRL_SETLPBK:	//set can device in loop back mode or normal mode
		if(arg){
			MCP2510_Write(CLKCTRL, MODE_LOOPBACK| CLKEN | CLK1);
			mcp2510dev.loopbackmode=1;
		}
		else{
			MCP2510_Write(CLKCTRL, MODE_NORMAL| CLKEN | CLK1);
			mcp2510dev.loopbackmode=0;
		}

		break;
	case UPCAN_IOCTRL_SETFILTER://set a filter for can device
		MCP2510_SetFilter((PCanFilter)arg);
		break;
	case UPCAN_IOCTRL_PRINTRIGISTER://set a filter for can device
		printGPE();
		printSPI();
		printRegisters();
		break;
		
	}
	
local_irq_restore(flags);
	
	DPRINTK("IO control command=0x%x\n", cmd);
	return 0;
}

/***************************************************************\
*	write and send Can data interface for can device	 file					*
*	there are 2 mode for send data:									*
*		1, if write data size = sizeof(CanData) then send a full can frame	*
*		2, if write data size <=8 then send can data,					*
*		    we must set frame id first									*
\****************************************************************/
static ssize_t s3c2410_mcp2510_write(struct file *file, const char *buffer, 
				    size_t count, loff_t * ppos)
{
	char sendbuffer[sizeof(CanData)];

	if(count==sizeof(CanData)){
		//send full Can frame---frame id and frame data
		copy_from_user(sendbuffer, buffer, sizeof(CanData));
		MCP2510_canWrite((PCanData)sendbuffer);

		DPRINTK("Send a Full Frame\n");
		return count;
	}

	if(count>8)
		return 0;

	//count <= 8

	copy_from_user(sendbuffer, buffer, count);
	MCP2510_canWriteData(sendbuffer, count);

	DPRINTK("Send data size=%d\n", count);
       DPRINTK("data=%x,%x,%x,%x,%x,%x,%x,%x\n",
		sendbuffer[0],sendbuffer[1],sendbuffer[2],sendbuffer[3],
		sendbuffer[4],sendbuffer[5],sendbuffer[6],sendbuffer[7]);
	return count;
}

static int RevRead(CanData *candata_ret)
{
	spin_lock_irq(&(mcp2510dev.lock));

	memcpy(candata_ret, 
		&(mcp2510dev.MCP2510_Candata[mcp2510dev.nCanReadpos]), 
		sizeof(CanData));
	NextCanDataPos(mcp2510dev.nCanReadpos);

	spin_unlock_irq(&(mcp2510dev.lock));

	return sizeof(CanData);
}


static ssize_t s3c2410_mcp2510_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	CanData candata_ret;
	
	DPRINTK("run in s3c2410_mcp2510_read\n");

retry: 
	if (mcp2510dev.nCanReadpos !=  mcp2510dev.nCanRevpos) {
		int count;
		count = RevRead(&candata_ret);
		if (count) copy_to_user(buffer, (char *)&candata_ret, count);
		DPRINTK("read data size=%d\n", count);
		DPRINTK("id=%x, data=%x,%x,%x,%x,%x,%x,%x,%x\n", 
			candata_ret.id, candata_ret.data[0],
			candata_ret.data[1], candata_ret.data[2], 
			candata_ret.data[3], candata_ret.data[4],
			candata_ret.data[5], candata_ret.data[6],
			candata_ret.data[7]);
		return count;
	} else {
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}
		interruptible_sleep_on(&(mcp2510dev.wq));
		if (signal_pending(current)){
			return -ERESTARTSYS;
		}
		goto retry;
	}
	DPRINTK("read data size=%d\n", sizeof(candata_ret));
	return sizeof(candata_ret);
}

static int s3c2410_mcp2510_open(struct inode *inode, struct file *file)
{
	int i,j,a;

	if(opencount==1)
		return -EBUSY;
	opencount++;

	memset(&mcp2510dev, 0 ,sizeof(mcp2510dev));

	init_waitqueue_head(&(mcp2510dev.wq));

	//Enable clock output
	MCP2510_Write(CLKCTRL, MODE_NORMAL| CLKEN | CLK1);
	// Clear, deactivate the three transmit buffers
	a = TXB0CTRL;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 14; j++) {
			MCP2510_Write(a, 0);
			a++;
	    }
	    a += 2; // We did not clear CANSTAT or CANCTRL
	}
	// and the two receive buffers.
	MCP2510_Write(RXB0CTRL, 0);
	MCP2510_Write(RXB1CTRL, 0);

	//Open Interrupt
	MCP2510_Write(CANINTE, RX0IE|RX1IE);
	MCP2510_Setup(NULL);

	MCP2510_OPEN_INT();
	set_gpio_ctrl(GPIO_MCP2510_CS);

	MOD_INC_USE_COUNT;
	DPRINTK("device open\n");
	return 0;
}

static int s3c2410_mcp2510_release(struct inode *inode, struct file *filp)
{
	opencount--;

	MCP2510_Write(CANINTE, NO_IE);
	MCP2510_Write(CLKCTRL, MODE_LOOPBACK| CLKEN | CLK1);

	MCP2510_CLOSE_INT();

	MOD_DEC_USE_COUNT;
	DPRINTK("device release\n");
	return 0;
}

static struct file_operations s3c2410_fops = {
	owner:	THIS_MODULE,
	write:	s3c2410_mcp2510_write,
	read:	s3c2410_mcp2510_read,
	ioctl:	s3c2410_mcp2510_ioctl,
	open:	s3c2410_mcp2510_open,
	release:	s3c2410_mcp2510_release,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_spi_dir, devfs_spiraw;
#endif

static int __init s3c2410_mcp2510_init(void)
{
	int ret;
	int flags;

	set_gpio_ctrl(GPIO_MCP2510_CS);
	printGPE();
  	printSPI();
	printRegisters();

	local_irq_save(flags);
	
	init_MCP2510(BandRate_250kbps);
	/* Register IRQ handlers */
	
	ret = set_external_irq(MCP2510_IRQ, EXT_LOWLEVEL, GPIO_PULLUP_DIS);
	if (ret)
		return ret;
	local_irq_restore(flags);

	ret = register_chrdev(0, DEVICE_NAME, &s3c2410_fops);
	if (ret < 0) {
		printk(DEVICE_NAME " can't get major number\n");
		return ret;
	}
	Major = ret;

	/* Enable touch interrupt */
	ret = request_irq(MCP2510_IRQ, s3c2410_isr_mcp2510, SA_INTERRUPT, 
			  DEVICE_NAME, s3c2410_isr_mcp2510);
	if (ret) 	
		return ret;

	MCP2510_CLOSE_INT();

#ifdef CONFIG_DEVFS_FS
	devfs_spi_dir = devfs_mk_dir(NULL, "can", NULL);
	devfs_spiraw = devfs_register(devfs_spi_dir, "0", DEVFS_FL_DEFAULT,
			Major, SPIRAW_MINOR, S_IFCHR | S_IRUSR | S_IWUSR,
			&s3c2410_fops, NULL);
#endif
	printk(DEVICE_NAME " initialized\n");
	
	return 0;
}

static void __exit s3c2410_mcp2510_exit(void)
{

	printk(DEVICE_NAME " unloaded\n");
}

module_init(s3c2410_mcp2510_init);
module_exit(s3c2410_mcp2510_exit);
