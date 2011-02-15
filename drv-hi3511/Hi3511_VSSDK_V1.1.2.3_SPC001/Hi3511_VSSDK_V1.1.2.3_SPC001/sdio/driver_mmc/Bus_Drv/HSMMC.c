/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   HSMMC.c
	Start Date :   21 April,2004.
	Last Update:   

	Reference  :   MMC specifications 4.0

    Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the routines for HSMMC interface.

    REVISION HISTORY:

***************************************************************/

#include "Externfns.h"

#define  CMD_SET         0
#define  PWR_CLASS       1
#define  SET_HS_TIMING   2
#define  BUS_WIDTH       3

#define  CMD_SET_INDEX       191
#define  PWR_CLASS_INDEX     187
#define  HS_TIMING_INDEX     185
#define  BUS_WIDTH_INDEX     183

#define  MAX_CSD_BYTE_INDEX  191

#define  CARD_TYPE_INDEX     196
#define  S_CMD_SET_INDEX     504

#define  PWRCLASS_LV52mhz_INDEX     200
#define  PWRCLASS_LV26mhz_INDEX     201
#define  PWRCLASS_HV52mhz_INDEX     202
#define  PWRCLASS_HV26mhz_INDEX     203

static DWORD  G_CURR_HSMMC_CARDCOUNT =0;


//HSMMC routines.
DWORD Send_CMD6(int nCardNo,DWORD dwArg);
DWORD Set_HSmodeSettings(int nCardNo, BYTE bModeType, BYTE bValue);
DWORD Set_Clear_EXTCSDbits(int nCardNo,BYTE bValue, BYTE bIndex, BOOL nSet);
DWORD Write_EXTCSD (int nCardNo,BYTE bValue, BYTE bIndex);
DWORD HSMMC_ExtCSDHandler(int nCardNo);
DWORD Read_ExtCSD(int nCardNo);
void  Save_HSMMC_data(int nHSMMCindex , BYTE *pbEXTcsdData);
DWORD Bus_Testing(int nCardNo,BYTE* pbLineStatus);
//HSMMC routines.
/* Send_CMD6:Send CMD6 for HSMMC card. 

   DWORD Send_CMD6(int nCardNo,DWORD dwArg)

  *Called from: Change_HSMMCcmdset,Set_Clear_EXTCSDbits
   Write_EXTCSD

  *Parameters:
   nCardNo:(i/p):Card number.
   dwArg  :(i/p):Commnand argument for CMD6.

  *Return Value: 0 for SUCCESS.
   Errors returned from Form_Send_Cmd & Validate_Response.

  *Flow:
*/
DWORD Send_CMD6(int nCardNo,DWORD dwArg)
{
  BYTE  CmdRespBuff[4];
  DWORD dwRetVal;

  //Check if the card is HSMMC or not.
  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(nCardNo,00,00,HSMMC_Command) )
      != SUCCESS)  return dwRetVal;

  //CMD6 is valid in transfer state only.Check card status.
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(nCardNo))!=SUCCESS)  
    return dwRetVal;

  //Now send CMD6.  
  if((dwRetVal = form_send_cmd(nCardNo, CMD6,dwArg,CmdRespBuff,0))
     !=SUCCESS)  return  dwRetVal;

  return Validate_Response(CMD6,CmdRespBuff,nCardNo,DONOT_COMPARE_STATE);  
}

/*Set_HSmodeSettings:Change the current mode settings in Extended CSD.
  This function selects the index of the mode type and write the 
  given value at the indexed byte.Accordingly updates the local
  HSMMC info structure for given card.

  DWORD Set_HSmodeSettings(int nCardNo, BYTE bModeType, BYTE bValue)

  *Called from: User application. 

  *Parameters:
   nCardNo      :(i/p):Card number.
   bModeType    :(i/p):Identifier for mode field.
                       0 = CMD_SET          command set change.
                       1 = PWR_CLASS        power class change.
                       2 = SET_HS_TIMING    Set HS timing mode.
                       3 = BUS_WIDTH        bus width setting.
   bValue       :(i/p):Value to be wriiten for the selected mode field.
                       Valid Range :
                       CMD_SET       : 0-2 
                       PWR_CLASS     : 0-15
                       SET_HS_TIMING : 0-1
                       BUS_WIDTH     : 0-2
  *Return Value: 

  *Flow:
*/
DWORD Set_HSmodeSettings(int nCardNo, BYTE bModeType, BYTE bValue)
{
  BYTE bIndex;
  DWORD dwRetVal,dwData=0;

  //Validate arguments as per mode type.

  switch(bModeType)
  {

    case CMD_SET:    
      //Write value at index=191.
      if(bValue>2) return Err_Invalid_Arg;         

      bIndex = CMD_SET_INDEX;    
      break;     

    case PWR_CLASS:    
      //Write value at index=187. 
      //Refer table 38 from MMC sepcs 4.0 As per voltage type of card,
      //value in pwr_class register corresspond to the current rating.
      if(bValue>15) return Err_Invalid_Arg;                  

      bIndex = PWR_CLASS_INDEX;
      break;     

    case SET_HS_TIMING:    
      //Write 1 at index=185.
      if(bValue>1) return Err_Invalid_Arg;                  

      bIndex = HS_TIMING_INDEX;

      break;     

    case BUS_WIDTH:    
      //Write value at index=183.         
      if(bValue>2) return Err_Invalid_Arg;                  

      bIndex = BUS_WIDTH_INDEX;
      break;     

    default :
      return Err_Invalid_Arg;
  }

  if((dwRetVal=Write_EXTCSD(nCardNo,bValue,bIndex))== SUCCESS)
  {
    int nHSMMCindex = (Card_Info[nCardNo].HSdata & 0x7F);

    //Update local HSMMC info structure.
    switch(bModeType)
    {
      case CMD_SET:    
        HSMMC_EXTCSD_Info[nHSMMCindex].Current_cmdset = bValue;
        break;

      case PWR_CLASS:    
        HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.PowerClass = bValue;
        break;

      case SET_HS_TIMING:    
        HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.HStiming = bValue;
        break;

      case BUS_WIDTH:    
        HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.Buswidth = bValue;
        //Update card type register.

        Read_Write_IPreg(CTYPE_off,&dwData,READFLG);
        if(bValue==2)
        {
          dwData |= (1<<(nCardNo+16));              
        }
        else
        {
          if(bValue==1)
             dwData |= (1<<nCardNo);              
          else
             dwData &= ~(1<<nCardNo);              

          dwData &= ~(1<<(nCardNo+16));              
        }
         
        Read_Write_IPreg(CTYPE_off,&dwData,WRITEFLG);

        break;
    }
  }//end of if.

  return dwRetVal; 
}

/*Set_Clear_EXTCSDbits:Set/clear the extended CSD register settings.

  DWORD Set_Clear_EXTCSDbits(int nCardNo,BYTE bValue, BYTE bIndex, BOOL nSet)

  *Called from : User application. 

  *Parameters:
   nCardNo:(i/p):Card number.
   bValue :(i/p):As per the number of 1's in this value,bits in indexed byte
                 are set or cleared.
   bIndex :(i/p):Index of the byte in ExtCSD register.Should be from 0-191.
                 Card treats other indexes as invalid.
   nSet   :(i/p): 1= Set the bits.    0= Clear the bits.

  *Return Value: 

  *Flow:
*/
DWORD Set_Clear_EXTCSDbits(int nCardNo,BYTE bValue, BYTE bIndex, BOOL nSet)
{
  DWORD dwArg=0;

  //Check if index value is between 0-191.Refer note3 on pg28 of MMC sepcs 4.0
  if(bIndex>MAX_CSD_BYTE_INDEX) return Err_Invalid_Arg;

  //Write index number.
  *((BYTE*) &dwArg+2) = bIndex;

  //Write value.
  *((BYTE*) &dwArg+1) = bValue;

  //Write access mode.
  *((BYTE*) &dwArg+3) = (nSet) ? 1 : 2;

  return Send_CMD6( nCardNo,dwArg);
}


/*Write_EXTCSD:Write given byte value at the indexed byte.

  DWORD Write_EXTCSD(int nCardNo,BYTE bValue, BYTE bIndex)

  *Called from : User application. 

  *Parameters:
   nCardNo:(i/p):Card number.
   bValue :(i/p):Value to be written at the indexed byte.
   bIndex :(i/p):Index of the byte in ExtCSD register.

  *Return Value: 

  *Flow:
*/
DWORD Write_EXTCSD(int nCardNo,BYTE bValue, BYTE bIndex)
{
  DWORD dwArg=0;

  //Check if index value is between 0-191.Refer note3 on pg28 of MMC sepcs 4.0
  if(bIndex>MAX_CSD_BYTE_INDEX) return Err_Invalid_Arg;

  //Write index number.
  *((BYTE*) &dwArg+2) = bIndex;

  //Write value.
  *((BYTE*) &dwArg+1) = bValue;

  //Write access mode.
  *((BYTE*) &dwArg+3) = 3;

  return Send_CMD6( nCardNo,dwArg);
}

/*HSMMC_ExtCSDHandler:Handle HSMMC card.

  DWORD HSMMC_ExtCSDHandler(int nCardNo)

  *Called from :Enumerate_MMC_card. 

  *Parameters:
   nCardNo:(i/p):Card number.

  *Return Value: 

  *Flow:
*/
//Caller of this function has confirmed that enumerated card is HSMMC
//and hence, extended csd needs to be read.If CSD::CSD_structure = 3
//or CSD::Spec_versions = 4,then the card is HSMMC.
DWORD HSMMC_ExtCSDHandler(int nCardNo)
{
  //Check if HSMMC card index exceeds the maximum count.
  //eg For maximum 4 HSMMC cards,this count ranges from 0-3.
 // if(G_CURR_HSMMC_CARDCOUNT >= MAX_HSMMC_CARDS)
 //   return Err_HSMMC_CARDCOUNTOVER;

  //G_CURR_HSMMC_CARDCOUNT is now index for HSMMC_EXTCSD_Info.
  //Save bit7 = card type and bit0-bit4 = index.
  Card_Info[nCardNo].HSdata = (BYTE)((G_CURR_HSMMC_CARDCOUNT & 0x7F) | 0x80);

  //G_CURR_HSMMC_CARDCOUNT++;

  return Read_ExtCSD(nCardNo);
}


/*Read_ExtCSD:Reads extended CSD using CMD8.

  DWORD  Read_ExtCSD(int nCardNo)

  *Called from : HSMMC_ExtCSDHandler

  *Parameters:
   nCardNo:(i/p):Card number.

  *Return Value: 

  *Flow:
*/
DWORD  Read_ExtCSD(int nCardNo)
{
  BYTE pbCmdRespBuff[4];
  BYTE ExtCSDdata[512];
  COMMAND_INFO  tCmdInfo={0x00};
  DWORD dwRetVal=0,dwOrgBlkLength;

  memset(ExtCSDdata,00,sizeof(ExtCSDdata));

  //This function will be called, if enumeration of card is 
  //successful. So, no need to check the validity of card number.

  //Read extended CSD.

  //**IMP** : Single block of 512 bytes will be sent by card.
  //Set block length =512.
  dwOrgBlkLength = Card_Info[nCardNo].Card_Read_BlkSize;
  //Set block length.This is not partial reading.Card will send 
  //block of 512 bytes.
  if(dwOrgBlkLength != 512)
     dwRetVal= SetBlockLength(nCardNo,512,READFLG,1);

  //printk("SetBlockLength returns %lx \n",dwRetVal);
  if(dwRetVal!=SUCCESS) return dwRetVal;

  //start addr ie arg = 0, bytecount=512,flags=0x2,block mode.
  Form_Data_Command(nCardNo,0,512,0x2,&tCmdInfo,00);

  tCmdInfo.CmdReg.Bits.cmd_index = CMD8;

  //** No need of auto stop as card will send 512 bytes and return to
  //transfer state.

  //No error buffer,no callback,no retries,data command,kernel mode buffer.

  dwRetVal = Send_cmd_to_host(&tCmdInfo,pbCmdRespBuff,
                              NULL,//No error buffer. 
                              ExtCSDdata,
   	                          NULL,//No callback pointer
                              0x2, //Read, wait for prev.data=1.
                              0,   //no retries.
 	                          TRUE,//Data command
                              FALSE,//Read 
                              TRUE);//Kernel mode buffer.

  if(dwRetVal==SUCCESS)
     Validate_Response(CMD8,pbCmdRespBuff,nCardNo,CARD_STATE_TRAN);

  //Ext.csd is read. Now, save it. 
  //Save data in HSMMC_EXTCSD_Info.Pass index for both arrays.
  Save_HSMMC_data((Card_Info[nCardNo].HSdata & 0x7F), ExtCSDdata);

  //If original block length is 512,donot set block length.
  if(dwOrgBlkLength!=512)
     SetBlockLength(nCardNo,dwOrgBlkLength,READFLG,1);
    
  return dwRetVal;        
}


/*Save_HSMMC_data:Saves ExtCSD data in structure.

  void Save_HSMMC_data(int nHSMMCindex , BYTE *pbEXTcsdData)

  *Called from : Read_ExtCSD

  *Parameters:
   nHSMMCindex  :(i/p):Index of array of HSMMCinfo structures.
   pbEXTcsdData :(i/p):Data buffer holding ExtCSD data read from card.

  *Return Value: 

  *Flow:
*/
void Save_HSMMC_data(int nHSMMCindex , BYTE *pbEXTcsdData)
{
  HSMMC_EXTCSD_Info[nHSMMCindex].Supported_pwr_HV26mhz = *(pbEXTcsdData+PWRCLASS_HV26mhz_INDEX); 
  HSMMC_EXTCSD_Info[nHSMMCindex].Supported_pwr_HV52mhz = *(pbEXTcsdData+PWRCLASS_HV52mhz_INDEX); 
  HSMMC_EXTCSD_Info[nHSMMCindex].Supported_pwr_LV26mhz = *(pbEXTcsdData+PWRCLASS_LV26mhz_INDEX); 
  HSMMC_EXTCSD_Info[nHSMMCindex].Supported_pwr_LV52mhz = *(pbEXTcsdData+PWRCLASS_LV52mhz_INDEX); 
  //b0-b1=last 2 bits from 196th byte. HSMMC card type.
  //b2-b4=last 3 bits from 504th byte. Supported command sets.
  HSMMC_EXTCSD_Info[nHSMMCindex].Supp_Settings_Info.Bits.S_CMD_SET = (*(pbEXTcsdData+S_CMD_SET_INDEX) & 7);
  HSMMC_EXTCSD_Info[nHSMMCindex].Supp_Settings_Info.Bits.CARD_TYPE = (*(pbEXTcsdData+CARD_TYPE_INDEX) & 3); 

  HSMMC_EXTCSD_Info[nHSMMCindex].Current_cmdset = *(pbEXTcsdData+CMD_SET_INDEX); 
  //byte187::last 4 bits. Current power class.
  //byte185::lastbit.     HS interface timing. 
  //byte183::last2 bits.  Current bus width.
  HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.Buswidth =  *(pbEXTcsdData+BUS_WIDTH_INDEX) & 3;
  HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.HStiming =  *(pbEXTcsdData+HS_TIMING_INDEX) & 1;
  HSMMC_EXTCSD_Info[nHSMMCindex].Curr_Settings_Info.Bits.PowerClass = *(pbEXTcsdData+PWR_CLASS_INDEX) & 0xf;

  return;
}

/*Bus_Testing:Checks how many bus lines are functional between 
  card and host.Sends CMD19,followed by CMD14 and deduce the number
  of functional lines.Returns the functional status of d0 to d7 lines.
  Then, caller function can decide what bus width to be used.

  DWORD  Bus_Testing(int nCardNo, BYTE* pbLineStatus)

  *Called from : User application. 

  *Parameters:
   nCardNo     :(i/p):Card number.
   pbLineStatus:(o/p):Pointer to byte data.Each bit corresponds
                      to the corresponding data line status.
                      1 = functional,  0 = non functional.
                      eg bit0=1, d0 line is functional.

  *Return Value: 

  *Flow:
*/
DWORD  Bus_Testing(int nCardNo,BYTE* pbLineStatus)
{
  COMMAND_INFO  tCmdInfo={0x00};
  BYTE  pbCmdRespBuff[4];
  BYTE  TestData[16];
  DWORD dwRetVal,dwOrgCtypeData,dwData;
  WORD  const wSentData=0xA55A;
  WORD  wIsolate;
  int   i;

  //Check if the card is HSMMC or not.
  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(nCardNo,00,00,HSMMC_Command) )
      != SUCCESS)  return dwRetVal;

  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(nCardNo))!=SUCCESS)  
    return dwRetVal;
 
  //Set 8 bit width.
  Read_Write_IPreg(CTYPE_off,&dwOrgCtypeData,READFLG);
  dwData = dwOrgCtypeData;
  dwData |= (1<<(nCardNo+16));   
  Read_Write_IPreg(CTYPE_off,&dwData,WRITEFLG);           

  //Send CMD19.(Data write command).Write 16 bytes,irrespective 
  //of block size in card.Card ignores the 3rd bit onwards on each
  //data line.
  //start addr ie arg = 0, bytecount=16,flags=0x2,block mode,write.
  Form_Data_Command(nCardNo,0,16,0x82,&tCmdInfo,00);
  tCmdInfo.CmdReg.Bits.cmd_index = CMD19;
  tCmdInfo.dwBlkSizeReg = 16;
  //Set test data. 10100101
  memset(TestData,00,16);
  TestData[0] = *((BYTE*)(&wSentData));//0xA5
  //Next byte = 01011010
  TestData[1] = *((BYTE*)(&wSentData)+1);//0x5A

  //No error buffer,no callback,no retries,data command,kernel mode buffer.
  dwRetVal = Send_cmd_to_host(&tCmdInfo,pbCmdRespBuff,
                              NULL,//No error buffer. 
                              TestData,
                              NULL,//No callback pointer
                              0x82,//Write, wait for prev.data=1.
                              0,   //no retries.
 	                          TRUE,//Data command
                              TRUE,//Write 
                              TRUE);//Kernel mode buffer.

  //If Data CRC error or Write no crc error, ignore them.
  if((dwRetVal !=SUCCESS) && (dwRetVal != 0x8000) && 
     (dwRetVal != 0x80)) return dwRetVal;

  //Send CMD14.(Data read command).
  memset(TestData,00,16);
  tCmdInfo.CmdReg.Bits.cmd_index  = CMD14;
  tCmdInfo.CmdReg.Bits.read_write = 0;
  tCmdInfo.CmdReg.Bits.start_cmd  = 1;

  //No error buffer,no callback,no retries,data command,kernel mode buffer.
  dwRetVal = Send_cmd_to_host(&tCmdInfo,pbCmdRespBuff,
                              NULL,//No error buffer. 
                              TestData,
   	                          NULL,//No callback pointer
                              0x2, //Read, wait for prev.data=1.
                              0,   //no retries.
 	                          TRUE,//Data command
                              FALSE,//Read 
                              TRUE);//Kernel mode buffer.

  //If Data CRC error or Write no crc error, ignore them.
  if((dwRetVal !=SUCCESS) && (dwRetVal != 0x8000) && 
     (dwRetVal != 0x80)) return dwRetVal;

  //check for bits inversion and if successful,set corresponding 
  //bit in pbLineStatus.
  wIsolate = 0x0101;//Bits 0 and 8 for D0 line.
  *pbLineStatus=00;
  for(i=0;i<8;i++)
  {
    if((*((WORD*)TestData) & wIsolate)== (~wSentData & wIsolate))
     *pbLineStatus |= (BYTE)(1<<i);
   
    wIsolate <<= 1;
  }

  //Re-set original CTYPE register contents.
  Read_Write_IPreg(CTYPE_off,&dwOrgCtypeData,WRITEFLG);           
    
  return dwRetVal;        

}

//--------------------------------------------------------------
