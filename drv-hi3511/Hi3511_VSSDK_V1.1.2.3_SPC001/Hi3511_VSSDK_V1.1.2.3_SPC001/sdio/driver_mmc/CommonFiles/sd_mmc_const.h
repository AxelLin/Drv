
/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   sd_mmc_const.h
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   registers.pdf

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the common constants.

    REVISION HISTORY:

***************************************************************/

#ifndef SD_MMC_CONST_H
#define SD_MMC_CONST_H

#define SDMMC_DRV_VER "1.2"

//#define DEBUG_DRIVER   
//#define HI_DEBUG
//These parameters moved here from HOST.h
#define EINVAL  22

typedef unsigned short WORD; 
typedef int BOOL;                       // f
typedef unsigned char BYTE;             // b
#define FALSE  0
#define TRUE   1
#define DWORD  unsigned long
//#define NULL   00                 //Linux:DEfined in /include/linux/stddef.h

//Other constants
#define MAX_RETRY_COUNT  1000 		//20071122 modify from 100 to 1000
#define MAX_REG_OFFSET   0x0FF

//Used for read/write commands/functions.
#define READFLG   FALSE    
#define WRITEFLG   TRUE    

//Used for UserFlags
#define HANDLE_DATA_ERRORS 0x10
#define SEND_ABORT         0x20  

//Error codes
//After removing compilation errors,try to merge these
//codes into standard codes.
#define SUCCESS  0
#define Err_Invalid_ClkSource  30
#define Err_Invalid_ClkDivider 31
#define Err_CmdInvalid_ForCardType 32
#define Err_CMD_Failed         33   
#define Err_Hardware_Locked    34  
#define Err_Retries_Over       35
#define Err_Not_Supported      36
#define Err_Invalid_RegOffset  37
#define Err_Invalid_ByteCount  38

#define Err_SDIO_Resp_COM_CRC  39
#define Err_SDIO_Resp_ILLEGAL_CMD  40 
#define Err_SDIO_Resp_UKNOWN       41
#define Err_SDIO_Resp_INVALID_FUNCTION_NO  42
#define Err_SDIO_Resp_OUT_OF_RANGE  43

#define Err_Enum_CMD0_Failed  44
#define Err_Enum_CMD1_Failed  45
#define Err_Enum_CMD2_Failed  46
#define Err_Enum_CMD3_Failed  47
#define Err_Enum_CMD2_RespTimeout  48


#define Err_Card_Unknown      49

#define Err_Card_OCR_Mismatched     50
#define Err_Enum_SendOCR_Failed     51
#define Err_Enum_ACMD41_Failed      52

#define Err_Invalid_ThresholdVal    53

#define Err_Invalid_CardNo          54

#define Err_Card_Not_Connected      55
#define Err_CmdNotAllowed_ForCard   56
#define Err_ArrSize_Mismatch        57

#define Err_MultiBlk_Not_Allowed    58
#define Err_Not_BlkAlign            59
#define Err_Suspend_Not_Allowed     60   
#define Err_Function_Mismatch       61 
#define Err_Suspend_Not_Granted     62
#define Err_Card_Busy               63
#define Err_NoData_FromCard         64

#define Err_IntrMask_Not_Allowed    65
#define Err_Command_not_Accepted    66
#define Err_PWDLEN_Exceeds          67
#define Err_CMD_Not_Supported       68
#define Err_Addr_OutOfRange         69
#define Err_Partial_Blk_Not_Allowed 70

#define Err_Invalid_Command         71
#define Err_Blocksize_Mismatch      72
#define Err_WrProt_Not_Supported    73

#define Err_Invalid_FunctionNo      74

#define Err_Invalid_Cardstate       75
#define Err_Enum_Failed             76
#define Err_NoCard_Present          77

#define Err_State_Mismatch          78
#define Err_SetBlkLength_Exceeds    79
#define Err_PartialBlk_Not_Allowed  80
#define Err_CMD16_Failed            81
#define Err_CMD55_Failed            82

#define Err_Blk_Misaligned          83

#define Err_ClkFreqSet_Failed       84
#define Err_ClkEnable_Failed        85

#define Err_partial_write_failed    86
#define Err_partial_read_failed     87

#define Err_Invalid_CardType        88
#define Err_DataCmd_InProcess       89

#define Err_DataBus_Free            90
#define Err_CardTransfer_Suspended  91

#define Err_CMD40_InProcess         92

#define Err_HSMMC_CARDCOUNTOVER     95
#define Err_Invalid_Arg             96
#define Err_Invalid_CardWidth       97

#define Err_BYTECOPY_FAILED         98

#define Err_HSSD_CARDCOUNTOVER      99
#define Err_HSSD_Current_Exceeds   100
#define Err_HSSD_CMD6Status        101

#define Err_TmpWrProt_Failed       102

#define Err_Function_NotReady      103

#define Err_SDIOPwrMode_NotSupported   104
#define Err_SDIOPwrSelect_NotSupported 105
#define Err_SDIOTuple_NotFound         106
#define Err_SDIOTuple_AddrExceeds      107
#define Err_SDIOblksize_Exceeds        108
#define Err_Invalid_CISPtr             109

#define Err_AfterExecution_check_Failed 110

//MMC errors from 100 onwards.Refer Utilityfs.h
//Warning errors.
#define Warning_DataTMout_Changed   150

//Case statement constants

#define SET_RECV_FIFO_THRESHOLD       1
#define SET_TRANS_FIFO_THRESHOLD      2
#define SET_DATA_TIMEOUT              3
#define SET_RESPONSE_TIMEOUT          4
#define SET_SDWIDTH_FOURBIT           5
#define SET_SDWIDTH_ONEBIT            6
#define SET_INTR_MASK                 7
#define CLEAR_INTR_MASK               8
#define FIFO_access                   9
#define POWER_ON                     10
#define POWER_OFF                    11
#define ENABLE_OD_PULL_UP            12
#define SET_CONTROLREG_BIT           13 
#define CLEAR_CONTROLREG_BIT         14
#define RESET_FIFO                   15
#define RESET_DMA                    16
#define RESET_CONTROLLER             17
#define RESET_FIFO_DMA_CONTROLLER    18


//DeviceIO call constants:
#define   GENERAL_ONE                  0x06b01
#define   GENERAL_TWO                  0x06b02

#define   INITIALISE                   0x06b10
#define   SEND_RAW_COMMAND             0x06b11
#define   RD_WR_BLK_DATA               0x06b12
#define   RD_WR_BLK_DATA1               0x06b70
#define   RD_WR_STREAM_DATA            0x06b13
#define   SDIO_RD_WR_EXTENDED          0x06b14
#define   SDIO_RD_WR_DIRECT            0x06b15
#define   SDIO_RESUME_TRANSFER         0x06b16
#define   WR_PROTECT_CARD              0x06b17
#define   SET_CLR_WR_PROTECT           0x06b18
#define   GET_WR_PROTECT_STATUS        0x06b19
#define   ERASE_CARD_DATA              0x06b20
#define   READ_WRITE_IP_REG            0x06b21
#define   STOP_TRANSFER                0x06b22
#define   CARD_LOCK                    0x06b23
#define   SET_BLK_LENGTH               0x06b24
#define   SET_SD_4BIT_MODE             0x06b25
#define   SET_SD_ONEBIT_MODE           0x06b26
#define   GENERAL_TESTING              0x06b27
#define   SEND_COMMAND                 0x06b28
#define   READ_CARD_STATUS             0x06b29
#define   ENABLE_DISABLE_SDIOFUNCTION  0x06b30
#define   ENUMERATE_SINGLE_CARD        0X06b31
#define   ENABLE_DISABLE_SDIOIRQ       0x06b32
#define   SDIO_ABORT_TRANSFER          0x06b33
#define   SDIO_SUSPEND_TRANSFER        0x06b34
#define   SDIO_READ_WAIT               0x06b35
#define   SET_RESET_INTR_MODE          0x06b36
#define   HSSD_SEND_CMD6               0x06b37
#define   HSSD_FUNCTION_SWITCH         0x06b38
#define   CHECK_HSSD_TYPE              0x06b39
#define   READ_SD_REG                  0x06b40
#define   HSSD_CHECKFN_SUPPORT         0x06b41
#define   CHANGE_SDIO_BLKSIZE          0x06b42
#define   GET_DATA_FROM_TUPLE          0x06b43
#define   ENABLE_MSTRPWR_CONTROL       0x06b44
#define   CHANGE_FN_PWRMODE            0x06b45
#define   SET_HSMODE_SETTINGS          0x06b46
#define   SET_CLEAR_EXTCSDBITS         0x06b47
#define   BUS_TESTING                  0x06b48
#define   RD_WR_BLK_DATA_USER_BUF      0x06b49
//y00107738 add for getting card read only status
#define   HDIO_GETRO		0x0330

typedef struct DATA_CMD_INFO_Tag
{
    DWORD dwCmdRegParams;//Only for RAW data command.
    DWORD dwCmdArgReg ;  //Only for RAW data command.

    int   nCardNo;       //
    DWORD dwAddr;        //

    DWORD dwByteCount;   //Data size.
    DWORD dwBlockSizeReg;//Only for RAW data command.
    BYTE  *pbCmdRespBuff;
    BYTE  *pbErrRespBuff;
    BYTE  *pbDataBuff;
    void (*callback)(DWORD dwErrData);
    int   nNoOfRetries;
    int   nFlags;
}DATA_CMD_INFO;

typedef struct SDIO_CMD_INFO_Tag
{
    int   nCardNo;       //
    int   nFunctionNo;   //
    int   nRegAddr;      //
    BYTE  *pbCmdRespBuff;
    BYTE  *pbDataBuff;
    DWORD dwByteCount;   //Data size. SDIO_Extended only
    BYTE  *pbErrRespBuff;//SDIO_Extended only
    int   nCmdOptions;//SDIO_Extended only
    void  (*callback)(DWORD dwErrData);//SDIO_Extended only
    int   nNoOfRetries;//SDIO_Extended only
    BOOL  fRAW;//SDIO_Direct only
    int   nFlags;
}SDIO_CMD_INFO;


typedef struct CARD_PROTECT_ERASE_CMD_Tag
{
    int   nCardNo;       
    DWORD dwAddr;        
    BYTE  *pbCmdRespBuff;
    DWORD dwDataParam1;//To be used if any data exchange required
                       //between application and driver.
    DWORD dwDataParam2;//To be used if any data exchange required
                       //between application and driver.
    BOOL  fflag1;//General flag1.
    BOOL  fSet;//Set or clear.
}CARD_PROTECT_ERASE_CMD;

typedef struct RD_WR_IPREG_CMD_Tag
{
  int   nOffset;
  DWORD *pdwData;
  BOOL  fWrite;
}RD_WR_IPREG_CMD;

/* This structure moved here from  sd_mmc_bus.h as it is required 
   for application.*/
typedef struct card_lock_info_tag
{
 BYTE bCommandMode;// 0 =Unlock card     ; 1=Set_Pwd; 
                   // 2 =CLR_PWD         ; 4=Lock card;
                   // 5 =Lock and set_pwd; 8=Erase card. ;
 BYTE  bOldPwdLength;
 BYTE  *pbOldPassword;
 BYTE  bNewPwdLength;
 BYTE  *pbNewPassword;
}card_lock_info;


typedef struct APPLICATION_card_lock_info_Tag
{
    int   nCardNo;  
    card_lock_info *ptCardLockInfo;
    BYTE  *pbCmdRespBuff;
}APPLICATION_CARD_LOCK_CMD;

typedef struct CARD_GEN_CMD_Tag
{
    int   nCardNo;       
    DWORD dwParam1;
    DWORD dwParam2;
    DWORD dwParam3;
    int   nFlag;  
    BYTE  *Data;      
    void  (*Gen_callback)(DWORD dwParam1,DWORD dwParam2);
}CARD_GEN_CMD;

#endif
