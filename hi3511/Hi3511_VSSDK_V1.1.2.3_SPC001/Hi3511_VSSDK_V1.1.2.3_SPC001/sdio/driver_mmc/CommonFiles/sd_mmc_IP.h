/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   sd_mmc_IP.h
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   registers.pdf

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the common constants.

    REVISION HISTORY:

***************************************************************/

#ifndef SD_MMC_IP_H
#define SD_MMC_IP_H

//Compile time constants:
//#define MMC_ONLY_MODE   //To enable MMC_only specific part.

//To enable interrupt.For polled mode,comment this.
#define INTR_ENABLE  1
#define DMA_ENABLE 1

//To add HSMMC support.
#define HSMMC_SUPPORT   

//To add HSSD SUPPORT.
#define HSSD_SUPPORT   

#define PROCESSOR_CLK       50000     //In KHz. 50MHz.

#define CIU_CLK             40000     //In KHz. 24 MHz
#define SD_MAX_OPFREQUENCY  25000 
#define MMC_MAX_OPFREQUENCY 20000 

#define  CARD_OCR_RANGE  0x00300000 // 3.2-3.4V bits 20,21 set.
#define  SD_VOLT_RANGE   0x80ff8000 // 2.7-3.6V bits 20,21 set.
#define  MMC_33_34V      0x80ff8000 // 2.7-3.6V bits 20,21 set.
#define  FIFO_DEPTH      8
#define  SD_MMC_IRQ      5         //IRQ number for core.

//This file contains constants to be used in all drivers.

//Note:Card no.starts from 0.So for 30 cards,card no.is from 0-29.

  #define MAX_CARDS  1     //SD_MMC mode
  #define MAX_HSMMC_CARDS  2 //HSMMC sepcific constant.
  #define MAX_HSSD_CARDS   2 //HSSD sepcific constant.


//Divider value for MMC mode. Basic freq.is divided by
// 2*MMC_FREQDIVIDER.  Currently for MMC type, freq.=20MHz and
//for SD type, freq.=25 MHz is used.

//CIU_CLK is in KHz eg 24000
//MMC :1 for 25 ,2 for 50.  SD :0 for 25 ,1 for 50.
//#define MMC_FREQDIVIDER    CIU_CLK/(MMC_MAX_OPFREQUENCY+1)   //MMC work at 12.5MHz
//#define SD_FREQDIVIDER     CIU_CLK/(SD_MAX_OPFREQUENCY+1)       //sd and sdio, work at 25MHZ
#define MMC_FREQDIVIDER    CIU_CLK/(SD_MAX_OPFREQUENCY)   //MMC work at 12.5MHz
#define SD_FREQDIVIDER     CIU_CLK/(SD_MAX_OPFREQUENCY*2)       //sd and sdio, work at 25MHZ

#define MAX_THRESHOLD  0x0fff
#define MIN_THRESHOLD  100

//25000/(400*2)+1=32 25000/32*2=390
//22000/(400*2)+1=28 22000/28*2=392
//24000/(400*2)+1=31 24000/31*2=387
#define FOD_VALUE  400000 //400 KHz
#define MMC_FOD_VALUE     125  //125 KHz
#define SD_FOD_VALUE      300  //125 KHz

#define FOD_DIVIDER_VALUE  (CIU_CLK/(FOD_VALUE*2))+1 

#define TOTAL_CLK_SOURCES  3 //0,1,2,3 are 4 Clock sources.

//IP reg constants.This offset is actual register address.
#define  CTRL_off    0x0
#define  PWREN_off   0x4
#define  CLKDIV_off  0x8
#define  CLKSRC_off  0xC
#define  CLKENA_off  0x10
#define  TMOUT_off   0x14
#define  CTYPE_off   0x18
#define  BLKSIZ_off  0x1C
#define  BYTCNT_off  0x20
#define  INTMSK_off  0x24
#define  CMDARG_off  0x28
#define  CMD_off     0x2C
#define  RESP0_off   0x30
#define  RESP1_off   0x34
#define  RESP2_off   0x38
#define  RESP3_off   0x3C
#define  MINTSTS_off 0x40
#define  RINTSTS_off 0x44
#define  STATUS_off  0x48
#define  FIFOTH_off  0x4C
#define  CDETECT_off 0x50
#define  WRTPRT_off  0x54
#define  GPIO_off    0x58
#define  TCBCNT_off  0x5C
#define  TBBCNT_off  0x60
#define  DEBNCE_off  0x64
#define  USRID_off   0x68
#define  VERID_off   0x6C
#define  HCON_off    0x70

//Reserved = 0x070 to 0x0ff
#define  FIFO_START  0x100

//Structures used. :-
typedef union {
       WORD  wReg;
       struct IntstsbitsTag{ //Tag required by Linux
         unsigned Card_Detect            :  1; //LSB
         unsigned Rsp_Err                :  1; //LSB
         unsigned Cmd_Done               :  1; //LSB
         unsigned Data_trans_over        :  1; //LSB
         unsigned Tx_FIFO_datareq        :  1; //LSB
         unsigned Rx_FIFO_datareq        :  1; //LSB
         unsigned Rsp_crc_err            :  1; //LSB
         unsigned Data_crc_err           :  1; //LSB
         unsigned Rsp_tmout              :  1; //LSB
         unsigned Data_read_tmout        :  1; //LSB
         unsigned Data_starv_tmout       :  1; //LSB
         unsigned FIFO_run_err           :  1; //LSB
         unsigned HLE                    :  1; //LSB
         unsigned SBE                    :  1; //LSB
         unsigned ACD                    :  1; //LSB
         unsigned EBE                    :  1; //LSB
       }Bits; /* struct */
}Intr_Status_Bits;

//IP register structure.

//0x000
typedef union {
       DWORD  dwReg;
       struct CtrlbitsTag{//Tag required by Linux
         unsigned controller_reset       :  1; //LSB
         unsigned fifo_reset             :  1;
         unsigned dma_reset              :  1;
         unsigned auto_clear_int         :  1;
         unsigned int_enable             :  1;
         unsigned dma_enable             :  1;
         unsigned read_wait              :  1;
         unsigned send_irq_response      :  1;
         unsigned abort_read_data        :  1; 
         unsigned Reserved1              :  7;
         unsigned Card_voltage_a         :  4; 
         unsigned Card_voltage_b         :  4;
         unsigned enable_OD_pullup       :  1;
         unsigned Reserved2              :  7;
       }Bits; /* struct */
}ControlReg;

//0x008
typedef union {
       BYTE bArr[4];
       struct clkdivbitsTag{
         unsigned clk_divider0         :  8;//LSB
         unsigned clk_divider1         :  8;
         unsigned clk_divider2         :  8;
         unsigned clk_divider3         :  8;
       }Bits; /* struct */
}ClockDividerReg;

//0x010
typedef union {
       DWORD  dwReg;
       struct clkenbitsTag{
         unsigned cclk_enable            :  16; //LSB
         unsigned cclk_low_powER         :  16;
       }Bits; /* struct */
}ClockEnableReg;


//0x014
typedef union {
       DWORD  TMOUTReg;
       struct TmoutbitsTag{
         unsigned ResponseTimeOut      :  8; //LSB
         unsigned DataTimeOut          : 24;
       }Bits; /* struct */
}TimeOutReg;

//0x018
typedef union {
       DWORD  dwReg;
       struct CTypebitsTag{
         unsigned card_width           :  16; //LSB
         unsigned card_type            :  16;
       }Bits; /* struct */
}CardTypeReg;

//0x01C
typedef union {
       DWORD  dwReg;
       struct BlksizbitsTag{
         unsigned block_size           :  16; //LSB
         unsigned Reserved             :  16;
       }Bits; /* struct */
}BlockSizeReg;

//0x024
typedef union {
       DWORD  dwReg;
       struct IntMaskbitsTag{
         unsigned int_mask            :  16; //LSB
         unsigned sdio_int_mask       :  16;
       }Bits; /* struct */
}IntrMaskReg;

//0x02C
typedef union {
       DWORD  dwReg;
       struct CmdbitsTag{
         unsigned cmd_index            :  6;
         unsigned response_expect      :  1;
         unsigned response_length      :  1;
         unsigned check_response_crc   :  1;
         unsigned data_expected        :  1;
         unsigned read_write           :  1;
         unsigned transfer_mode        :  1;
         unsigned auto_stop            :  1;
         unsigned wait_prvdata_complete:  1;
         unsigned stop_abort_cmd       :  1;
         unsigned send_initialization  :  1;
         unsigned card_number          :  5;
         unsigned Update_clk_regs_only :  1; //Bit 21
         unsigned Reserved             :  9;
         unsigned start_cmd            :  1; //LSB
       }Bits; /* struct */
}CommandReg;

//0x040
typedef union {
       DWORD  dwReg;
       struct MskIntstsbitsTag{
         unsigned sdio_interrupt         :  16; //LSB
         unsigned int_status             :  16;
       }Bits; /* struct */
}MaskIntrStsReg;

//0x044
typedef union {
       DWORD  dwReg;
       struct RawintstsbitsTag{
         Intr_Status_Bits           int_status;//Lower 16 bits.0-15.
         unsigned sdio_interrupt         :  16;//bits 16-31
       }Bits; /* struct */
}RawIntrStsReg;
		
//0x048			
typedef union {
       DWORD  dwReg;
       struct STsbitsTag{
         unsigned fifo_rx_watermark      :  1; //LSB
         unsigned fifo_tx_watermark      :  1;
         unsigned fifo_half_empty        :  1;
         unsigned fifo_half_full         :  1;
         unsigned fifo_empty             :  1;
         unsigned fifo_full              :  1;
         unsigned Reserved1              :  1;
         unsigned data_busy              :  1;
         unsigned data_3_status          :  1;
         unsigned data_state_mc_busy     :  1;
         unsigned cmd_state_mc_busy      :  1;
         unsigned Response_Index         :  6;
         unsigned fifo_count             :  12;
         unsigned Reserved2              :  30;

       }Bits; /* struct */
}StatusReg;

//0x04C
typedef union {
       DWORD  dwReg;
       struct FifoThbitsTag{
         unsigned TX_WMark             :  12; //LSB
         unsigned Reserved1            :   4;
         unsigned RX_WMark             :  12;
         unsigned DW_DMA_Mutiple_Transaction_Size :  3;
         unsigned Reserved2            :  1;
       }Bits; /* struct */
}FIFOthreshold;			


//0X050		
typedef union {
       DWORD  dwReg;
       struct CdetBitsTag{
         unsigned card_detect_n        : 30; //LSB
         unsigned Reserved             :  2;
       }Bits; /* struct */
}CardDetectReg;


//0X054		
typedef union {
       DWORD  dwReg;
       struct WrProtbitsTag{
         unsigned write_protect        : 30; //LSB
         unsigned Reserved             :  2;
       }Bits; /* struct */
}WriteProtectReg;			
	
//0x058	
typedef union {
       DWORD  dwReg;
       struct GenIObitsTag{
         unsigned start_cmd            :  8; //LSB
         unsigned gpo                  : 16;
         unsigned gpi                  :  8;
       }Bits; /* struct */
}GeneralIOReg;

typedef union {
	      //To use this array,use BYTE_Offset/4.
          DWORD   IPRegArr[0x100/4];// Map array over all regs 
          struct  IPRegsTag{
			DWORD            CTRL;//0X00
			DWORD            PWREN;//0X04
			DWORD            CLKDIV;//0X08
			DWORD            CLKSRC;//0X0C
			DWORD            CLKENA;//0X010
			DWORD            TMOUT;//0X014
			DWORD            CTYPE;//0X018
			DWORD            BLKSIZ;//0X01C
			DWORD            BYTCNT;//0X020
			DWORD            INTMASK;//0X024
			DWORD            CMDARG;//0X028
			DWORD            CMD;//0X02C
			DWORD            RESP0;//0X030
			DWORD            RESP1;//0X034
			DWORD            RESP2;//0X038
			DWORD            RESP3;//0X03C
                        DWORD            MINTSTS;//0X040
			DWORD            RINTSTS;//0X044
			DWORD            STATUS;//0X048
			DWORD            FIFOTH;//0X04C
			DWORD            CDETECT;//0X050
			DWORD            WRTPRT;//0X054
			DWORD            GPIO;//0X058
			DWORD            TCBCNT;//0X05C
			DWORD            TBBCNT;//0X060
			DWORD            DEBNCE;//0X064
			DWORD            USRID;//0X068
			DWORD            VERID;//0X06C
			DWORD            Reserved;//0x06C-0x0ff. 
		  }Regs; /* struct */
} IPRegStruct, *IPRegStructure;


typedef struct COMMAND_INFO_Tag
{
  //CommandReg    CmdReg;
    DWORD         dwByteCntReg;
    DWORD         dwBlkSizeReg;
    DWORD         dwCmdArg;
    CommandReg    CmdReg;
}COMMAND_INFO;


typedef struct STATUS_INFO_Tag 
  {
    DWORD dwMaskIntrSts;
    DWORD dwRawIntrSts;
    DWORD dwStatus;
  }STATUS_INF;

typedef struct RESPONSE_INFO_Tag
{ 
  DWORD dwResp0;
  DWORD dwResp1;
  DWORD dwResp2;
  DWORD dwResp3;
}RESPONSE_INFO;


typedef struct TRANSFERRED_BYTE_INFO_Tag
{ 
  DWORD dwCIUByteCount;
  DWORD dwBIUFIFOByteCount;
}TRANSFERRED_BYTE_INFO;

#endif
