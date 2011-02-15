/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   sd_mmc_bus.h
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   registers.pdf

        Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the constants for bus driver functions.
	Bus driver is SD-MMC protocol specific driver.It 
	communicates with SD-MMC IP ,through Host Driver.


    REVISION HISTORY:

***************************************************************/

#ifndef _SDMMC_BUS_H_
#define _SDMMC_BUS_H_

//Required for  building on Linux

#include "../CommonFiles/sd_mmc_IP.h"


//Defines and constants:
/*20071016 modify the value from 0x07 to 0x06 */
#define  RESET_FIFO_DMA_CONTROLLER_Data  0x06 //bits0,1,2 all ones.
#define  DEFUALT_DEBNCE_VAL         0x0fffff
#define  DEFAULT_THRESHOLD_REGVAL   FIFO_DEPTH/2
#define  MAX_RESPONSE_TMOUT 0xff //For SD memory it is 64.For SDIO 1 sec. 

#define  INVALID                   -1
#define  MMC                        0
#define  SD                         1
#define  SD2                        2
#define  SD2_1                     3
//SDIO starts with 4 as for all subtypes,bit2 is 1
//and can be used to identify SDIO as a main card type.
#define  SDIO                       4
#define  SDIO_IO                    5
#define  SDIO_MEM                   6
#define  SDIO_COMBO                 7

#define CLKSRC0                      0
#define CLKSRC1                      1
#define CLKSRC2                      2
#define CLKSRC3                      3


#define CMD0_RETRY_COUNT            150
#define CMD1_RETRY_COUNT            500
#define CMD2_RETRY_COUNT            150
#define CMD3_RETRY_COUNT            150
#define CMD5_RETRY_COUNT            700
#define ACMD41_RETRY_COUNT          500
#define CMD8_RETRY_COUNT            150
#define SUSPEND_RETRY_COUNT         150

//Error bits in response R1,which are set during execution
//of data command. Bits 19,20,21,22,23,24,26,27,29,30,31.
#define R1_EXECUTION_ERR_BITS       0xEDF80000 

//This value is ANDed with response to isolate response timeout.
//Bit 8 in Raw intr status.
#define RESP_TIMEOUT       0x00000100 

#define DONOT_COMPARE_STATE 0x0ff
#define MAX_CLK_DIVIDER    0x0ff
#define PREDEF_TRANSFER  8 //Used in data transfer commands.

#define    KILO  1000
#define    MEGA  1000000

//SDIO current state constants.
#define  DIS_STATE       0
#define  CMD_STATE       1
#define  TRN_STATE       2

//Constants used for case staement switch.
#define General_cmd          -1
#define Blk_Transfer         10
#define Stream_Xfer          11
#define Wr_Protect_Data      12  
#define Rd_Protect_Data      13
#define Erase_Data           14
#define Card_Lock_Unlock     15
#define SDIO_Command         16
#define HSMMC_Command        17
#define HSSD_Command         18



//Constants used in interrupt handler.
//Data timeout,CRC,End bit error,FIFO overrun/underrun.
//(bit9,7,15,10,11)
#define DATA_OR_FIFO_ERROR   0x00008E80

//Defines used in New SDIO functions.
#define TPL_CODE_CISTPL_FUNCE 0x22 
#define NULL_TUPLE            0x00 

// 1ns,10,100,1uS,10,100,1mS,10.Exponent is negative.
static DWORD TAAC_EXP[]={1,10,100,1*KILO,10*KILO,100*KILO,1*MEGA,10*MEGA};

//Value is with exponent -1.
static const int TAAC_VAL[]={0,10,12,13,15,20,25,30,35,40,45,50,55,60,70,80};


//Structures used. :-

//HSSD
#ifdef HSSD_SUPPORT
  //HSSD : structure for the status received in response to CMD6.
  //512 bytes structure.

  typedef union HSSD_Switch_Status_info_Tag{
    BYTE bArr[512/8];
    struct{
            unsigned int Unused1             : 32;//bits 0 - 31
            unsigned int Unused2             : 32;
            unsigned int Unused3             : 32;
            unsigned int Unused4             : 32;
            unsigned int Unused5             : 32;
            unsigned int Unused6             : 32;
            unsigned int Unused7             : 32;
            unsigned int Unused8             : 32;
            unsigned int Unused9             : 32;
            unsigned int Unused10            : 32;
            unsigned int Unused11            : 32;
            unsigned int Unused12            : 24;//bits 352-375
            unsigned int Grp1_Func_Switch    :  4;//bits 376-379
            unsigned int Grp2_Func_Switch    :  4;//bits 380-383
            unsigned int Grp3_Func_Switch    :  4;//bits 384-387
            unsigned int Grp4_Func_Switch    :  4;//bits 388-391
            unsigned int Grp5_Func_Switch    :  4;//bits 392-395
            unsigned int Grp6_Func_Switch    :  4;//bits 395-399
            unsigned int Grp1_Func_Support   : 16;//bits 400-415
            unsigned int Grp2_Func_Support   : 16;//bits 416-431
            unsigned int Grp3_Func_Support   : 16;//bits 432-447
            unsigned int Grp4_Func_Support   : 16;//bits 448-463
            unsigned int Grp5_Func_Support   : 16;//bits 464-479
            unsigned int Grp6_Func_Support   : 16;//bits 480-495
            unsigned int Max_Curr_Consumption: 16;//bits 496-511
          }Bits;//End of structure.

    }HSSD_Switch_Status_Info;

#endif

//HSMMC 
#ifdef HSMMC_SUPPORT
  //HSMMC structure.
  //Index of this array,coresponding to the card number
  //will be stored in card_info_structure.For non_HSMMC 
  //cards,this index will be 0.

  static struct HSMMC_Card_info_Tag
  {
    BYTE  Supported_pwr_HV26mhz;
    BYTE  Supported_pwr_HV52mhz;
    BYTE  Supported_pwr_LV26mhz;
    BYTE  Supported_pwr_LV52mhz;
    union 
    {
      BYTE  Supported_Settings;
      struct 
      {
        unsigned  CARD_TYPE  : 2; //b0-b1 = HSMMC card type.
        unsigned  S_CMD_SET  : 3; //b2-b4 = Supported cmd sets.
        unsigned  Unused1     : 3; 
      }Bits; 
    }Supp_Settings_Info;
                               
    BYTE  Current_cmdset;    // 1 byte.
    union 
    {
      BYTE  Current_Settings;  
      struct 
      {
        unsigned  PowerClass : 4; //b0-b3
        unsigned  Unused2    : 1; //b4
        unsigned  HStiming   : 1; //b5
        unsigned  Buswidth   : 2; //b6-b7
      }Bits; 
    }Curr_Settings_Info;
  }HSMMC_EXTCSD_Info[MAX_HSMMC_CARDS];
#endif

typedef union SDIO_Function_Info_Tag{
	BYTE bArr[10];
	struct{
		unsigned int IO_Dev_Interface :  8;
		unsigned int CIS_Ptr          : 24;
		unsigned int CSA_Ptr          : 24;
		unsigned int CSA_Data_Access  :  8;
		unsigned int IO_Blk_Size      :  2;
	}Bits;//End of structure.
}SDIO_Function_Info;


typedef union mmc_csd {
	BYTE bArr[128/8];
	struct{
	u8  csd_structure;
	u8  spec_vers;
	u8  taac;
	u8  nsac;
	u8  tran_speed;
	u16 ccc;
	u8  read_bl_len;
	u8  read_bl_partial;
	u8  write_blk_misalign;
	u8  read_blk_misalign;
	u8  dsr_imp;
	unsigned int c_size;
	u8  vdd_r_curr_min;
	u8  vdd_r_curr_max;
	u8  vdd_w_curr_min;
	u8  vdd_w_curr_max;
	u8  c_size_mult;
	union {
		struct { /* MMC system specification version 3.1 */
			u8  erase_grp_size;  
			u8  erase_grp_mult; 
		} v31;
		struct { /* MMC system specification version 2.2 */
			u8  sector_size;   //sector_size
			u8  erase_grp_size;//erase_blk_en
		} v22;
	} erase;
	u8  wp_grp_size;
	u8  wp_grp_enable;
	u8  default_ecc;
	u8  r2w_factor;
	u8  write_bl_len;
	u8  write_bl_partial;
	u8  file_format_grp;
	u8  copy;
	u8  perm_write_protect;
	u8  tmp_write_protect;
	u8  file_format;
	u8  ecc;
	}Fields;//End of struct
} CSDreg;//End of union.

typedef struct mmc_cid_Tag {
	u8  mid;
	u16 oid;
	u8  pnm[7];   // Product name (we null-terminate)
	u8  prv;
	u32 psn;
	u8  mdt;
}CIDreg;

typedef struct CCCR_Tag {
    BYTE  CCCR_Revision;
    BYTE  Specs_Revision;
	BYTE  IO_Enable;
	BYTE  Int_Enable;
    BYTE  Bus_Interface;
    BYTE  Card_Capability;
    DWORD Common_CIS_ptr;
    BYTE  Power_Control;
}cccrreg;

typedef struct{
	DWORD dwUserID;
	DWORD dwVerID;
}id_reg_info;


typedef struct  {
	BYTE bResp[128/8];//4 DWORDs
}R2_RESPONSE;

typedef union  {
    DWORD dwReg;
	struct{
		unsigned int  Card_status  :16 ;//bits0-15
		unsigned int  Card_RCA     :16 ;//bits16-31
	}Bits;
}R6_RESPONSE;

typedef union  {
	DWORD dwStatusReg;
	struct 
	{
	  unsigned int Reserved1           :  2;//bits0,1
	  unsigned int Reserved2           :  1;//bit2
	  unsigned int AKE_SEQ_ERR         :  1;//bit3. Reserved for MMC.
	  unsigned int Reserved3		   :  1;//bit4
	  unsigned int APP_CMD             :  1;//bit5
	  unsigned int Unused              :  2;//bit6,7
	  unsigned int READY_FOR_DATA      :  1;//bit8
	  unsigned int CURRENT_SATTE       :  4;//bit9,10,11,12
	  unsigned int ERASE_RESET         :  1;//bit13
	  unsigned int CARD_ECC_DISABLED   :  1;//bit14
	  unsigned int WP_ERASE_SKIP       :  1;//bit15
	  unsigned int CID_CSD_OVERWRITE   :  1;//bit16
	  unsigned int OVERRUN             :  1;//bit17 Resvd for SD
	  unsigned int UNDERRUN            :  1;//bit18 Resvd.for SD
	  unsigned int ERROR               :  1;//bit19
	  unsigned int CC_ERROR            :  1;//bit20
	  unsigned int CARD_ECC_FAILED     :  1;//bit21
	  unsigned int ILLEGAL_COMMAND     :  1;//bit22
	  unsigned int COM_CRC_ERROR       :  1;//bit23
	  unsigned int LOCK_UNLOCK_FAILED  :  1;//bit24
	  unsigned int CARD_IS_LOCKED      :  1;//bit25
	  unsigned int WP_VIOLATION        :  1;//bit26
	  unsigned int ERASE_PARAM         :  1;//bit27
	  unsigned int ERASE_SEQ_ERROR     :  1;//bit28
	  unsigned int BLOCK_LEN_ERROR     :  1;//bit29
	  unsigned int ADDRESS_ERROR       :  1;//bit30
	  unsigned int OUT_OF_RANGE        :  1;//bit31
	}Bits;//End of struct.
}R1_RESPONSE;

typedef union  {
	DWORD dwReg;
	struct 
	{
	  unsigned int Rd_Wr_data          :  8;//bit0-7
	  unsigned int Out_of_range        :  1;//bit8.
	  unsigned int Function_Number	   :  1;//bit9
	  unsigned int Unused1             :  1;//bit10
	  unsigned int Error               :  1;//bit11
	  unsigned int IO_Current_state    :  2;//bits12,13
	  unsigned int Illegal_command     :  1;//bit14
	  unsigned int Com_crc_error       :  1;//bit15
	  unsigned int Unused2             :  16;//bits16-31
	}Bits;//End of struct.
}R5_RESPONSE;

typedef union  {
	DWORD dwReg;
	struct 
	{
	  unsigned int IO_OCR              : 24;//bit0-23
	  unsigned int Stuff_bits          :  3;//bit24-26
	  unsigned int MemPresent    	   :  1;//bit27
	  unsigned int NoOfFunctions       :  3;//bit28-30
	  unsigned int IO_Ready            :  1;//bit31  C bit
	}Bits;//End of struct.
}R4_RESPONSE;

static struct Card_info_Tag
{
  BYTE  HSdata   ;//bit7:For HS card type.1=High speed.eg HSMMC,HSSD.
                  //bit0-4: HSMMC_info array index.(0-31).
                  //
  BYTE  Card_type;//SD/SDIO/MMC type.Identified during
                  //enumeration only. 0=MMC, 1=SD, 2=SDIO
  //20071115 modify from unsigned long to long long type, in order to support 8G SD card!
  unsigned long long Card_size;//Calculated and stored during enumerate_card().
                  //in MBs.Important if any data is split
                  //between two cards.
  int   Card_RCA; //Allocated during enumeration.

  CSDreg CSD;
  CIDreg CID;

  //Following block lengths are current block sizes and are
  //updated,when SetBlockLength() is called.Maximum size is
  //given by write_bl_len & read_bl_len in CSD.
  // **** CSD values should not be changed. ****
  DWORD Card_Read_BlkSize;
  DWORD Card_Write_BlkSize;

  cccrreg CCCR;

  DWORD Card_OCR;

  BOOL  Card_Connected;//Refreshed whenever Card_Detect interrupt is 
                       //sensed.
  BOOL  Card_Insert;
  BOOL  Card_Eject;
  BOOL  Do_init;

  BYTE  no_of_io_funcs;
  BYTE  memory_yes;

  union 
  { 
    int  Card_Attributes;
	struct
	{
      unsigned int En_Partial_blk : 1 ;// 1 allowed.
      unsigned int Card_Locked    : 1 ;    
      unsigned int PWD_Set        : 1 ;    
	}Bits;

  }Card_Attrib_Info;//end of union.

  union 
  { 
    int  Current_card_status;
	struct
	{
      unsigned int Data_path_busy : 1 ;//Not used currently.
      unsigned int Cmd_path_Busy  : 1 ;//Not used currently.
	}Bits;

  }Card_status_info;//end of union.

  WORD  SDIO_Fn_Blksize[8];//Stores block size of 8 functions.
}Card_Info[MAX_CARDS];

typedef struct _card_capability
{
    u8 num_of_io_funcs;         /* Number of i/o functions */
    u8 memory_yes;              /* Memory present ? */
    u16 rca;                    /* Relative Card Address */
    u32 ocr;                    /* Operation Condition register */
    u16 fnblksz[8];
    u32 cisptr[8];
} card_capability;


//Info about current settings of IP.
typedef struct IP_Curent_Status_Info_Tag
{
  unsigned int Operating_Mode; // 1=MMC only,2=SD/MMC.Read only.Not settable.
  unsigned int Total_Cards;
  unsigned int Total_MMC_Cards;//|Incremented whenever enumerate_card finds
  unsigned int Total_SD_Cards ;//|this card type.
  unsigned int Total_SDIO_Cards ;//
  BOOL         Command_Path_Busy;// 1=busy.
  BOOL         Data_Path_Busy;// 1=busy.
  unsigned int Max_TAAC_Value;//|Both these values are in CSD.After enumeration
  unsigned int Max_NSAC_Value;//|of card stack,CSD for each card is read.        
                 //|From each CSD,maximum of NSAC and TAAC are 
				 //|found out and stored here.

  //unsigned int MaxOperatingFreq;//Maximum operating frequency.
                   //Calculated and stored after timeout values
                   //are calculated,during initialisation.
  DWORD        MiniDividerVal;//Minimum of the divider values of clk sources.
}IP_STATUS_INFO;

typedef struct CLK_SOURCE_INFO_tag 
{
  DWORD  dwDividerVal;//Contents of Clk divider Reg.@0x008
  DWORD  dwClkSource;//Contents of Clk sourcer Reg.@0x00C
}CLK_SOURCE_INFO;


//Card parameters as supplied by IOCTL.
typedef struct CMD_PARAMETERS_Tag 
{
  unsigned int CardNo;   
  DWORD        DataAddr;//Passed as cmd.Arg.
  DWORD        DataSize ;//
  BYTE         *pbDataBuff ;//Buffer where data is to be copied.
}CMD_PARAMETERS;

typedef struct CURRENT_CMD_INFO_tag
{
   BYTE *pbCurrDataBuff;
   BYTE *pbCurrCmdRespBuff;
   BYTE *pbCurrErrRespBuff;
   int   nCurrCmdOptions;
   DWORD dwCurrErrorSts;
   BOOL  fCurrCmdInprocess;// 1=in process, 0=not in process or over.
   BOOL  fCurrErrorSet;
   int   nResponsetype; // 1=long 0=short.
}CURRENT_CMD_INFO;


//Since,for retrying data command,it will be initiated by driver
//control need not be sent back to user.So,response buffer and 
//error response buffers should be local.Error status,error set,
//command over variables can be used from current command info.
typedef struct INPROCESS_DATA_CMD_Tag
{
   BYTE *pbCurrDataBuff;
   BOOL  fWrite;
   DWORD dwByteCount;
   DWORD dwRemainedBytes;
   int   nCurrCmdOptions;
   BOOL  fCmdInProcess;// 1=> data command is in process.
   void (*callback)(DWORD dwErrData);
   int  DataBuffType;
  DWORD dwDataErrSts;//stores data errors.
}INPROCESS_DATA_CMD;

typedef union
{
  DWORD dwReg;
  struct
  {
	  unsigned int WrData     :8 ;//bits0-7
	  unsigned int Unused2    :1 ;//Unused bit8
	  unsigned int RegAddress :17;//bits9-25
	  unsigned int Unused1    :1 ;//Unused bit26
	  unsigned int RAW_Flag   :1 ;//bit27
	  unsigned int FunctionNo :3 ;//bits30-28
	  unsigned int RW_Flag    :1 ;//bit31
  }Bits;//end of structure
}SDIO_RW_DIRECT_INFO;//end of union.

typedef union
{
  DWORD dwReg;
  struct
  {
	  unsigned int Byte_Count :9 ;//bits0-8
	  unsigned int RegAddress :17;//bits9-25
	  unsigned int Op_Code    :1 ;//bit26
	  unsigned int Blk_Mode   :1 ;//bit27
	  unsigned int FunctionNo :3 ;//bits30-28
	  unsigned int RW_Flag    :1 ;//bit31
  }Bits;//end of structure
}SDIO_RW_EXTENDED_INFO;//end of union.

#endif

