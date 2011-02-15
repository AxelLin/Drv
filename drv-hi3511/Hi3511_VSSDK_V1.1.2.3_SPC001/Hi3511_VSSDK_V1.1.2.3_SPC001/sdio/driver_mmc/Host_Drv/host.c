/***************************************************************
	PROJECT    :  "SD-MMC IP Driver" 
	------------------------------------
	File       :   Host.c
	Start Date :   03 March,2003
	Last Update:   31 Oct.,2003

	Reference  :   registers.pdf

        Environment:   Kernel mode

	OVERVIEW
	========
	This file contains the algorithm for host driver functions.
	Host driver implements IP specific functionality thereby
	making bus driver independent of IP details.


    REVISION HISTORY:

***************************************************************/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/wait.h>


#include "../CommonFiles/pal.h" //To add includes and version related defines. 
#include "host.h"

#include "../CommonFiles/sd_mmc_const.h"
#include "../CommonFiles/sd_mmc_IP.h"
#include "sd_mmc_host.h"
#include <linux/kcom.h>
#include <kcom/hidmac.h>

#include "w_debug.h"


#define OSDRV_MODULE_VERSION_STRING "SDIO-M05C0302 @Hi3511v110_OSDrv_1_0_0_8 2009-05-20 14:58:07"
//static struct kcom_object *kcom;
//static struct kcom_hi_dmac *kcom_hidmac;
DECLARE_KCOM_HI_DMAC();

unsigned long FIFO_Depth=4;//Default=RxWatermark value.
static unsigned long FIFO_Width=32;//As per H_Datawidth.

struct work_struct sd_mmc_int_workstruct;
//wait_queue_head_t   sd_mmc_cmd_wqueue;
//wait_queue_head_t   sd_mmc_data_wqueue;

//static unsigned device_major;
//void __iomem* map_adr;
unsigned int map_adr;
int sd_dma_cha;
/*
func[0]:sd dma complete hook
func[1]:sdio dma complete hook
func[2]:sd detect irq hook
func[3]:sdio detect irq hook
*/
void (*func[5])(int);

//extern struct Card_info_Tag Card_Info[];

#define   mem_size         EXC_PLD_BLOCK0_SIZE //0x04000  //PLD0 size.Refer excalibur.h
//#define DMA_ENABLE

//Global variables :
#ifdef _GPIO_CARD_DETECT_
extern  wait_queue_head_t ksdmmc_wait;
#endif

extern void ISR_Callback(void);
extern int Create_Intr_Thread(void);
extern int Kill_Intr_Thread(void);
extern int SD_MMC_BUS_ioctl (struct inode *inode, struct file *file,
                               unsigned int cmd, unsigned long arg);
extern int sd_mmc_sdio_initialise(void);
extern void dma_module_exit(void);
extern int dmac_module_init(void);

static ssize_t SD_MMC_Host_read(struct file *file, char *buf, size_t buflen, loff_t *ppos);

static ssize_t SD_MMC_Host_write(struct file *file, const char *buf, size_t count, loff_t *ppos);

static int SD_MMC_Host_open(struct inode *inode, struct file *file);

static int SD_MMC_Host_release(struct inode *inode, struct file *file);


int  Send_SD_MMC_Command(COMMAND_INFO *ptCmdInfo);
int  Read_PendingData_Status(struct TRANSFERRED_BYTE_INFO_Tag 
									*ptTransferByteInfo) ;
int  Poll_Cmd_Accept(void);
int Assert_n_Poll_startcmd(void);
int set_ip_parameters(int nCmdOffset, DWORD *pdwRegData);
int  Read_Write_IPreg(int nRegOffset,DWORD *pdwRegData,BOOL fWrite);
int  Read_Status (struct STATUS_INFO_Tag *pdwStatusInfo);
int  Set_ClkDivider_Freq(int nClkSource,int nDivider,
				    BYTE *pbPrevVal);
int  Read_Write_MultiRegdata( int nRegOffset, int nByteCount,
                           BYTE *pbDataBuff, BOOL fWrite);
int Read_Write_FIFOdata(int nByteCount, BYTE *pbDataBuff,BOOL fWrite,
                         BOOL fKernSpaceBuff);

int Set_Clock_Source(int nCardNo,int nSourceNo);
int  Clk_Enable(int nCardNo,BOOL fEnable);
int  Set_Reset_CNTRLreg(DWORD dwRegBits,BOOL fSet);
int  Read_Response(RESPONSE_INFO *ptResponseInfo,int nRespType);

#ifdef INTR_ENABLE
void SD_MMC_intr_handler(void);
static irqreturn_t SD_MMC_host_intr_handler(int irq, void *dev_id ,struct pt_regs * regs);
#endif

#ifdef _GPIO_CARD_DETECT_
	static void cartd_detect_int_init(void);
	void card_detect_clear_intflag(void);
	void card_detect_int_enable(int flag);
	static irqreturn_t card_detect_intr_handler(int irq, void *dev_id ,struct pt_regs * regs);
#endif

static int sd_mmc_host_driver_init(void);
void sd_mmc_enable_card_power(int);

//Exported functions:
//If this is not used,then all non-static symbols-functions and
//variables are exported.
EXPORT_SYMBOL(Send_SD_MMC_Command);
EXPORT_SYMBOL(Read_PendingData_Status);
EXPORT_SYMBOL(Read_Write_IPreg);
EXPORT_SYMBOL(Read_Status);
EXPORT_SYMBOL(Set_ClkDivider_Freq);
EXPORT_SYMBOL(Read_Write_MultiRegdata);
EXPORT_SYMBOL(Read_Write_FIFOdata);
EXPORT_SYMBOL(Set_Reset_CNTRLreg);
EXPORT_SYMBOL(set_ip_parameters);
EXPORT_SYMBOL(Set_Clock_Source);
EXPORT_SYMBOL(Clk_Enable);
EXPORT_SYMBOL(Read_Response);



/*--------------------------------------------------------------
//Host APIs specifications.

Host Driver:
A.1. Features :
 - Direct interface with IP.
 - Provide access to each functionality presented by IP.
 - No SD/MMC specific functionality.
 - IP specific Functionality is : accessing register rd/wr.
 - Access to EPXA1 specific functionality.

//--------------------------------------------------------------

A.2.Functions : Groups as below.
  1. Card interface
  2. IP interface
  3. Register related 
  3. IRQ
  4. DMA controls

//------------------------------------------------------------*/           
//A.2.1. Card interface : 
//==============================================================

/*1.Send_SD_MMC_Command :
  Send_SD_MMC_Command(COMMAND_INFO *ptCmdInfo)

  *Called from : BSD.

  *Parameters  :
         ptCmdInfo->Cmdreg ;//Pointer to command register structure.
	                       //Defined in Host drv.
         ptCmdInfo->dwCmdArgReg ;//Command argument reg.contents
         ptCmdInfo->dwByteCountReg ;//Byte count register contents
         ptCmdInfo->dwBlockSizeReg ;//Block size register contents

  *Flow:
        -Check if adapter address is not NULL.
        -Write byte count register,using writereg().
        -Write block size register,using writereg().
        -Write command argument register,using writereg().
        -Write command register,using writereg().    
        -Return.
*/
int  Send_SD_MMC_Command(COMMAND_INFO *ptCmdInfo)
{
    DWORD dwCMDReg;

    dwCMDReg=00;

    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
	return -EINVAL;
 
    //Write byte count register,using writereg().
    WriteReg(BYTCNT_off,ptCmdInfo->dwByteCntReg);

    //Write block size register,using writereg().
    WriteReg(BLKSIZ_off,ptCmdInfo->dwBlkSizeReg);
    
    //Write command argument register,using writereg()
    WriteReg(CMDARG_off,ptCmdInfo->dwCmdArg);

    //Write command register,using writereg().    
    WriteReg(CMD_off,ptCmdInfo->CmdReg.dwReg);

    //Returns SUCCESS on success or else returns error.
    return Poll_Cmd_Accept();
}


//--------------------------------------------------------------
/*2.Read_Response : Reads rep0-3 registers and fill in
  given structure. Note that "ptResponseInfo" should be the kernel
  mode structure.

  Read_Response(struct RESPONSE_INFO *ptResponseInfo,int nRespType)

  *Called from : BSD.

  *Parameters  :
         ptResponseInfo:(o/p):Respose info.structure.
		 {
		   RESP0,RESP1,RESP2.RESP3 data is stored in
           structure.
		 }

  *Flow:
        -Check if adapter address is not NULL.
        -Check if structure pointer is not null.
        -Read response register(s) using Read_Reg and
		 write it into structure.
*/
int Read_Response(RESPONSE_INFO *ptResponseInfo,int nRespType)
{
    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
		return -EINVAL;

    //Check if structure pointer is not null.
    if(!ptResponseInfo)
		return -EINVAL;

    ptResponseInfo->dwResp0=ReadReg(RESP0_off);    
    if(nRespType) //Long response
	{
          ptResponseInfo->dwResp1=ReadReg(RESP1_off);
          ptResponseInfo->dwResp2=ReadReg(RESP2_off);
          ptResponseInfo->dwResp3=ReadReg(RESP3_off);        
	}
    //printk("Host_drv: Returning after copying response of type %x \n",nRespType); 
    return SUCCESS;
}
//--------------------------------------------------------------
/*
3. Read_PendingData_Status: Reads Pending Byte count
  register and pending FIFO count register.

  Read_PendingData_Status(TRANSFERRED_BYTE_INFO 
                          *ptTransferByteInfo) 


  *Called from : BSD.

  *Parameters  :
         ptTransferByteInfo:(o/p):Command status info.structure.
         {
           Pending card byte count (@0x05c) and pending
           FIFO byte count (0x060) data is stored in structure.
         }

  *Flow:
        -Check if adapter address is not NULL.
        -Check if structure pointer is not null.
        -Read (@0x05c) and (@0x060) using Read_Reg and
		 write it into structure.
*/
int  Read_PendingData_Status(struct TRANSFERRED_BYTE_INFO_Tag 
                             *ptTransferByteInfo) 

{
    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
      return -EINVAL;

    //Check if structure pointer is not null.
    if(ptTransferByteInfo==NULL)
      return -EINVAL;

    //Read status register(s).
    ptTransferByteInfo->dwCIUByteCount=ReadReg(TCBCNT_off);    
    ptTransferByteInfo->dwBIUFIFOByteCount=ReadReg(TBBCNT_off);  
    return SUCCESS;
}
//--------------------------------------------------------------
/*Poll_Cmd_Accept:Poll for command acceptance or any error.

  Poll_Cmd_Accept()

  *Called from : HSD.

  *Parameters  :Nothing.

  *Flow:
        -while()
	     {
           -Check if CMD::start_cmd bit is clear.
		    If yes,return Cmd_Accepted;

           -Check if Raw_Intr_Status::HLE bit is set.
		    If yes,return Err_Hardware_Locked;

           -Check if number of retries for this are over.
		    If yes,return Err_Retries_Over;
	     }
*/
int   Poll_Cmd_Accept(void)
{
  int nRetryCount,nWait;
  DWORD dwRegdata=0;
  nRetryCount=0;

    while(1)
      {
        //Check if CMD::start_cmd bit is clear.
        //start_cmd=0 means BIU has loaded registers and
        //next command can be loaded in.
        if( (ReadReg(CMD_off) & start_cmd_bit )==0 ) 
          {
            //Without this delay/printk,this routine returns error 
            //as  Err_Retries_Over. Delay didnot worked.So,\b used
            //dec23 printk(" \b");
            Read_Write_IPreg(CMD_off,&dwRegdata,FALSE);
            return SUCCESS;
          }
        //Check if Raw_Intr_Status::HLE bit is set.
        if(ReadReg(RINTSTS_off) & HLE_bit)
          return Err_Hardware_Locked;
        
      //Check if number of retries for this are over.
      for(nWait=0;nWait<0x0f;nWait++);

      nRetryCount++;

      if(nRetryCount >= MAX_RETRY_COUNT)
        return Err_Retries_Over; 

      };//End of while loop.
}

//--------------------------------------------------------------

//A.2.2. IP interface : Subgroups as below.
//==============================================================

/*1.Set_IP_Parameters: Set parameters in IP register.
Set_IP_Parameters(int nCmdOffset, DWORD *pdwRegData)

  *Called from : BSD.

  *Parameters  :
         nCmdOffset:(i/p):command no.
         pdwRegData:(i/p or o/p):Pointer for returning data read.
  *Flow:
        -Check if adapter address is not NULL.
        -Check if command_offset is valid.
		-For unknown offeset, cmd_offset is treated as 
		 register offset
        -Depending upon command offset,register offset is 
		 selected.
        -*dwDestination = *( Card_Adapter_Addr+Reg_offset)
		 switch(nCmdOffset)
		 {
           case a:
		   case b:
         }

		-Return.
*/
int set_ip_parameters(int nCmdOffset, DWORD *pdwRegData)
{
	//Temporary storage.
    DWORD dwData=00;
    BOOL fIntEnabled;

    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
		return -EINVAL;

    switch(nCmdOffset)
	{
	  case SET_RECV_FIFO_THRESHOLD:
            //pdwRegData holds actual threshold value.
            dwData= (ReadReg(FIFOTH_off) & 0xf000ffff)| ((*pdwRegData &0x0fff)<<16);
            WriteReg(FIFOTH_off,dwData);
            
	    break;
            
      case SET_TRANS_FIFO_THRESHOLD:
            //pdwRegData holds actual threshold value.
            dwData = (ReadReg(FIFOTH_off) & 0xfffff000)| (*pdwRegData &0x0fff)  ;
            WriteReg(FIFOTH_off,dwData);
            
	    break;

	  case SET_DATA_TIMEOUT:
            //pdwRegData holds actual timeout value.
            dwData = (ReadReg(TMOUT_off) | (*pdwRegData &0x0ffffff) ) ;
            WriteReg(TMOUT_off,dwData);
            
            //**No need to issue start_cmd command as it will be issued
	    //when actual command will be sent.
            
            break;

	  case SET_RESPONSE_TIMEOUT:
            dwData = ReadReg(TMOUT_off) & DataTimeOut_bit;
            dwData |= *pdwRegData;
            WriteReg(TMOUT_off,dwData );
            
            //**No need to issue start_cmd command as it will be 
            //issued when actual command will be sent.
            
            break;

	  case SET_SDWIDTH_FOURBIT:
            //Width element is of word size.
            //*pdwRegData has card number.
      //      printk("SET_SDWIDTH_FOURBIT!\n");
            dwData = ReadReg(CTYPE_off);
            dwData |= (1 << *pdwRegData);

            //Clear corresponding bit for 8 bit width. 
            dwData &= ~ (1 << (*pdwRegData+16));

            WriteReg(CTYPE_off,dwData);
            break;

	  case SET_SDWIDTH_ONEBIT:
            //Width element is of word size.
            //*pdwRegData has card number.       
            dwData = ReadReg(CTYPE_off);
            dwData &= ~ (0x01 << *pdwRegData);

            //Clear corresponding bit for 8 bit width. 
            dwData &= ~ (1 << (*pdwRegData+16));

            WriteReg(CTYPE_off,dwData);
            
            break;

            //****For interrupts only,this is provided as at a time 
            //user may want to enable/disable more than one inters.
        case SET_INTR_MASK:
          //Argument passed has 1 at bits related to inters
          //to be masked.
          WriteReg(INTMSK_off,*pdwRegData);
          break;

        case CLEAR_INTR_MASK:
          //Argument passed has 1 at bits related to inters
          //to be unmasked.
          dwData = ReadReg(INTMSK_off) & (~(*pdwRegData)) ;
          WriteReg(INTMSK_off,dwData);
          break;

        case FIFO_access:
          break;
          
        case POWER_ON:
        	
	#ifdef _POWER_EN_NEED_        	
		#ifdef _GPIO_POWEREN_        	
			GPIO_POWER_EN_OUT;
			GPIO_POWER_EN_ENABLE;
		#else
			//pdwRegData holds card number.
			if(*pdwRegData==0x0ff)
				//Enable power to all cards.
				WriteReg(PWREN_off,0x03fffffff);
			else
			{
				dwData=0x01;
				WriteReg( PWREN_off,(ReadReg(PWREN_off)|(dwData<<(*pdwRegData))));
			}		
		#endif
	#endif
	
          break;
          
        case POWER_OFF:
        	
        #ifdef _POWER_EN_NEED_        	
		#ifdef _GPIO_POWEREN_        	
			GPIO_POWER_EN_OUT;
			GPIO_POWER_EN_DISABLE;
		#else
			//pdwRegData holds card number.
			if(*pdwRegData==0x0ff)
				//Disable power to all cards.
				WriteReg(PWREN_off,00); 
			else
			{
				dwData=0x01;
				dwData=(dwData<<(*pdwRegData));
				dwData= ~dwData;

				//#ifdef DEBUG_DRIVER
				WriteReg( PWREN_off,ReadReg(PWREN_off)&(dwData));
			}
		#endif
	#endif
          
          break;
          
        case ENABLE_OD_PULL_UP:
          //Set/reset bit 24 in control register.
          //Bit value is specified in *pdwRegData.
          //G_dwBaseAddress->CTRL.Enable_od_pull_up =0x01;
          dwData = ReadReg(CTRL_off) & enable_OD_pullup_bit;
          dwData |= ((*pdwRegData & 01) << 24);
          WriteReg(CTRL_off,dwData) ;
          break;
          
        case SET_CONTROLREG_BIT:
          //pdwRegData has bit number.
          dwData = ReadReg(CTRL_off) | ( 0x1 << *pdwRegData);
          WriteReg(CTRL_off,dwData);
          break; 
          
        case CLEAR_CONTROLREG_BIT:
          //pdwRegData has bit number.
          dwData = ReadReg(CTRL_off);
          dwData &= ~(0x01 << *pdwRegData);
          WriteReg(CTRL_off,dwData);
          
          break; 
          
        case RESET_FIFO:
          //Set CTRL::bit1 to 1 and then to 0.
          dwData = ReadReg(CTRL_off);
          
          //Disable interrupts,if currently enabled.
          if(dwData & 0x10)
            {
              fIntEnabled = TRUE;
              dwData &= 0xFFFFFFEF;
              WriteReg(CTRL_off,dwData);//CNTRL_OFF::bit4=0
            }
            else fIntEnabled = FALSE;

          //Assert fifo.
          dwData |= fifo_set_bit;
          WriteReg(CTRL_off,dwData);
          
          //Wait till Reset bit gets cleared.
          do
          {
            dwData = ReadReg(CTRL_off);
          }while (dwData & fifo_set_bit);

          //Re-enable interrupts,if user has previously enabled them.
          if(fIntEnabled)
            {
              dwData |= 0x10;
              WriteReg(CTRL_off,dwData);//CNTRL_OFF::bit4=0
            }

          break; 
          
        case RESET_DMA:
          //Set CTRL::bit2 to 1.
          dwData = ReadReg(CTRL_off);
          dwData |= dma_set_bit;
          WriteReg(CTRL_off,dwData);

          //Wait till Reset bit gets cleared.
          do
          {
            dwData = ReadReg(CTRL_off);
          }while (dwData & dma_set_bit);
          
          break; 
          
        case RESET_CONTROLLER:
          //Set CTRL::bit0 to 1 and then to 0.
          dwData = ReadReg(CTRL_off);
          dwData |= controller_set_bit;
          WriteReg(CTRL_off,dwData);
          
          //Wait till Reset bit gets cleared.
          do
          {
            dwData = ReadReg(CTRL_off);
          }while (dwData & controller_set_bit);
          
          break; 
          
        case RESET_FIFO_DMA_CONTROLLER:
          //Set CTRL::bit0,1,2 to 1 and then bits0,1 to 0.
          
          dwData = ReadReg(CTRL_off);
          dwData |= all_set_bits;
          WriteReg(CTRL_off,dwData);
                
          //Wait till Reset bit gets cleared.
          do
          {
            dwData = ReadReg(CTRL_off);
          }while (dwData & all_set_bits);
                
          break; 
                
        default:
          //Command offset is invalid.
          return Err_Not_Supported;
        }
    
    return SUCCESS;
}

//--------------------------------------------------------------
//A.2.3 Register related : Actual functions as below.
//==============================================================
/*** To make Drv1 independent of IP sepcific register 
  offset deinitions, pass command offset.Drv0 will have 
  case statement which will guide the command to proper reg.
  ** Command offsets for register access :
*/


/*2.Read_Write_IPreg: Used to read or write any register of IP.

  Read_Write_IPreg(int nRegOffset,DWORD *pdwRegData,BOOL fWrite)

  *Called from : BSD.

  *Parameters  :
         nRegOffset:(i/p):Register offset in BYTES.
         pdwRegData:(i/p):Pointer to hold register data.
         fWrite:(i/p):1=Write,0=Read.

  *Flow:
        -Check if adapter address is not NULL.
        -Check if register offset is valid.
		-Depending upon fWrite flag, either read or write data.
		-Return.
*/
int  Read_Write_IPreg(int nRegOffset,DWORD *pdwRegData,BOOL fWrite)
{
    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
      return -EINVAL;

    //Check if register offset is valid.
    if(nRegOffset > MAX_REG_OFFSET)
      return Err_Invalid_RegOffset;

    //Read / write data.Since offset is in BYTES while array 
    //declared in IPRegStruct is DWORD array,divide offset by 4.
    if(fWrite)
       WriteReg(nRegOffset,*pdwRegData);
    else
      *pdwRegData = ReadReg(nRegOffset);
      return 0;
}

//--------------------------------------------------------------		 
/*3.Read_Status : Reads masked_interrupt status register,
  raw interrupt status register and status register.

  Read_Status (STATUS_INFO *pdwStatusInfo)
  *Called from : BSD.

  *Parameters  :
         pdwStatusInfo:(i/p):Pointer to STATUS_INFO.
         {
		  pdwStatusInfo->MaskedIntrStatus = Masked intr.status.
                                           reg.value @.0x040 
          pdwStatusInfo->RawIntrStatus    = raw intr.status.
                                           reg.value @.0x044 
          pdwStatusInfo->Status           = Status reg.contents.
                                           reg.value @.0x048 
         }

  *Flow:
        -Check if adapter address is not NULL.
        -Read DWORD value at 0x040,0x044,0x048 and assign to 
		 respective members in pdwStatusInfo. 
        -Return. 
*/
int  Read_Status (struct STATUS_INFO_Tag *pdwStatusInfo)
{
    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
		return -EINVAL;

    //Check if structure pointer is not null.
    if(pdwStatusInfo==NULL)
		return -EINVAL;

    pdwStatusInfo->dwMaskIntrSts =ReadReg(MINTSTS_off);
    pdwStatusInfo->dwRawIntrSts  =ReadReg(RINTSTS_off);
    pdwStatusInfo->dwStatus      =ReadReg(STATUS_off);

	return SUCCESS;
}

//--------------------------------------------------------------
/*4.Read_Write_MultiRegdata. : Reads/writes multiple consecutive 
  bytes from start.
  For read,data is read from consecutive IP registers and stored
  in data buffer.

  For write,data from data buffer is written in consecutive IP
  registers.

  Read_Write_MultiRegdata( int nRegOffset, int nByteCount,
                           BYTE *pbDataBuff, BOOL fWrite)

  *Called from : BSD.

  *Parameters  :
         nRegOffset:(i/p):Register offset from where to start 
		                  reading.
         nByteCount:(i/p):Total bytes to read
         pbDataBuff:(i/p):Data buffer to hold/holding data.
         fWrite:(i/p):1=Write data into registers. 0=Read.

  *Flow:
        -Check if adapter address is not NULL.
        -Check if register offset is valid.
        -Check (nReg_offset+nByteCount) is within valid limits.
		-Read/write data using memcpy.
		-Return.
*/  
int  Read_Write_MultiRegdata( int nRegOffset, int nByteCount,
                           BYTE *pbDataBuff, BOOL fWrite)

{
    //Check if adapter address is not NULL.
    if(!G_ptBaseAddress)
		return -EINVAL;

    //Check if register offset is valid.
    if(nRegOffset > MAX_REG_OFFSET)
		return Err_Invalid_RegOffset;

    //Check (nReg_offset+nByteCount) is within valid limits.
    if( (nRegOffset+nByteCount) > MAX_REG_OFFSET)
		return Err_Invalid_ByteCount;

    //This is reading/writing with memory on IP.so,use 
    //memcpy_fromio & memcpy_toio.
    if(fWrite)
      //dest,source,count
      memcpy_toio((void *)(IPregarrelement(nRegOffset)),
                  (void *)pbDataBuff,//Source
                  nByteCount);
    
    else
      //dest,source,count
      memcpy_toio((void *)pbDataBuff, 
                  (void *)(IPregarrelement(nRegOffset)),
                  nByteCount);
    
    return SUCCESS;
}
//--------------------------------------------------------------
/*Reads/writes data to/from FIFO. FIFO access is from same 
  location always.
  data size access = FIFO width = 32,if H_DATA_WIDTH=32/16
                                = 64,if H_DATA_WIDTH=64
  FIFO depth=FIFO size= Power ON value of RxWatreMark  

  *Called from : BSD.

  *Parameters  :
         nByteCount:(i/p):Register offset from where to start 
		                  reading.
         pbDataBuff:(i/p-o/p):Data buffer to hold/holding data.
         fWrite:(i/p):1=Write data into FIFO. 0=Read.

  *Flow:
        -Read status register.
		-Decide bytes to be accessed.
		 For write, maximum=FIFO_depth or = FIFO_depth-current count.
		 For read,  maximum=FIFO_depth or = current count.
*/
int Read_Write_FIFOdata(int nByteCount, BYTE *pbDataBuff,BOOL fWrite,
                         BOOL fKernSpaceBuff)
{
   DWORD dwStatusData,dwCurrFIFOcount,ddw64bitData,dwRetVal;
   DWORD dwDatalength=0;
   int   nDataBytes,i;

   ddw64bitData=00;

   //This routine might have been called after threshold interrupt
   //or FIFO_empty or FIFO full.So,read current FIFO count from status 
   //register and accordingly read/write data.
   dwStatusData = ReadReg(STATUS_off);

   //printk(":Host_drv:rd_wr_fifodata FIFO_Width=%lx \n",FIFO_Width);
   //printk(":Host_drv:rd_wr_fifodata status reg.data=%lx \n",dwStatusData);
   if(nByteCount==0 || pbDataBuff==NULL)
	   return 0;

   if(fWrite)
   {
     //Check if FIFO is full.
     if(dwStatusData & 0x8)
       return 0;//No bytes written.

     //If FIFO empty.
     if(dwStatusData & 0x4)
       dwDatalength = FIFO_Depth;
     else
      {
	//Fill partially empty FIFO.	
	dwCurrFIFOcount = (dwStatusData & 0x3ffe0000) >> 17;
        dwDatalength = FIFO_Depth - dwCurrFIFOcount;
      }
	//Check if bytes to write are more than dwDatalength.
        if(dwDatalength > (nByteCount/(FIFO_Width/8)))
         dwDatalength = (nByteCount/(FIFO_Width/8));

	//If byte count is < FIFO width.
	if(dwDatalength==0) 
	{
           dwDatalength=1;
	   nDataBytes=nByteCount;
        }
        else
           nDataBytes = (FIFO_Width/8);
        //dwDatalength is in terms of FIFO size units.
        for(i=0;i<dwDatalength;i++)
          {
            //Read from user buffer.
           if(fKernSpaceBuff)
           {
             //Data buffer is allocated in kernel.
             memcpy(&ddw64bitData,(pbDataBuff+(i*nDataBytes)), nDataBytes); 
           }
           else
           {
             if (copy_from_user((char *) &ddw64bitData,
		        (pbDataBuff+(i*nDataBytes)), nDataBytes)) 
	     {
	      dwRetVal = (i*nDataBytes);//i holds bytes read and stored.
	      goto done;
	     }
           }

           //Write FIFO.  Note: WriteReg returns only 32 bit data.
           WriteReg(FIFO_START,(DWORD)ddw64bitData);
          }//End of for loop.
   }
   else  //Read FIFO.
   {
     //Check if FIFO is not empty.
     if(dwStatusData & 0x4)
       return 0;//No bytes read.

     //If FIFO full
     if(dwStatusData & 0x8)
       dwDatalength = FIFO_Depth;
     else
     {
       //Read partially empty FIFO.	
       dwCurrFIFOcount = (dwStatusData & 0x3ffe0000) >> 17;
       dwDatalength = dwCurrFIFOcount;
     }

     //Check if bytes to read are less than dwDatalength.
     if(dwDatalength > (nByteCount/(FIFO_Width/8)))//FIFO width is in bits.
        dwDatalength = (nByteCount/(FIFO_Width/8));

	 //If byte count is < FIFO width.
	 if(dwDatalength==0) 
	 {
           dwDatalength=1;
	   nDataBytes=nByteCount;
	 }
	 else
           nDataBytes = (FIFO_Width/8);

	 //dwDatalength is in terms of FIFO size units.
	 for(i=0;i<dwDatalength;i++)
	 {
           //Read FIFO.  Note: ReadReg returns only 32 bit data.
           ddw64bitData =ReadReg(FIFO_START);

           //printk("%lx ",ddw64bitData);

           //store it in user buffer.
           if(fKernSpaceBuff)
           {
             //Data buffer is allocated in kernel.
             memcpy((pbDataBuff+(i*nDataBytes)),&ddw64bitData,nDataBytes); 
           }
           else
           {
               if (copy_to_user((pbDataBuff+(i*nDataBytes)), 
		     (char *) &ddw64bitData, nDataBytes)) 
	      {
	        dwRetVal = (i*nDataBytes);//i holds bytes read and stored.
	        goto done;
	      }
           }

	 }//End of for loop.

   } //End of read FIFO.

   //printk("Value of i variable is %x \n",i);
   dwRetVal = (i*nDataBytes);

done:
     return dwRetVal;

}
//--------------------------------------------------------------
/*6. Set_ClkDivider_Freq(int nClkSource,int nDivider,
                         BYTE *pbPrevVal)      

To change clock frequency or clock divider source:
  - Program "CLKEN" with inactive state for all clocks
  - set  update_clock_registers_only and  start_cmd
  - wait for command done interrupt

  - Program CLKDIV,CLKSRC to the required values
  - set  update_clock_registers_only and  start_cmd
  - wait for command done interrupt
  
  - Program "CLKEN" with active for the required clocks.
  - set  update_clock_registers_only and  start_cmd
  - wait for command done interrupt
    

  *Called From:BSD.

  *Parameters :
          nClkSource:(i/p):Clock divider section number (0-3).
          nDivider:(i/p):Divider value.
		  pbPrevVal:(i/p):Pointer to hold value of clksource 
		                   prior to change.

  *Flow :
        -Read clock enable register.This value is required to 
		 retrieve original status of individual clocks.
        -Stop clock for all cards.Issue start_cmd to use this
		 value with immediate effect.
        -Read and store current value of clock divider.
 	    -Set required clock divider value for given clock source.
		 Donot issue start_cmd.It can be issued after clock 
		 enable data is retrieved.
        -Retrieve original clock enable register contents.
		 Issue start_cmd to load values of CLKDIV and CLKENA  
		 with immediate effect.
        -Return.
*/
//To disable/enable clocks,start_cmd is required.
//To change clock freq./source allocation,start_cmd is required.
//otherwise if they will get loaded when clocks are enabled again.
//This may introduce glitch.

int Set_ClkDivider_Freq(int nClkSource,int nDivider,
				    BYTE *pbPrevVal)
{
    DWORD dwClkEnableData,dwData;

    //Read clock enable register.
    Read_Write_IPreg(CLKENA_off,&dwClkEnableData,FALSE);

    //Stop clock for all cards.Clk_Enable issues start_cmd.
    Clk_Enable(0x0ff,FALSE);

    //Read and store current value of clk source divider.
    dwData = ReadReg(CLKDIV_off);     
    *pbPrevVal = *((BYTE*)(&dwData) + nClkSource);
    *((BYTE*)(&dwData) + nClkSource) = nDivider;
	
    //Set required clock divider value for given clock source.
    WriteReg(CLKDIV_off,dwData);
    //printk("clkdiv=%x\n",nDivider);
    //TODO:Issue start_cmd to get this command into effect.

    //Retrieve original clock enable register contents.
    //Donot send start_cmd.So,use read_write_IPReg.
    Read_Write_IPreg(CLKENA_off,&dwClkEnableData,TRUE);

    //Issue start_cmd for immediate loading of CLKDIV,CLKENA,
    //CLKSRC.Check for command acceptance and return.
    return Assert_n_Poll_startcmd();
}

//--------------------------------------------------------------
/*7. Set_Clock_Source(int nCardNo,int nSourceNo)
 
  *Called From:BSD.

  *Parameters :
          nCardNo:(i/p):Card whose clk source is to be changed.
          nSourceNo:(i/p):Clock source which is to be assigned.

  *Flow :
        -Read clock enable register.This value is required to 
		 retrieve original status of individual clocks.

        -Stop clock for all cards.Issue start_cmd to use this
		 value with immediate effect.

		-Read clock source register.
		 Set clock source bits for given card number.
		 Write clock source register. 

		 Donot issue start_cmd.It can be issued after clock 
		 enable data is retrieved.

        -Retrieve original clock enable register contents.
		 Issue start_cmd to load values of CLKDIV and CLKENA with 
		 immediate effect.

        -Return.
*/
//To disable/enable clocks,start_cmd is required.
//To change clock freq./source allocation,start_cmd is required.
//otherwise if they will get loaded when clocks are enabled again.
//This may introduce glitch.

int Set_Clock_Source(int nCardNo,int nSourceNo)
{
    DWORD dwData,dwClkEnableData,dwCardSrcPosition;

    //Read clock enable register.
    Read_Write_IPreg(CLKENA_off,&dwClkEnableData,FALSE);

    //Stop clock for all cards.Clk_Enable issues start_cmd.
    Clk_Enable(0x0ff,FALSE);

    //Set clock source bits.
    //Read clock source register.
    dwData=ReadReg(CLKSRC_off);
    
    //Shift 1 to required card no.bit.
    dwCardSrcPosition=0x03;
    dwCardSrcPosition = (dwCardSrcPosition << (nCardNo*2) ) ;

    //Clear clock source bits.
    dwCardSrcPosition = ~ dwCardSrcPosition;
    dwData &= dwCardSrcPosition; 
    
    //Set clock source bits for given card number.
    dwData |= (nSourceNo << (nCardNo*2) );

    //Write data into clock source register.
    WriteReg(CLKSRC_off,dwData);
    
    //TODO:Issue start_cmd to get this command into effect.

    //Retrieve original clock enable register contents.
    //Donot send start_cmd.So,use read_write_IPReg.
    Read_Write_IPreg(CLKENA_off,&dwClkEnableData,TRUE);

    //Issue start_cmd for immediate loading of CLKDIV,CLKENA,
    //CLKSRC.Check for command acceptance and return.
    return Assert_n_Poll_startcmd();
}

//--------------------------------------------------------------
/*8. Clk_Enable(int nCardNo,BOOL fEnable)

  *Called From:BSD.

  *Parameters :
          nCardNo:(i/p):Card whose clk source is to be changed.
		               //0x0ff = for all cards.
					   //Card number starts from 0 ie 
					   //0-15 for SD_MMC_MODE and 0-29 for MMC.
          fEnable:(i/p):1=Enable clock. 0=Disable clock.

  *Flow :-Read clock enable register. @0x010.
		 -Set/clear clock enable bits for given card number.
		 -Write clock enable register.
		 -Write command reg.with start_cmd,wait_prvdata_complete,
		  update_clock_regs_only.
         -Return.
*/
int Clk_Enable(int nCardNo,BOOL fEnable)
{
    DWORD  dwEnableData,dwRegData;

    //Read clock enable register. @0x010.
    dwRegData=ReadReg(CLKENA_off);
    
    //Set enable/disable data.
    if(nCardNo==0x0ff)
      //Command for all cards.
      dwEnableData=0x0000ffff;
    else
      {  //Command for single card.
        //Get bits to change. 
        dwEnableData = 0x1;
        
        //Shift 1 to required card no.
        dwEnableData = (dwEnableData << nCardNo );
      }
    
    //Set/clear clock enable bits for given card number.
    if(fEnable)
      //Set required bit.Enable clock for given card.
      dwRegData |= dwEnableData;
    else
      {
        dwEnableData = ~dwEnableData;   //eg 1101 1111
        //clear required bit.
        dwRegData &= dwEnableData;
      }
    
    //Write clock enable register.
    WriteReg(CLKENA_off,dwRegData);
    
    //Check for command acceptance and return.
    return Assert_n_Poll_startcmd();
}
//--------------------------------------------------------------
/*9. Assert_n_Poll_startcmd:Asserts start_cmd and poll for its 
  acceptance.

  Assert_n_Poll_startcmd()
  *Called From:Clk_Enable,Set_Clock_Source,Set_ClkDivider_Freq

  *Parameters :Nothing.

  *Flow :-Set CMD::start_cmd.
         -Call Poll_Cmd_Accept
		 -Return.
*/
int Assert_n_Poll_startcmd(void)
{
  DWORD dwRetVal,dwRegData ;
  
  dwRetVal =0;
  
  //Setup update_clk_registers_only and set start_cmd bit,
  //and wait_prvdata_complete.(bit31,21,13).
  dwRegData =0x080200000;// 0x080202000;//1000 0000 0010 0000 0010 0 0 0
  WriteReg(CMD_off,dwRegData);  
  return Poll_Cmd_Accept();
}
//--------------------------------------------------------------
/*10.Set_Reset_CNTRLreg:Sets or clears specified bit in control 
  register.Using structure concept,user can set 1 at selected
  bits.

  Set_Reset_CNTRLreg(DWORD dwRegBits,BOOL fSet)
  *Called From:BSD

  *Parameters :
         dwRegBits:(i/p):This DWORD has 1 at bits to be set or
		                 cleared.
		 fSet:(i/p): Set or clear. 1=set, 0=clear.

  *Flow :
*/
int  Set_Reset_CNTRLreg(DWORD dwRegBits,BOOL fSet)
{
  DWORD dwData = ReadReg(CTRL_off);
  if(fSet)  
    WriteReg(CTRL_off,(dwData |dwRegBits));
  else
    WriteReg(CTRL_off,(dwData & ~dwRegBits));
  
  return SUCCESS;
}

//--------------------------------------------------------------

//A.2.4. IRQ : Subgroups as below.
//==============================================================
/*1.Caller will set required mask bits.Here it is required just
  to copy data in Interrupt mask register.So,use function 
  Read_Write_IPreg ().

  **No need for seperate function.

  Set_Interrupt_Mask :Set masks for  IRQ.
*/

//--------------------------------------------------------------

/*A.2.5. DMA controls : Subgroups as below.
//==============================================================
	   1. Initialise DMA.
       2. Start DMA. 
       3. Stop DMA.
       4. Read DMA status.
//------------------------------------------------------------*/

//ssk June 2,03 

static struct file_operations sd_mmc_host_fops =
{ 
    	owner:    THIS_MODULE, 
    	open:     SD_MMC_Host_open,
    	release:  SD_MMC_Host_release,
    	ioctl:    SD_MMC_BUS_ioctl,    
    	read:    SD_MMC_Host_read,
    	write:   SD_MMC_Host_write
}; 

  static ssize_t
  SD_MMC_Host_read(struct file *file, char *buf, size_t buflen, loff_t *ppos)
  {
    return -ENODEV; 
  }

  static ssize_t
  SD_MMC_Host_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
  {
    return -ENODEV; 
  }

  static int
  SD_MMC_Host_open(struct inode *inode, struct file *file)
  {
    return 0;
  }

  static int
  SD_MMC_Host_release(struct inode *inode, struct file *file)
  {
    //printk(": SD_MMC_Host_release start \n");
    return 0;
  }

static struct miscdevice sd_mmc_sdio_dev = {
	MISC_DYNAMIC_MINOR,
	"sd_host",
	&sd_mmc_host_fops,
};
#ifdef DMA_ENABLE
static int sd_request_dma ( void *pisr)
{
    int cha;
    cha=dmac_channel_allocate((void *)pisr);
    if(cha<0)
    {
        printk("DMA channels are all busy now!\n");
        return -1;
    }
    return cha;
}
#endif
void register_dma_hook(void *dmaisr)
{
//注册sd、mmc驱动模块dma完成回调函数
    if (func[0]==NULL)
         func[0]=(void(*)(int))dmaisr;
}
EXPORT_SYMBOL(register_dma_hook);

void sd_register_detect_hook(int ncardno,void *detectisr)
{
    if(ncardno != 0 ){
        printk("warning! cardno must be zero, sd register detect hook fail!\n");
        return;
    }
    
    func[2]=(void(*)(int))detectisr;
}
EXPORT_SYMBOL(sd_register_detect_hook);

void sdio_register_dma_hook(void *dmaisr)
{
//注册sdio驱动模块dma完成回调函数    
    if (func[1]==NULL)
         func[1]=(void(*)(int))dmaisr;
}
EXPORT_SYMBOL(sdio_register_dma_hook);

void sdio_register_detect_hook(int ncardno,void *detectisr)
{
    if(ncardno != 0 ){
        printk("warning! cardno must be zero, register detect hook fail!\n");
        return;
    }    
    func[3]=(void(*)(int))detectisr;
}
EXPORT_SYMBOL(sdio_register_detect_hook);

void sdio_register_int_hook(int ncardno, void *intisr)
{
    if(ncardno != 0 ){
        printk("warning! cardno must be zero, register interrupt hook fail!\n");
        return;
    }
    if(func[4]==NULL)
        func[4]=(void(*)(int))intisr;
}
EXPORT_SYMBOL(sdio_register_int_hook);

void sdio_unregister_int_hook(int ncardno)
{
    if(ncardno != 0 ){
        printk("warning! cardno must be zero, unregister interrupt hook fail!\n");
        return;
    }    
    func[4]=NULL;
}
EXPORT_SYMBOL(sdio_unregister_int_hook);

unsigned int read_dma_channelnum(void)
{
    return sd_dma_cha;
}
EXPORT_SYMBOL(read_dma_channelnum);

void sd_sdio_dma_hook(int ret)
{
  // printk("Entry : sd_sdio_dma_hook!\n");
    if(func[0] != NULL)
         func[0](ret);
    if(func[1] != NULL)
         func[1](ret);    
}

void unregister_hook_all(void)
{
    func[0]=NULL;
    func[1]=NULL;
    func[2]=NULL;
    func[3]=NULL;
    func[4]=NULL;    
}

void unregister_dma_hook(void)
{
    func[0]=NULL;
    func[1]=NULL;   
}
EXPORT_SYMBOL(unregister_dma_hook);

static int sd_mmc_host_driver_init(void)
{
      unsigned long dwRetval;
      int pid,ret;
	int i;

      //map_adr = ioremap_nocache((unsigned int) SD_MMC_SDIO_BASE, mem_size);
      map_adr = IO_ADDRESS(SD_MMC_SDIO_BASE);
      if (!map_adr)
         return -ENOMEM;      

      G_ptBaseAddress = (DWORD)map_adr;

	ret = misc_register(&sd_mmc_sdio_dev);
	if (ret) 
	{ 
    		printk("sd_mmc_host ip can't register major number\n"); 
             //iounmap(map_adr);
             map_adr = 0;    	
             G_ptBaseAddress=0;
    	       return ret; 
	}

	//*******SDIO controller software reset**********************************
#ifdef _HI_3560_
	dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
      	dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);	
     	*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval; 
	for(i=0; i<0xff; i++);
     	dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
      	dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);	
     	*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval; 
#endif

#ifdef _HI_3511_
	dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
	dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);	
	*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
	for(i=0; i<0xff; i++);
	dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
	dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);	
	*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;
#endif
	INIT_WORK(&sd_mmc_int_workstruct,(void(*)(void*))SD_MMC_intr_handler,NULL);
	Read_Write_IPreg(RINTSTS_off,&dwRetval,READFLG);
	Read_Write_IPreg(RINTSTS_off,&dwRetval,WRITEFLG);

#ifdef INTR_ENABLE
      if (request_irq(MMC_INT_NUM, SD_MMC_host_intr_handler, 0, "SD_MMC", &SD_MMC_dev) != 0)
      	{
      	        misc_deregister(&sd_mmc_sdio_dev);  
   		  return -EBUSY;
      	}
#endif

/*
#ifdef _GPIO_CARD_DETECT_
      if(request_irq(8, card_detect_intr_handler, SA_SHIRQ, "SD_MMC_detect", &SD_MMC_dev) != 0)
      	{
      	         free_irq(24, &SD_MMC_dev); 
      	         misc_deregister(&sd_mmc_sdio_dev);
      	         //iounmap(map_adr);
      	         return -EBUSY;     
      	}
#endif

#ifdef _GPIO_CARD_DETECT_
      cartd_detect_int_init();    
#endif
*/
      unregister_hook_all();       

     FIFO_Depth = ( (ReadReg(FIFOTH_off) & 0x0fff0000) >> 16) + 1;

     //Start thread 2.
     pid = Create_Intr_Thread();
     if (pid < 0){
		free_irq(MMC_INT_NUM, &SD_MMC_dev); 
	#ifdef _GPIO_CARD_DETECT_		
		free_irq(GPIO_INT_NUM, &SD_MMC_dev);
	#endif
		misc_deregister(&sd_mmc_sdio_dev);    
		//iounmap(map_adr);
		return -1;
     }

      #ifdef DMA_ENABLE
		//dmac_module_init();
/*		dwRetval = kcom_getby_uuid(UUID_HI_DMAC_V_1_0_0, &kcom);
		if(dwRetval != 0) 
		{	
		printk(KERN_ERR "sd_mmc_host_init kcom_getby_uuid error!\n");	
		return -1;
		}
		kcom_hidmac = kcom_to_instance(kcom, struct kcom_hi_dmac, kcom);
*/
	if( KCOM_HI_DMAC_INIT() == 0 )
	{	
	      sd_dma_cha = sd_request_dma(sd_sdio_dma_hook);
	}else
	{
	      printk(KERN_ERR "sd_mmc_host_init kcom_getby_uuid error!\n");
	      return -1;
	}
	#endif

/*
#ifdef _HI_3511_
	dwRetval = sd_mmc_sdio_initialise();     
	if(dwRetval!=SUCCESS)
		printk("sd_mmc_host_init: Initialise Failed!\n");         
	else
		printk("Card stack initialised success!\n");
#endif	
*/

#ifdef _GPIO_CARD_DETECT_
	cartd_detect_int_init();    
      if(request_irq(GPIO_INT_NUM, card_detect_intr_handler, SA_SHIRQ, "SD_MMC_detect", &SD_MMC_dev) != 0)
      	{
      	         free_irq(MMC_INT_NUM, &SD_MMC_dev); 
      	         misc_deregister(&sd_mmc_sdio_dev);
      	         //iounmap(map_adr);
      	         return -EBUSY;     
      	}
#endif

     return 0;
}

static void sd_mmc_init_gpio(void);

int __init
sd_mmc_host_init(void)
{
     _ENTER();

    sd_mmc_init_gpio();
    sd_mmc_enable_card_power(ENABLE);
    if(sd_mmc_host_driver_init()){
    	 sd_mmc_enable_card_power(DISABLE);
    	 return -1;
    }
 
#ifdef _GPIO_CARD_DETECT_	
      card_detect_int_enable(ENABLE);
#endif
     _LEAVE();

	//print the module version
	printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
	
     return 0;
}

static void sd_mmc_init_gpio(void)
{
	unsigned long result;

#ifdef _HI_3560_
	sd_readl(SDIO_EN_SCTL_ADDR, result);
      	result &= ~(0x1<<SDIO_EN_SCTL_ADDR_LOC);   
 	sd_writel(SDIO_EN_SCTL_ADDR, result);     
	//3560 gpio select
	sd_readl(SDIO_GPIO_SELECT, result);
      	result &= ~(0x1<<SDIO_GPIO_SELECT_LOC1);   
 	result &= ~(0x1<<SDIO_GPIO_SELECT_LOC2);   
 	sd_writel(SDIO_GPIO_SELECT, result);    
	//sd_readl(SDIO_EN_SCTL_FPGA_ADDR,result);
      	//result &= ~(0x1<<17);
 	//sd_writel(SDIO_EN_SCTL_FPGA_ADDR,result);   
#endif

	sd_readl(SDIO_FDIV_SCTL, result);
      	result |= (0x2<<SDIO_FDIV_SCTL_LOC);                 		/*set sdio sys frequency divider to 2*(n+1), 132/4=22MHz*/
 	sd_writel(SDIO_FDIV_SCTL, result); 
//	sd_readb(CARD_DETECT_GPIO_DIR, ret);
//       ret &= ~(0x1<<0);                   /*set  card detect pin dir to in*/
// 	sd_writeb(CARD_DETECT_GPIO_DIR, ret); 
#ifdef _GPIO_CARD_DETECT_
 	GPIO_CARD_DETECT_IN;
#endif
#ifdef _GPIO_WR_PROTECT_
 	GPIO_WRITE_PRT_IN;
#endif
#ifdef _GPIO_POWEREN_
 	GPIO_POWER_EN_OUT;
#endif
}

void sd_mmc_enable_card_power(int flag)
{
#ifdef _POWER_EN_NEED_	
	#ifdef _GPIO_POWEREN_        	
		GPIO_POWER_EN_OUT;
		if (flag)
			GPIO_POWER_EN_ENABLE;
		else
			GPIO_POWER_EN_DISABLE;
	#else
		if(flag)
			WriteReg(PWREN_off, 0x1); 
		else
			WriteReg(PWREN_off, 0x0); 
	#endif
#endif
}

#ifdef _GPIO_CARD_DETECT_
void card_detect_int_enable(int flag)
{
	unsigned char result;
	sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x410), result); 
	if(flag)
		result |= (0x1<<CARD_DETECT_GPIO_PNUM);    
	else
		result &= ~(0x1<<CARD_DETECT_GPIO_PNUM); 
	sd_writeb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x410), result); 
}

void card_detect_clear_intflag(void)
{
	unsigned char result;
	sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x41c), result); 
	result |= (0x1<<CARD_DETECT_GPIO_PNUM);
	sd_writeb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x41c), result);   
}


static void cartd_detect_int_init(void)
{
      unsigned char result;
      sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x404),result); 
       result &= ~(0x1<<CARD_DETECT_GPIO_PNUM);     
      sd_writeb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x404),result);    

      sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x408),result); 
       result |= (0x1<<CARD_DETECT_GPIO_PNUM);       
      sd_writeb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x408),result);    

      card_detect_clear_intflag();
}
#endif

void __exit
sd_mmc_host_exit(void)
{
     int ret;
     unsigned long dwTempData=0;
     
    //Since we have modified FIFO reg 
     dwTempData = ReadReg(FIFOTH_off);
     dwTempData &= 0xf000ffff;
     FIFO_Depth <<= 16;
     dwTempData |= (FIFO_Depth - 1);
     WriteReg(FIFOTH_off,dwTempData);
  
     ret = Kill_Intr_Thread();

     //dma_module_exit();
//     kcom_put(kcom);
#ifdef INTR_ENABLE
     //Release interrupt.
     
      free_irq(24, &SD_MMC_dev);

#endif     

     //Unregister device.
     misc_deregister(&sd_mmc_sdio_dev);
#ifdef _GPIO_CARD_DETECT_		
	free_irq(GPIO_INT_NUM, &SD_MMC_dev);
#endif
#ifdef DMA_ENABLE
	dmac_channel_free(sd_dma_cha);
	KCOM_HI_DMAC_EXIT();
#endif
     //Release virtual memory.
     if (map_adr) {
       //iounmap(map_adr);
       map_adr = 0;
     G_ptBaseAddress = 00;
     }
   
     //If module is removed,then global variables which are initialised
     //in init(), should be reset.
     G_ptBaseAddress = 00;
     FIFO_Depth = 1;//Depth is 1 unsigned long. 
     FIFO_Width = 32;//width of 1 unsigned long.

    _LEAVE();
}


#ifdef INTR_ENABLE
static irqreturn_t SD_MMC_host_intr_handler(int irq, void *dev_id ,struct pt_regs * regs)
{
//     unsigned long dwBitno=4;
     //weimx modify 
//      set_ip_parameters(CLEAR_CONTROLREG_BIT,&dwBitno);     //disable irq  
	SD_MMC_intr_handler();		
	return IRQ_HANDLED;
}

/*
 * IRQ handler, dispatches interrupts to the Tx, Rx, or Device handler
 */
void SD_MMC_intr_handler(void)
{
	ISR_Callback();
	return ;
}
#endif

#ifdef _GPIO_CARD_DETECT_
static irqreturn_t card_detect_intr_handler(int irq, void *dev_id ,struct pt_regs * regs)
{
	unsigned char intstatus=0;
	sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x418), intstatus); 
	if(intstatus & (0x01<<CARD_DETECT_GPIO_PNUM))
	{
		card_detect_int_enable(DISABLE); 
		wake_up_interruptible(&ksdmmc_wait);
		card_detect_clear_intflag();
		card_detect_int_enable(ENABLE);	   

		return IRQ_HANDLED;
	}
	else
	return IRQ_NONE;
	}
#endif

module_init(sd_mmc_host_init);
module_exit(sd_mmc_host_exit);
MODULE_AUTHOR("hisilicon");
MODULE_LICENSE("GPL");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);
MODULE_DESCRIPTION("MMC bus protocol interfaces");

