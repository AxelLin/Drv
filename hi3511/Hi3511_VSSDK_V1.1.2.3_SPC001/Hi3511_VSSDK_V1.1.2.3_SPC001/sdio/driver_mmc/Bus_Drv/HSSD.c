/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   HSSD.c
	Start Date :   7th July,2004.
	Last Update:   

	Reference  :   Specdraft110e.pdf

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the routines for HS SD interface.

    REVISION HISTORY:

***************************************************************/

#include "Externfns.h"

// HSSD specific constants and defines.:
#define ACMD51_DATA_SIZE   8  //in bytes.
#define ACMD13_DATA_SIZE   64 //in bytes.
#define HSSD_MAX_CURRENT   200//Actual value in mA.
#define HSSD_CMD6_DATAWIDTH  64 //512 bits/8 = 64 bytes

//extern void __iomem* map_adr;
extern unsigned int map_adr;

static DWORD  G_CURR_HSSD_CARDCOUNT =0;

//FUNCTION PROTOTYPES.
DWORD HSSD_Send_CMD6(int nCardNo,BYTE* DataBuff,DWORD dwFunctions,
      BOOL fMode);
DWORD HSSD_Function_Switch(int nCardNo,DWORD dwFunctionNos,
      BYTE *pbErrGroup,BOOL fBuffMode);
void  Wait_for_CardClkCycles(unsigned int nCardNo,unsigned int nCycles);
DWORD Validate_CMD6_Switch(HSSD_Switch_Status_Info *pStatusData, 
      BYTE *pbGroupErrSts,BOOL fBuffMode);
DWORD Check_HSSD_Type (int nCardNo, unsigned char *pcCardType);
DWORD Read_SD_Reg(int nCardNo, int nCmdIndex, BYTE *pbDataBuff,
      int nBuffSize);
void  ChangeEndian (BYTE *pData, int nBytes);
DWORD HSSD_CheckFn_Support(int nCardNo,DWORD dwFunctionNos,
      BYTE *pbStatusData, int nBuffSize,BOOL fBuffMode);
void  HSSD_Initialise_Handler(int nCardNo);

//=============== Actual implementation starts ======================

//HSSD routines.

/*.DWORD HSSD_Send_CMD6(int nCardNo,BYTE* DataBuff,DWORD       
                         dwFunctions, BOOL fMode)

  *Description : Send CMD6 for HSSD card. Internal function.Caller 
   should validate the card number and card type.
 
  *Called from: HSSD_Function_Switch.

  *Parameters:
   nCardNo    :(i/p):Card number.
   DataBuff   :(o/p):Buffer to receive the 64 bytes (512 bits) data
                     received as a status, in response to CMD6.
   dwFunctions:(i/p):Commnand argument for CMD6.4 bits per function 
                     group. 
   fMode      :(i/p):CMD6 mode. 1=Switch mode. 0=Check mode.
 
  *Return Value: 0 for SUCCESS.
   Err_Invalid_Cardstate
   Err_Invalid_CardNo
   Err_Card_Not_Connected
   Err_SetBlkLength_Exceeds
   Err_CMD16_Failed
   Errors in command execution.
   Errors from R1 response.

  *Flow:
      - Check if is in transfer state else issue CMD7. Use
        Check_TranState (..) for this.
      - //CMD6 is data command with block of 512 bytes.
        If(current block size != 64 bytes)
          change read block size to 64 bytes.
      - Form data command for CMD6. 
        CmdArg = Function numbers | (nCmdFlags & 1) //bit 0 for CMD6 
        mode..
        Bytecount = 64
        Blocksize = 64
        Read Block transfer with wait_for_prev_data flag=1.
      - Send CMD6                
      - Validate response for errors.
      - Retrieve the original block length,if it has been changed.
      - Return SUCCESS.

*/
DWORD HSSD_Send_CMD6(int nCardNo, BYTE* DataBuff,DWORD dwFunctions,
                     BOOL fMode)
{
  BYTE CmdRespBuff[4];
  COMMAND_INFO  tCmdInfo={0x00};
  DWORD dwRetVal=0,dwOrgBlkLength,dwArg=0;

  //CMD6 is valid in transfer state only.Check card status.
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(nCardNo))!=SUCCESS)  
    return dwRetVal;

  //Data received will a block of 512/8 =64 bytes.So,set block length.
  dwOrgBlkLength = Card_Info[nCardNo].Card_Read_BlkSize;

  //Card will send the status as a block of 512 bits.But,it is 
  //not partial read.
  if(dwOrgBlkLength != HSSD_CMD6_DATAWIDTH)
     dwRetVal= SetBlockLength(nCardNo,HSSD_CMD6_DATAWIDTH,READFLG,1);

  if(dwRetVal!=SUCCESS) return dwRetVal;

  //Form data command. read,bytecount=512,blockmode,
  dwArg = dwFunctions & 0xffffff;
  if(fMode)
    dwArg |= 0x80000000;
  Form_Data_Command(nCardNo,dwArg,HSSD_CMD6_DATAWIDTH,0x2,&tCmdInfo,00);
  tCmdInfo.CmdReg.Bits.cmd_index = CMD6;  

  //Send CMD6.
  dwRetVal = Send_cmd_to_host(&tCmdInfo,CmdRespBuff,
                              NULL,//No error buffer. 
                              DataBuff,
                              NULL,//No callback pointer
                              0x2, //wait for prev.data=1.
                              0,   //no retries.
                              TRUE,//Data command
                              FALSE,//Read 
                              TRUE);//Kernel mode buffer.

  if(dwRetVal==SUCCESS)
     dwRetVal = Validate_Response(CMD6,CmdRespBuff,nCardNo,CARD_STATE_TRAN);

  //If original block length is 512,donot set block length.
  if(dwOrgBlkLength!=HSSD_CMD6_DATAWIDTH)
     SetBlockLength(nCardNo,dwOrgBlkLength,READFLG,1);

  return dwRetVal;        
   
}


/*3.DWORD HSSD_Function_Switch(int nCardNo,DWORD dwFunctionNos,
                               BYTE *pbErrGroup)
  *Description : Switches the HS-SD memory card functions,using CMD6.
   For the user supplied function numbers, support in card & current
   change are checked.

  *Called from: Application.

  *Parameters:
   nCardNo      :(i/p):Card number.
   dwFunctionNos:(i/p):Commnand argument for CMD6.
                       4 bits per function group. eg 
                       bits  0-3  =function no.for group1.
                       bits  4-7  =function no.for group2.
                       bits  8-11 =function no.for group3.
                       bits 12-15 =function no.for group4.
                       bits 16-19 =function no.for group5.
                       bits 20-23 =function no.for group6.

   pbErrGroup   :(o/p):Pointer to a byte. If error occures in the 
                       function switch or check,this byte reflects 
                       the status for each group. One bit / group has
                       been allocated.  0=No error. 1=Error occured.
                       bit 0 = group1     bit 1 = group2
                       bit 2 = group3     bit 3 = group4
                       bit 4 = group5     bit 5 = group6
                       bit 6 = Unused     bit 7 = Unused.

  fBuffMode     :(i/p):Buffer mode. 1 = kernel mode buffer.  
                       0 = user mode buffer.

  *Return Value: 0 for SUCCESS.

  *Flow:
      - Validate the pbErrGroup.
      - Validate card type (HS-SD) and card number.
      - Check if user supplied function numbers are supported by card.
        Using HSSD_Send_CMD6(..), send CMD6 in mode0.
        If this command fails, return error.
      - Validate the status response to check any error.(Bits 376-399 
        & 496-511).Use Validate_CMD6_Switch(..) for this.
      - if(changed current consumption > HSSD_MAX_CURRENT) 
          return Err_HSSD_Current_Exceeds;
      - Switch the functions. Using HSSD_Send_CMD6(..), send CMD6 in 
        mode1.If this command fails, return error.
      - Validate the status response to check any error.(Bits 376-399
        & 496-511).Use Validate_CMD6_Switch(..) for this.
      - Wait for 8 cycles of card clock. (Next command after switch 
        command is accepted after 8 clocks of card clock).
      - Return SUCCESS.
*/
//Switches the function. Check support & current first.
//If error, returns.Caller of this function should have checked 
//that functions to be switched are supported or else,
//if not supported, this function will return error.
DWORD HSSD_Function_Switch(int nCardNo,DWORD dwFunctionNos,
                           BYTE *pbErrGroup,BOOL fBuffMode)
{
  HSSD_Switch_Status_Info  CMD6_Data_Struct;
  DWORD dwRetVal;

  //Clear structure.
  memset(&CMD6_Data_Struct,0,sizeof(HSSD_Switch_Status_Info));

  if(pbErrGroup==NULL) return -EINVAL;

  //Validate command. ie if card is HSSD,card number.
  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(nCardNo,00,00,HSSD_Command) )
      != SUCCESS)  return dwRetVal;

  //Send CMD6 with check mode & with functions to be switched.
  if((dwRetVal=HSSD_Send_CMD6(nCardNo,CMD6_Data_Struct.bArr,
      dwFunctionNos,0)) != SUCCESS) return dwRetVal;
  
  //Check if there is any error set in bits 376-399 & 496-511.
  //Checks for error in function no.argument and current consumption.
  if((dwRetVal=Validate_CMD6_Switch(&CMD6_Data_Struct,pbErrGroup,
      fBuffMode)) !=SUCCESS) return dwRetVal;

  //Check if changed current consumption is within limits.
  if(CMD6_Data_Struct.Bits.Max_Curr_Consumption > HSSD_MAX_CURRENT) 
    return Err_HSSD_Current_Exceeds;

  //Changed current consumption is acceptable.Send CMD6 in switch mode.
  memset(&CMD6_Data_Struct,0,sizeof(HSSD_Switch_Status_Info));

  if((dwRetVal=HSSD_Send_CMD6(nCardNo,CMD6_Data_Struct.bArr,
      dwFunctionNos,1)) != SUCCESS) return dwRetVal;

  //Before returning, wait atleast for 8 cycles of card clock.
  //Card clock = cclk_in/ cclk_div.So, wait for cclk_div * 8 cycles.
  Wait_for_CardClkCycles (nCardNo,10);
 
  dwRetVal=Validate_CMD6_Switch(&CMD6_Data_Struct,pbErrGroup,fBuffMode);
 
  return dwRetVal;
}

void Wait_for_CardClkCycles(unsigned int nCardNo,unsigned int nCycles)
{

  CLK_SOURCE_INFO    tClkSource = {0};
  DWORD dwWait =0,i=0;
  DWORD dwPROCclk=0, dwCIUclk=0;
  int nRemainder=0;

  //Read CLK_DIV value for clock source assigned for given card no.
  Read_ClkSource_Info(&tClkSource,nCardNo);

  //Convert both clocks in MHz.Defined in KHz,in h file.
  dwPROCclk = PROCESSOR_CLK / 0x3E8; 
  dwCIUclk  = CIU_CLK / 0x3E8;

  //For Card_clock = CIU_CLK/ clkdiv,  nCycles,
  //then, for PROCESSOR_CLK, 
  //      cycles = PROCESSOR_CLK * nCycles / Card_clock  
  //One cycle is required to execute this for loop.
  //This loop may wait for more than given cycles,but assures to wait 
  //ATLEAST for given cycles.
  dwWait = (dwPROCclk * tClkSource.dwDividerVal * nCycles); 

  if(((dwWait * 100)/ dwCIUclk) % 100)
   nRemainder = 1;

  dwWait = ((dwWait * 100)/ dwCIUclk) / 100;

  if(dwWait==0 || nRemainder)
    dwWait++;
  
  for(i=0; i< dwWait;i++);

  return;
}

/* Validate_CMD6_Switch:Validates status data received in response 
   to CMD6.If error is found in the status,then error data is copied
   in the pbGroupErrSts and function returns error.

   DWORD Validate_CMD6_Switch(HSSD_Switch_Status_Info *pStatusData, 
                              BYTE *pbGroupErrSts,BOOL fBuffMode)

  *Called from: HSSD_Function_Switch

  *Parameters:
   pStatusData  :(i/p):Pointer to structure HSSD_Switch_Status_Info.
                       It holds the status data received for CMD6.
   pbGroupErrSts:(o/p):Byte pointer to return group error status.
                       One bit per group. 0=no error, 1=error.
   fBuffMode    :(i/p):Buffer mode. 1 = kernel mode buffer.  
                       0 = user mode buffer.

  *Return Value: 
   0 for SUCCESS.
   Err_HSSD_CMD6Status

  *Flow:
*/

DWORD  Validate_CMD6_Switch(HSSD_Switch_Status_Info *pStatusData, 
                     BYTE *pbGroupErrSts,BOOL fBuffMode)
{
  unsigned char bData=0;
  int i;
  BYTE bErrorData=0;
  DWORD dwRetVal=SUCCESS;

  //eg Byte 48 ie index 47, gives status for grp1 & 2.(bits 376-383).
  for(i=0;i<3;i++)
  {
    bData = pStatusData->bArr[47+i];

    if( (bData & 0xF) == 0xF) 
       bErrorData |= (1<<(i*2));//bits 0,2,4.

    if( ((bData & 0xF0)>>0x4) == 0xF) 
       bErrorData |= (1<<((i*2)+1));//bits 1,3,5.

  }//end of for loop.

  //Store error data.
  if(fBuffMode)
    //Data buffer is allocated in kernel.
    *pbGroupErrSts = bErrorData; 
  else
  {
    if (put_user(bErrorData,pbGroupErrSts))
	      dwRetVal = Err_BYTECOPY_FAILED;
  }
  //Send Err_BYTECOPY_FAILED first.
  if(bErrorData!=0 && dwRetVal!=Err_BYTECOPY_FAILED)
   dwRetVal = Err_HSSD_CMD6Status;

  return dwRetVal;
}


//=====================================================================
/* DWORD  Check_HSSD_Type (int nCardNo, unsigned char *pcCardType)
  *Description : This is an internal function. Reads the SCR of SD 
   card and returns the card type in pcCardType. This field will hold
   the valid data if the return value of the function is SUCCESS.
 
  *Parameters :
   nCardNo   (i/p): Card number
   pcCardType(o/p): Pointer to variable. 1=HS-SD card. 0=Non HS-SD.
                                    
  *Return Value : 0 for Success, Error if command failed.
 
  *Flow : 
      - Validate pointer ie pcCardType.
      - Allocate data buffer of size 8 bytes .
      - Read SCR (width=64 bits) using ACMD51. Use Read_SD_Reg(..).
      - Check SD_SPEC field.
        If SD_SPEC=1, (*pcCardType)=1, else (*pcCardType) =0.
      - Return.
*/
DWORD  Check_HSSD_Type (int nCardNo, unsigned char *pcCardType)
{
  unsigned char arrSCRdata [8];//To store SCR data.
  DWORD dwRetVal=0xFF;
  //unsigned int FIFO_value;
//  printk("Entry Check_HSSD_Type!\n");
  //Validate pointer ie pcCardType.
  if(pcCardType==NULL)
    return Err_Invalid_Arg;

  //Read SCR (width=64 bits) using ACMD51. Use Read_SD_Reg(..).
  memset(arrSCRdata,00,8);
//  printk("Check_HSSD_Type : memset ok!\n");  

  if((dwRetVal = Read_SD_Reg(nCardNo,ACMD51,arrSCRdata,8)) !=SUCCESS)
    return  dwRetVal;             
//  printk("Check_HSSD_Type : Read_SD_Reg ok!\n");  

  //Check SD_SPEC field.
// printk("arrSCRdata=%x %x %x %x %x %x %x %x\n",arrSCRdata[0],arrSCRdata[1],arrSCRdata[2],arrSCRdata[3],arrSCRdata[4],arrSCRdata[5],arrSCRdata[6],arrSCRdata[7]);
  *pcCardType = ((arrSCRdata[7] & 0xF) ==1) ? 1 : 0;

  return SUCCESS;
} 

/* DWORD Read_SD_Reg(int nCardNo, int nCmdIndex, Byte *pbDataBuff,
                      int nBuffSize)
  *Description : This is an internal function. Caller function should
   confirm that card is connected and allocate the data buffer of 
   size 8 bytes. SCR data will be stored in the data buffer.
   Please note that for wide data width ie register access, MSByte is 
   sent irst  while  for normal data (8 bit width, memory location), 
   LSByte  is sent first.This function will change the bytes from Big
   Endian to little endian.
 
  *Parameters :
   nCardNo   (i/p):Card number
   nCmdIndex (i/p):Command index. Valid are ACMD51, ACMD13.
   pbDataBuff(o/p):Pointer to data buffer.Buffer size should be >= 8 
                   bytes
   nBuffSize (i/p):Buffer size.(8 bytes for ACMD51,64 bytes for 
                   ACMD13).
 
  *Return Value : 0 for Success, Error if command failed.
 
  *Flow : 
      - Validate data buffer pointer. Otherwise crash may occur.
      - Validate buffer size for given command index.
      - Read current block length set for the card, from the 
        card_info structure.
      - If current Rd_Blk_Length != nBuffSize, set the block length 
        by calling SetBlockLength(..).  
      - Form the data command. Use Wait_for_prev_data_complete flag.
      - Send the data command with command index nCmdIndex.
      - Validate the response.
      - Change the byte order from Big endian to Little endian.
      - Restore the original block length.
      - Return.
*/
DWORD Read_SD_Reg(int nCardNo, int nCmdIndex, BYTE *pbDataBuff,
                  int nBuffSize)
{
  BYTE CmdRespBuff[4];
  COMMAND_INFO  tCmdInfo={0x00};
  DWORD dwRetVal=0;
  int nOrgBlkLength,nRequdBytes=0;

  //Validate data buffer pointer. Otherwise crash may occur.
  if(pbDataBuff==NULL)
    return -EINVAL;

  //Validate buffer size for given command index.
  switch(nCmdIndex)
  {
    case ACMD51:
      nRequdBytes = ACMD51_DATA_SIZE;
      break;

    case ACMD13:
      nRequdBytes = ACMD13_DATA_SIZE;
      break;
    default:
      return Err_Invalid_Arg;
  }//end of switch.

  if(nBuffSize < nRequdBytes) return Err_Invalid_Arg;

  //Check current block length.
  nOrgBlkLength = Card_Info[nCardNo].Card_Read_BlkSize;

  //Register reading is not partial block reading.
  if(nOrgBlkLength != nRequdBytes)
     dwRetVal= SetBlockLength(nCardNo,nRequdBytes,READFLG,1);

  if(dwRetVal!=SUCCESS) return dwRetVal;

  //Form the data command. Argument is stuff bits.
  Form_Data_Command(nCardNo,0,nRequdBytes,0x2,&tCmdInfo,00);
  tCmdInfo.CmdReg.Bits.cmd_index = nCmdIndex;  

  //Send ACMD.
  //Send CMD55 first.
  if((dwRetVal = Send_CMD55(nCardNo)) != SUCCESS )
     return Err_CMD55_Failed;

  dwRetVal = Send_cmd_to_host(&tCmdInfo,CmdRespBuff,
                             NULL,//No error buffer. 
                             pbDataBuff,
   	                         NULL,//No callback pointer
                             0x2, //wait for prev.data=1.
                             0,   //no retries.
 	                         TRUE,//Data command
                             FALSE,//Read 
                             TRUE);//Kernel mode buffer.

  //Validate the response.
  if(dwRetVal==SUCCESS)
  {
    dwRetVal= Validate_Response(nCmdIndex,CmdRespBuff,nCardNo,CARD_STATE_TRAN);

    //Change the byte order from Big endian to Little endian.
    ChangeEndian (pbDataBuff, nRequdBytes);
  }

  //If original block length is 512,donot set block length.
  if(nOrgBlkLength!=nRequdBytes)
     SetBlockLength(nCardNo,nOrgBlkLength,READFLG,1);

  return dwRetVal;        
}

//Changes the order of nBytes in pData order.
//This changes the endianness.
void ChangeEndian (BYTE *pData, int nBytes)
{

  unsigned char ch1;
  int i=0;

  for (i=0;i<(nBytes/2);i++)
  {
    ch1 = pData[i];
     
    pData[i] = pData[nBytes-1-i];
    pData[nBytes-1-i] = ch1;
  }//end of for loop.

}  

/* DWORD HSSD_CheckFn_Support(int nCardNo,DWORD dwFunctionNos,
                               BYTE *pbStatusData, int nBuffSize,
                               BOOL fBuffMode)
  *Description : Sends the CMD6 in mode0 and returns the switch 
   status data in the buffer  supplied by user. Using this user can 
   check which functions are supported by the card and current 
   consumption with selected functions. By specifying the function 
   numbers in dwFunctionNos ,user can check  changed current 
   consumption.To keep the functions unchaged,user should pass 0xF in 
   this.

   Size of pbStatusData should be 64 bytes minimum as status data for
   CMD6 is of  512 bits.

  *Called from: Application.

  *Parameters:
   nCardNo      :(i/p):Card number.
   dwFunctionNos:(i/p):Commnand argument for CMD6.
                       4 bits per function group. eg 
                       bits  0-3  =function no.for group1.
                       bits  4-7  =function no.for group2.
                       bits  8-11 =function no.for group3.
                       bits 12-15 =function no.for group4.
                       bits 16-19 =function no.for group5.
                       bits 20-23 =function no.for group6.

   PbStatusData :(o/p):Pointer to a buffer to store the status data 
                       received.
   nBuffSize    :(i/p):Buffer size in bytes.Used only to validate 
                       buffer.Not used in function.
   fBuffMode    :(i/p):Buffer mode. 1 = kernel mode buffer.  
                       0 = user mode buffer.

  *Return Value: 0 for SUCCESS.

  *Flow:
      - Validate the PbStatusData.
        If(nBuffSize < 64 ) return error.
      - Validate card type (HS-SD) and card number.
      - Send CMD6 with mode0 and with function numbers as supplied by
        user in dwFunctionNos.Use HSSD_Send_CMD6(..) for this.
        If this command fails, return error.
      - Validate the status response to check any error.(Bits 376-399 
        & 496-511).Use Validate_CMD6_Switch(..) for this.
      - Return SUCCESS.
*/
DWORD HSSD_CheckFn_Support(int nCardNo,DWORD dwFunctionNos,
                           BYTE *pbStatusData, int nBuffSize,
                           BOOL fBuffMode)
{
  HSSD_Switch_Status_Info  CMD6_Data_Struct;
  DWORD dwRetVal;
  BYTE  bErrGroup=0;

  //Clear structure.
  memset(&CMD6_Data_Struct,0,sizeof(HSSD_Switch_Status_Info));

  //Validate the PbStatusData & Buffersize.
  if(pbStatusData==NULL) return -EINVAL;
  if(nBuffSize < HSSD_CMD6_DATAWIDTH ) return Err_Invalid_Arg;

  //Validate command. ie if card is HSSD,card number.
  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(nCardNo,00,00,HSSD_Command) )
      != SUCCESS)  return dwRetVal;

  //Send CMD6 with check mode & with functions to be switched.
  if((dwRetVal=HSSD_Send_CMD6(nCardNo,CMD6_Data_Struct.bArr,
      dwFunctionNos,0)) != SUCCESS) return dwRetVal;

  //Check if there is any error set in bits 376-399 & 496-511.
  //If yes, return error AFTER copying the status in statusbuffer.
  dwRetVal=Validate_CMD6_Switch(&CMD6_Data_Struct,&bErrGroup,TRUE);

  //HSSD_Send_CMD6 requires kernel mode buffer.
  //So,copy the status data in user supplied buffer.
  if(fBuffMode)
    //Data buffer is allocated in kernel.
    memcpy(pbStatusData,CMD6_Data_Struct.bArr, HSSD_CMD6_DATAWIDTH); 
  else
  {
    if(copy_from_user(pbStatusData,CMD6_Data_Struct.bArr,
        HSSD_CMD6_DATAWIDTH)) 
       dwRetVal = Err_BYTECOPY_FAILED;
  }

  return dwRetVal;
}

/*void HSSD_Initialise_Handler(int nCardNo)

  *Description : Handles the HSSD sepcific initialisation tasks.

  *Called from :Enumerate_MMC_card. 

  *Parameters:
   nCardNo:(i/p):Card number.

  *Return Value: 

  *Flow:
*/
void HSSD_Initialise_Handler(int nCardNo)
{
  DWORD dwRetVal;
  unsigned char bHSSDtype=0;
//  printk("handler HSSD card!\n");
  //Check if card is HS-SD type.
  if((dwRetVal=Check_HSSD_Type(nCardNo, &bHSSDtype))==SUCCESS)
  {
 //   printk("detected HSSD card!\n");
    if(bHSSDtype)
    {
      //Check if HSMMC card index exceeds the maximum count.
 //     if(G_CURR_HSSD_CARDCOUNT >= MAX_HSSD_CARDS)
 //     {
 //      printk("Error : Err_HSSD_CARDCOUNTOVER.HS-SD functions will not be available.\n");
 //      return ;
 //     }

      //G_CURR_HSSD_CARDCOUNT is now index for HSSD_EXTCSD_Info.
      //Save bit7 = card type and bit0-bit4 = index.
      Card_Info[nCardNo].HSdata = (BYTE)((G_CURR_HSSD_CARDCOUNT & 0x7F) | 0x80);
     
//      G_CURR_HSSD_CARDCOUNT++;
    }
  }
//  printk("HSSD card handle over!\n");
  return ;
}
