/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   Externfns.h
	Start Date :   24 Sep.2004
	Last Update:   24 Sep.2004

	Reference  :   

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the declarations of the external 
    functions used by the HSMMC.c and HSSD.c


    REVISION HISTORY:

***************************************************************/

#ifndef _EXTERNFNS_H_
#define _EXTERNFNS_H_

//Routines used from Bus.c
extern int   Form_Data_Command(int nCardNo, DWORD dwDataAddr,
                               DWORD dwDataSize, int nFlags,
                               COMMAND_INFO *ptCmdInfo,
                               int nTransferMode);

extern int  form_send_cmd(int ncardno,int nCmdID,unsigned long dwcmdarg,
                unsigned char *pbcmdrespbuff,int nflags);

extern int   Send_cmd_to_host(COMMAND_INFO *ptCmdInfo,
                              BYTE  *pdwCmdRespBuff,
                              BYTE  *pdwErrRespBuff,
                              BYTE  *pdwDataBuff,
                              void  (*callback)(DWORD dwErrData),
                              int   nFlags,
                              int   nNoOfRetries,
                              BOOL  fDataCmd,
                              BOOL  fWrite,
                              BOOL  fKernDataBuff);

extern int   Validate_Command (int nCardNo,DWORD dwAddress,DWORD dwParam2,
                               int nCommandType);

extern int   Validate_Response(int nCmdIndex, BYTE *pbResponse,
                               int nCardNo,int nCompareState);

extern int  SetBlockLength(int nCardNo,DWORD dwBlkLength, int nWrite,int nSkipPartialchk);

extern int  Read_Write_IPreg(int nRegOffset,DWORD *pdwRegData,BOOL fWrite);

extern DWORD Check_TranState(int nCardNo);

extern int   Read_ClkSource_Info(CLK_SOURCE_INFO *tpClkSource,
             int nCardNo);

extern int   Send_CMD55(int nCardNo);


#endif
