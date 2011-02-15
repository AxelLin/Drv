/***************************************************************
    PROJECT    :  "SD-MMC IP Driver" 
    ------------------------------------
    File       :   Bus_Drv.c
    Start Date :   03 March,2003
    Last Update:   18 Dec.,2003

    Reference  :   registers.pdf

    Environment:   Kernel mode

    OVERVIEW
    ========
    This file contains the algorithm for bus driver functions.
    Bus driver is SD-MMC protocol specific driver.It 
    communicates with SD-MMC IP ,through Host Driver.


    REVISION HISTORY:

***************************************************************/
#define FALSE  0
#define TRUE   1

#define NULL   00

extern unsigned long FIFO_Depth;


#include "../CommonFiles/pal.h"
#include "../CommonFiles/sd_mmc_const.h"
#include "sd_mmc_bus.h" //Constants,structures and inlines.
#include "Utility_Fs.h"
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include "../Host_Drv/w_debug.h"
#include "../Host_Drv/host.h"

#ifdef HSMMC_SUPPORT
  #include "HSMMC.c"
#endif

#ifdef HSSD_SUPPORT
  #include "HSSD.c"
#endif

//Declarations or thread2.
static int ksdmmc_pid = 0;            /* PID of ksdmmc */
static DECLARE_COMPLETION(ksdmmc_exited);
DECLARE_WAIT_QUEUE_HEAD(ksdmmc_wait);
 DECLARE_WAIT_QUEUE_HEAD(command_wait);
 DECLARE_WAIT_QUEUE_HEAD(data_wait);
//External functions:
#ifdef DEBUG_DRIVER
  extern void Register_Read_Callback(void *Arg);
  extern void Register_Write_Callback(void *Arg);
#endif
extern int  Read_Write_IPreg(int nRegOffset,unsigned long *pdwRegData,
                             int fwrite);
extern int  set_ip_parameters(int nCmdOffset, unsigned long *pdwRegData);
extern int  Set_Clock_Source(int nCardNo,int nSourceNo);
extern int  Clk_Enable(int nCardNo,int fenable);
extern int  Set_ClkDivider_Freq(int nclksource,int nDivider,
                      unsigned char *pbprevval);
extern int  Set_Reset_CNTRLreg(unsigned long dwRegBits,int fset);
extern int  Read_Write_MultiRegdata( int nRegOffset, int nByteCount,
                           unsigned char *pbdatabuff, int fwrite);
extern int  Read_Write_FIFOdata(int nByteCount, unsigned char *pbdatabuff,
                           int fwrite, int fKernSpaceBuff);
extern int  Send_SD_MMC_Command(COMMAND_INFO *ptCmdInfo);
extern int  Read_Response(RESPONSE_INFO *ptResponseInfo,int nRespType);

extern void (*func[4])(void);
//extern void __iomem* map_adr;
extern unsigned int map_adr;
//extern int sd_dam_cha;

//weimx add "volatile" on 20070905 
#define WriteReg(x,z)   *((volatile DWORD*)((volatile BYTE*)map_adr+x))=(DWORD)z
#define ReadReg(x)      *((volatile DWORD*)((volatile BYTE*)map_adr+x))


//Switches,global flags used in driver.
//==============================================================
//Global variables.
//weimx add volatile on 20070914
volatile IP_STATUS_INFO tIPstatusInfo;  
volatile CURRENT_CMD_INFO tCurrCmdInfo;
volatile INPROCESS_DATA_CMD tDatCmdInfo;
volatile unsigned long G_NEWRCA=0;
volatile unsigned long G_dwIntrStatus=0; //Interrupt status register read on intr.
volatile unsigned long G_SEND_ABORT=0;
volatile unsigned long G_PENDING_CARD_ENUM=0;
volatile unsigned long G_ABORT_IN_PROCESS=0;
volatile int   G_CARDS_CONFIGURED=0;
volatile int   G_CMD40_In_Process=0;
volatile unsigned int intr_thread_kill=0;


//Function prototypes
int   Create_Intr_Thread(void);
int   Kill_Intr_Thread(void);
int   sd_mmc_sdio_initialise(void);
int   Enumerate_card(int nPort_No);
int   Enumerate_MMC_card_Stack(void);         
int   Enumerate_All_MMC_Cards(void);
int   Enumerate_SD_card_Stack(void);
void  Set_timeout_values(void);
int   Set_FIFO_Threshold(unsigned long dwFIFOThreshold,int fRcvFIFO,
                         int fDefault);
void  Set_debounce_val(unsigned long dwDebounceVal);
int   Enumerate_MMC_Card(int nCardNo,int nPortNo,unsigned long dwStartRCA);
int   set_clk_frequency(int nclksource, int ndividerval,
                        unsigned char *pbprevval);
int   clk_enable_disable(int nCardNo, int fenable);
int   form_send_cmd(int nCardNo,int nCmdID,unsigned long dwcmdarg,
                    unsigned char *pbcmdrespbuff,int nflags);
void  Increment_Card_Count(int ncardType);
int   Send_ACMD41(int nCardNo,unsigned long dwOCR, unsigned char* pbR3Response);
int   sd_mmc_sdio_initialise_SD(int nPortNo,unsigned long dwRCA,int *cardtype);
int   sd_mmc_sdio_initialise_SDIO(int nPortNo,unsigned long dwRCA,int *pnSDIO_type,unsigned long dwResp);
int   Validate_SD_CMD3_Response(int nCard_Status);
int   SDIO_Reset(int nCardNo);
int   Identify_Card_Type(int CardNo,unsigned long *pdwResp);
unsigned long Calculate_Data_Timeout(void);
int   Set_default_Clk(void);
int   Check_All_Standby(void);
unsigned long Assign_Clk_Sources(int nCardType1,int nCardType2,
                         unsigned long dwSourceNo,int nClkSrcs,//nCardsPerSource,
                         unsigned long dwClkSourceRegData);

int   Read_ClkSource_Info(CLK_SOURCE_INFO *tpClkSource,int nCardNo);
int   sd_mmc_sdio_rw_timeout(unsigned long *pdwtimeoutval, int fwrite);
int   sd_mmc_send_command(unsigned long dwcmdreg,unsigned long dwcmdarg, 
                   unsigned char *pbcmdrespbuff);
int   power_on_off(int nCardNo, int fpwron);
int   Set_Clk_Source(int nCardNo, int nsourceno);
void  Enable_OD_Pullup(int nEnable) ;
int   sd_set_mode(int nCardNo, int fmode4bit);
int   Send_CMD55(int nCardNo);
int   Set_Interrupt_Mode(void (*CMD40_callback)(unsigned long dwRespVal,unsigned long dwReturnValue),
                         unsigned long dwIntrStatus);

int   Send_CMD40_response(void);
int   sd_mmc_send_raw_data_cmd(unsigned long dwcmdregParams,
                        unsigned long dwcmdargReg ,
                        unsigned long dwbytecount,
                        unsigned long dwblocksizereg,
                        unsigned char  *pbcmdrespbuff,
                        unsigned char  *pberrrespbuff,
                        unsigned char  *pbdatabuff,
                        void  (*callback)(unsigned long dwerrdata),
                        int   nnoofretries,
                        int   nflags);
int   SetBlockLength(int nCardNo,unsigned long dwBlkLength, int nWrite, int nSkipPartialchk);
int   Read_TotalPorts(int nCardType,int *pnArrPortNo,
                      int nArraySize,int *pnTotalPorts);
int   Read_total_cards_connected(void);
int   Read_Op_Mode(void);
int   sd_mmc_sdio_get_id(id_reg_info *ptidinfo);
int   sd_mmc_read_rca(int nCardNo);
int   sd_mmc_sdio_get_card_type(int nCardNo) ;
int   sd_mmc_sdio_get_currstate (int nCardNo) ;
unsigned long long  sd_mmc_get_card_size(int nCardNo);
int   Get_Card_Attributes (int nCardNo);

int   sdio_rdwr_direct(int nCardNo, int nfunctionno,
                       int nregaddr,
                       unsigned char *pbdata,
                       unsigned char *pbrespbuff,
                       int fwrite,
                       int fraw,
                       int  nflags);

int   sdio_rdwr_extended(int nCardNo, int nfunctionno,int nregaddr,
                         unsigned long dwbytecount, 
                         unsigned char  *pbdatabuff,
                         unsigned char  *pbcmdrespbuff,
                         unsigned char  *pberrrespbuff,
                         int   ncmdoptions,
                         void  (*callback)(unsigned long dwerrdata),
                         int   nnoofretries,
                         int   nflags);

int   sdio_read_wait(int ncardno,int fAssert);

unsigned long sdio_enable_disable_function(int ncardno,int nfunctionno,
                                  int fenable);
 
unsigned long sdio_enable_disable_irq(int ncardno,int nfunctionno,
                             int fenable);
//Function prototypes for New SDIO functions..
unsigned long  sdio_change_blksize (int ncardno, int nfunctionno, 
                            unsigned short wioblocksize);
unsigned long Send_SDIO_DummyRead(int ncardno, unsigned char *StatusData);
unsigned long Get_Data_fromTuple (int ncardno, int nfunctionno,
                          unsigned int nTupleCode, 
                          unsigned int nFieldOffset,
                          unsigned int nBytes,  
                          unsigned char* pBuffer, 
                          unsigned int nBuffsize , 
                          int fBuffMode);
unsigned long sdio_enable_masterpwrcontrol (int ncardno, int fenable);
unsigned long sdio_change_pwrmode_forfunction (int ncardno, int nfunctionno, 
                                  int flowpwrmode);

int   read_write_blkdata(int ncardno, unsigned long dwaddr,
                         unsigned long dwdatasize,
                         unsigned char  *pbdatabuff,
                         unsigned char  *pbcmdrespbuff,
                         unsigned char  *pberrrespbuff,
                         void  (*callback)(unsigned long dwerrdata),
                         int   nnoofretries,
                         int   nflags);

int   Partial_Blk_Xfer(int ncardno,unsigned long dwaddress, unsigned long dwTotalBytes, 
                       unsigned char  *pbdatabuff,
                       unsigned char  *pbcmdrespbuff,
                       unsigned char  *pberrrespbuff,
                       int   nnoofretries,
                       int   nflags);

int   Blksize_Data_Transfer(int ncardno, unsigned long dwaddr,
                            unsigned long dwdatasize,
                            unsigned char  *pbdatabuff,
                            unsigned char  *pbcmdrespbuff,
                            unsigned char  *pberrrespbuff,
                            void  (*callback)(unsigned long dwerrdata),
                            int   nnoofretries,
                            int   nflags);

void  Set_Auto_stop(int ncardno,COMMAND_INFO *ptCmdInfo);

int   read_write_streamdata (int ncardno, unsigned long dwaddr,
                             unsigned long dwdatasize,
                             unsigned char  *pbdatabuff,
                             unsigned char  *pbcmdrespbuff,
                             unsigned char  *pberrrespbuff,
                             void  (*callback)(unsigned long dwerrdata),
                             int nnoofretries,
                             int nflags);

int   Form_Data_Command(int ncardno, unsigned long dwDataAddr,
                        unsigned long dwdatasize, int nflags,
                        COMMAND_INFO *ptCmdInfo,
                        int nTransferMode);

int   sd_mmc_stop_transfer(int ncardno);

int   Read_Write_FIFO(unsigned char *pbdatabuff,int nByteCount,int fwrite,
                      int fKerSpaceBuff);
unsigned long Check_TranState(int ncardno);

int   sdio_abort_transfer(int ncardno,int nfunctionno);
int   sdio_suspend_transfer(int ncardno, int nfunction,
                       unsigned long *pdwpendingbytes);
int   sdio_resume_transfer(int ncardno, int nfunctionno,
                      unsigned long dwpendingdatacount, 
                      unsigned char  *pbdatabuff,
                      unsigned char  *pbcmdrespbuff,
                      unsigned char  *pberrrespbuff,
                      void  (*callback)(unsigned long dwerrdata),
                      int nnoofretries,
                      int nflags);

int   Form_SDIO_CmdReg(COMMAND_INFO *ptCmdInfo,int ncardno,
                       int nCMDIndex,int nflags);
int   sd_mmc_wr_protect_carddata(int ncardno,unsigned long dwaddress,unsigned char *pbrespbuff,
                          int fprotect);
int   sd_mmc_set_clr_wrprot(int ncardno,int ftemporary, int fset);
int   Form_CSD_Write_Cmd(COMMAND_INFO *ptCmdInfo,int ncardno);
int   Default_CmdReg_forDataCmd(COMMAND_INFO *ptCmdInfo,
                                int ncardno,int nCmdIndex,
                                int fwrite);
int   sd_mmc_get_wrprotect_status (int ncardno, unsigned long dwaddress, 
                            unsigned long *pdwstatus);
int   sd_mmc_erase_data(unsigned char *pbrespbuff,int ncardno,
                      unsigned long dwerasestart,unsigned long dweraseend);
int   sd_mmc_card_lock(int ncardno,card_lock_info *ptcardlockinfo,
                unsigned char *pbrespbuff);
void  Set_CardLock_Attributes(int ncardno,unsigned char bLockCmd);
int   Mask_Interrupt(unsigned long dwIntrMask,int fsetMask);
int   Validate_Command (int ncardno,unsigned long dwaddress,unsigned long dwParam2,
                        int nCommandType);
int   sd_mmc_event_Handler(void *pdwIntrStatus);

void  ISR_Callback(void);
int   Send_cmd_to_host(COMMAND_INFO *ptCmdInfo,
                       unsigned char         *pdwCmdRespBuff,
                       unsigned char         *pdwErrRespBuff,
                       unsigned char         *pdwDataBuff,
                       void        (*callback)(unsigned long dwerrdata),
                       int          nflags,
                       int          nnoofretries,
                       int         fDataCmd,
                       int         fwrite,
                       int         fKernDataBuff);

int   Validate_Response(int nCmdIndex, unsigned char *pbResponse,
                        int ncardno,int nCompareState);
void  Form_STOP_Cmd(COMMAND_INFO *ptCmdinfo);
void  sd_mmc_copy_csd(CSDreg *ptcsd,  int ncardno);
int   Check_Card_Eject(int nslotno);
int   sd_mmc_check_card(int nslotno);
int   sdio_check_card(int ncardno);
void  Card_Detect_Handler(void);

void  sdio_copy_cccr(cccrreg *ptccccr,  int ncardno);
void read_sdio_capability(int ncardno, card_capability *capability);
int sd_mmc_check_ro(int nslotno);



//Exported functions.
EXPORT_SYMBOL(read_write_blkdata);
EXPORT_SYMBOL(sd_mmc_check_ro);
EXPORT_SYMBOL(sd_mmc_sdio_initialise);
EXPORT_SYMBOL(sd_mmc_copy_csd);
EXPORT_SYMBOL(sd_mmc_check_card);
EXPORT_SYMBOL(sdio_check_card);
EXPORT_SYMBOL(read_write_streamdata);
EXPORT_SYMBOL(sd_mmc_stop_transfer);
EXPORT_SYMBOL(sd_mmc_read_rca);
EXPORT_SYMBOL(sd_mmc_get_card_size);
EXPORT_SYMBOL(sd_mmc_wr_protect_carddata);
EXPORT_SYMBOL(sd_mmc_set_clr_wrprot);
EXPORT_SYMBOL(sd_mmc_get_wrprotect_status);
EXPORT_SYMBOL(sd_mmc_erase_data);
EXPORT_SYMBOL(sd_mmc_card_lock);
EXPORT_SYMBOL(sd_mmc_send_command);
EXPORT_SYMBOL(sd_mmc_send_raw_data_cmd);
EXPORT_SYMBOL(sd_mmc_sdio_rw_timeout);
EXPORT_SYMBOL(sd_mmc_sdio_get_id);
EXPORT_SYMBOL(sd_mmc_sdio_get_card_type);
EXPORT_SYMBOL(sd_mmc_sdio_get_currstate);


EXPORT_SYMBOL(sdio_rdwr_direct);
EXPORT_SYMBOL(sdio_rdwr_extended);
EXPORT_SYMBOL(sdio_abort_transfer);
EXPORT_SYMBOL(sdio_suspend_transfer);
EXPORT_SYMBOL(sdio_resume_transfer);
EXPORT_SYMBOL(sdio_read_wait);
EXPORT_SYMBOL(sdio_enable_disable_function);
EXPORT_SYMBOL(sdio_enable_disable_irq);
EXPORT_SYMBOL(sdio_change_blksize);
EXPORT_SYMBOL(sdio_change_pwrmode_forfunction);
EXPORT_SYMBOL(sdio_enable_masterpwrcontrol);
EXPORT_SYMBOL(sdio_copy_cccr);
EXPORT_SYMBOL(read_sdio_capability);

//==============================================================
//Actual functions called by IOCTL().
//--------------------------------------------------------------
/*
B. Bus driver.: SD/MMC specific driver.
B.1.Basic Features :
 - Provides each functionality required for SD/MMC card 
   interface.
 - Acts as interface between Host driver and Higher level 
   driver.
 - Set debug status buffer.

B.2.Functions : Groups as below.
    
1. Basic.Set IP Base address.
2. IP register read/write.
3. General operational commands.
4. Read IP info.
5. Read card info.
6. Read response.
7. SDIO functions.
8. Data transfer commands.Data buffer supplied by caller.
9. SDIO Data transfer commands.
10.Card write protect/erase/lock functions.
11.Interrupt handlers.
12.DMA functionality.
13.Internal functions
   a.Thread2 functions
   b.Data analysis/Error checking
   c.General functions.
   d Debug function.
14.Driver specific.

*/
//--------------------------------------------------------------
/*B.1.Basic.Set IP Base address.:
      //a.Initialisation.      
      Set adapter address
      sd_mmc_sdio_initialise
      Enumerate_MMC_card_Stack
      Enumerate_SD_card_Stack
      Enumerate_card
      Enumerate_MMC_Card

      //b.Setting IP parameters.
      Set_timeout_values
      Calculate_Data_Timeout
      MiniDividerVal
      Set_debounce_val
      Set_FIFO_Threshold
      Enable_OD_Pullup
      Set_default_Clk
      Set_fod_clock 

      //c.Close
      Check_Cmd_Pending 
      DeallocateAllMemory
*/
//--------------------------------------------------------------
/*B.1.2 sd_mmc_sdio_initialise() : sd_mmc_sdio_initialise the IP,driver structures,and enumerate the card stack.

   int sd_mmc_sdio_initialise(void)

  *Called from:module_init() which is called when module is invoked.

  *Parameters: Nothing

  *Return value : 0

  *Flow : 
       -sd_mmc_sdio_initialise structures used in driver.
        for MMC_ONLY mode,enable open drain pullup.
       -Read HCON register to get the no.of cards for which IP is configured
       -Enable power to all cards.Give ramp up time.
       -Set interrupt mask reg.Mask interrupts due to response
        error,Resp.CRC.
       -Clear RINTSTS reg.
       -Configure control reg.
       -If no card is connected,print message.
        else Enumerate_Card_Stack.
       -sd_mmc_sdio_initialise IP.
           Set timeout values.
           Set_debounce_val
           Set_FIFO_Threshold
           Set card type register.
       -Set Card Type(if SD MMC mode)
       -Remove open drain pullup after enumeration.(for MMC_ONLY mode)
       -Return SUCCESS always.
*/
int  sd_mmc_sdio_initialise(void)
{
  ControlReg  Cntrlreg={0X00};
  unsigned long dwData,dwretval;
  #ifdef _GPIO_CARD_DETECT_
    unsigned char cardstatus;
  #endif
 _ENTER();

  //Clear global structures.
  memset((unsigned char *)&tIPstatusInfo,0,sizeof(tIPstatusInfo));
  //since Card_Info array is allocated all structures are consecutive 
  memset(&Card_Info[0],0,sizeof(struct Card_info_Tag)*MAX_CARDS);

  memset((unsigned char *)&tCurrCmdInfo,0,sizeof(tCurrCmdInfo));
  memset((unsigned char *)&tDatCmdInfo,0,sizeof(tDatCmdInfo));
  
  /*sd_mmc_sdio_initialise IP.:CTRL,*/
  dwretval=SUCCESS;

  //CTRL register.
  Cntrlreg.dwReg = RESET_FIFO_DMA_CONTROLLER_Data;//Reset ALL.

 Cntrlreg.Bits.dma_enable=0;

  Cntrlreg.Bits.int_enable=0;//Disable interrupt.

#ifdef INTR_ENABLE
  //Default value is 0.
  Cntrlreg.Bits.int_enable=1;//Enable interrupt.
#endif
  //sd_mmc_sdio_initialise should be the first function to be called
  //after invoking driver.
  Read_Write_IPreg(HCON_off,&dwData,READFLG);
  G_CARDS_CONFIGURED = ((dwData& 0x03e) >> 1) + 1;

  //Enable power to all cards.HCON::NUM_CARDS give number of
  //cards for which IP is configured
  dwData=0x0000001;      //only open the power of NO1 slot

 //wait.To give time for power up for card.
  //set mask.
 // dwData = 0x01f3bd;
  dwData = 0x01ffff;

  Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);
  //Clear RINTSTS.
  Read_Write_IPreg(RINTSTS_off,&dwData,READFLG);
  Read_Write_IPreg(RINTSTS_off,&dwData,WRITEFLG);
  //Write ctrl reg.
  Read_Write_IPreg(CTRL_off,&Cntrlreg.dwReg,WRITEFLG);

  do
  {
      dwData=00;
      Read_Write_IPreg(CTRL_off,&dwData,READFLG);
  }while (dwData & (0x00000001));

  //Set timeout values.For SDIO maximum timeout is required. 
  Set_timeout_values();
  
#ifdef _GPIO_CARD_DETECT_
    //sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x004),cardstatus);
    GPIO_CARD_DETECT_IN;
    cardstatus = GPIO_CARD_DETECT_READ;
    if(!cardstatus)
#else        
    dwData = 0;
      Read_Write_IPreg(CDETECT_off,&dwData,READFLG);
    if((dwData & 0x0ffff) != ~(0xfffffffe << (G_CARDS_CONFIGURED-1)))  
#endif        
             dwretval=Enumerate_SD_card_Stack();
      else
             printk("No card connected.\n");
      
  //if enumeration fails.
  if(dwretval!=SUCCESS) return dwretval;

  /*Post enumeration settings.  */

  /*sd_mmc_sdio_initialise IP.*/
  
  //Set_debounce_val
  Set_debounce_val(DEFUALT_DEBNCE_VAL);

  //Set_FIFO_Threshold
  Set_FIFO_Threshold(00,00,TRUE);//Set default values flag.

/*the sd_set_mode function will do later in the function:mmc_select_card, for some card can't  be set!*/
/*  
   if(Card_Info[0].Card_type!=MMC && Card_Info[0].Card_type != SDIO_IO)
   {
      if(sd_set_mode(0, TRUE))
              printk("set sd mode as 4bit mode,fail!\n");; 
   }
   else     */
   if(Card_Info[0].Card_type == SDIO_IO)
   {
       dwData=0;
       set_ip_parameters(SET_SDWIDTH_FOURBIT,&dwData);
   }


   Read_Write_IPreg(FIFOTH_off,&dwData,READFLG); 
   dwData |= 0x20000000;
   Read_Write_IPreg(FIFOTH_off,&dwData,WRITEFLG);

  _LEAVE();
  return SUCCESS ;
}

int sd_mmc_check_ro(nsoltno)
{
#ifdef _GPIO_WR_PROTECT_   
    unsigned char cardstatus;
//    sd_readb((GPIO_BASE_ADDR(CARD_WRITE_PRT_GNUM)+0x008), cardstatus);
    GPIO_WRITE_PRT_IN;
    cardstatus = GPIO_WRITE_PRT_READ;
    return cardstatus;
#else
    unsigned long cardstatus;
    Read_Write_IPreg(WRTPRT_off, &cardstatus, READFLG); 
    return (cardstatus & 0x01);
#endif
}

//--------------------------------------------------------------
static int sd_mmc_thread(void *__hub)
{
#ifndef _GPIO_CARD_DETECT_     //not gpio detect mode
    unsigned long dwData=0;
#endif 
    unsigned long i, dwRetval;
  /* Setup a nice name */
  strcpy(current->comm, "k_sdmmc");

    /*the first time load the sdio module, must initialize the below job!*/
 #ifdef _HI_3560_  //3560 branch
    dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
    dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);    
    *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
    for(i=0;i<0xff;i++);
    dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
    dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);            
    *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;
    for(i=0;i<0xff;i++);
    printk("Card stack initializing, please wait ................................\n\n");
    dwRetval = sd_mmc_sdio_initialise();         
    printk("Check card stack end,now you can do your job......................\n");
#endif

#ifdef _HI_3511_  //3511 branch
    dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
    dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);    
    *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
    for(i=0; i<0xff; i++);
    dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
    dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);    
    *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;
    for(i=0;i<0xff;i++);    

    printk("Card stack initializing, please wait ................................\n\n");
    dwRetval = sd_mmc_sdio_initialise();         
    printk("Check card stack end,now you can do your job......................\n");
#endif

  do 
    {
          HI_TRACE(3, "Entry : sd_mmc_thread!\n");
          
    #ifdef _GPIO_CARD_DETECT_          
           Card_Detect_Handler();
    #else
//        if(G_PENDING_CARD_ENUM==TRUE)
        {
            //Need to grab semaphore to prevent other
            //function issuing command ??
            //To prevent furhter card detect interrupt,mask it.(bit0)
            Read_Write_IPreg(INTMSK_off,&dwData,READFLG);
            dwData &=0xfffffffe;
            Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);     //disable card detect int

            G_PENDING_CARD_ENUM=FALSE;

            ////weimx 20071123 add the below lines*******SDIO controller software reset**********************************
            dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
            dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);    
            *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
            for(i=0;i<0xff;i++);
            dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
            dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);    
            *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
                for(i=0;i<0xff;i++);

            printk("Card stack checking, please wait ................................\n\n");
            dwRetval = sd_mmc_sdio_initialise();         
            printk("Check card stack end,now you can your job......................\n");
            if(dwRetval != SUCCESS)
                printk("Insert card, card stack initialize failed!\n");         
            else
            {
                if(func[2] != NULL)
                {
                    func[2]();     //block device detect hook func
                }

                if(func[3] != NULL)
                {
                    func[3]();    //wifi device detect hook func
                }
            }

            //unmask the interrupt  
            Read_Write_IPreg(INTMSK_off,&dwData,READFLG);
            dwData |=0x00000001;
            Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);    //enalbe card detect int

        }
    #endif
          interruptible_sleep_on(&ksdmmc_wait);   
    } while (!intr_thread_kill);

  HI_TRACE(2,"sd_mmc_thread exiting!!\n");
  complete_and_exit(&ksdmmc_exited, 0);
}
//Called during module_init
int Create_Intr_Thread(void)
{
  //Create thread : sd_mmc_event_Handler.
  int pid;

  pid = kernel_thread(sd_mmc_thread, NULL,
                      CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
  if (pid >= 0) {
        ksdmmc_pid = pid;
        return 0;
  }
  return -1;
}

//Called during module_exit
int Kill_Intr_Thread(void)
{
  intr_thread_kill=1;
  wake_up_interruptible(&ksdmmc_wait);
  wait_for_completion(&ksdmmc_exited);
  return 0;
}


//--------------------------------------------------------------
/*B.1.3. Enumerate_MMC_card_Stack.: Enumerates all cards in MMC stack.

  Enumerate_MMC_card_Stack().

  *Called from: BSD::sd_mmc_sdio_initialise().

  *Parameters:Nothing.

  *Return value : SUCCESS always

  *Flow:
       -sd_mmc_sdio_initialise global variable for RCA.
       -Set enumeration clock frequency. to Fod value.
       -Enable clk0.It is required to send any cmd to cards.
       -call Enumerate_All_MMC_Cards().
       -Update total cards count.
       -Set clock frequency for MMC mode.
       -Enable the clock for port0.
       -Return SUCCESS always
*/

//**** If MMC card doesnot support card detect output,then
//IP will not generate interrupt for this.
//In this case,user has to specifically call Enumerate
//routine.In this case,enumerating single card will be difficult.
//So,user should add the card and enumerate whole card stack again.
int Enumerate_MMC_card_Stack(void)
{   
  unsigned long dwRetVal; unsigned char bOrgVal;
   
  //sd_mmc_sdio_initialise G_NEWRCA.
  G_NEWRCA=00;

  //Set clock frequency = fod. Assuming clk=25/2=12.5 MHz.clkdiv is 0x14.
  dwRetVal = set_clk_frequency(CLKSRC0,(CIU_CLK/(MMC_FOD_VALUE*2)),&bOrgVal);

  //Enable clk0.Required to send any cmd to cards.
  clk_enable_disable(0,TRUE);

  //MMC only mode,uses port 0 only.Enumerate MMC stack.
  Enumerate_All_MMC_Cards();

  //Only MMC cards will be connected in MMC_ONLY mode.
  tIPstatusInfo.Total_Cards = tIPstatusInfo.Total_MMC_Cards;

  //Set clock frequency for MMC mode.Set clk source0=12.5Mhz.
  dwRetVal = set_clk_frequency(CLKSRC0,MMC_FREQDIVIDER,// 1
                                 &bOrgVal);
  //Enable the clock for port0.
  dwRetVal = clk_enable_disable(0, TRUE);

  return SUCCESS;
}
//--------------------------------------------------------------
/* Enumerate_All_MMC_Cards : Enumerates all MMC cards.

   Enumerate_All_MMC_Cards()

   *Called from : BSD::Enumerate_MMC_card_Stack()

   *Parameters  : Nothing

   *Return Value: SUCCESS always

   *Flow:
       -Send CMD0.
        If command not taken by IP,repeat it.
        After retry count over,return error.
        
       -Send CMD1 with arg 0, to read voltage range supported by card.

       -Send CMD1 with voltage range as argument. 
        Repeat the command till card busy bit (bit31) in response,
        is 0.
        After retry count over,return error.

       -Store voltage range in card_info structure,for future reference.

       -Repeat following steps,till all cards in stack,get enumerated.
        for(nCardCount=0;nCardCount< G_CARDS_CONFIGURED ;nCardCount++)
        {
         -Check if card is connected.If not,return error.
         -Increment RCA count.
         -Send CMD2.If response timeout occurs,then no card remained for
          enumeration.Break out of the loop.
          If command not taken by IP,repeat it.
          After retry count over,return error.

         -If CMD2 is successful,store CID received in response,in 
          card_info structure.
 
         -Send CMD3 wirh RCA.
          If command not taken by IP,repeat it.
          After retry count over,return error.

         -If CMD3 is successful,validate the CMD3 response.
         -If CMD3 response indicate no errors,initiaise
          members in card_info structure,for given card.

        }-Loop till all cards are enumerated.
       
       -Read CSD of all cards.
        for(nCardCount=0;nCardCount< G_CARDS_CONFIGURED ;nCardCount++)
        {
          -If card not connected,go for next card.
          -Read CSD card by sending CMD9.
          -Validate Response for CMD9 and store CSD info in card_info structure.
          -Store following details for given card ,in card_info.:
           Maximum TAAC and NSAC values.
           Card_size.
           Read_Block_size.
           Write_Block_size.
        }repeat for all cards configured.

       -Return SUCCESS always.
*/
int Enumerate_All_MMC_Cards(void)
{
  unsigned long dwRetVal,dwOCRrange,dwCMD1Resp,dwData;
#ifdef _GPIO_CARD_DETECT_  
  unsigned char cardstatus;
#endif
  
  R2_RESPONSE tR2;
  R1_RESPONSE tR1;
  
  int nRetry,nWait,nPortNo,nCardCount;

  nRetry=CMD1_RETRY_COUNT;
  nPortNo=0;

  //Clear card type register.For HSMMC,RTL reads this register to check
  //card width.In MMC_ONLY mode,only bit0 and bit16 are used.
  dwData=0;
  Read_Write_IPreg(CTYPE_off,&dwData,WRITEFLG);

  //Check if card is connected.If yes,then only proceed.
  dwData=0xffffffff;
  #ifdef _GPIO_CARD_DETECT_
     //sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x004),cardstatus);
      GPIO_CARD_DETECT_IN;
    cardstatus = GPIO_CARD_DETECT_READ;
    if(!cardstatus)
         dwData=0xfffffffe;
    else
         dwData=0xffffffff;
 #else
     Read_Write_IPreg(CDETECT_off,&dwData,READFLG);
 #endif
  //Clocksrc0 frequency is already set by enumerate_MMC_stack().

  dwRetVal=SUCCESS;
  /*Send CMD0.(GO_IDLE) Card number is same as port number.
  For MMC Only mode,port no.=0 always while card no.will vary.*/ 
  for(nRetry=0;nRetry<CMD0_RETRY_COUNT;nRetry++)
  {
    if((dwRetVal= form_send_cmd(nPortNo,CMD0,00,(unsigned char*)(&tR1.dwStatusReg),1))
       ==SUCCESS)   break;

    for(nWait=0;nWait<0x0ff;nWait++);
  }//end of for loop

  if(nRetry>=CMD0_RETRY_COUNT || dwRetVal!=SUCCESS)   
  {
    dwRetVal = Err_Enum_CMD0_Failed;
    goto End_MMC_Enumerate;
  }
 
  /*Send CMD1.(SEND_OP_COND)*/
  //Set required operating voltage.
  /* required for forced setting only  dwOCRrange =MMC_33_34V; */
  dwRetVal = 0;
  // send cmd1 with arg0.
  dwOCRrange = 0;
  dwCMD1Resp=0;
  dwRetVal = form_send_cmd(nPortNo,CMD1,dwOCRrange,(unsigned char*)(&dwCMD1Resp),1);
  if(dwRetVal!=0)
    printk("enumerate_All_MMC_cards: cmd1 with arg0 failed.\n");
  
  //Allow predefined range only.
  dwCMD1Resp &= MMC_33_34V;
  dwOCRrange = dwCMD1Resp | 0x80000000;
  // 
  for(nRetry=0;nRetry<CMD1_RETRY_COUNT;nRetry++)
  {
    dwCMD1Resp=0;
    //****Check: If customer provided range is wide,
    //**then all cards will fit into this ??
    //If response error/timeout/crc error occurs,then response may be
    //invalid.send_cmd_to_host doesnot copy response in this case.
    //So,repeat this command to get valid response.
    //If card has changed the state,it will not respond to this retried command.
    dwRetVal =form_send_cmd(nPortNo,CMD1,dwOCRrange,(unsigned char*)(&dwCMD1Resp),1);
    //MSB=0 =>card not ready.
    //Cards which donot support given WV,go in inactive state.
    if((dwCMD1Resp & 0x080000000) && (dwRetVal==SUCCESS))  break;

    for(nWait=0;nWait<0x0ffff;nWait++);
    for(nWait=0;nWait<0x0ffff;nWait++);
  }//end of for loop

  //if cmd1 receives response errors,stop enumeratrion as response received
  //maynot be valid.
  if(nRetry>=CMD1_RETRY_COUNT || dwRetVal != SUCCESS)
  {
    dwRetVal = Err_Enum_CMD1_Failed;
    goto End_MMC_Enumerate;
  }

  //Validate response.Response doesnot have any error bit.
  Validate_Response(CMD1,(unsigned char*)(&dwCMD1Resp),nPortNo,DONOT_COMPARE_STATE);
  
  
  /*Repeat CMD2 and CMD3 till all cards are enumerated.*/
  for(nCardCount=0;nCardCount< G_CARDS_CONFIGURED ;nCardCount++)
  {
    //Card not connected.
    if(dwData & (0x01<<nCardCount)) continue;
      
    G_NEWRCA +=1;
    for(nRetry=0;nRetry<CMD2_RETRY_COUNT;nRetry++)
    {
      /*Send CMD2.(ALL_SEND_CID)*/
      //CMD2 may fail due to:
      // 1.Command not accepted IP : Repeat command.
      // 2.Card not responded (Resp.timeout error) : Card will not respond 
      //  to retried CMD2.So,donot repeat CMD2.
      // 3.Card responded,but reponse error or crc error: Card has changed
      //  state and further CMD2 will be illegal for it.If CMD2 retried,
      //  another card in stack will go in identification state.So,donot
      //  repeat CMD2.    
      //Response timeout indicates,no card is unidentified.

      if((dwRetVal =form_send_cmd(nPortNo,CMD2,00,(&tR2.bResp[0]),1))
         == SUCCESS) break;

      if(dwRetVal !=Err_Command_not_Accepted) break;

      for(nWait=0;nWait<0x0ffff;nWait++);
    }//end of retry loop.

    //If no response to CMD2,all cards are enumerated. 
    if(dwRetVal == RESP_TIMEOUT)
    {
      dwRetVal = SUCCESS;
      break;
    }
    //for other response errors,continue enumeration.
    if(nRetry>=CMD2_RETRY_COUNT)
    {
      dwRetVal = Err_Enum_CMD2_Failed;
      goto End_MMC_Enumerate;
    }

    //Validate response.Donot check for any error.Just store CID value.
    //Proceed for cmd3,evenif cmd2 returns response error.
    if(dwRetVal == SUCCESS)
       dwRetVal=Validate_Response(CMD2,&tR2.bResp[0],nCardCount,DONOT_COMPARE_STATE);
    //else if response ie CID data is to be read in case of reponse errors also,
    //read it from response0 to response3 registers.
         
    /*Send CMD3.(SET_RELATIVE_ADDR)*/
    //Note: For CMD3, argument = RCA (bit31:16) + stuff bits(bits15:0)
    //CMD3 may fail due to:
    // 1.Command not accepted IP : Repeat command.
    // 2.Card not responded (Resp.timeout error) : Card will not respond 
    //  to retried CMD.So,donot repeat CMD.
    // 3.Card responded,but reponse error or crc error: Card has changed
    //  state and will not respond it.So,donot repeat CMD2.    
    //Ignore response errors.For Err_Command_not_Accepted,retry CMD.
    for(nRetry=0;nRetry<CMD3_RETRY_COUNT;nRetry++)
    {
      if((dwRetVal = form_send_cmd(nPortNo,CMD3,(G_NEWRCA << 16),
         (unsigned char*)(&tR1.dwStatusReg),1))==SUCCESS)  break;
          
      if(dwRetVal !=Err_Command_not_Accepted) break;
          
      for(nWait=0;nWait<0x0ffff;nWait++);
    }

    if(dwRetVal==SUCCESS)
    {
      //Validate response.Card should be in Identification state
      //If CMD3 returns error in status,then card is not enumerated properly.
          
      if((dwRetVal=Validate_Response(CMD3,(unsigned char*)(&tR1.dwStatusReg),nCardCount, CARD_STATE_IDENT))== SUCCESS)
      {
        //Increment card count for given type.
        Increment_Card_Count(MMC);
              
        //Save info in Card_Info structure.
        Card_Info[nCardCount].Card_RCA= G_NEWRCA;
        Card_Info[nCardCount].Card_Connected=TRUE;
        Card_Info[nCardCount].Card_type=MMC;
              
        //Used in media driver.
        Card_Info[nCardCount].Card_Insert=1;

        //Read card status to decide if card is locked.
        if(tR1.Bits.CARD_IS_LOCKED)
           Card_Info[nCardCount].Card_Attrib_Info.Bits.PWD_Set=1;

      }
    }//end of if CMD3 is successful.
  }//end of for loop.Loop back to enumerate next card.

  //Read CSD of all cards.
  for(nCardCount=0;nCardCount< G_CARDS_CONFIGURED ;nCardCount++)
  {
    if(Card_Info[nCardCount].Card_Connected==FALSE) continue;
      
    //If card is connected,CSD is valid.
    if((dwRetVal = form_send_cmd(nPortNo,CMD9,00,(&tR2.bResp[0]),1))
       ==SUCCESS)   
    {
      //Validate response.Card should be in Identification state
      //Unpacks CSD information and store it in card_info.
      dwRetVal=Validate_Response(CMD9,&tR2.bResp[0],nCardCount,
                                 DONOT_COMPARE_STATE);
      //Store maximum TAAC and NSAC values for timeout 
      //calculation.
      if(Card_Info[nCardCount].CSD.Fields.taac > tIPstatusInfo.Max_TAAC_Value)
         tIPstatusInfo.Max_TAAC_Value = Card_Info[nCardCount].CSD.Fields.taac;
         
      if(Card_Info[nCardCount].CSD.Fields.nsac > tIPstatusInfo.Max_NSAC_Value)
         tIPstatusInfo.Max_NSAC_Value = Card_Info[nCardCount].CSD.Fields.nsac;
          
      /* read/write block size */
      Card_Info[nCardCount].Card_Read_BlkSize = 
        ((0x0001 <<(Card_Info[nCardCount].CSD.Fields.read_bl_len)) > 512) ? 512 : 
        (0x0001 <<(Card_Info[nCardCount].CSD.Fields.read_bl_len));
      Card_Info[nCardCount].Card_Write_BlkSize = 
        ((0x0001 <<(Card_Info[nCardCount].CSD.Fields.write_bl_len)) > 512) ? 512 : 
        (0x0001 <<(Card_Info[nCardCount].CSD.Fields.write_bl_len));
          
      /*Calculate and store total card size in bytes */
      Card_Info[nCardCount].Card_size = (1 + Card_Info[nCardCount].CSD.Fields.c_size) * 
        (0x01 << (Card_Info[nCardCount].CSD.Fields.c_size_mult + 2)) * 
        (Card_Info[nCardCount].Card_Read_BlkSize);
    }
  }//end of for loop. Go for sending CMD9 for next card.

End_MMC_Enumerate:

  //To return success always ?? If some cards are failed to enumerate but,
  //some are successful,then ??
  return SUCCESS;
}

//--------------------------------------------------------------
/*B.1.6.Enumerate_MMC_Card:Enumerates single MMC card.

  int Enumerate_MMC_Card(int ncardno,int nPortNo,unsigned long dwStartRCA)

  *Parameters:
  ncardno:(i/p):Card number.
  nPortNo :(i/p):Port number.Enumearte card attached to 
  this port.
  dwStartRCA:(i/p):Relative card address to be assigned to
  card.

  *Flow:
       -Check if card is connected.If not,return error.
       -Assign Card_Info[ncardno].Do_init=1;
       -Read clock source assigned to specified card and set its
        frequency to enumeration frequency.
       -Send CMD0 to reset the card.
        If command failed,repeat it for given retries.
        After retries,return error.

       -Send CMD1 with arg 0 to read voltage range supported by card.

       -Send CMD1 with voltage window as an argument.
        Repeat the command till card busy bit (bit31) in response,
        is 0.
        After retry count over,return error.

       -Store voltage range in card_info structure,for future reference.

       -Send CMD2.
        If command not taken by IP,repeat it.
        After retry count over orresponse timeout error,return error.

       -If CMD2 is successful,store CID received in response,in 
        card_info structure.
 
       -Send CMD3.
        If command not taken by IP,repeat it.
        After retry count over,return error.

       -If CMD3 is successful,validate the CMD3 response.
       -If CMD3 response indicate no errors,initiaise
        members in card_info structure,for given card.

       -Send CMD9 to read CSD info.
       -Validate Response for CMD9 and store CSD info in card_info structure.
       -Store following details for given card ,in card_info.:
          Maximum TAAC and NSAC values.
          Card_size.
          Read_Block_size.
          Write_Block_size.

       -Set clock frequency of the clock cource for given card,to original value.
 
       -Return.
*/
//Always this will enumerate only one card.
int Enumerate_MMC_Card(int ncardno,int nPortNo,unsigned long dwStartRCA)
{
  unsigned long dwRCA,dwRetVal,dwOCRrange,dwCMD1Resp,dwData,dwClksrcNo,dwCMD9RetVal;
  unsigned char  bOrgClkFreq;
#ifdef _GPIO_CARD_DETECT_
    unsigned char cardstatus;
#endif
  R2_RESPONSE tR2;
  R1_RESPONSE tR1;
  
  int nRetry,nWait;
  CLK_SOURCE_INFO tClkSource;

  nRetry=CMD1_RETRY_COUNT;
    _ENTER();  
  //Check if card is connected.If yes,then only proceed.
 dwData=0xffffffff;
#ifdef _GPIO_CARD_DETECT_
     //sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x004),cardstatus);
    GPIO_CARD_DETECT_IN;
    cardstatus = GPIO_CARD_DETECT_READ;
    if(!cardstatus)
         dwData=0xfffffffe;
    else
         dwData=0xffffffff;
#else
     Read_Write_IPreg(CDETECT_off,&dwData,READFLG);
#endif

  if(dwData & (0x01<<ncardno)) return Err_Card_Not_Connected;

  //Card info is initialised in card_detect_handler and
  //also initialise.This setting is repeated here for 
  //MMC_ONLY mode.
  Card_Info[ncardno].Do_init=1;

  /*Assign clock source.Make its frequency=fod.*/
  //Stop clock,check clock source.Change clk frequency to fod.
  //Clock source allocation info.to be retrieved from CLKSRC reg.
  Read_ClkSource_Info(&tClkSource,ncardno);
  dwClksrcNo = tClkSource.dwClkSource;

  // ** Fod range for MMC is 0-400khz while for
  // SD it is 100-400khz.

  dwRetVal= set_clk_frequency(dwClksrcNo,(CIU_CLK/(MMC_FOD_VALUE*2)), &bOrgClkFreq);
  if(dwRetVal!=SUCCESS)
  {
    printk("Enumerate_MMC_Card:set_clk_frequency failed. %lx",dwRetVal);
    return dwRetVal;
  }

  dwRetVal=SUCCESS;
  /*Send CMD0.(GO_IDLE) Card number is same as port number.
    For MMC Only mode,port no.=0 always while card no.will vary.*/ 
  for(nRetry=0;nRetry<CMD0_RETRY_COUNT;nRetry++)
  {
    if((dwRetVal= form_send_cmd(ncardno,CMD0,00,(unsigned char*)(&tR1.dwStatusReg),1))
       ==SUCCESS)   break;
    for(nWait=0;nWait<0x0ff;nWait++);
  }//end of for loop

  //to debug
  if(nRetry>=CMD0_RETRY_COUNT || dwRetVal!=SUCCESS)   
  {
    dwRetVal = Err_Enum_CMD0_Failed;
    goto End_Enumerate;
  }
  
  /*Send CMD1.(SEND_OP_COND)*/
  //Set required operating voltage.
  /* required for forced setting only  dwOCRrange =MMC_33_34V; */
  dwRetVal = 0;
  // send cmd1 with arg0.
  dwOCRrange = 0;
  dwCMD1Resp=0;
  dwRetVal = form_send_cmd(ncardno,CMD1,dwOCRrange,(unsigned char*)(&dwCMD1Resp),1);
//  if(dwRetVal!=0)
 //   HI_TRACE(2,"enumerate_MMC_card: cmd1 with arg0 failed.dwRetVal=%ul\n",dwRetVal);
  //Allow predefined range only.
  dwCMD1Resp &= MMC_33_34V;
  dwOCRrange = dwCMD1Resp | 0x80000000;
  // 
  for(nRetry=0;nRetry<CMD1_RETRY_COUNT;nRetry++)
  {
    dwCMD1Resp=0;
    //****Check: If customer provided range is wide,
    //**then all cards will fit into this ??
    //If response error/timeout/crc error occurs,then response may be
    //invalid.send_cmd_to_host doesnot copy response in this case.
    //So,repeat this command to get valid response.
    //If card has changed the state,it will not respond to this retried command.
    dwRetVal =form_send_cmd(ncardno,CMD1,dwOCRrange,(unsigned char*)(&dwCMD1Resp),1);

    //MSB=0 =>card not ready.
    //Cards which donot support given WV,go in inactive state.
    if((dwCMD1Resp & 0x080000000) && (dwRetVal==SUCCESS)) break;    

    for(nWait=0;nWait<0x0ffff;nWait++);
    for(nWait=0;nWait<0x0ffff;nWait++);      
  }//end of repeatation for CMD1,if error.

  //if cmd1 receives response errors,stop enumeratrion as response received
  //maynot be valid.Return error,if ready bit not set even after retries.
  if(nRetry>=CMD1_RETRY_COUNT || dwRetVal!=SUCCESS)   
  {
    dwRetVal = Err_Enum_CMD1_Failed;
    Card_Info[ncardno].Card_type=INVALID;
    HI_TRACE(2, "Enumerate card failed!\n");
    goto End_Enumerate;
  }

  //Validate response.Response doesnot have any error bit.
  Validate_Response(CMD1,(unsigned char*)(&dwCMD1Resp),ncardno,DONOT_COMPARE_STATE);
  

  /*Repeat CMD2 and CMD3 till all cards are enumerated.*/
  dwRCA = dwStartRCA;
  for(nRetry=0;nRetry<CMD2_RETRY_COUNT;nRetry++)
  {
    /*Send CMD2.(ALL_SEND_CID)*/
    //CMD2 may fail due to:
    // 1.Command not accepted IP : Repeat command.
    // 2.Card not responded (Resp.timeout error) : Card will not respond 
    //  to retried CMD2.So,donot repeat CMD2.
    // 3.Card responded,but reponse error or crc error: Card has changed
    //  state and further CMD2 will be illegal for it.So,donot
    //  repeat CMD2.    
    //Response timeout indicates,no card is unidentified.
    if((dwRetVal =form_send_cmd(ncardno,CMD2,00,(&tR2.bResp[0]),1))
       == SUCCESS) break;

    if(dwRetVal!=Err_Command_not_Accepted) break;
      
    for(nWait=0;nWait<0x0ffff;nWait++);
  }//end of retry loop.

  //If no response to CMD2,card is not in identification state.
  //for other response errors,continue enumeration.
  if((nRetry>=CMD2_RETRY_COUNT) || (dwRetVal == RESP_TIMEOUT))
  {
    dwRetVal = Err_Enum_CMD2_Failed;
    goto End_Enumerate;
  }
  
  //Validate response.Donot check for any error.Just store CID value.
  //Proceed for cmd3,evenif cmd2 returns response error.
  if(dwRetVal == SUCCESS)
    dwRetVal=Validate_Response(CMD2,&tR2.bResp[0],ncardno,DONOT_COMPARE_STATE);
  //else if response ie CID data is to be read in case of reponse errors also,
  //read it from response0 to response3 registers.
  
  /*Send CMD3.(SET_RELATIVE_ADDR)*/
  //Note: For CMD3, argument = RCA (bit31:16) + stuff bits(bits15:0)
  //CMD3 may fail due to:
  // 1.Command not accepted IP : Repeat command.
  // 2.Card not responded (Resp.timeout error) : Card will not respond 
  //  to retried CMD.So,donot repeat CMD.
  // 3.Card responded,but reponse error or crc error: Card has changed
  //  state and will not respond it.So,donot repeat CMD2.    
  //Ignore response errors.For Err_Command_not_Accepted,retry CMD.
  for(nRetry=0;nRetry<CMD3_RETRY_COUNT;nRetry++)
  {
    if((dwRetVal = form_send_cmd(ncardno,CMD3,(dwRCA << 16),
                  (unsigned char*)(&tR1.dwStatusReg),1))  ==SUCCESS)  break;
      
    if(dwRetVal !=Err_Command_not_Accepted) break;
      
    for(nWait=0;nWait<0x0ffff;nWait++);
      
  }//end of repeatation of CMD3,if Err_Command_not_Accepted.


  //If CMD3 failed or no valid response for CMD3 then,card is not
  //enumerated properly.return error.
  if(dwRetVal==SUCCESS)
  {
    //Validate response.Card should be in Identification state
    if((dwRetVal=Validate_Response(CMD3,(unsigned char*)(&tR1.dwStatusReg),ncardno,
       CARD_STATE_IDENT))== SUCCESS) 
    {         
      /*If enumeration successful,store card information.*/
      //Increment card count for given type.
      Increment_Card_Count(MMC);
          
      //Save info in Card_Info structure.
      Card_Info[ncardno].Card_RCA=dwRCA;
      Card_Info[ncardno].Card_Connected=TRUE;
      Card_Info[ncardno].Card_type=MMC;
          
      //Used in media driver.
      Card_Info[nPortNo].Card_Insert=1;
          
    }//end if response for CMD3 doesnot have any error.
  }//end of if CMD3 is successful.
  
  //dwRetVal = error code to return,if any of the enumeration command fails.
  //If CMD9 fails,then it is not enumeration failure.CSD data for card will 
  //be 0,in this case.
  

  //Read CSD register of the card.
  //If card is connected,CSD is valid.
  if((dwCMD9RetVal = form_send_cmd(ncardno,CMD9,00,(&tR2.bResp[0]),1))
     ==SUCCESS)   
  {
    //Validate response.Card should be in Identification state
    //Unpacks CSD information and store it in card_info.
    Validate_Response(CMD9,&tR2.bResp[0],ncardno,
                      DONOT_COMPARE_STATE);
    //Store maximum TAAC and NSAC values for timeout 
    //calculation.
    if(Card_Info[ncardno].CSD.Fields.taac > tIPstatusInfo.Max_TAAC_Value)
       tIPstatusInfo.Max_TAAC_Value = Card_Info[ncardno].CSD.Fields.taac;
      
    if(Card_Info[ncardno].CSD.Fields.nsac > tIPstatusInfo.Max_NSAC_Value)
       tIPstatusInfo.Max_NSAC_Value = Card_Info[ncardno].CSD.Fields.nsac;
      
    /* read/write block size */
    Card_Info[ncardno].Card_Read_BlkSize = 
       ((0x0001<<(Card_Info[ncardno].CSD.Fields.read_bl_len)) > 512) ? 512 : 
       (0x0001 <<(Card_Info[ncardno].CSD.Fields.read_bl_len));

    Card_Info[ncardno].Card_Write_BlkSize = 
       ((0x0001<<(Card_Info[ncardno].CSD.Fields.write_bl_len)) > 512) ? 512 : 
       (0x0001 <<(Card_Info[ncardno].CSD.Fields.write_bl_len));
      
    /*Calculate and store total card size in bytes */
    Card_Info[ncardno].Card_size=(unsigned long long)(1+Card_Info[ncardno].CSD.Fields.c_size) * 
       (0x01 << (Card_Info[ncardno].CSD.Fields.c_size_mult + 2)) * 
       (Card_Info[ncardno].Card_Read_BlkSize);
    
  #ifdef HSMMC_SUPPORT
    //If CSD reading ie CMD9 is successful and card has Ext.CSD,read it.
    if((Card_Info[ncardno].CSD.Fields.csd_structure >=3) &&
       (Card_Info[ncardno].Card_Connected==TRUE) )
       HSMMC_ExtCSDHandler(ncardno);
  #endif

  }//end if CMD9 is succesful.

End_Enumerate:
  //Stop clock,check clock source.Change clk frequency to org.
  set_clk_frequency(dwClksrcNo,bOrgClkFreq, &bOrgClkFreq);
  _LEAVE();
  return dwRetVal;
}
//--------------------------------------------------------------      

/*B.1.4.Enumerate_SD_card_Stack().: 
  Enumerates all cards in SD stack.

  *Called from: BSD::sd_mmc_sdio_initialise().

  *Parameters:Nothing.

  *Return value : 
   0: for SUCCESS
  
  *Flow:
       -Set clock source0 to Fod.
        For enumeration all cards will use this source.
       -Enable clk to all cards.
       -Read HCON reg to get the no. of card for which IP is configured.
       -Loop: SD_MAX_cards = 16.
        for(i=0;i<cards_configured;i++)
        {
          -Call Enumerate_Card(Port_no),to enumerate card connected
           to given port.
           If failed, set Card_Info[i].Card_Connected=FALSE;
        }
       -Update no. of total card connected.             
       -If at least one card is connected,then assign and enable clock
        sources to cards.
       -Return SUCCESS always.
*/
int Enumerate_SD_card_Stack(void)
{
  int i;
  unsigned long dwCards;
  unsigned char bOrgVal;
  int dwRetVal;
 _ENTER();
  //sd_mmc_sdio_initialise G_NEWRCA.
  G_NEWRCA=00;
  //Set clock frequency = fod. Assuming clk=25/2=12.5 MHz.clkdiv is 0x14.
  //At power on,clksrc for all is clksrc0.
  dwRetVal=set_clk_frequency(CLKSRC0,(CIU_CLK/(SD_FOD_VALUE*2)),&bOrgVal);
   
  //Enable clk to all cards.Required to send any cmd to cards.
  clk_enable_disable(0xff,TRUE);
     
  //Scan cards = HCON::NUM_CARDS.Identify type,call corresponding
  //init.routine,if successful,store type and connected,in
  //card info.

  //Read cards for which IP is configured. HCON::bits 1-5
  Read_Write_IPreg(HCON_off,&dwCards,READFLG);
  dwCards = ((dwCards & 0x03e) >> 1);
  //NUM_CARDS start with 0.
  dwCards+=1;
  for(i=0;i<dwCards;i++)
  {
    if(Enumerate_card(i) != SUCCESS)
       Card_Info[i].Card_Connected=FALSE;
    else
       printk("Enumerate_card successful for card in slot %x \n",i);  
  }
  tIPstatusInfo.Total_Cards = tIPstatusInfo.Total_MMC_Cards +
                              tIPstatusInfo.Total_SD_Cards  +
                              tIPstatusInfo.Total_SDIO_Cards;

  //Assign and enable clock sources to cards.
  if(tIPstatusInfo.Total_Cards >0)
     Set_default_Clk(); 

  _LEAVE();
  return SUCCESS;
}
//--------------------------------------------------------------
/*B.1.5.Enumerate_card :Enumerates a specified card.
  Called when card is detected or during enumeration.

  Enumerate_card(int nPort_No)

  *Called from:Enumerate_SD_Card_Stack().

  *Parameters:
   nPort_No:(i/p):Port number.card connected to this is to
                       be enumerated.
  *Return value : 
   0: for SUCCESS
   Err_Card_Not_Connected: 
   Err_Enum_SendOCR_Failed
   Err_Enum_CMD3_Failed;
   Err_Enum_ACMD41_Failed;
   Err_Enum_CMD2_Failed;
   Err_Enum_CMD0_Failed
   Err_Enum_CMD1_Failed
   Err_Command_not_Accepted
   Err_Card_Unknown  : card type unknown.

  *Flow:
       -Check if card is connected.If not,return error.
       -Get RCA for card.
       -Set Card_Info[nPortNo].Do_init=1 to indicate that attempt
        to enumerate card is being done.
       -Read card type by calling Identify_Card_Type().
       -Call corresponding card initialisation routine.
       -For non MMC cards,if enumeration is successful,
         -Initiaise members in card_info structure,for given card.
         -After power on,SD/SDIO card is in 1-bit transfer mode.
          So,program corresponding mode bit in Card Type register.
         -For SD / SDIO_COMBO / SDIO_MEM card,Read CSD/CID register.
         -For SDIO/SDIO_COMBO card,store card specific CCCR register
          info in Card_Info.
       -Increment RCA count.
       - Return.
*/
int Enumerate_card(int nPortNo)
{
  R2_RESPONSE  tR2;
  R5_RESPONSE  tR5;
  R1_RESPONSE  tR1;

  unsigned char   bCCCRdata[20];
  int    nCardType,j,ncardno;
  unsigned long  dwNewRCA,dwRetVal,dwResp,dwReturn;

  ncardno=nPortNo;
 _ENTER();
  //Check if card is connected.If yes,then only proceed.
 // dwData=00;
 // Read_Write_IPreg(CDETECT_off,&dwData,READFLG);
    
 // if( dwData & (0x01<<nPortNo) )
 //   return Err_Card_Not_Connected;
  
  //Get RCA for card.
  dwNewRCA=G_NEWRCA+1;

   for(j=0;j<20;j++)
  {
    bCCCRdata[j]  = 0;
  }


  //Card info is initialised in card_detect_handler and
  //also initialise.
  Card_Info[nPortNo].Do_init=1;

  //After power on,SD/SDIO card is in 1-bit transfer mode.
  //So,program corresponding mode bit in Card Type register.
  //Also,for MMC type,corresponding bit should be cleared.
  set_ip_parameters(SET_SDWIDTH_ONEBIT,(unsigned long*)(&ncardno));

    
  /*Identify card type.(SDIO/MMC/SDmem)*/
  nCardType=Identify_Card_Type( nPortNo,&dwResp);
  //nCardType = 4;

  /*Call corresponding initialisation routine.*/
  switch(nCardType)
  {
    case SDIO:
      //This routine returns SDIO subtype in nCardType.
      dwRetVal=sd_mmc_sdio_initialise_SDIO(nPortNo,dwNewRCA,&nCardType,dwResp);
      break;

    case SD:
    case SD2:
      dwRetVal = sd_mmc_sdio_initialise_SD(nPortNo,dwNewRCA,&nCardType);
      break;

    case MMC:
      //Enumerate only one MMC card.
      dwRetVal = Enumerate_MMC_Card(nPortNo,nPortNo,dwNewRCA);
      break;

    default:
    printk("enumerate_card returning err_card_unknown,nCardType=%d\n", nCardType);
      return Err_Card_Unknown;
  }//End of switch
  //printk("sdio card init ok!\n");
  //Enumerate_MMC has already done following
  //processing.So,donot repeat it.This is required 
  //for SD_MMC mode.
  if(dwRetVal==SUCCESS && nCardType!=MMC)
  {

      //Save info in Card_Info structure.
    Card_Info[nPortNo].Card_Connected=TRUE;
    Card_Info[nPortNo].Card_type=nCardType;

    //Used in media driver.
    Card_Info[nPortNo].Card_Insert=1;

    //Increment card count for given type.
    Increment_Card_Count(nCardType);
    if(nCardType != SDIO_IO)
    {
      if((dwRetVal = form_send_cmd(nPortNo,CMD9,00,(&tR2.bResp[0]),1))
         ==SUCCESS)   
      {
              //Validate response.Card should be in Identification state
        if((dwRetVal=Validate_Response(CMD9,&tR2.bResp[0],nPortNo,
           DONOT_COMPARE_STATE))==SUCCESS)
        {
          //Store maximum TAAC and NSAC values for timeout 
          //calculation.
          if(Card_Info[nPortNo].CSD.Fields.taac > tIPstatusInfo.Max_TAAC_Value)
             tIPstatusInfo.Max_TAAC_Value = Card_Info[nPortNo].CSD.Fields.taac;

          if(Card_Info[nPortNo].CSD.Fields.nsac > tIPstatusInfo.Max_NSAC_Value)
             tIPstatusInfo.Max_NSAC_Value = Card_Info[nPortNo].CSD.Fields.nsac;
          /* read/write block size */
          
          Card_Info[nPortNo].Card_Read_BlkSize = 
             ((0x0001 <<(Card_Info[nPortNo].CSD.Fields.read_bl_len)) > 512) ? 512 : 
             (0x0001 <<(Card_Info[nPortNo].CSD.Fields.read_bl_len));

       //   Card_Info[nPortNo].Card_Read_BlkSize = (0x0001 <<(Card_Info[nPortNo].CSD.Fields.read_bl_len));       

          Card_Info[nPortNo].Card_Write_BlkSize = 
             ((0x0001 <<(Card_Info[nPortNo].CSD.Fields.write_bl_len)) > 512) ? 512 : 
             (0x0001 <<(Card_Info[nPortNo].CSD.Fields.write_bl_len));

//          Card_Info[nPortNo].Card_Write_BlkSize = (0x0001 <<(Card_Info[nPortNo].CSD.Fields.write_bl_len));   
          //Calculate and store total card size in bytes */
          if(Card_Info[nPortNo].CSD.Fields.csd_structure){
              Card_Info[nPortNo].Card_size=(unsigned long long)(Card_Info[nPortNo].CSD.Fields.c_size+1)*512*1024;
          }else{
              Card_Info[nPortNo].Card_size=(unsigned long long)(1+Card_Info[nPortNo].CSD.Fields.c_size)* 
                 (0x01 << (Card_Info[nPortNo].CSD.Fields.c_size_mult + 2)) * 
                 (0x0001 <<(Card_Info[nPortNo].CSD.Fields.read_bl_len)); 
       }
          
         }//End of if response is valid.
      }//End of if command is successfully sent.

    #ifdef HSSD_SUPPORT
      //Check if card is HS-SD type and handle initialisation
      //tasks for that.
      HSSD_Initialise_Handler(nPortNo);
    #endif 

    }//End of if(nCardType != SDIO_IO) 
    //SDIO IO only card,doesnot support CSD and CID registers.

    if(nCardType==SDIO_IO || nCardType==SDIO_COMBO)
    {
      //printk("Enumerate card:Entered in reading CCCR register.\n");
      //Read CCCR for SDIO card using CMD53..

      //CCCR is func=0,start from address=00,read 15 bytes
      //nflags : bit6=1 for kernel mode buffer,bit4=1 for handle data errors.
  /*    
      dwRetVal= sdio_rdwr_extended(nPortNo, 00,00,0x12,
                                   &bCCCRdata[0], (unsigned char*)(&tR5.dwReg),
                                   00,  //No err response buffer
                                   0x01,//incre,addr.(bit0), Cmdoptions
                                   NULL,//No callback.
                                   00,0x50);//unsigned char mode,no wait for prev data,read.
    */                               

      /*
      dwRetVal= sdio_rdwr_extended(nPortNo, 00,00,0x12,
                                   &bCCCRdata[0], (unsigned char*)(&tR5.dwReg),
                                   00,  //No err response buffer
                                   0x01,//incre,addr.(bit0), Cmdoptions
                                   NULL,//No callback.
                                   00,0x50);//unsigned char mode,no wait for prev data,read.
       */
    for(j = 0; j<20; j++)
    {
    dwRetVal = sdio_rdwr_direct(ncardno, 00,
                     j,
                     &bCCCRdata[j],
                     (unsigned char*)(&tR5.dwReg),
                     00,
                     00,
                     0x02);
       if(dwRetVal != SUCCESS)
               break;
    }

                                   
                                   
                                   
      //Store card specific CCCR register info in Card_Info.
      if(dwRetVal==SUCCESS)
      {
         Unpack_CCCR_reg(nPortNo,&bCCCRdata[0]);
         #if 0
         //Set IO_BLOCK size, only if CCCR reading succeeds and SMB=1.
         //As there is no point otherwise.
         if((Card_Info[nPortNo].CCCR.Card_Capability & 0x02))        
         {
           /* IO Block size is 00 on power on reset.
           Host has to set it after enumeration.*/
            
           //Store IO block size=8 for 1-7 functions.
           //Function0,address:110-111,210-211 etc.
           bCCCRdata[0] = 0x8 ; //LSB first
           bCCCRdata[1] = 0   ; //MSB last
           for(i=0;i<8;i++)
           {
             dwRetVal= sdio_rdwr_extended(nPortNo,00,((i<<8)|0x10),0x02,
                                 &bCCCRdata[0], (unsigned char*)(&tR5.dwReg),
                                 00,//No err response buffer.
                                 0x01,//incre,addr.(bit0), Cmdoptions.
                                 NULL,//No callback.
                                 00,0xd0);//unsigned char mode,no wait for prev data,write.
             if(dwRetVal==SUCCESS)
               //Blksize set for function.Set it in card_info.
               //Whenever blksize is changed,it will be reflected in card_info.
               Card_Info[nPortNo].SDIO_Fn_Blksize[i] = 0x0008;        
             else
               printk("Enumerate_card: IO blocksize setting failed for function no %x \n",i);

               //printk("SDIO blksize set for function %x returned value = %lx \n",i,dwRetVal);                 
           }//end of for loop.
         }//end of if SMB==1.
         #endif
      }
      else
        printk("Enumerate_card:CCCR read failed.Reinitialise SDIO card in slot %x.n",nPortNo);
    
    }//End of if(nCardtype!=SD || nCardType!=MMC)

  } //End of if(dwRetVal==SUCCESS)

//  printk(" : Exiting Enumerate_card with dwREtval= %lx \n",dwRetVal);

  // G_NEWRCA is not required for SD and SDIO
  if(dwRetVal==SUCCESS)
  {
     G_NEWRCA++;

     //For memory portion,read card status to decide if card is locked.
     //For SDIO_COMBO,SDIO_MEM,SD_MEM,MMC_MEM, read card status.
     if(nCardType != SDIO_IO)
     {
       if((dwReturn=form_send_cmd(nPortNo,CMD13,00,
          (unsigned char*)(&tR1.dwStatusReg),1)) ==SUCCESS)
       {
         if(tR1.Bits.CARD_IS_LOCKED)
            Card_Info[nPortNo].Card_Attrib_Info.Bits.PWD_Set=1;
       }
     }
   }//end of if loop.

  _LEAVE();
   return dwRetVal;
}//End of Enumerate_SD_MODE_Card.
//--------------------------------------------------------------      
/*Enumerate_Single_Card
 
 This is function is called when new card insertion or ejection
 is detected.

  *Parameters:
  *i/p : nslotno : Slot number of the card to be enumerated.

  *Return Value :
   0 on Success 
   Err_Invalid_ClkSource
   -EINVAL
   Err_Invalid_ClkDivider
   Err_Hardware_Locked
   Err_Retries_Over
   Warning_DataTMout_Changed

  *Flow:
       For MMC_ONLY mode,whole card stack has to be enumerated.
       For SD_MMC mode,single card can be enumerated.
       -Wait for some time,to give power on time for card.
       -For MMC_ONLY mode,call Enumerate_MMC_card_Stack().

       -For SD_MMC mode,
         -Read clock source and set its frequency to enumeration
          frequency.
         -Enable clock for given slot.
         -Call Enumerate_card() to enumerate the card in given slot.
         -If enumeration is successful,set 
            Card_Info[nslotno].Card_Connected=FALSE;
            Set clock source frequency to original value.
          else
            Set clock frequency as per card type.

         -Update total cards count in card_info structure.

       -return.
*/

int Enumerate_Single_Card(int nslotno)
{
  int nWait;
  unsigned long dwRetVal,ret,dwretval,dwData;
   ControlReg  Cntrlreg={0X00};
   unsigned char bOrgClkFreq;

  //Card is re-inserted.Give some power up time.
  for(nWait=0;nWait<0x0ff;nWait++);


  memset((unsigned char *)&tIPstatusInfo,0,sizeof(tIPstatusInfo));
  //since Card_Info array is allocated all structures are consecutive 
  memset(&Card_Info[0],0,sizeof(struct Card_info_Tag)*MAX_CARDS);

  /*added by csbo*/
  memset((unsigned char *)&tCurrCmdInfo,0,sizeof(tCurrCmdInfo));
  memset((unsigned char *)&tDatCmdInfo,0,sizeof(tDatCmdInfo));
  /*added by csbo*/
  
  /*sd_mmc_sdio_initialise IP.:CTRL,*/
  dwretval=SUCCESS;

  //CTRL register.
 Cntrlreg.Bits.dma_enable=0;

  Cntrlreg.Bits.int_enable=0;//Disable interrupt.

#ifdef INTR_ENABLE
  Cntrlreg.Bits.int_enable=1;//Enable interrupt.
#endif

  //set mask.
  dwData = 0x01ffff;

  Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);
  Read_Write_IPreg(RINTSTS_off,&dwData,READFLG);
  Read_Write_IPreg(RINTSTS_off,&dwData,WRITEFLG);
  Read_Write_IPreg(CTRL_off,&Cntrlreg.dwReg,WRITEFLG);

  do
  {
      dwData=00;
      Read_Write_IPreg(CTRL_off,&dwData,READFLG);
  }while (dwData & (0x00000001));

  //Set clock frequency = fod. Assuming clk=25/2=12.5 MHz.clkdiv is 0x14.
  //At power on,clksrc for all is clksrc0.
  if((dwRetVal=set_clk_frequency(CLKSRC0,(CIU_CLK/(SD_FOD_VALUE*2)), 
     &bOrgClkFreq)) != SUCCESS) 
  {
     printk("Enumerate_Single_Card:Set_Clk_Freq failed. %lx",dwRetVal);
     return dwRetVal;
  }
  //printk("enumerate_single_card: clkfreq.set to be %x\n",(CIU_CLK/(SD_FOD_VALUE*2)));

  //Enable clk to given slot number.
  if((dwRetVal= clk_enable_disable(nslotno,TRUE)) !=SUCCESS)
  {
     printk("Enumerate_Single_Card:Clk_Enable failed.%lx",dwRetVal);
     return dwRetVal;
  }
  //Single card can be enumerated.
  dwRetVal= Enumerate_card(nslotno);
  if(dwRetVal != SUCCESS)  
     Card_Info[nslotno].Card_Connected=FALSE;
  else
  {
    //If enumeration is successful,reassign the clock divider
    //value to suit the card type.Else,keep the card slot
    //frequency as it is.HCLK=25 Mhz ia assumed.
    if(Card_Info[nslotno].Card_type==MMC)
       bOrgClkFreq = MMC_FREQDIVIDER;//divide by 2.
    else
       bOrgClkFreq = SD_FREQDIVIDER;//No divide.
  }
  //Reset card clock frequency to operating frequency value 
  
  //Enumerate_card calls Increment_card_count,if enum.succeeds.
  tIPstatusInfo.Total_Cards = tIPstatusInfo.Total_MMC_Cards +
                              tIPstatusInfo.Total_SD_Cards  +
                              tIPstatusInfo.Total_SDIO_Cards;

 if(tIPstatusInfo.Total_Cards >0)
 {
      Set_default_Clk(); 

/*the sd_set_mode function will do later in the function:mmc_select_card, for some card can't  be set!*/
/*
    if(Card_Info[0].Card_type!=MMC && Card_Info[0].Card_type != SDIO_IO)
    {
      if(sd_set_mode(0,TRUE))
             printk("set sd mode as 4bit mode,fail???!\n");; 
    }
    else 
*/    
    if(Card_Info[0].Card_type == SDIO_IO)
    {
       ret=0;
       set_ip_parameters(SET_SDWIDTH_FOURBIT,&ret);
    }
 }

  if(dwRetVal==SUCCESS)
     printk("Successful enumeration of card in slot %x \n",nslotno);
  return dwRetVal;    
}

//--------------------------------------------------------------      

void Increment_Card_Count(int nCardType)
{
  switch(nCardType)
  {
     case MMC:
       tIPstatusInfo.Total_MMC_Cards += 1;
       break;

     case SD:
     case SD2:
     case SD2_1:
       tIPstatusInfo.Total_SD_Cards += 1;
       break;

     case SDIO:
     case SDIO_COMBO:
     case SDIO_MEM:
     case SDIO_IO:
       tIPstatusInfo.Total_SDIO_Cards += 1;
       break;

     default :
       break;
  }//End of switch.

  return ;
}
//--------------------------------------------------------------      
/* sd_mmc_sdio_initialise_SD : sd_mmc_sdio_initialises SD card.
   
   *Called from  : Enumerate_card()
    
   *Parameters   : 
    nPortNo(i/p): Port no.
    dwRCA (i/p): card RCA

   *Return Value :
    0 on Success
    Err_Enum_CMD0_Failed
    Err_Enum_ACMD41_Failed
    Err_Enum_CMD2_Failed
    Err_Command_not_Accepted
    Err_Enum_CMD3_Failed

   *Flow:         : 
       -Send CMD0
        repeat if unsuccessfull for CMD0_RETRY_COUNT times.
        After retries,return error.
       -Send ACMD41 with arg0 to read voltage range supported by card.   
       -Send ACMD41 with voltage window as an argument.
        Repeat the command till card busy bit (bit31) in response,
        is 0.
        After retry count over,return error.

       -Send CMD2.(ALL_SEND_CID)
        If command not taken by IP,repeat it.
        After retry count over orresponse timeout error,return error.
       -If CMD2 is successful,store CID received in response,in 
        card_info structure.

       -Send CMD3.(SEND_RELATIVE_ADDR).
        If command not taken by IP,repeat it.
        After retry count over,return error.
       -If CMD3 is successful,validate the CMD3 response.
       -If CMD3 response indicate no errors,store the card RCA
        received in response to CMD3,in card_info structure.

       -return

*/

int sd_mmc_sdio_initialise_SD(int nPortNo,unsigned long dwRCA,int *cardtype)
{
  int         nRetry,nWait;
  unsigned long       dwRetVal,dwR3response,dwVoltWindow ;
  R2_RESPONSE tR2;
  R6_RESPONSE tR6;

  //printk(" : Entering sd_mmc_sdio_initialise_SD \n");
  
  dwRetVal=SUCCESS;
  /*/Send CMD0
  for(nRetry=0;nRetry<CMD0_RETRY_COUNT;nRetry++)
  {
    if((dwRetVal = form_send_cmd(nPortNo,CMD0,00,(unsigned char*)(&dwResp),1))
       ==SUCCESS)
      break;
    for(nWait=0;nWait<0x0ffff;nWait++);
  }*/ //end of for loop
  
  //printk("sd_mmc_sdio_initialise_sd:CMD0 retval=  %lx\n",dwRetVal);
  //printk("sd_mmc_sdio_initialise_sd:CMD0 retry count=  %x\n",nRetry);
  //printk("sd_mmc_sdio_initialise_sd:CMD0 reeponse=  %lx\n",dwResp);
/*
  if(nRetry>=CMD0_RETRY_COUNT || dwRetVal!=SUCCESS)   
  {
    dwRetVal = Err_Enum_CMD0_Failed;
    goto End_CMD2;
  }
*/
  //Send ACMD41 with Argument=voltage window.
  /*  Required only for forced voltage setting
      dwVoltWindow = SD_VOLT_RANGE; */
  dwRetVal=0;
  // *CMD1 with arg=0 not worked for mmc. 
  //Send ACMD41 with arg0.
  dwR3response=0; 
  dwVoltWindow =0;
  if(*cardtype==SD2)
      dwVoltWindow |= 0x40000000;
  dwRetVal=Send_ACMD41(nPortNo,dwVoltWindow,(unsigned char*)(&dwR3response));
  if(dwRetVal!=0)
    printk("sd_mmc_sdio_initialise_SD card: acmd41 with arg0 failed.\n");
  //Allow predefined range only. 
  dwR3response &= SD_VOLT_RANGE;
  dwVoltWindow = dwR3response | 0x80000000;
  if(*cardtype==SD2)
  {
      dwVoltWindow |= 0x40000000;
      dwVoltWindow &= 0x7FFFFFFF;
  }
  for(nRetry=0;nRetry<ACMD41_RETRY_COUNT;nRetry++)
  {
    dwR3response=0;
    //Send ACMD41. 
    dwRetVal=Send_ACMD41(nPortNo,dwVoltWindow,(unsigned char*)(&dwR3response));

    //printk("response received is %lx \n",dwR3response);

    //After bit31 is set,if ACMD41 is sent again,card goes into inactive state.
    if((dwRetVal==SUCCESS) && (dwR3response & 0x80000000))   break;
    for(nWait=0;nWait<0x0ffff;nWait++);
    for(nWait=0;nWait<0x0ffff;nWait++);
  }//end of for loop

  if(dwR3response & 0x40000000)
      *cardtype=SD2_1;

  if(nRetry>=ACMD41_RETRY_COUNT || dwRetVal!=SUCCESS)   
  {
    dwRetVal = Err_Enum_ACMD41_Failed;
    goto End_CMD2;
  }

  //Send CMD2.
  for(nRetry=0;nRetry<CMD2_RETRY_COUNT;nRetry++)
  {
    /*Send CMD2.(ALL_SEND_CID)*/
    //CMD2 may fail due to:
    // 1.Command not accepted IP : Repeat command.
    // 2.Card not responded (Resp.timeout error) : Card will not respond 
    //  to retried CMD2.So,donot repeat CMD2.
    // 3.Card responded,but reponse error or crc error: Card has changed
    //  state and further CMD2 will be illegal for it.So,donot
    //  repeat CMD2.    
    //Response timeout indicates,no card is unidentified.
    if((dwRetVal=form_send_cmd(nPortNo,CMD2,00,&tR2.bResp[0],1))== SUCCESS)
      break;
  
    if(dwRetVal!=Err_Command_not_Accepted) break;

    for(nWait=0;nWait<0x0ffff;nWait++);
  }//end of retry loop.
    
  //printk("sd_mmc_sdio_initialise_sd:CMD2 retval=  %lx\n",dwRetVal);
  //printk("sd_mmc_sdio_initialise_sd:CMD2 retry count=  %x\n",nRetry);
  //printk("sd_mmc_sdio_initialise_sd:CMD2 reeponse=  %lx\n",*((unsigned long*)&tR2.bResp[0]));

  //If no response to CMD2,card is not in identification state.
  //for other response errors,continue enumeration.
  if(nRetry>=CMD2_RETRY_COUNT || (dwRetVal == RESP_TIMEOUT))   
  {
    dwRetVal = Err_Enum_CMD2_Failed;
    goto End_CMD2;    
  }
  
  //Validate response.Donot check for any error.Just store CID value.
  //Proceed for cmd3,evenif cmd2 returns response error.
  if(dwRetVal == SUCCESS)
    dwRetVal=Validate_Response(CMD2,&tR2.bResp[0],nPortNo,DONOT_COMPARE_STATE);
  //else if response ie CID data is to be read in case of reponse errors also,
  //read it from response0 to response3 registers.
  
  /*Send CMD3.(SEND_RELATIVE_ADDR)*/
  //Note: For CMD3, argument = RCA (bit31:16) + stuff bits(bits15:0)
  //CMD3 may fail due to:
  // 1.Command not accepted IP : Repeat command.
  // 2.Card not responded (Resp.timeout error) : Card will not respond 
  //  to retried CMD.So,donot repeat CMD.
  // 3.Card responded,but reponse error or crc error: Card has changed
  //  state and will not respond it.So,donot repeat CMD2.    
  //Ignore response errors.For Err_Command_not_Accepted,retry CMD.
  for(nRetry=0;nRetry<CMD3_RETRY_COUNT;nRetry++)
    {
      if((dwRetVal = form_send_cmd(nPortNo,CMD3,dwRCA,
                                   (unsigned char*)(&tR6.dwReg),1)) ==SUCCESS) break;
      
      if(dwRetVal !=Err_Command_not_Accepted) break;
      
      for(nWait=0;nWait<0x0ffff;nWait++);
      
    }//end of repeatation of CMD3,if Err_Command_not_Accepted.
  
  //printk("sd_mmc_sdio_initialise_sd:CMD2ssss retval=  %lx\n",dwRetVal);
  //printk("sd_mmc_sdio_initialise_sd:CMD2 retry count=  %x\n",nRetry);
  //printk("sd_mmc_sdio_initialise_sd:CMD2 reeponse=  %lx\n",tR6.dwReg);

  // printk("CMD3 response received is %lx ,,dwRetVal=%d\n",tR6.dwReg,dwRetVal);
  
  //Validate response.Card should be in Identification state
  //This response is not same as R1.Card status register is not
  //fully received in response.
  //If CMD3 failed or no valid response for CMD3 then,card is not
  //enumerated properly.return error.
  if(dwRetVal==SUCCESS)
  {
    if((dwRetVal=Validate_SD_CMD3_Response(tR6.Bits.Card_status))
       == SUCCESS)     
      //Save card_RCA in Card_Info structure.
      Card_Info[nPortNo].Card_RCA= tR6.Bits.Card_RCA;
      //printk("Card RCA is %x \n",Card_Info[nPortNo].Card_RCA); 
  } 
    
 End_CMD2:
  //printk(" : Exiting initialise_sd with dwREtval= %lx \n",dwRetVal);
  return dwRetVal;
}//End of sd_mmc_sdio_initialise_SD().

//--------------------------------------------------------------      
int Validate_SD_CMD3_Response(int nCard_Status)
{
  int nState;
  
  //Check if bits 13,14,15 are set.Error bits 19,22,23 in card 
  //status register.
  if(nCard_Status & 0xe000)
    return Err_Enum_CMD3_Failed;

  //Check if card is in identification state.
  nState = (nCard_Status & 0x1e00) >> 9;
  if(nState!=2)
    return Err_Enum_CMD3_Failed;

  return SUCCESS;
}

//--------------------------------------------------------------      
int Send_ACMD41(int ncardno,unsigned long dwOCR, unsigned char* pbR3Response)
{
  unsigned long dwRetVal;

  R1_RESPONSE  tR1;

  //Send CMD55 first.
  if((dwRetVal = form_send_cmd(ncardno,CMD55,00,
      (unsigned char*)(&tR1.dwStatusReg),1)) == SUCCESS )
  {
    //Send ACMD41.
    dwRetVal=form_send_cmd(ncardno,ACMD41,dwOCR,pbR3Response,00);
  }
  return dwRetVal; 
}

//--------------------------------------------------------------      
/*sd_mmc_sdio_initialise_SDIO :sd_mmc_sdio_initialises SDIO card.
   
   *Called from  : Enumerate_card()
    
   *Parameters   : 
    nPortNo(i/p): Port no.
    dwRCA(i/p): card RCA
    pnSDIO_type(i/p): card type

   *Return Value :
    0 on Success
    Err_Enum_SendOCR_Failed

   *Flow:
       -Send CMD5 with arg0 to read voltage range supported by card.
       -Send CMD5 with voltage window as an argument.
        Repeat the command till card busy bit (bit31) in response,
        is 0.
        After retry count over,return error.
       -Save card OCR in Card_Info. 
       -if (memory is present)
            Call sd_mmc_sdio_initialise_SD() to enumearte SDIO_memory portion.
        else
        {
            Set RCA for SDIO_only card using CMD3
            Return if status returned in response,indicate any error.
            Save card RCA in Card_Info structure.
        }
       -If initialisation successful
          -Set Card_Info[nPortNo].Card_Connected=TRUE;
          -Send CMD7 to bring card from standby to command state.     
          -Set card type.(SDIO_COMBO/SDIO_MEM/SDIO_IO)
       -return. 
 */
int sd_mmc_sdio_initialise_SDIO(int nPortNo,unsigned long dwRCA,int *pnSDIO_type,unsigned long dwResp)
{
   R4_RESPONSE  tR4={0x00};
   R6_RESPONSE  tR6={0x00};

   unsigned long dwRetVal,dwVoltWindow;
   int nRetry,FuncCount,MP,IO,MEM,nWait;

   IO =0;
   MEM=0;

   //Not allowed before enumeration.
   //Send IO reset. Using CMD52,set RES (bit3,address 6) of CCCR.

   //Send CMD5 with arg0 to read voltage range supported by card.
   dwVoltWindow=0;
  // dwRetVal = form_send_cmd(nPortNo,CMD5,00,(unsigned char*)(&tR4.dwReg),1);
   //printk("SDIO card: CMD5 with arg0 returns %lx with response as  %lx \n",dwRetVal,tR4.dwReg);
   tR4.dwReg=dwResp;
   //Set voltage window.Repeat till C bit is set.
   dwVoltWindow = (tR4.dwReg & 0x00ffffff) & SD_VOLT_RANGE;
   //printk("CMD5 voltage window is %lx \n",dwVoltWindow);

   nRetry = CMD5_RETRY_COUNT;
   do
   {
      if(!(nRetry--))
        return Err_Enum_SendOCR_Failed;

      dwRetVal= form_send_cmd(nPortNo,CMD5,dwVoltWindow,(unsigned char*)(&tR4.dwReg),1);
      //      printk("SDIO card: CMD5 returns %lx with response as  %lx \n",dwRetVal,tR4.dwReg);

      //Wait till IORDY signal gets cleared.1 indicates,card not
      //yet ready.

      for(nWait=0;nWait<0x0ffff;nWait++);
      for(nWait=0;nWait<0x0ffff;nWait++);

   }while(!(dwRetVal==SUCCESS && (tR4.Bits.IO_Ready)));

  //printk("\n Value of nRetry : %x",nRetry);    
  //Save card OCR in Card_Info.Return when asked by user.
  Card_Info[nPortNo].Card_OCR=dwVoltWindow;

  //Card is SDIO card.
  MP=tR4.Bits.MemPresent;
  FuncCount=tR4.Bits.NoOfFunctions;
  Card_Info[nPortNo].no_of_io_funcs = FuncCount;
  //printk("SDIO card: MP=%x, FuncCount=%x \n",MP,FuncCount);

  //Check if OCR sent by card matches the requirement.If not,
  //send the card in inactive state.
  //Optional:Send Voltage window for required card.
  //If doesnot match with card,card goes into inactive state.
  if(FuncCount>0)
  {
    //Check if OCR is valid ie if it matches the requirement.
    //OCR range is to be supplied by user.
    //SDIO combo card has different voltage ranges for
    //IO and MEM.   
    IO=1;
  }

  //Init SDIO_Mem similar to SD mem,if memory is present.
  if(MP)
  {
    if( (dwRetVal = sd_mmc_sdio_initialise_SD(nPortNo,dwRCA,&MP))==SUCCESS)
    //SDIO card has memory.
      MEM=1;
      Card_Info[nPortNo].memory_yes = 1;
  }
  else
    //Set RCA for SDIO_only card. 
    if(IO)
    {
      if((dwRetVal = form_send_cmd(nPortNo,CMD3,dwRCA,
         (unsigned char*)(&tR6.dwReg),1)) ==SUCCESS)
      {
        //check if any error bit is set.
        if(tR6.Bits.Card_status & 0xe010)
          return Err_Enum_CMD3_Failed;
 
        //Save info in Card_Info structure.
        Card_Info[nPortNo].Card_RCA= tR6.Bits.Card_RCA;
      }
    }

   //printk("SDIO card: RCA returned is =%x \n",Card_Info[nPortNo].Card_RCA);
   // printk("SDIO card: retval after CMD3 is %lx \n",dwRetVal);

   if(dwRetVal==SUCCESS)
   {

    //without this cmd7 will fail. 
    Card_Info[nPortNo].Card_Connected=TRUE;
        
    //send CMD7 to bring card from standby to command state.
    if((dwRetVal = form_send_cmd(nPortNo,CMD7,00,(unsigned char*)(&tR6.dwReg),
       1))  !=SUCCESS)  return dwRetVal;

    //Set card type.
    if(IO==1 && MEM==1)    *pnSDIO_type  = SDIO_COMBO;
    else 
      if(IO==0 && MEM==1) *pnSDIO_type = SDIO_MEM;
      else
        if(IO==1 && MEM==0) *pnSDIO_type = SDIO_IO;

        if(IO || MEM)
          Card_Info[nPortNo].Card_Connected=TRUE;
       
   }

   return dwRetVal;
}//End of sd_mmc_sdio_initialise_SDIO.

//--------------------------------------------------------------      
int Identify_Card_Type(int ncardno,unsigned long *pdwResp)
{
    int nRetry,nCardType,nWait ;
    unsigned long dwRetVal ; 
    R4_RESPONSE tR4;
    int R7_RESPONSE[4];

    //sdio volt.window should be 0.
    unsigned long dwVoltWindow = 0;
    _ENTER();  
    nCardType=INVALID;

    //After initialisation,if card is to be enumerated
    //further,reset command is necessary.After power-on,this command
    //will fail.So,ignore it.
    dwRetVal = SDIO_Reset(ncardno);

      //Send CMD5.
    nRetry = CMD5_RETRY_COUNT;
      nRetry = 5;
    do
    {
        if(!(nRetry--))
        return Err_Enum_SendOCR_Failed;

        dwRetVal=form_send_cmd(ncardno,CMD5,dwVoltWindow,(unsigned char*)(&tR4.dwReg),1);
        msleep(8);
    }while( !(dwRetVal & RESP_TIMEOUT) && (dwRetVal !=SUCCESS) ) ;
  
    if(dwRetVal==SUCCESS)
    {
        nCardType=SDIO;
        printk("detect SDIO device!\n");
        *pdwResp=tR4.dwReg;
    }
    else    if(dwRetVal & RESP_TIMEOUT) //SDmem/MMC
    {
        //Send CMD0 first.
        dwRetVal = form_send_cmd(ncardno,CMD0,00,(unsigned char*)(&tR4.dwReg),1);
        for(nWait=0;nWait<0x0ffff;nWait++);
        
        //Send ACMD41 to differentiate between SD mem and MMC.
        nRetry = CMD8_RETRY_COUNT;
        nRetry = 5;
        do
        {
            if(!(nRetry--))
            break;

            //CMD8 to be sent with ARG=0x00155,.
            dwVoltWindow = 0x00155;        

            dwRetVal= form_send_cmd(ncardno,CMD8,dwVoltWindow,(unsigned char*)(R7_RESPONSE),00);
            //for(nWait=0;nWait<0x0ffff;nWait++);
            msleep(8);

        }while(!(dwRetVal & RESP_TIMEOUT) && nRetry>0 && (dwRetVal!=SUCCESS));

        if(dwRetVal==SUCCESS)
        {
            printk("detect SD2.0 card\n");
            nCardType=SD2;
            return nCardType;
        }      
      
        nRetry = ACMD41_RETRY_COUNT;
        nRetry = 5;
        do
        {
            if(!(nRetry--))
                return Err_Enum_ACMD41_Failed;

            //ACMD41 to be sent with ARG=0.
            dwVoltWindow = 0;

            //Send ACMD41. 
            dwRetVal= Send_ACMD41(ncardno,dwVoltWindow,(unsigned char*)(&tR4.dwReg));
            //       printk("send acmd41!\n");
            //for(nWait=0;nWait<0x0ffff;nWait++);
            msleep(8);
        }while(!(dwRetVal & RESP_TIMEOUT) && nRetry>0 && (dwRetVal!=SUCCESS));

        if(dwRetVal & RESP_TIMEOUT) 
        {
            nCardType=MMC;
            printk("detect mmc card!\n");
        }
        else if(dwRetVal==SUCCESS) 
        {
            printk("detect SD1.0 card!\n");
            nCardType=SD;
        }
    }//
    _LEAVE();
    return nCardType;

}
//--------------------------------------------------------------
//Using CMD52,isue IO_Reset command for SDIO card.
int SDIO_Reset(int ncardno)
{
  unsigned long dwRetVal,dwResp;
  SDIO_RW_DIRECT_INFO tRWdirectInfo={0x00};

  dwResp=00;

  /*Sanity check:*/
  //Since,during enumeration,card status is "Not Connectes",
  //it is of no use to check sanity.

  //For IO command,no need to check the current state of card.

  /*Form command:Send CMD52 (IO_RW_Direct command).*/
  //command argument for CMD52. 
  tRWdirectInfo.Bits.WrData = 0x8;
  tRWdirectInfo.Bits.RegAddress = 0x6 ;
  tRWdirectInfo.Bits.RAW_Flag = 0  ;
  tRWdirectInfo.Bits.FunctionNo = 00 ;
  tRWdirectInfo.Bits.RW_Flag = TRUE  ;

  //Non data command.So,use form_send_cmd
//  printk("send sdio_reset cmd!\n");
  if(form_send_cmd(ncardno,SDIO_RESET,tRWdirectInfo.dwReg,
     (unsigned char*)&dwResp,00) != SUCCESS)  
    return Err_CMD_Failed; 
// printk("SDIO_reset send cmd success!\n"); 
  //Validate_Response.
  dwRetVal=Validate_Response(CMD52,(unsigned char*)&dwResp,ncardno,
                             DONOT_COMPARE_STATE);
  return dwRetVal;     
}

//--------------------------------------------------------------
//This is called from sd_mmc_event_handler when card detect
//interrupt is detected.
//Card_Eject  = 1 when there is a transition from connected to 
//                disconnected state.
//Card_Insert = 1 when there is a transition from disconnected to 
//                connected state.

void Card_Detect_Handler()
{
    int i,j, nDetectBit=0;
    unsigned long dwRetval, dwRegData=0;
    #ifdef _GPIO_CARD_DETECT_
        unsigned char cardstatus,cardstatus1;
        //sd_readb((GPIO_BASE_ADDR(CARD_DETECT_GPIO_GNUM)+0x004),cardstatus); 
	GPIO_CARD_DETECT_IN;
	i = 0;
	j = 0;
	do{
		cardstatus = GPIO_CARD_DETECT_READ;
		udelay(1000); //61104
		cardstatus1 = GPIO_CARD_DETECT_READ;
		if(cardstatus1 == cardstatus) j++;
		else j--; 
		i++;
		if(i >= 100){ 
			printk("too long for pull card tremble!!\n");
			break;
		}
	}while(j < 5);



        if(!cardstatus)
            dwRegData=0xfffffffe;
        else
            dwRegData=0xffffffff;  
    #else
        Read_Write_IPreg(CDETECT_off,&dwRegData,READFLG);
    #endif
    //Read_Write_IPreg(CDETECT_off,&dwRegData,READFLG);
    dwRegData = (dwRegData & 0x01);
    G_CARDS_CONFIGURED = 1;
    //Check status of all cards connected.
    for(i=0;i<G_CARDS_CONFIGURED;i++)
    {
        nDetectBit = (dwRegData >> i) & 0x1;

        if(nDetectBit) // 1=>card disconnected.
        {
            HI_TRACE(2, "card removed!\n");
            wake_up_interruptible(&data_wait);        //20071225 add this line

            //y00107738 add for problem No. AE6D03528
            Read_Write_IPreg(CLKENA_off, &dwRegData, READFLG);
            dwRegData = dwRegData & (~0x1);           
            Read_Write_IPreg(CLKENA_off, &dwRegData, WRITEFLG);
            //y00107738 change end.
            
            if(Card_Info[i].Card_Connected==TRUE)
            {
                //since the card is removed decrement the card count
                if(Card_Info[i].Card_type == MMC)
                    tIPstatusInfo.Total_MMC_Cards -= 1;
                else
                {
                    if(Card_Info[i].Card_type == SD)
                        tIPstatusInfo.Total_SD_Cards -= 1;
                    else
                        tIPstatusInfo.Total_SDIO_Cards -= 1;
                }
                memset(&Card_Info[i],0,sizeof(struct Card_info_Tag));
                Card_Info[i].Card_Eject=TRUE;
                Card_Info[i].Card_Insert=FALSE;
                //Update the total count            
                tIPstatusInfo.Total_Cards = tIPstatusInfo.Total_MMC_Cards +
                                    tIPstatusInfo.Total_SD_Cards  +
                                    tIPstatusInfo.Total_SDIO_Cards;
            }
            Card_Info[i].Card_Connected=FALSE;
            Card_Info[i].Do_init=0;
        }
        else
        {
            HI_TRACE(2, "card detected!\n");
            ////weimx 20071123 add the below lines*******SDIO controller software reset**********************************
            #ifdef _HI_3560_
                dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
                dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);    
                *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
                for(i=0;i<0xff;i++);
                dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
                dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);            
                *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
            #endif

            #ifdef _HI_3511_  //3511 branch
                dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
                dwRetval &= ~(1<<SDIO_SOFT_RESET_SCTL_LOC);    
                *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;  
                for(i=0; i<0xff; i++);
                dwRetval=*((volatile unsigned long *)SDIO_SOFT_RESET_SCTL);
                dwRetval |= (1<<SDIO_SOFT_RESET_SCTL_LOC);    
                *((volatile unsigned long *)SDIO_SOFT_RESET_SCTL)=dwRetval;
                for(i=0;i<0xff;i++);    
            #endif

            //y00107738 add for problem No. AE6D03528
            Read_Write_IPreg(CLKENA_off, &dwRegData, READFLG);
            dwRegData = dwRegData | 0x1;            
            Read_Write_IPreg(CLKENA_off, &dwRegData, WRITEFLG);
            //y00107738 change end.
            
            //Card_Detect_Handler();                //20071123 add this line
            printk("Card stack checking, please wait ................................\n\n");
            dwRetval = sd_mmc_sdio_initialise();        //weimx 20071123 add the below lines
            printk("Check card stack end,now you can your job......................\n");
            if(dwRetval != SUCCESS)
            {
                printk("Insert card, card stack initialize failed!\n");
                return;
            }
        }//end of if loop.

    }//End of for loop.  

    if(func[2] != NULL)
    {
        func[2]();     //block device detect hook func
    }
    if(func[3] != NULL)
    {
        func[3]();    //wifi device detect hook func
    }

    return ;
}

//--------------------------------------------------------------

//Setting IP parameters : 
//==============================================================
/*B.1.7. Set timeout values.:Store values for read data timeout 
   and response time out .

  Read data timeout = Nac = 10 * ( (TAAC*Fop) + (100*NSAC) )
  Response timeout  = 64 fixed.

  Set_timeout_values()

  *Called from : BSD::sd_mmc_sdio_initialise()

  *Paramemters : Nothing.

  *Flow :
         -Set Read access timeout value=0xffffff. 
         -Set Response timeout value=64 fixed.
         -Store timeout values.Write at 0x014.
         -Return;
*/
void Set_timeout_values(void)
{
  TimeOutReg  tTimeOut = {0x00000000};

  //Calculate Data timeout value.
  //tTimeOut.Bits.DataTimeOut= Calculate_Data_Timeout();
  tTimeOut.Bits.DataTimeOut= 0xffffff;

  //Response timeout value = 64 fixed.
  tTimeOut.Bits.ResponseTimeOut = MAX_RESPONSE_TMOUT;

  //Write both timeout value in TMOUT register. Host function.
  Read_Write_IPreg(TMOUT_off,&tTimeOut.TMOUTReg,
                   WRITEFLG);
  //**No need to issue start_cmd command as it will be issued
  //when actual command will be sent.

  return;
}
//--------------------------------------------------------------
/*B.1.7.Calculate_Data_Timeout:calculates data timeout value.

  Nac = 10 * ( (TAAC*Fop) + (100*NSAC) )
  TAAC and NSAC from CSD data.
  Take maximum values for this.During enumeration,when CSD
  data is read,find out maximum values and store them in 
  IP data.
  Fop = Maximum clock frequency amongst all the cards.
      = Basic CIU clock frequ./Minimum clk.divider value.

  Calculate_Data_Timeout()

  *Called from : BSD::Set_timeout_values()

  *Parameters:Nothing.

  *Flow:
  -Get maximum of TAAC and NSAC,amongst all connected
  cards.
  tpIPCurrentInfo->Max_TAAC_Value;
  tpIPCurrentInfo->Max_NSAC_Value;
  
  -Get Minimum clock divider value.
  tpIPCurrentInfo->Clk_Divider_Reg = MiniDividerVal()
  
  -Fop= Basic CIU clk/Minimum divider value
  -DatareadTimeout = 10 * ( (TAAC*Fop) + (100*NSAC) )
  
  -Store Fop in system info ie 
  (tpIPCurrentInfo->MaxOperatingFreq).
  -Return DatareadTimeout;
  
  Nac=10[ (Val*10exp-1)(10exp-n)(25*10Exp6)/Minimum_divider)
  + (100*NSAC) ]
  =[ (Val)(25/Divider)(10Exp(n-6) + (1000*NSAC) ]
  n varies from -9 to -2 ie EXP varies from -3 to +4
  ie (1 to 7) /1000
  =[(Val)(25/Divider)(TAAC_EXP[i]/1000) + (1000*NSAC) ]
*/
unsigned long Calculate_Data_Timeout()
{
  unsigned long dwTAACVal,dwNSACVal,Nac;
  //To check if float is required for this? long double dwtemp1;
  unsigned long dwtemp1;

  int nVal,nExp;


  //Get maximum of TAAC and NSAC,amongst all connected cards.
  dwTAACVal = tIPstatusInfo.Max_TAAC_Value;
  dwNSACVal = tIPstatusInfo.Max_NSAC_Value;
    
  nVal = (dwTAACVal & 0x078) >> 3;
  nExp = (dwTAACVal & 0x07);
        
  //Maximum Fop = 12.5 Mhz for MMC only & =25 Mhz for SD_MMC mode.
  //TAAC_EXP[nExp] gives value in nS.Clock is in MHz,so
  //EXP is divided by 1000.(10exp-9*10exp6=10exp-3)
#ifdef MMC_ONLY_MODE
  //First perform multiplication & then divide so that 
  //fractions will be taken care of.
  //Nac = ((TAAC_VAL[nVal] * 125 * TAAC_EXP[nExp])/(10*1000) )
  //      +  (1000*dwNSACVal);

  dwtemp1 = (TAAC_VAL[nVal] * 125);
  dwtemp1 = (dwtemp1 * TAAC_EXP[nExp]);
  dwtemp1 = (dwtemp1 /(10*1000)) +  (1000*dwNSACVal);

  Nac = (unsigned long)dwtemp1;

#else
//    Nac = ((TAAC_VAL[nVal] * 25) * (TAAC_EXP[nExp])/1000) ) + 
//          (1000*dwNSACVal);

  dwtemp1 = (TAAC_VAL[nVal] * 25);
  dwtemp1 = (dwtemp1 * TAAC_EXP[nExp]);
  dwtemp1 = (dwtemp1 / 1000) +  (1000*dwNSACVal);

  Nac = (unsigned long)dwtemp1;

#endif

  //To cater for fractional part in TAAC calculation,add 1.
  Nac += 1;

  //eg 0x26 gives,val=4,exp=6
  //Nac= 15*25*(1x 1000000)/1000+1000NSAC =375*1000 + 1000NSAC

  return Nac;
}
//--------------------------------------------------------------
/*B.1.9.  Minimum divider value is saved in IP_Info.
  Maximum frequency for MMC_only mode is 12.5 Mhz and for SD_MMC
  mode,it is 25 Mhz.
*/
//--------------------------------------------------------------
/*B.1.10.Set debounce period.: Writes debounce value.

  Set_debounce_val(unsigned long dwDebounceVal)

  *Called from : BSD::sd_mmc_sdio_initialise()
 
  *Paramemters :
  nDebounceVal:(i/p):Debounce value to be programmed.
  *Flow :
  -Write debounce count in Debounce count register @0x064.
  Call Read_Write_IPreg(SetDebouncereg, &nDebounceVal,
  False)
  -Return;
*/
void Set_debounce_val(unsigned long dwDebounceVal)
{
  //Host function.
  //Write debounce count in Debounce count register @0x064.
  Read_Write_IPreg(DEBNCE_off, &dwDebounceVal,WRITEFLG);
  
  return ;
}

//--------------------------------------------------------------
/*B.1.11.Set fifo threshold value. 

  Set_FIFO_Threshold(unsigned long dwFIFOThreshold,int fRcvFIFO,
                     int fDefault)

  *Called from : BSD::sd_mmc_sdio_initialise()

  *Paramemters :
         dwFIFOThreshold:(i/p):FIFO threshold value to be
         programmed.
         fRcvFIFO:(i/p):1=Receive FIFO threshold.
                        0=Transmit FIFO threshold.
         fDefault:(i/p):1=Write default values for Receive FIFO
                          and transmit FIFO thresholds.

  *Flow :
        -If default flag is set,write default values in 
        Threshold register.Ignore other two parameters
        supplied with this. Return.
        
        -Check if threshold value is more than 12 bits.
        If yes,return error.

        -If(fRcvFIFO)
        Write threshold value as Receive FIFO watermark.
           
        Call Read_Write_IPreg(Set_RECV_FIFO_THRESHOLD,
        &dwFIFOThreshold,
        False)
        else
        Write threshold value as Transmit FIFO watermark.
        Call Read_Write_IPreg(Set_TRANS_FIFO_THRESHOLD,
        &dwFIFOThreshold,
        False)
        -Return;
*/
int Set_FIFO_Threshold(unsigned long dwFIFOThreshold,int fRcvFIFO,
                       int fDefault)

{
  FIFOthreshold  tThreshold = {0x00000000};
    
  //Check if default flag is set.
  if(fDefault)
  {
    //Write default values.
    //tThreshold.Bits.RX_WMark=DEFAULT_THRESHOLD_REGVAL;
    //tThreshold.Bits.TX_WMark=DEFAULT_THRESHOLD_REGVAL;
    tThreshold.Bits.RX_WMark=FIFO_Depth/2 - 1;    //w53349修改，接收水线设为7，发送水线设为8
    tThreshold.Bits.TX_WMark=FIFO_Depth/2;

    Read_Write_IPreg(FIFOTH_off,&tThreshold.dwReg,WRITEFLG);
  }
  else //Customised threshold value.
  { 
    //Check if threshold value is more than 12 bits.
    if(dwFIFOThreshold > MAX_THRESHOLD ||
       dwFIFOThreshold < MIN_THRESHOLD)
      return Err_Invalid_ThresholdVal;  

    //Write threshold value in register.
    if(fRcvFIFO)
       set_ip_parameters(SET_RECV_FIFO_THRESHOLD,
                         &dwFIFOThreshold);
    else
       set_ip_parameters(SET_TRANS_FIFO_THRESHOLD,
                         &dwFIFOThreshold);
  }//End of else part.

  return SUCCESS;
}


//--------------------------------------------------------------
/*B.1.12.Enable_OD_Pullup() :For MMC only mode.
  During initialisation,enable pull up resistor.

  Added as a case in Read_Reg() of host driver.
  Set bit24 of control register.@0x00.

  void Enable_OD_Pullup(int nEnable) 

  *Called from : BSD::sd_mmc_sdio_initialise()

  *Paramemters:
        nEnable:(i/p):1=Enable, 0=disable.

  *Return value : 
   Nothing

  *Flow :
        -Call Read_Write_IPreg(Enable_OD_Pullup, &nEnableVal,
                               False) 
        -Return;
*/
void Enable_OD_Pullup(int nEnable) 
{
  unsigned long dwEnable = (unsigned long)nEnable;
  
  set_ip_parameters(ENABLE_OD_PULL_UP,&dwEnable);

  return ;
}
//--------------------------------------------------------------
/*B.1.13.Set_default_Clk:
  Out of four clock sources,two are given to SD/SDIO
  and two are given to MMC cards.
  
  If other type of card is not present,all the four clocks 
  will be given to the existing type of card.

  Once,number of clk resources are fixed,they are equally 
  divided amongst available cards.

  For SD clock source, default frequency = 25 MHz
  ie divider=1.

  For MMC clock source, default frequency = 12.5 MHz
  ie divider=2.

  *Called From:sd_mmc_sdio_initialise().

  *Parameters : Nothing.

  *Flow:
       -Get total MMC and SD cards.
        nMMCcards = tpIPCurrentInfo->Total_MMC_Cards;
        nSDcards  = tpIPCurrentInfo->Total_SD_Cards;
       -If(nMMCcards==0)
          SDClkSources = 4.
        else
          If(nMMCcards<2)
            SDClkSources = 3
          else
            SDClkSources = 2
       -MMCClkSources = TotalSources-SDClkSources;//eg 4-3=1
       -Distribute SD clock sources to cards.
       -Distribute MMC clock sources to cards.
       -Store clock resource in each card info structure.
       -Set SD clock divider = 0;//25 MHz frequency.
       -Set MMC clock divider = 1;//12.5 MHz frequency.
       -Enable all clocks.
       -Return.
*/
int Set_default_Clk()
{
//  int nMMCcards, nSDcards ;
  unsigned long dwRegData,dwClkEnable;
  unsigned long dwClkDivdata;

  int i;
//***************************************
    if(Card_Info[0].Card_type==MMC)
       dwClkDivdata = MMC_FREQDIVIDER;//divide by 2.
    else
       dwClkDivdata = SD_FREQDIVIDER;//No divide.

  //Reset card clock frequency to operating frequency value 
  set_clk_frequency(0,dwClkDivdata, (unsigned char *)&dwClkEnable);

//***************************************************

  //Set clock Enable bits for all connected cards.
  dwClkEnable=0x1;
  dwRegData=00;
  for(i=0;i<MAX_CARDS;i++)
  {
    //Check if card is connected.Rotate 1 for all cards.
    if(Card_Info[i].Card_Connected==TRUE)
       dwRegData |= (dwClkEnable << i);
  }

  //Write clock enable register.
  //printk("Writing data= %lx in clockenable register i.e.at offset= %x\n",dwRegData,CLKENA_off);
  Read_Write_IPreg(CLKENA_off,&dwRegData,WRITEFLG);
    
  //Issue start command with update clock bit,to load these parameters.
  dwRegData=0x80200000;
  //For the time being let it be default clock. 
  Read_Write_IPreg(CMD_off,&dwRegData,WRITEFLG);

  return SUCCESS;
}
//--------------------------------------------------------------
/*Assign_Clk_Sources:Assign clock resources for SD and MMC.
  SD and SDIO cards have clock source in common.

  Assign_Clk_Sources(int nCardType1,int nCardType2,
                     unsigned long dwSourceNo, int nClkSrcs,
                     unsigned long dwClkSourceRegData);

  *Called From:Set_default_Clk().

  *Parameters :
         nCardType1:(i/p):Card type.
         nCardType2:(i/p):Alternative card type.
         dwSourceNo:(i/p):Starting clock source number.
         nClkSrcs:(i/p):Total no.of clock sources allocated for
         given type of card(s).
         dwClkSourceRegData:(i/p):Clock source register data.
                 
  *Flow:
        for (total 16 cards )
        {
          -From card info structure,search for given card types.
          -If (card found)
           {
             -set clock source bits = given source number.
                     -Decrement per clock source count.
             -If count of cards per clock resource finished,
                     -Reinitialise the count.
                     -Increment clock source no.
           }
        }
*/
//Total clock sources to be allocated for this type.
unsigned long  Assign_Clk_Sources(int nCardType1,int nCardType2,
                          unsigned long dwSourceNo,int nClkSrcs,//nCardsPerSource,
                          unsigned long dwClkSourceRegData)
{
  int nClksrcNo,nSrccount,i;

  nClksrcNo = dwSourceNo;
  nSrccount = 0;

  //MAX_CARDS=16 for SD_MMC mode.This routine is for SD_MMC 
  //mode only.
  for(i=0;i<MAX_CARDS;i++)
  {
    //Check if card is connected.
    if((Card_Info[i].Card_type==nCardType1 ||
       (Card_Info[i].Card_type & 0x4)==nCardType2) &&
       Card_Info[i].Card_Connected==TRUE   )
        
    {
      dwClkSourceRegData |= (nClksrcNo << (i*2));        
      nSrccount++;
      nClksrcNo++;

      //If clock resource count reaches limit,start from first
      //clock again.
      if(nSrccount==nClkSrcs)
      {
        nClksrcNo=dwSourceNo;
        nSrccount=0; 
      }
      //No need to check if Total_SD_Cards are over.
      //If they are over,card_type will not match and
      //this part will not be executed.     
    }//End of IF loop.
      
  }//End of for loop.

  return dwClkSourceRegData;
}

//--------------------------------------------------------------
//3.Driver stage register setting for each card ?
//--------------------------------------------------------------


//==============================================================
/*B.2.IP register read/write:
      Read_Write_IPreg
      Read_ClkSource_Info
      sd_mmc_sdio_rw_timeout
*/
//--------------------------------------------------------------
/*B.2.2.Read_ClkSource_Info()
  Reads clock source info ie divider value and cards using
  this resource.Reads registers @ 0x008 and 0x00c.

  int Read_ClkSource_Info(CLK_SOURCE_INFO *tpClkSource,int ncardno)

  *Called from: FSD.

  *Parameters:
        -tpClkSource:(o/p):Structure with following members.
         tpClkSource->dwDividerVal=Contents of Clk divider Reg.
                      @0x008
         tpClkSource->dwClkSource =Contents of Clk source Reg.
                      @0x00C
        -ncardno:(i/p):Card number for whom clock source and divider
                       value for that source is to be returned.
  *Return value : 
   0: for SUCCESS
   -EINVAL: Pointer to CLK_SOURCE_INFO is NULL.

  *Flow:
  Sanity_check:
        -Check if tpClkSource is not null.
        -Read Clk divider Reg and store it in tpClkSource.
         Call Read_Write_IPreg(ClkDividerReg, 
                               &tpClkSource->dwDividerVal,TRUE)
        -Read Clk source Reg and store it in tpClkSource.
         Call Read_Write_IPreg(ClkSourceReg,
                               &tpClkSource->dwClkSource,TRUE)
        -Return;               

*/
//This function returns clock source no.and 
//its divider value,for given source.
int Read_ClkSource_Info(CLK_SOURCE_INFO *tpClkSource,int ncardno)
{
  unsigned long dwData=00;
  //Check if tpClkSource is not null.
  if(tpClkSource==NULL)
    return -EINVAL;
  
  //Read Clk source Reg and store it in tpClkSource.
  Read_Write_IPreg(CLKSRC_off,&dwData,READFLG);
  tpClkSource->dwClkSource = (dwData >> (ncardno*2) & 0x3);
  
  //Read Clk divider Reg and store it in tpClkSource.
  Read_Write_IPreg(CLKDIV_off,&dwData,READFLG);
  tpClkSource->dwDividerVal= (dwData >>
                              (tpClkSource->dwClkSource*8)
                              & 0xff);

  return SUCCESS;               
}
//--------------------------------------------------------------
/*B.2.3.sd_mmc_sdio_rw_timeout(unsigned long *pdwtimeoutval, int fwrite)
  Reads or writes supplied value in timeout register.

  *Called from: FSD,BSD::Internal functions.

  *Parameter:
        pdwtimeoutval:(i/p):Pointer to timeout data storage.
        fwrite:(i/p):0=Read ; 1=Write.

        In case of write,value pointed by pdwtimeoutval is 
        written in TMOUT register @0x14 while for read,contents 
        of TMOUT are returned in pdwtimeoutval.

  *Return value : 
   0: for SUCCESS

  *Flow:
        -Read/write to Timeout register using 
         Host::Read_Write_IPreg()

        -Return; 
*/
int sd_mmc_sdio_rw_timeout(unsigned long *pdwtimeoutval, int fwrite)
{
  //Read/write to Timeout register using 
  //Host::Read_Write_IPreg()
  Read_Write_IPreg(TMOUT_off,pdwtimeoutval,fwrite);

  return SUCCESS; 
}

//==============================================================
/*B.3.General operational commands.
      sd_mmc_send_command //Send any command using this.
      power_on_off
      clk_enable_disable
      set_clk_frequency
      Set_Clk_Source
      sd_set_mode
      Set_Interrupt_Mode
      sd_mmc_send_raw_data_cmd
*/
//--------------------------------------------------------------
/*B.3.1.sd_mmc_send_command:
  This is universal interface provided to send standard commands
  to card.(Only non data commands).

  int sd_mmc_send_command(unsigned long dwcmdreg,unsigned long dwcmdarg, 
                 unsigned char *pbcmdrespbuff)
  
  *Called from:Client program.

  *Parameters:
   dwcmdreg(i/p):Command reg
   dwcmdarg:(i/p):Argument to be passed.
   pbcmdrespbuff:(o/p):Response buffer to receive response.
        
  *Return value :
   0 for SUCCESS
   Err_Invalid_CardNo
   Err_Card_Not_Connected
   Err_CMD_Failed

  *Flow:
       -Confirm that card number is valid.
       -Check if card is connected.
       -Send this command for execution without any checking.
        Call form_send_cmd(ptAppInfo,nCommandIndex,ncardno,
                            dwArgument)
       -Return.
*/

int sd_mmc_send_command(unsigned long dwcmdreg,unsigned long dwcmdarg, 
                 unsigned char *pbcmdrespbuff)
{
  int ncardno;
  COMMAND_INFO tCmdInfo;
  //Confirm that card number is valid ie less than MAX_CARDS 
  //and card is connected.
  ncardno  =(dwcmdreg & 0x1f0000) >> 16;

  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;
  
  if(Card_Info[ncardno].Card_Connected==FALSE)
    return Err_Card_Not_Connected;
  
  //Send this command for execution without any checking.
  //So,use send_cmd_to_host.
  
  //Response gets copied in Response_Buffer /Error_Resp_Buff.
  //when command is accepted and executed by Thread2.
  
  //Send command:
  tCmdInfo.dwCmdArg = dwcmdarg;
  tCmdInfo.CmdReg.dwReg = dwcmdreg;
  
  tCmdInfo.dwByteCntReg=00 ;//Only for data command.
  tCmdInfo.dwBlkSizeReg=00 ;//Only for data command.
    
  if(!(dwcmdreg & 0x80000000))
  {   
    printk("start_cmd bit in CMD reg not set.\n Command done will not be received\n");
    return Err_CMD_Failed;
  } 
 // printk("sd_mmc_send_command : sent cmd!\n");
  return Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                          NULL,//No error    response buffer
                          NULL,//No data buffer
                          NULL,//No callback pointer
                          00,  //No flags from user
                          00,   //No retries.
                          FALSE,//No data command
                          FALSE,
                          FALSE );// 0= user address space buffer.
}
//--------------------------------------------------------------
/*B.3.2.Power_on : Power on/off : For SD/MMC mode only.
  Clear respective bit in "Power Enable Register" @0x04.
  Also, reflects this status in card_info structure.
  Aplicable to SD/SDIO/MMC cards.

  power_on_off(int ncardno, int fpwron)

  *Called From:FSD.
  *Parameter:
         ncardno:(i/p):Card number.0x0ff=All cards.
         fpwron:(i/p):Power on off.1=Power On, 0=Power off.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_CardNo
   
  *Flow:
        -If card number if 0x0ff,it means command is applicable
         to all cards.eg Power off all cards.
        -Check if card number is valid.
        -Program corresponding power enable bit in 
         Power enable register.
         Call Host::Set_IP_Parameters().
        -Return.
*/
int  power_on_off(int ncardno, int fpwron)
{
  //If card number if 0x0ff,skip card number validity check.
  //else check if card number is valid.
  if(ncardno !=0x0ff && ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //Program corresponding power enable bit in Power enable reg
  if(fpwron)
    set_ip_parameters(POWER_ON,(unsigned long*)(&ncardno));
  else
    set_ip_parameters(POWER_OFF,(unsigned long*)(&ncardno));

  return SUCCESS;
}

EXPORT_SYMBOL(power_on_off);
//--------------------------------------------------------------
/*B.3.3.Clock enable disable. : 
  - Program "CLKEN" with required inactive/active state 
  - set  update_clock_registers_only and  start_cmd
  - wait for command done interrupt

  clk_enable_disable(int ncardno, int fenable)
 
  *Called from:FSD.

  *Parameters:
        ncardno:(i/p):
        fenable:(i/p):1=Enable clock. 0=Disable clock.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_CardNo

  *Flow:
        -Check if card number is valid.
        -Program corresponding clock enable bit in 
         clock enable register.@0x010.
         Call Read_Write_IPreg(Clk_Enable,&dwClkEnable,FALSE) 
        -Return.

*/
int  clk_enable_disable(int ncardno, int fenable)
{
  //Check if card number is valid. 0x0ff for enabling clock to all cards.
  if(ncardno >= MAX_CARDS && ncardno != 0x0ff)
    return Err_Invalid_CardNo;

  //Program corresponding enable bit in clk enable reg.@0x010.
  Clk_Enable(ncardno,fenable);

  return SUCCESS;
}
EXPORT_SYMBOL(clk_enable_disable);

//--------------------------------------------------------------
/*B.3.4.Change clock frequency : How to select clock divider values?

  With this change,timeout value gets affected.
  If maximum clock frequency is getting incresed,then timeout 
  value has to be increased.Driver will not increase it on its 
  own.

  It is user's responsibility,to change the timeout value
  Change_TimeOut().Also,if clock source for any card
  needs to be changed due to this change,then user can use
  Set_Clk_Source(cardno,sourceno),for this.

  set_clk_frequency(int nclksource, int ndividerval,
                    unsigned char *pbprevval)

  *Called from: FSD,BSD::Internal functions.

  *Parameters:
         nclksource:(i/p):Clock source number(0 to 3)
         ndividerval:(i/p):Value by which clock to be divided.
         pbprevval:(o/p):Pointer to hold value of clksource 
                          prior to change.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_ClkSource:Clock source number more than 3
   Err_Invalid_ClkDivider:Divider value to be set is more than 0x0ff.
   Warning_DataTMout_Changed:Data time out value changed due to
                        change in maximum clock frequency.

  *Flow:
   Sanity_Check:
        -Check clock source value is less than 3.
        -Check divider value is <=0x0ff.
         Else return error.

   Functional Flow:
//No need.        -Grab Host_Cmd semaphore.

        -Write clock divider value in given source.
         Call Host_Drv::Set_ClkDivider_Freq(nclksource,
                                            ndividerval)
         **Set_ClkDivider_Freq disbales all clocks,sets
         clock divider register data,and then rewrites clock
         enable register data.
         **Please note that this routine retains the current 
         state of enable/disable of given source.On its own,
         it doesnot change this status.

        -If Fop with this,is more than the maximum clock freq.
         (tpIPCurrentInfo->MaxOperatingFreq), implemented with 
         the clock sources,then return warning error.
 
         This error signifies that,Data read timeout value needs
         to be changed.User,if required,can take action on this.

         **Store new minimum value of clock divider in IP_Info so
         that when user calls Set_Timeout routine,timeouts can be
         calculated using this minimum value.

//No need.        -Release Host_Cmd semaphore.

        -Return.
*/
int  set_clk_frequency(int nclksource, int ndividerval,
                       unsigned char *pbprevval)
{
  int nRetVal;

  //Check clock source value is less than 3.
  if(nclksource > TOTAL_CLK_SOURCES)
    return Err_Invalid_ClkSource;
  
  //Check divider value is <=0x0ff.8bit space foreach source.
  if(ndividerval > MAX_CLK_DIVIDER)
    return Err_Invalid_ClkDivider;

  //Validate pointer for clock source value storage.
  if(pbprevval==NULL)
    return -EINVAL;

  //Write clock divider value in given source.
  nRetVal = Set_ClkDivider_Freq(nclksource,ndividerval,
                                pbprevval);

  //To change enable/disable state of given source,user has to
  //issue Clock_enable command seperately.This routine retains
  //the current status of the clock source.
  
  //Check if clock divider value is less than minimum divider
  //value.This will affect data read timeout value.
  if((unsigned int)ndividerval < tIPstatusInfo.MiniDividerVal)
  {
    tIPstatusInfo.MiniDividerVal = ndividerval ;
    return  Warning_DataTMout_Changed; 
  }
  
  return nRetVal;
}
EXPORT_SYMBOL(set_clk_frequency);

//--------------------------------------------------------------
/*B.3.5.Set_Clk_Source(int ncardno, int nsourceno).
  Sets the given clock source for given card.

  *Called from: FSD,BSD::Internal functions.

  *Parameters:
         ncardno:(i/p):Card for whom,clk source is to be selected.
         nsourceno:(i/p):Clock source which is to be assigned for
         given card.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_ClkSource
   Err_Invalid_CardNo

  *Flow :
        -Check that given card number and source number are valid.

        -Change clock source for given card.
         Call Host_Drv::Set_Clk_Source(int ncardno,int nSourceNo)

         This routine stops all clocks,sets clock source 
         register data,re-enables clocks.
         **This routine doesnot alter enable/disable state of 
         any clocks source.If user intends to do so,he can issue
         Clock_enable command seperately.

//no need        -Release Host_Cmd semaphore.

        -Return.  
*/
int Set_Clk_Source(int ncardno, int nSourceNo)
{
  //Check clock source value is less than 3.
  if(nSourceNo > TOTAL_CLK_SOURCES)
    return Err_Invalid_ClkSource;

  //Check that given card number and source number are valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //Change clock source for given card.
  return Set_Clock_Source(ncardno,nSourceNo);

  //To change enable/disable state of given source,user has to
  //issue Clock_enable command seperately.This routine retains
  //the current status of the clock source.
}

//--------------------------------------------------------------
/*B.3.6.Set SD mode (1bit/4bit) for SD/SDIO card.:Since,this is 
  not frequently changed by user,it is not added as a 
  command parameter and provided as a seperate function.

  Sets mode of operation for SD/SDIO cards.Writes into card
  registers @0x018.

  sd_set_mode(int ncardno, int fmode4bit)

  *Called from: FSD.

  *Parameters:
         ncardno:(i/p):Card number whose mode is to be set.
         fmode4bit:(i/p):Mode . 0=1-bit mode, 1=4-bit mode.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_CardNo
   Err_CmdInvalid_ForCardType

  *Flow:
        -Check if card number is valid.
        -Check that card type is SD/SDIO.
        -Program corresponding mode bit in Card Type register.
         Call Read_Write_IPreg(Card_Type,&dwMode,FALSE) 
        -Return.
*/
int sd_set_mode(int ncardno, int fmode4bit)
{
  unsigned long  dwRetVal,dwArg,dwR1Resp;
   
  dwArg = 0x0;

  //Check that given card number and source number are valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;
  
  //Check that card type is SD/SDIO.
  if(Card_Info[ncardno].Card_Connected != TRUE)
    return Err_Card_Not_Connected;

  if(Card_Info[ncardno].Card_type==MMC)
    return Err_CmdInvalid_ForCardType;
    
  //Card needs to be programmed for bus width.
  //Send ACMD6 command to card.
  //Send CMD55 first.
  if((dwRetVal = Send_CMD55(ncardno)) != SUCCESS )
    return Err_CMD55_Failed;

  //Card is in transfer state.Now send ACMD6.
  //For one bit mode,arg.should be 00.
  if(fmode4bit)
     dwArg = 0x02;
  
  if( (dwRetVal=form_send_cmd(ncardno,CMD6,dwArg,(unsigned char*)&dwR1Resp,00))
      !=SUCCESS)  return dwRetVal;
 
  //Validate response.
  if((dwRetVal=Validate_Response(CMD6,(unsigned char*)&dwR1Resp,ncardno,
                                 CARD_STATE_TRAN))!= SUCCESS)  return dwRetVal;
    
  //Program corresponding mode bit in Card Type register.
  if(fmode4bit)
    set_ip_parameters(SET_SDWIDTH_FOURBIT,(unsigned long*)(&ncardno));
  else
    set_ip_parameters(SET_SDWIDTH_ONEBIT,(unsigned long*)(&ncardno));
    
  return dwRetVal;
}
EXPORT_SYMBOL(sd_set_mode);

//--------------------------------------------------------------
//This command is required to be preceded for each ACMD.
//This routine sends CMD55,and brings the card in transfer state.
// ** If card state change is not desired,donot call this routine.
// ** All application specific commands for SD card,are valid in
//    transfer state only.
int Send_CMD55(int ncardno)
{
  unsigned long dwRetVal;
  R1_RESPONSE  tR1;

  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //Send CMD55. This comand is valid in all states.
  dwRetVal = form_send_cmd(ncardno,CMD55,00,(unsigned char*)(&tR1.dwStatusReg),1);

  return dwRetVal;
}
//--------------------------------------------------------------
/*B.3.7.Set interrupt mode.: Only applicable to MMC only mode.

  int Set_Interrupt_Mode(void (*CMD40_callback)(unsigned long dwResponse,
                         unsigned long dwRetVal),unsigned long dwIntrStatus)


  *Called from: Client program.
  *Parameters : 
         CMD40_callback : Callback to be invoked when interrupt
                          mode ends.
                          dwResponse = CMD40 response.
                          dwRetVal = 0,for euccess else response err.code.
                        : NULL ,when called from ISR.
         dwIntrStatus   : Interrupt status                        

  *Return value : 
   0: for SUCCESS
   Err_Command_not_Accepted

  *Flow :
*/
int Set_Interrupt_Mode(void (*CMD40_callback)(unsigned long dwRespVal,unsigned long dwReturnValue),
                       unsigned long dwIntrStatus)
{
  #ifdef MMC_ONLY_MODE
   static unsigned char bOrgVal;
   unsigned long dwBitno=24;
  #endif

  unsigned long dwRetVal=0,dwResponse=0;
  static void (*pLocalCallback)(unsigned long dwResp,unsigned long dwReturnVal);
  COMMAND_INFO tCmdInfo = {0x00};

  if(G_CMD40_In_Process)
  {
    //Read response register.
    Read_Write_IPreg(RESP0_off,&dwResponse,READFLG);

    //Invoke the callback,if command done received..
    if((pLocalCallback!=NULL) && (dwIntrStatus & 0x4))
       pLocalCallback(dwResponse,(dwIntrStatus & 0x00002142));

    //Clear G_CMD40_In_Process.
    G_CMD40_In_Process=0;
       
    pLocalCallback = NULL;

    printk("Interrupt_Mode:Invoked callback.Interrupt mode terminated.\n");
    printk("Interrupt_Mode:Response =%lx ,Intr.Status=%lx\n",dwResponse,dwIntrStatus);

   #ifdef MMC_ONLY_MODE
    //Set clock frequency as original.
    dwRetVal = set_clk_frequency(CLKSRC0,bOrgVal,&bOrgVal);

    //Cards now came out of open drain mode.Disable pull-up.
    set_ip_parameters(CLEAR_CONTROLREG_BIT,&dwBitno);
   #endif

    //Return to ISR.
    return SUCCESS;
  }    
  else
  {
    //Check that all cards are in stand-by state.
    //If any card is not,wait till it goes to standby.
    //Otherwise return error on timeout.
    if((dwRetVal=Check_All_Standby()) != SUCCESS) return dwRetVal;

    //Store callback pointer in local.
    pLocalCallback = CMD40_callback;
      
    //Set G_CMD40_In_Process flag.
    //For CMD40,driver willnot wait for cmd done.So,prior to setting
    //G_CMD40_In_Process=1, no other command should be in process.This  
    //should be checked in send_cmd_to_host.
    G_CMD40_In_Process=1;

   #ifdef MMC_ONLY_MODE
    //In open drain mode,clock should be 0-400Khz.
    //Set clock frequency = fod. Assuming clk=25/2=12.5 MHz.clkdiv is 0x14.
    dwRetVal = set_clk_frequency(CLKSRC0,(CIU_CLK/(MMC_FOD_VALUE*2)),&bOrgVal);

    //CMD40 forces all cards in open drain.Enable pull-up.
    set_ip_parameters(SET_CONTROLREG_BIT,&dwBitno);
   #endif

    //Send CMD40.Wait for command acceptance.
    //Load command in IP.If Resp.Expected bit is not set,CMD_done
    //is received immediately.
    tCmdInfo.dwCmdArg = 00;
    tCmdInfo.CmdReg.dwReg = 0x80000068;

    if((dwRetVal = Send_SD_MMC_Command(&tCmdInfo)) != SUCCESS)
    {
      G_CMD40_In_Process=0;          
      return Err_Command_not_Accepted;
    }      
    else
      return SUCCESS;
  }
  return SUCCESS;
}
//--------------------------------------------------------------
//Set_Interrupt_Mode is available only for MMC_only mode.
//No need for SD/MMC mode.
int Check_All_Standby(void)
{
  int nCardCount;
  unsigned long dwRetVal,dwR1Resp,dwCardStatus;
  COMMAND_INFO tCmdInfo = {0x00};

  //Check if all cards in standby mode.
  //If last command is of data transfer,then that card will be in
  //transfer state.Other states indicate,data transfer is in 
  //progress.
  //For transfer state,send CMD7 with RCA=0x0000.
  for(nCardCount=0;nCardCount< G_CARDS_CONFIGURED ;nCardCount++)      
  {
    //Send CMD13.
    //Check_Card_Status: form_send_cmd(CMD13)
    //If IP reports any error,it is received in dwRetVal.
    if(! Card_Info[nCardCount].Card_Connected) continue;

   // printk("Check_All_Standby : form_send_cmd!\n");
    if( (dwRetVal = form_send_cmd(nCardCount,CMD13,00,(unsigned char*)&dwR1Resp,1))
        !=SUCCESS)  return dwRetVal;

    //Return error if card state is other than standby/transfer.
    dwCardStatus = R1_CURRENT_STATE(dwR1Resp);

    if((dwCardStatus != (unsigned int)CARD_STATE_STBY) &&
       (dwCardStatus != (unsigned int)CARD_STATE_TRAN) )
      return Err_Invalid_Cardstate;         

    //For transfer state,send CMD7 with host address.
    if(dwCardStatus == (unsigned int)CARD_STATE_TRAN)
    {
      tCmdInfo.dwCmdArg = 00;
      tCmdInfo.CmdReg.dwReg = 0x80000047;//0x8000 0047
      tCmdInfo.CmdReg.Bits.card_number = nCardCount;
     // printk("Check_All_Standby : Send_cmd_to_host!\n");
      dwRetVal = Send_cmd_to_host(&tCmdInfo,(unsigned char*)&dwR1Resp,
                          NULL,//No error    response buffer
                          NULL,//No data buffer
                          NULL,//No callback pointer
                          00,  //No flags from user
                          00,   //No retries.
                          FALSE,//No data command
                          FALSE,//
                          FALSE );// 0= user address space buffer.

      //For this command,no response will be received.
      if((dwRetVal !=SUCCESS) && (dwRetVal !=RESP_TIMEOUT)) 
        return dwRetVal;
    }//end of if loop.

  }//end of for loop.
  
  return SUCCESS;
}

//--------------------------------------------------------------
/*B.3.7.Send_CMD40_response.: Only applicable to MMC only mode.

   int Send_CMD40_response(void)

  *Called from: Client program.
  *Parameters : Nothing.
  *Return value : 
   0: for SUCCESS

  *Flow :
*/
int Send_CMD40_response(void)
{
  unsigned long dwBitno=7;
  //Set controlreg::bit7.(Send irq response).
  set_ip_parameters(SET_CONTROLREG_BIT,&dwBitno);
        
  return SUCCESS;
}

//--------------------------------------------------------------
/*B.3.8.sd_mmc_send_raw_data_cmd :Command register is exposed to FSD.
  User can set each bit of command register and send the 
  command directly.Data buffer supplied by caller.

  All registers are filled by user.No validity is checked.
  Command is straightway added in queue.
  .
  int  sd_mmc_send_raw_data_cmd(unsigned long dwcmdregParams,
                           unsigned long dwcmdargReg ,
                        unsigned long dwbytecount,
                        unsigned long dwblocksizereg,
                        unsigned char  *pbcmdrespbuff,
                        unsigned char  *pberrrespbuff,
                        unsigned char  *pbdatabuff,
                        int   nnoofretries,
                          int   nflags)
  *Parameters :
   
         dwcmdregParams:(i/p):Command register contents.
         dwcmdargReg :(i/p):Command argument register.
         dwbytecount:(i/p):unsigned char count register contents.
         dwblocksizereg:(i/p):Block size register contents.
         pbcmdrespbuff:(o/p):Command response buffer.
         pberrrespbuff:(o/p):Error response buffer,if STOP 
                             command to be sent,in case of 
                             error
         pbdatabuff:(i/p-o/p):Data buffer.Holds data for write
                             operation.Data copied in this,for 
                             read.
         nnoofretries:(i/p):Number of retries in case of error.

         nflags:(i/p):Flags.
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                  transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                      Send cmd23 first.
         Handle_Data_Errors (bit4)=1;Handle data errors.
         Send_Abort (bit5)=1;
         Read_Write   (bit7)=1; Write operation. 0=Read.   


  **Error response buffer :If STOP command to be sent,in case of 
  error in data transfer,then response to tjhis is copied in this
  error response buffer.

  *Return value : 
   0: for SUCCESS
   -EINVAL:Command response buffer is NULL.

  *Flow :
         -Check ptCmdRegParams is not NULL.
         -Send command for execution.
          Call Host::Send_SD_MMC_Command().
         -Return.
*/
int  sd_mmc_send_raw_data_cmd(unsigned long dwcmdregParams,
                       unsigned long dwcmdargReg ,
                       unsigned long dwbytecount,
                       unsigned long dwblocksizereg,
                       unsigned char  *pbcmdrespbuff,
                       unsigned char  *pberrrespbuff,
                       unsigned char  *pbdatabuff,
                       void (*callback)(unsigned long dwerrdata),
                       int   nnoofretries,
                       int   nflags)
{
  COMMAND_INFO tCmdInfo;
  unsigned long dwRetVal;
  //Check ptCmdRegParams is not NULL.
  if(pbcmdrespbuff ==NULL )
    return -EINVAL;

  //Fill cmdinfo.
  tCmdInfo.CmdReg.dwReg= dwcmdregParams;
  tCmdInfo.dwByteCntReg = dwbytecount;
  tCmdInfo.dwBlkSizeReg = dwblocksizereg;
  tCmdInfo.dwCmdArg = dwcmdargReg;
  
  if(!(dwcmdregParams & 0x80000000))
  {   
    printk("start_cmd bit in CMD reg not set.\n Command done will not be received\n");
    return Err_CMD_Failed;
  }
  //Send command to host.
  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                            pberrrespbuff,pbdatabuff,
                            callback,
                            nflags,nnoofretries,
                            TRUE,(nflags&0x080),
                            FALSE );// 0= user address space buffer.

  return dwRetVal;
}

//--------------------------------------------------------------
//CSD gives maximum size of wr_block_length and rd_blk_length.
//Maximum size cannot be modified.
//Check if partial block rd/wr is supported.If yes,it means 
//block size can be changed to be less than maximum size given 
//in CSD.

//nSkipPartialchk => To skip partial check or not. 1= skip partial length check.
int  SetBlockLength(int ncardno,unsigned long dwBlkLength, int nWrite,int nSkipPartialchk)
{
  unsigned long dwR1Resp,dwRetVal;
  //Set block length using CMD16.
  //Save block length in card info,after successful execution.
  //If failed,return Err_SetBlkLength_Failed

  //printk("Entered in Setblocklength. \n");

  //printk("Current block size for read= %lx & for write= %lx \n", Card_Info[ncardno].Card_Read_BlkSize, Card_Info[ncardno].Card_Write_BlkSize);  


  //Restrict block size change to 512 bytes only.For 2GB cards,
  //Rd_Blk_Len could be > 512 bytes, but for block transfer is done
  //only with 512 bytes block size, at the maximum.
  if(dwBlkLength > 512) return Err_Invalid_Arg;

  //check if block length exceeds maximum limit.
  if(nWrite) //Write operation.
  {    
    //Check wr_blk_size.
    if(dwBlkLength > (0x0001 << Card_Info[ncardno].CSD.Fields.write_bl_len))
      return Err_SetBlkLength_Exceeds;

    //If partial block check is not to skip,then check if partial blocks are allowed.
    if((!nSkipPartialchk) && (!Card_Info[ncardno].CSD.Fields.write_bl_partial))
      return Err_PartialBlk_Not_Allowed;
  }
  else //Read operation.
  {
    //Check rd_blk_size
    if(dwBlkLength > (0x0001 <<Card_Info[ncardno].CSD.Fields.read_bl_len))
      return Err_SetBlkLength_Exceeds;

    //If partial block check is not to skip,then check if partial blocks are allowed.
    if((!nSkipPartialchk) && (!Card_Info[ncardno].CSD.Fields.read_bl_partial))
      return Err_PartialBlk_Not_Allowed;
  }

  //This status checking is mostly required when user 
  //directly issues this command.Within driver,while
  //calling this routine,usually card will be in TRAN state.

  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //Now send CMD16.
  if(form_send_cmd(ncardno,CMD16,dwBlkLength,(unsigned char*)&dwR1Resp,
     0x2) != SUCCESS)  return Err_CMD16_Failed; 

  if(Validate_Response(CMD16,(unsigned char*)&dwR1Resp,ncardno,
     CARD_STATE_TRAN)!= SUCCESS) return Err_CMD16_Failed;

  //Save changed block length in card_info.
  if(nWrite) //Write operation.
    Card_Info[ncardno].Card_Write_BlkSize = dwBlkLength;
  else
    Card_Info[ncardno].Card_Read_BlkSize = dwBlkLength;

  return SUCCESS;

}


//==============================================================
/*B.4.Read IP info.
        Read_TotalPorts
        Read_total_cards_connected
        Read_Op_Mode
        sd_mmc_sdio_get_id 
*/
//--------------------------------------------------------------
/*B.4.1.Read_Port_numbers : Reads ports to which given 
   type of card is connected. (sd/mmc/sdio) and stores
   port numbers in array provided.

   Array size should be of MAX_CARDS.
   
  int  Read_TotalPorts(int nCardType,int *pnArrPortNo,
                     int nArraySize,int *pnTotalPorts)

  *Called from : FSD

  *Parameters :
         nCardType:(i/p): card type  (SD/SDIO/MMC).
         pnArrPortNo:(o/p):Array holding port numbers having 
                          given type of card connected.
         nArraySize:(i/p):Size of array.
         pnTotalPorts:(o/p):Pointer to store total number of
                            ports.
         
  *Return value : 
   0: for SUCCESS
   -EINVAL:Array for port number storage is NULL.
   Err_ArrSize_Mismatch:Array size is less than total MAX_CARDS. 

  *Flow:-Check if given array pointer is not null.
        -Using (Card_Info->Card_Type),calculate total number
         of ports having given type of card.
        -Store port number in given array.
        -Return total port number.
*/
int  Read_TotalPorts(int nCardType,int *pnArrPortNo,
                     int nArraySize,int *pnTotalPorts)

{
  int i,nArrIndex; 

  //sd_mmc_sdio_initialise array index.
  nArrIndex=0;
    
  //Check if given array pointer is not null.
  if(pnArrPortNo==NULL)
    return -EINVAL;

  //Check if array size is equal to MAX_CARDS.
  //Total cards of given type could be equal to
  //MAX_CARDS.
  if(nArraySize < MAX_CARDS)
    return Err_ArrSize_Mismatch;

  //Check all ports for given card type.
  for(i=0; i<MAX_CARDS;i++)
  {
    //Check if card is connected.
    if(Card_Info[i].Card_Connected)
    {
      //Check if card type is matches.
      if(Card_Info[i].Card_type==nCardType)
      {
        pnArrPortNo[nArrIndex] = i;
        nArrIndex++;
      }//End of if card type matches.
    }//End of if card connected.
  }//End of for loop.

  //Save total card count in pointer.
  *pnTotalPorts = nArrIndex+1;

  return SUCCESS;
}
//--------------------------------------------------------------
/*B.4.2.Read_total_cards_connected.:Total cards connected to IP.
  This info is returned from local structure and not through
  reading IP reg.

  int Read_total_cards_connected()

  *Called from : FSD
  *Parameters : Nothing.

  *Return value :
   Total cards connected to IP.

  *Flow:-Return (IP_Curent_Status_Info->Total_cards_connected);
*/
int Read_total_cards_connected(void)
{
  return tIPstatusInfo.Total_Cards;
}
//--------------------------------------------------------------
/*B.4.3.Read mode of operation: (SD/MMC Only).
  
  Read_Op_Mode()
  *Called from : FSD
  *Parameters : Nothing.
  *Return value :
   Mode of operation of IP. ie SD/MMC or MMC Only.

  *Flow:-Return (IP_Current_Status_Info->Operating_Mode);
*/
int Read_Op_Mode(void)
{
  //Reading HCON will give configuration of RTL.
  //This API will give configuration for which driver is being 
  //compiled.   
  return tIPstatusInfo.Operating_Mode;
}
//--------------------------------------------------------------
/*B.4.3.sd_mmc_sdio_get_id :Reads value stored in stored in 
   IPreg0 and 1.

  sd_mmc_sdio_get_id(id_reg_info *ptidinfo)
  *Called from : FSD
  *Parameters :
        ptidinfo:(o/p):Pointer to id_reg_info.
        {
          ptidinfo->dwUserID: IP ID0 @0x068
          ptidinfo->dwVersionID: IP ID1 @0x06C.
         }

  *Return value :
   0: for SUCCESS
  -EINVAL:for pointer to id_reg_info is NULL.

  *Flow:
        -Read User ID and store it in ptidinfo->dwUserID.
        -Read Version ID and store it in ptidinfo->dwVersionID.

  **Use Read_Write_IPreg() to read this. 
*/
int sd_mmc_sdio_get_id(id_reg_info *ptidinfo)
{
  //Check pointer is not NULL.
  if(ptidinfo==NULL)
    return -EINVAL;

  //Read user ID and version ID.
  Read_Write_IPreg(USRID_off,&(ptidinfo->dwUserID),READFLG);
  Read_Write_IPreg(VERID_off,&(ptidinfo->dwVerID),READFLG);
    
  return SUCCESS;
}
//==============================================================
/*B.5.Read card info.
      Read_RCA 
      sd_mmc_sdio_get_card_type
      sd_mmc_sdio_get_currstate 
      sd_mmc_get_card_size
      Get_Card_Atributes 
*/
//--------------------------------------------------------------
/*B.5.1.Read_RCA : of given card.Stored locally in Host 
   driver.

   int sd_mmc_read_rca(int ncardno)

  *Called from : FSD
  *Parameters :
         ncardno:(i/p); card number whose RCA to be read.
    
  *Return value :
   RCA of card.Total port numbers.: for SUCCESS
   Err_Invalid_CardNo
   Err_Card_Not_Connected

  *Flow:-check if card number is valid.Else return error.
        -If card is connected,return Card_info->Card_RCA
         else return error.
*/
int sd_mmc_read_rca(int ncardno)
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //If card is connected,return Card_info->Card_RCA
  //else return error.
  if(Card_Info[ncardno].Card_Connected==TRUE)
    return  Card_Info[ncardno].Card_RCA;
  else
    return Err_Card_Not_Connected;
}

int sd_mmc_read_state(int ncardno,int *card_connect,int *card_type)
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //If card is connected,return Card_info->Card_RCA
  //else return error.
  *card_connect = Card_Info[ncardno].Card_Connected;
  *card_type = (int)Card_Info[ncardno].Card_type;
  return 0;
}
EXPORT_SYMBOL(sd_mmc_read_state);

//--------------------------------------------------------------
/*B.5.2.sd_mmc_sdio_get_card_type : For given port number.

  int sd_mmc_sdio_get_card_type(int ncardno) 

  *Called from : FSD
  *Parameters :
         ncardno :(i/p); card number whose type is to be 
         returned.
    
  *Return value :
   Type of card is returned: for SUCCESS.0=MMC, 1=SD, 2=SDIO
   Err_Card_Not_Connected: 
   Err_Invalid_CardNo


  *Flow:
        -check if card number is less than total cards 
         connected.Else return error.
        -If card is connected,return Card_info->Card_Type
         else return error.
*/
int sd_mmc_sdio_get_card_type(int ncardno) 
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;
    
  //If card is connected,return Card_info->Card_Type
  //else return error.
  if(Card_Info[ncardno].Card_Connected==TRUE)
    return  Card_Info[ncardno].Card_type;
  else
    return Err_Card_Not_Connected;
}

//--------------------------------------------------------------
/*B.5.3.Read_Current_State_of_card :Stored locally in 
  Host driver.Using this function application polls 
  for the command path and data path status.If it founds
  command path free,then it can send next command.
  If it founds data path free,then next command can be data
  transfer command or else it should be non data transfer 
  command.

  int sd_mmc_sdio_get_currstate (int ncardno) 

  *Called from : FSD
  *Parameters :
        -ncardno :(i/p); card number whose current state is
         to be returned.
     
         
  *Return value :
   Current state of card is returned.: for SUCCESS.
                                      bit0= 1 busy.  free/busy,
                                      bit1= 1 in process. command 
                                      bit2= 1 in process.data transfer
   Err_Card_Not_Connected: 
   Err_Invalid_CardNo

  *Flow:-check if card number is valid.
        -If card is connected,return 
         Card_info->Current_card_status  else return error.

         bit0= 1 busy.  free/busy,
         bit1= 1 in process. command 
         bit2= 1 in process.data transfer
*/
int sd_mmc_sdio_get_currstate (int ncardno) 
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //If card is connected,return Card_info->Current_card_status
  //else return error.
  if(Card_Info[ncardno].Card_Connected==TRUE)
    return  Card_Info[ncardno].Card_status_info.Current_card_status;
  else
    return Err_Card_Not_Connected;
}
//--------------------------------------------------------------
/*B.5.4.Read_card_size:Size stored in card register.   LEVEL2
  This is stored in cardInfo structure.After enumeration,
  CSD of card is read and size is calculated and stored.

  unsigned long long sd_mmc_get_card_size(int ncardno)

  *Called from : FSD
  *Parameters :
        -ncardno :(i/p); card number whose size is
         to be returned.
    
  *Return value :
   Size of card.: for SUCCESS.
   Err_Card_Not_Connected: 
   Err_Invalid_CardNo

  *Flow:-check if card number is less than total cards 
        -If card is connected,return Card_info->Card_size
         else return error.
*/
unsigned long long sd_mmc_get_card_size(int ncardno)
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //If card is connected,return Card_info->Card_size
  //else return error.
  if(Card_Info[ncardno].Card_Connected==TRUE)
    return  Card_Info[ncardno].Card_size;
  else
    return Err_Card_Not_Connected;
}
//--------------------------------------------------------------
/*B.5.5.Read_Card_Attributes :Reads card attributes as :
  locked/unlocked , write_protected/not.

  int Get_Card_Attributes (int ncardno)
  *Called from : FSD
  *Parameters :
        -ncardno = (i/p); card number whose attributes are
         to be returned.
    
  *Return value :
   card attributes.: for SUCCESS.
       0 = Neither locked nor protected.
       1 = Locked card but not write protected.
       2 = write protected card but not Locked.
       3 = Card write protected and locked.
   Err_Card_Not_Connected: 
   Err_Invalid_CardNo

  *Flow:-check if card number is less than total cards 
        -Check card attributes from card_info.
        -Return code for card attributes.
         0 = Neither locked nor protected.
         1 = Locked card but not write protected.
         2 = write protected card but not Locked.
         3 = Card write protected and locked.
*/
int Get_Card_Attributes (int ncardno)
{
  //Check if card number is valid.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //If card is connected,return Card_info->Card_size
  //else return error.
  if(Card_Info[ncardno].Card_Connected==TRUE)
    return  Card_Info[ncardno].Card_Attrib_Info.Card_Attributes;
  else
    return Err_Card_Not_Connected;
}

//==============================================================
/*B.6.Read response.
      Get_Response
      Get_Err_Response
*/
//==============================================================
/*B.7.SDIO functions.
      sdio_rdwr_direct
      sdio_rdwr_extended
      sdio_enable_disable_function
      sdio_enable_disable_irq
      sdio_read_wait
      sdio_change_blksize
      Get_Data_fromTuple
      sdio_enable_masterpwrcontrol
      sdio_change_pwrmode_forfunction
*/
//--------------------------------------------------------------
/*B.7.1.Read_write_to_function : ( SDIO only ).         LEVEL2
  IO read/write using CMD52.
  This is non data command as data for the single register
  is passed in command argument/response.So,no data transfer
  specific initialisation of IP.

  This is single register read/write command.

int sdio_rdwr_direct(int ncardno, int nfunctionno,
                   int nregaddr,
                   unsigned char *pbdata,
                   unsigned char *pbrespbuff,
                   int fwrite,
                   int fraw,
                   int  nflags)

  *Called from : FSD

  *Parameters:
         ncardno    :(i/p):Card to whom resume command is to be 
         send
         nfunctionno:(i/p):Function number to be resumed.
         nregaddr   :(i/p):Register address.
         pbdata     :(i/p-o/p):Pointer to unsigned char data.Pointer to 
                           storage of data to be written or read.
         pbrespbuff :(o/p):Response buffer.
         fwrite     :(i/p):Read or write data unsigned char.
                           0=Read,1=Write.
         fraw       :(i/p):Read data after write.
                           0=No Read, 1=Read after Write. 
         nflags     :(i/p):Flags for user settable options.
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                      Send cmd23 first.
         Handle_Data_Errors (bit4)=1;Handle data errors.
         Send_Abort (bit5)=1;
         Read_Write   (bit7)=1; Write operation. 0=Read.   

  *Return value : 
   0: for SUCCESS
   Err_CMD_Failed

   *Flow:
   Sanity check:
         -Check if given card is SDIO.
         -Check if data buffer is not NULL.

   Form command://Send CMD52 (IO_RW_Direct command).

         //command argument for CMD52. 
         ptCmdParams->dwcmdargReg = R/Wflag||FunctionNo||
                      RAW flag||Register Address||WriteData  ;

   Send command:Use form_send_cmd() as this is non data cmd.

         -If Read After Write OR Read operation,copy data 
          received in response,in data buffer.

   Validate_Response:
         -Use Validate_Response().Return if any error.

         -Return.
*/
int sdio_rdwr_direct(int ncardno, int nfunctionno,
                     int nregaddr,
                     unsigned char *pbdata,
                     unsigned char *pbrespbuff,
                     int fwrite,
                     int fraw,
                     int nflags)
     
{ 
  unsigned long dwRetVal;
  SDIO_RW_DIRECT_INFO tRWdirectInfo={0x00};

  //printk("sdio_rdwr_direct:entry\n");        

  /*Sanity check:*/
  //Validate card number,card type(SDIO),function number(1-7)
  //Parameter 2 is only used for validation.
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
      SDIO_Command) ) != SUCCESS)
    return dwRetVal;

  //For IO command,no need to check the current state of card.

  /*Form command:Send CMD52 (IO_RW_Direct command).*/
  //command argument for CMD52. 
  tRWdirectInfo.Bits.WrData = *pbdata ;
  tRWdirectInfo.Bits.RegAddress = nregaddr ;
  tRWdirectInfo.Bits.RAW_Flag = fraw  ;
  tRWdirectInfo.Bits.FunctionNo = nfunctionno ;
  tRWdirectInfo.Bits.RW_Flag = fwrite  ;
 dwRetVal=*pbdata;
 if(nregaddr==0x110)
     Card_Info[0].SDIO_Fn_Blksize[1]=dwRetVal;
 if(nregaddr==0x111)
     Card_Info[0].SDIO_Fn_Blksize[1] |= dwRetVal<<8;
    
  //Non data command.So,use form_send_cmd
  if(form_send_cmd(ncardno,CMD52,tRWdirectInfo.dwReg,pbrespbuff,
     nflags) != SUCCESS)  
    return Err_CMD_Failed; 

  //If RAW set or read operation,store read data in data buffer.
  if(fraw || !fwrite)
    *pbdata = *pbrespbuff;//pbrespbuff holds bits8-39 of resp.
    
  //Validate_Response.
  //If index mismatched,IP will raise interrupts.
  //check response flags for error.
  dwRetVal=Validate_Response(CMD52,pbrespbuff,ncardno,
                             DONOT_COMPARE_STATE);
  return dwRetVal;     
}
//--------------------------------------------------------------  
/*B.7.2.Read_write_to_function : ( SDIO only ).         LEVEL2
  IO read/write to multiple registers using CMD53.
  This is a data transfer command.So,requires data related
  initialisation of IP.
  ** User should read EXx flag and check that required function 
  is not suspended.Otherwise,wait till that function gets ready 
  for next data transfer.

  sdio_rdwr_extended(int ncardno, int nfunctionno,int nregaddr,
                   unsigned long dwbytecount, 
                   unsigned char  *pbdatabuff,
                      unsigned char  *pbcmdrespbuff,
                   unsigned char  *pberrrespbuff,
                   int   ncmdoptions
                   int   nnoofretries,
                   int   nflags)

  *Called from : FSD

  *Parameters:
         ncardno       :(i/p):Card to whom resume command is to be send.
         nfunctionno   :(i/p):Function number to resume.
         nregaddr      :(i/p):Register address.
         dwbytecount   :(i/p):Number of bytes to transfer. 
         pbdatabuff    :(i/p-o/p):Data buffer.
         pdwCmdRespBuff:(o/p):Command ressponse buffer.
         pdwErrRespBuff:(o/p):Error response buffer.
         ncmdoptions   :(i/p):SDIO extended command options.
           bit0= OpCode   0=MultiByte rd/wr to fixed address.
                          1=MultiByte rd/wr to incrementing addr
           bit1= BlkMode  0=Rd/Wr operation on unsigned char basis.
                          1=Rd/Wr operation on Block basis. 

         callback      : Callback pointer.
         nnoofretries  :(i/p):Number of retries in case of error.
         nflags:(i/p)  :Option for errors to be responded & other 
                        conditions..
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                  transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                    Send cmd23 first.
         Handle_Data_Errors (bit4)=1; Handle data errors.
         Send_Abort (bit5)=1;
         buffer_mode  (bit6) ; =1 =>kernel mode buffer
                               =0 =>user mode buffer 

         Read_Write   (bit7)=1; Write operation. 0=Read.   


**Currently last two options are not used.provided for future.

  *Return value : 
    0: for SUCCESS
    Err_MultiBlk_Not_Allowed
    Err_Not_BlkAlign
    Err_CMD_Failed

   *Flow:
   Sanity check:
         -Check if given card is SDIO.
         -Check if data buffer is not NULL.

         -If(fBlkMode)
            Check if card supports multi block operation.
            (SMB) bit in CCCR register.

   Allocate_Resp_Buffer:
         -Allocate response buffer.(ptRespBuff)

   Command_Info:
         -Create COMMAND_INFO instance.(*ptCmdParams)

   Form command://Send CMD53 (IO_RW_Direct_Extended command).

         //Configure IP for data transfer command.

        -//Set Block size and unsigned char count.
         ptCmdParams->dwblocksizereg = dwBlkSize;
         ptCmdParams->dwByteCountReg = dwbytecount;

        -//If block mode,calculate block count.
         If(fBlkMode)
          {
            ByteCount = ByteCount/BlkSize;
            If (Remainder)
               return Error;//User buffer will be for ByteCount
                            //only.So,return BlockAlignment Err.
          }

        -//command argument for CMD53. 
         ptCmdParams->dwcmdargReg = R/Wflag||FunctionNo||
                                    fBlkMode||fOpCode||
                                    Register Address||ByteCount;
         
         //Form cmd reg.
         call Form_SDIO_CmdReg(ncardno,fwrite,CMD53,
                               fWaitForPrvData);

        -Response buffer.
         ptCmdParams->ptResponse_info = ptRespBuff;

   Send command:
        -Command is formed,now add it in queue for execution.
         Call Add_command_in_queue(ptCmdParams,ptAppInfo)

        -Sleep (&ptAppInfo->Wqn)
        //Command will be processed when status poller calls
        //the switch to load next command.Sleep till command
        //is accepted.

   Read response.:
        //In response,written register is read again.

   Validate_Response:
        -If error,set error code.

        -Return.

   **On read data done or FIFO threshold,copy data from FIFO to 
     data buffer.
   **For write,on FIFO threshold,copy data from data buffer to 
     FIFO.

** For SDIO card,store blocksize for each function.ie each SDIO
card will hold data specific to each function.
*/
int sdio_rdwr_extended(int ncardno, int nfunctionno,int nregaddr,
                       unsigned long dwbytecount, 
                       unsigned char  *pbdatabuff,
                       unsigned char  *pbcmdrespbuff,
                       unsigned char  *pberrrespbuff,
                       int   ncmdoptions,
                       void  (*callback)(unsigned long dwerrdata),
                       int   nnoofretries,
                       int   nflags)
{
  unsigned long dwRetVal,dwByteCnt;
  SDIO_RW_EXTENDED_INFO tRWextendInfo;
  COMMAND_INFO tCmdInfo = {0x00};
  unsigned char bDummyResponse=0;
  /*Sanity check:*/
  //Validate card number,card type(SDIO),function number(1-7)
  //Parameter 2 is only used for validation.
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
      SDIO_Command) ) != SUCCESS)
    return dwRetVal;

  //This function should be called for the function that is 
  //not suspended.

  //Get user supplied unsigned char count.
  dwByteCnt = dwbytecount;

  //If block mode,calculate block count.
  if(dwByteCnt==0)  return -EINVAL;

  if(ncmdoptions & 0x02)//bit 1
  { //Check if card supports multi block operation.
    //SMB : bit1 at CCCR:Address 0x08.
    if(!(Card_Info[ncardno].CCCR.Card_Capability & 0x02))        
      return Err_MultiBlk_Not_Allowed;

    if(Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno]==0)
      return -EINVAL;

    dwByteCnt = 
       dwbytecount/Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno];

    if(dwbytecount % 
       Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno])
      return Err_Not_BlkAlign;//User buffer will be for ByteCount
                    //only.So,return BlockAlignment Err.
  }
  /* Form command:Send CMD53 (IO_RW_Direct_Extended command)*/
  //command argument for CMD53.
  tRWextendInfo.Bits.Byte_Count = dwByteCnt ;
  tRWextendInfo.Bits.RegAddress = nregaddr ;
  tRWextendInfo.Bits.Op_Code = ncmdoptions & 0x01;//SDIO_EXT_OPCODE;//bit0
  tRWextendInfo.Bits.Blk_Mode = (ncmdoptions & 0x02)>>1;//SDIO_EXT_BLKMODE;//1
  tRWextendInfo.Bits.FunctionNo = nfunctionno ;
  tRWextendInfo.Bits.RW_Flag = (nflags & 0x080)>>7;//SDIO_EXT_WRITE;//bit7

  tCmdInfo.dwCmdArg = tRWextendInfo.dwReg;

  //Set Block size and unsigned char count.
  //**Whenever user changes block size,reflect same in 
  //**Card_Info.
  if(ncmdoptions & 0x02)//Block mode.
     tCmdInfo.dwBlkSizeReg = 
       Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno];
  else //unsigned char mode.
     tCmdInfo.dwBlkSizeReg = dwbytecount ;
    
  tCmdInfo.dwByteCntReg = dwbytecount ;

  //Form cmd reg.
  Form_SDIO_CmdReg(&tCmdInfo,ncardno,CMD53,nflags);

  /*Send command:*/
  dwRetVal= Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                             pberrrespbuff,pbdatabuff,
                             NULL,//No callback pointer
                             nflags,nnoofretries,
                             TRUE,
                             tRWextendInfo.Bits.RW_Flag, //Data,write/read.
                             ((nflags & 0x40) >>6) );// 0= user address space buffer.

  HI_TRACE(2,"Send_cmd_to_host return = %lu\n ",dwRetVal);       
  
  if(dwRetVal != SUCCESS)
    return dwRetVal; 

  //Validate response.Response status is same as CMD52.
  if((dwRetVal=Validate_Response(CMD53,pbcmdrespbuff,ncardno,
      DONOT_COMPARE_STATE))!=SUCCESS) {
      HI_TRACE(2,"Validate_Response return = %lu\n ",dwRetVal);       
      return dwRetVal;
      }
 HI_TRACE(2,"Validate_Response return = %lu\n ",dwRetVal);    
  //Check if error occured during command execution.
 if((dwRetVal=Send_SDIO_DummyRead(ncardno, &bDummyResponse))
     !=SUCCESS) return dwRetVal;
  HI_TRACE(2,"Send_SDIO_DummyRead return = %lu\n ",dwRetVal);   
  //Check error bit 3 only.
  if(bDummyResponse & 0x8) return Err_SDIO_Resp_UKNOWN;
  HI_TRACE(2,"Send_cmd_to_host : cmd precess sucess!\n ");   
  return SUCCESS;        
}
//--------------------------------------------------------------
/*B.7.3.Enable_Disable_Function: SDIO only.
  //Enables or disables the given function number in given SDIO
  //card.

  sdio_enable_disable_function(int ncardno,int nfunctionno,
                              int fenable)
  *Called From:FSD
  *Parameters:
         ncardno:(i/p):Card number.
         nfunctionno:(i/p):Function number.
         fenable:(i/p): 1=Enable  0=Disable.

  *Return value : 
   0: for SUCCESS
   Err_CMD_Failed

  *Flow:
        -Check if card number is valid.
        -Check if given card is SDIO.
        -//Set/reset corresponding IOExbit in CCCR register.
         //Write 1 to bit,at regAddress=0x02.
         Call sdio_rdwr_direct(ncardno,0,0x02,
                               WriteData,TRUE,TRUE).
        -Validate response.
        -Return.
*/
unsigned long sdio_enable_disable_function(int ncardno,int nfunctionno,
                              int fenable)
{
  unsigned long dwRetVal;
  unsigned char  bWriteData,bMask;
  unsigned char  pbrespbuff[4];//R5 response is one unsigned long.
  
  
  /*Sanity check:*/
  //Validate card number,card type(SDIO),function number(1-7)
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
      SDIO_Command) ) != SUCCESS)
    return dwRetVal;
  
  //Set/reset corresponding IOExbit in CCCR register.
  //Write 1 to bit,at regAddress=0x02.
  //Read IO_Enable data ie CCCR::Address2
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,0x02,&bWriteData,
      pbrespbuff,FALSE,FALSE,0x02)) != SUCCESS)
    return dwRetVal;
  
  //To enable,set bit and to disable, clear it.
  bMask=(0x1 << nfunctionno);
  if(fenable)
    bWriteData |= bMask;
  else
    bWriteData &= (~bMask);
  
  //Save data in Card_Info structure.
  Card_Info[ncardno].CCCR.IO_Enable = bWriteData;//3rd Byte0.
  
  return sdio_rdwr_direct(ncardno,0,0x02,&bWriteData,pbrespbuff,
                          TRUE,TRUE,0x02);
}
//--------------------------------------------------------------
/*B.7.4.Enable_Disable_IRQ: SDIO only.
   Enables or disables the given function IRQ, in given SDIO
   card.

   sdio_enable_disable_irq(int ncardno,int nfunctionno,
                          int fenable)
  *Called From:FSD
  *Parameters:
         ncardno:(i/p):Card number.
         nfunctionno:(i/p):Function number.
         fenable:(i/p): 1=Enable  0=Disable.

  *Return value : 
   0: for SUCCESS
   Err_CMD_Failed

  *Flow:
        -Check if card number is valid.
        -Check if given card is SDIO.
        -Set/reset corresponding IENx bit in CCCR register.
         //Write 1 to bit,at regAddress=0x04.
         Call sdio_rdwr_direct(ncardno,0,0x04,
                               WriteData,TRUE,TRUE).
        -Validate response.
        -Return.
*/
unsigned long sdio_enable_disable_irq(int ncardno,int nfunctionno,
                          int fenable)
{
  unsigned long dwRetVal;
  unsigned char  bWriteData,bMask;
  unsigned char  pbrespbuff[4];//R5 response is one unsigned long.
  
  /*Sanity check:*/
  //Validate card number,card type(SDIO),function number(1-7)
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
      SDIO_Command) ) != SUCCESS)
    return dwRetVal;
  
  //Set/reset corresponding IENx in CCCR register.
  //Write 1 to bit,at regAddress=0x04.
  //bWriteData= Card_Info[ncardno].CCCR.Int_Enable;//5th Byte0.
  
  //Read Int_Enable data ie CCCR::Address4
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,0x04,&bWriteData,
      pbrespbuff,FALSE,FALSE,0x02)) != SUCCESS)
    return dwRetVal;
  
  //To enable,set bit and to disable, clear it.
  bMask=(0x1 << nfunctionno);
  if(fenable)
    bWriteData |= bMask;
  else
    bWriteData &= (~bMask);
  
  //Save data in Card_Info structure.
  Card_Info[ncardno].CCCR.Int_Enable = bWriteData;// 3rd Byte0.
  
  return sdio_rdwr_direct(ncardno,0,0x04,&bWriteData,pbrespbuff,
                          TRUE,TRUE,0x02);
}
//--------------------------------------------------------------
/*B.7.5.sdio_read_wait: SDIO only.
  Asserts or clears sdio_read_wait state.

  int sdio_read_wait(int ncardno,int fAssert)

  *Called From:FSD
  *Parameters:
         ncardno:(i/p):Card number.
         fAssert:(i/p): 1=Insert sdio_read_wait. 0=Deassert.

  *Return value : 
   0: for SUCCESS
   Err_Invalid_CardNo: 
   Err_CMD_Not_Supported
   Err_Card_Not_Connected

  *Flow:
        -Check if card number is valid and card is connected 
        -Check if sdio_read_wait feature is supported by card.
        -Set/clear the sdio_read_wait bit in control register.
        -Return.
*/

int sdio_read_wait(int ncardno,int fassert)
{
  unsigned long dwRetVal,dwBitno=0x06;

  //check if card is connected.
  if((dwRetVal=Validate_Command(ncardno,00,00,SDIO_Command)) 
      != SUCCESS)  return dwRetVal;

  //Check if card supports read wait.CCCR::SRW bit1
  if(Card_Info[ncardno].CCCR.Card_Capability & 0x02)
  {
    //Ser/clear sdio_read_wait bit in control register.
    if(fassert)
      set_ip_parameters(SET_CONTROLREG_BIT,&dwBitno);
    else
      set_ip_parameters(CLEAR_CONTROLREG_BIT,&dwBitno);
  }
  else
    return Err_CMD_Not_Supported;

  return SUCCESS;
}
//--------------------------------------------------------------
/*unsigned long  sdio_change_blksize (int ncardno, int nfunctionno, 
                              unsigned short wioblocksize)
  *Description : Changes the io block size for the given function
   of SDIO card.Scan through the Tuple list to findout the maximum
   size.If size to be changed is more than this, returns error.
 
  *Parameters :
   ncardno     :(i/p):Card number
   nfunctionno :(i/p):Function number.
   wioblocksize:(i/p):IO block size to be changed.
  
  *Return Value : 
   0 for Success, 
   Error if command failed.
 
  *Flow : 
      - Validate card number and function number.
      - if(wioblocksize ==0 || wioblocksize ==Current block size)
         return Error.
      - Read maximum I/O block size.Use Get_Data_fromTuple(..).
        (** Remember Max.size is in little endian format).
        if(wioblocksize > Maximum block size)
         return Error.
      - For changing block size, write to block size register for 
        respective function,in function 0. Call sdio_rdwr_extended().
      - If sdio_rdwr_extended returns error, return it.
      - Store changed block size in card_info structure for given 
        card number.
      - Return SUCCESS.
 */
unsigned long  sdio_change_blksize (int ncardno, int nfunctionno, 
                              unsigned short wioblocksize)
{
  R5_RESPONSE  tR5={0};
  unsigned short wMaxBlockSize=0,wRequdBlkSize=0;
  unsigned long dwRetVal;
  unsigned int nFieldOffset=0;

  //Validate card number and function number.
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
     SDIO_Command) ) != SUCCESS)
    return dwRetVal;

  //Check if invalid block size or same as currenlt set block size.s
  if(wioblocksize ==0 || 
     (wioblocksize==Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno]))
    return Err_Invalid_Arg;

  //Read maximum I/O block size.Use Get_Data_fromTuple(..).
  //** Remember Max.size is in little endian format.
  nFieldOffset = (nfunctionno==0) ? 0x03 : 0x0E;
  dwRetVal = Get_Data_fromTuple (ncardno,nfunctionno,
                                 TPL_CODE_CISTPL_FUNCE,//0x22
                                 nFieldOffset,         //blksize.offset
                                 2,                    //nBytes,  
                                 (unsigned char*)&wMaxBlockSize,//Buffer 
                                 2,                    //nBuffsize , 
                                 TRUE );               //BuffMode=kernel.

  if(dwRetVal!=SUCCESS)  return dwRetVal;

  //Check if block size to set is within limits of maximum.
  if(wioblocksize > wMaxBlockSize)
    return Err_SDIOblksize_Exceeds;

  //Change blocksize by setting blocksize register in function0.
  wRequdBlkSize = wioblocksize;

  dwRetVal= sdio_rdwr_extended(ncardno,00,((nfunctionno<<8)|0x10),0x02,
                               (unsigned char*)&wRequdBlkSize, (unsigned char*)(&tR5.dwReg),
                               00,//No err response buffer.
                               0x01,//incre,addr.(bit0), Cmdoptions.
                               NULL,//No callback.
                               00,0xd0);//unsigned char mode,no wait for prev data,write.

  if(dwRetVal!=SUCCESS)  return dwRetVal;

  //Store changed block size in card_info structure.
  Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno] = wioblocksize;        

  return SUCCESS;
}

/* unsigned long Send_SDIO_DummyRead(uint ncardno, unsigned char *StatusData)
 
  *Description : This is an internal function used to read the status
   of the I/O card,received in response. It is used to check if any 
   error occured during command execution or to check the current 
   state of the card. Sedns the CMD52 read command and returns the 
   response in StatusData.
 
  *Parameters :
   ncardno   :(i/p):Card number
   StatusData:(o/p):unsigned char pointer to return sttaus data.
 
  *Return Value : 
   0 for Success, 
   Error if command failed.
 
  *Flow : 
      - Validate ncardno & StatusData .
      - Read CCCR register @ 0x00. Use sdio_rdwr_direct(..).
      - If function returned error, send it back to caller function.
      - Copy  status unsigned char data from response(R5), into * StatusData.
      - Return SUCCESS..
*/
unsigned long Send_SDIO_DummyRead(int ncardno, unsigned char *StatusData)
{
  unsigned char  RespBuffer[4]={0};
  SDIO_RW_DIRECT_INFO tRWdirectInfo={0x00};

  //This is internal function.So no need to validate the parameters.
  //Form command:Send CMD52 (IO_RW_Direct command).
  tRWdirectInfo.Bits.WrData = 0 ;
  tRWdirectInfo.Bits.RegAddress = 0 ;
  tRWdirectInfo.Bits.RAW_Flag = 0  ;
  tRWdirectInfo.Bits.FunctionNo = 0 ;
  tRWdirectInfo.Bits.RW_Flag = 0  ;
    
  //Non data command.So,use form_send_cmd
  if(form_send_cmd(ncardno,CMD52,tRWdirectInfo.dwReg,RespBuffer,
     0) != SUCCESS) return Err_CMD_Failed; 

  //Store response data in status buffer.
  *StatusData= RespBuffer[1];

  return SUCCESS;     
}
//-------------------------------------------------------------- 
/* unsigned long Get_Data_fromTuple (uint ncardno, int nfunctionno,
                          unsigned int nTupleCode, 
                          unsigned int nFieldOffset,
                          unsigned int nBytes,  
                          unsigned char* pBuffer, 
                          unsigned int nBuffsize , 
                          int fBuffMode)

  *Description :  Searches the tuple with given code, in CIS area. 
   Pointer to start of the CIS area is stored in 0xn09 to 0xn0B 
   address in CCCR. After finding it, reads the specified number of
   bytes from given offset and copies them in pBuffer.Implementation
   of CIS is mandatory.
            
  *Called from: Application or internal functions.

  *Parameters:
   ncardno     :(i/p):Card number.
   nfunctionno :(i/p):Function number whose tuple is to be searched.
   nTupleCode  :(i/p):Tuple code to be searched.
   nFieldOffset:(i/p):unsigned char offset from which to read the data from 
                      Tuple.
   nBytes      :(i/p):Number of bytes to read.
   pBuffer     :(o/p):Buffer to hold the data.
   nBuffsize   :(i/p):Buffer size.Used only to validate the buffer.
   fBuffMode   :(i/p):Buffer mode . 1 = Kernel mode buffer. 0= user 
                      mode buffer.

  *Return Value: 
   0 for SUCCESS.

  *Flow:
      - Validate the card number.
      - if(nBuffsize < nBytes ) return Error.
      - Read CIS pointer for given function number. 
      - Note : This pointer is in little endian mode.
      - if(CIS pointer < 0x1000 || CIS pointer > 0x17FFF)
          return Err_Invalid_CISPtr.
        //CIS area is from 0x1000 to 0x17FFF in Function 0).
      - Read tuple code of the first tuple starting at CIS pointer 
        address.
      - //Parse Tuple chain till chain ends or given tuple code is 
        found.
        while( Tuplecode != Requd_Code || Tuplecode != 0xFF)
        {
         - if( Tuple_code != NULL_Tuple)
           // *(Tuple_start+1) + 2 ie tuple body +2.
           read unsigned char offset for next tuple. 

         - Read next tuple.
        }
      - if(Tuplecode != Requd_Code) return Err_Tuple_Not_Found;

      - // Check if bytes to read go beyond Tuple body.
        if( (Tuple_start + nFieldOffset  + nBytes) > 
           (Tuple_start+Tuple body +2) )
          return Err_SDIOTuple_AddrExceeds.
      - Copy specified number of bytes into pBuffer.
      - Return SUCCESS.

*/
unsigned long Get_Data_fromTuple (int ncardno, int nfunctionno,
                          unsigned int nTupleCode, 
                          unsigned int nFieldOffset,
                          unsigned int nBytes,  
                          unsigned char* pBuffer, 
                          unsigned int nBuffsize , 
                          int fBuffMode)
{
  R5_RESPONSE  tR5;
  unsigned long        dwCISptr=0;
  unsigned char         bCurrTuple=0;
  unsigned int nNxtTpleOffset=0,nflags=0x10;
  unsigned long dwRetVal;

  //Validate the card number.
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
     SDIO_Command) ) != SUCCESS)
    return dwRetVal;

  if(nBuffsize < nBytes ) return Err_Invalid_Arg;

  //Read CIS pointer for given function number.(0xn09-0xn0b). 
  //Note : This pointer is in little endian mode.
  dwRetVal= sdio_rdwr_extended(ncardno, 00,((nfunctionno<<8)|0x9),0x3,
                               (unsigned char*)(&dwCISptr), 
                               (unsigned char*)(&tR5.dwReg),
                               00,  //No err response buffer
                               0x01,//incre,addr.(bit0), Cmdoptions
                               NULL,//No callback.
                               00,0x50);//unsigned char mode,no wait for prev data,read.

  if(dwRetVal!=SUCCESS)  return dwRetVal;

  if((dwCISptr < 0x1000) || (dwCISptr > 0x17FFF))
    return Err_Invalid_CISPtr;

  //Read tuple code of the first tuple starting at CIS pointer address.
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,dwCISptr, &bCurrTuple,
      (unsigned char*)(&tR5.dwReg),FALSE,FALSE,0x02)) != SUCCESS)
    return dwRetVal;

  //Parse Tuple chain till chain ends or given tuple code is found.  
  while( (bCurrTuple!=nTupleCode) && (bCurrTuple!=0xFF))
  {
    //Read unsigned char offset for next tuple.  
    if(bCurrTuple != NULL_TUPLE)
    {
      //Read offset to next tuple.
      nNxtTpleOffset=0;

      if((dwRetVal=sdio_rdwr_direct(ncardno,0,(dwCISptr+1),
         (unsigned char*)&nNxtTpleOffset,(unsigned char*)(&tR5.dwReg),FALSE,FALSE,0x02)) 
         != SUCCESS)   return dwRetVal;

      //Next tuple starts at (tuple body +2).
      dwCISptr = dwCISptr + nNxtTpleOffset + 2;
    }
    else
      dwCISptr +=1; 
 
    //Clear the tuple unsigned char.
    bCurrTuple=0;

    //Read next tuple.
    if((dwRetVal=sdio_rdwr_direct(ncardno,0,dwCISptr, &bCurrTuple,
       (unsigned char*)(&tR5.dwReg),FALSE,FALSE,0x02)) != SUCCESS)
      return dwRetVal;
  }//end of while.

  //Tuple parsing finished.Check if tuple found or not.
  if(bCurrTuple != nTupleCode) return Err_SDIOTuple_NotFound;

  //Get tuple body size.
  nNxtTpleOffset=0;
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,(dwCISptr+1),(unsigned char*)&nNxtTpleOffset,
      (unsigned char*)(&tR5.dwReg),FALSE,FALSE,0x02)) != SUCCESS)
    return dwRetVal;

  //Check if bytes to read go beyond Tuple body.
  if((nFieldOffset+nBytes) > (nNxtTpleOffset+2))
    return Err_SDIOTuple_AddrExceeds;

  //Copy specified number of bytes into pBuffer.
  dwRetVal= sdio_rdwr_extended(ncardno, 00,(dwCISptr+nFieldOffset),
                               nBytes, pBuffer,(unsigned char*)(&tR5.dwReg),
                               00,  //No err response buffer
                               0x01,//incre,addr.(bit0), Cmdoptions
                               NULL,//No callback.
                               00,  //unsigned char mode
                               nflags|(fBuffMode<<6));//Buff.mode.

  return dwRetVal;
}
//--------------------------------------------------------------
/* unsigned long sdio_enable_masterpwrcontrol (uint ncardno, int fenable):   

  *Description : Sets the EMPC bit (Function0,Address 0x12, bit1) to
   indicate that host can support high power mode and total card 
   current is allowed to exceed 200mA. If EMPC=0, host indicates that
   card should restrict the total current to be less than 200 mA.
   If SMPC=1,  EMPC is available.
   If EMPC=1,  SPS & EPS are available for each function. 
      
  *Called from: Application.

  *Parameters:
   ncardno:(i/p):Card number.
   fenable:(i/p):= 1  to enable the master power control.Ie EMPC=1.
                 = 0  to disable ie EMPC=0.

  *Return Value: 0 for SUCCESS.

  *Flow:
      - Validate the card number.
      - Read SMPC bit ie CCCR @ 0x12, bit0.
      - if(SMPC==0) return Err_PwrMode_NotSupported.
      - Set EMPC bit by calling sdio_rdwr_direct(..).
      - If function returns error, send it back to caller function.
      - Validate the response.If error occured, return it.
      - Return SUCCESS.
*/
unsigned long sdio_enable_masterpwrcontrol (int ncardno, int fenable)   
{
  unsigned long dwRetVal;
  unsigned char  bWriteData=0;
  unsigned char  pbrespbuff[4]={0};//R5 response is one unsigned long.

  //Validate the card number.
  if((dwRetVal = Validate_Command(ncardno,00,00,
      General_cmd) ) != SUCCESS)  return dwRetVal;

  //Read SMPC bit ie CCCR @ 0x12, bit0.
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,0x12,&bWriteData, 
      pbrespbuff,FALSE,FALSE,0x02))!=SUCCESS) return dwRetVal; 

  if(!(bWriteData & 0x1))
    return Err_SDIOPwrMode_NotSupported;

  //Set/clear EMPC bit as per fenable.Use sdio_rdwr_direct(..).
  if(fenable)
    bWriteData |= 2;
  else
    bWriteData &= 0xFD;

  //Send CMD52 to write at 0x12 address.
  memset(pbrespbuff,0,4);

  dwRetVal=sdio_rdwr_direct(ncardno,0,0x12,&bWriteData, 
                            pbrespbuff,TRUE,FALSE,0x02); 

  return dwRetVal;
}
//--------------------------------------------------------------

/* unsigned long sdio_change_pwrmode_forfunction (uint ncardno, uint nfunctionno, 
                                     int flowpwrmode):   

  *Description : Sets the EMPC bit (Function0,Address 0x12, bit1) to 
   indicate that host can support high power mode and total card 
   current is allowed to exceed 200mA. If EMPC=0, host indicates that
   card should restrict the total current to be less than 200 mA.
   If SMPC=1,  EMPC is available.
   If EMPC=1,  SPS & EPS are available for each function. 
   EPS bit is CCCR @ 0xn02, bit 1.

  *Called from: Application.

  *Parameters:
   ncardno    :(i/p):Card number.
   nfunctionno:(i/p):Function number for which power mode is to 
                     be changed.
   flowpwrmode:(i/p):= 1  to enable low power mode of the card. Ie EPS=1.
                     = 0  to disable ie EPS=0.

  *Return Value: 0 for SUCCESS.

  *Flow:
      - Validate the card number.
      - Read SMPC bit ie CCCR @ 0x12, bit0.
      - if(SMPC==0) return Err_PwrMode_NotSupported.
      - Read SPS bit ie CCCR @ 0xn02, bit0.  N= function number.
      - if(SPS==0) return Err_PwrSelection_NotSupported.
      - Set EPS bit by calling sdio_rdwr_direct(..).
      - If function returns error, send it back to caller function.
      - Validate the response.If error occured, return it.
      - Return SUCCESS.
*/
unsigned long sdio_change_pwrmode_forfunction (int ncardno, int nfunctionno, 
                                  int flowpwrmode)
{
  unsigned long dwRetVal;
  unsigned char  bWriteData=0;
  unsigned char  pbrespbuff[4]={0};//R5 response is one unsigned long.

  //Validate the card number.
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
     SDIO_Command) ) != SUCCESS)
    return dwRetVal;

  if(nfunctionno==0) return Err_Invalid_Arg;

  //Read SMPC bit ie CCCR @ 0x12, bit0.
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,0x12,&bWriteData, 
      pbrespbuff,FALSE,FALSE,0x02))!=SUCCESS) return dwRetVal; 

  if(!(bWriteData & 0x1))
    return Err_SDIOPwrMode_NotSupported;

  //Read SPS bit ie CCCR @ 0xn02, bit0.  N= function number.
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,((nfunctionno<<8)|0x2),
      &bWriteData, pbrespbuff,FALSE,FALSE,0x02)) != SUCCESS)
    return dwRetVal;

  //Check SPS bit.
  if(!(bWriteData & 0x1))
    return Err_SDIOPwrSelect_NotSupported;

  //Set EPS bit.
  if(flowpwrmode)
    bWriteData |= 2;
  else
    bWriteData &= 0xFD;

  //Send CMD52 to write at 0x12 address.
  memset(pbrespbuff,0,4);

  dwRetVal=sdio_rdwr_direct(ncardno,0,((nfunctionno<<8)|0x2),
                            &bWriteData, pbrespbuff,TRUE,FALSE,0x02); 
  
  return dwRetVal;
}
//==============================================================
/*B.8.Data transfer commands.Data buffer supplied by caller.
      read_write_blkdata 
      read_write_streamdata 
      Form_Stream_Command
      Form_Blk_Command      
      Send_AppCMD55_Cmd
      sd_mmc_stop_transfer
      Read_Write_FIFO
*/
//--------------------------------------------------------------
/*B.8.1.read_write_blkdata:Data buffer supplied by caller. 
  Sends either single or multiple block data.   
  1. Read open ended block data.
  2. Read data with predefined block count.

  If Bytesize = BlockSize,then this is Single block transfer
  else multiple block transfer.

  **Since,Application info will be maintained in Interface
  driver,Buffers need to be passed here.
  Flags could be passed using OR condition. eg
  Flags = Wait_PrvData||Chk_RespCrc||Preedefineed_Xfer

  **Now,BSD will be receiving one command at a time.
  It should not store any command data/record with it as
  next command will overwrite that.

  int  read_write_blkdata(int ncardno, unsigned long dwaddr,
                     unsigned long dwdatasize,
                     unsigned char  *pbdatabuff,
                     unsigned char  *pbcmdrespbuff,
                     unsigned char  *pberrrespbuff,
                     void  (*callback)(unsigned long dwerrdata),
                     int nnoofretries,
                     int nflags)
 
  *Called from : FSD
  *Parameters  :
         ncardno:(i/p):Card to be addreessed.
         dwaddr:(i/p):Starting Address
         dwdatasize:(i/p):Total size of data.
         pbdatabuff:(i/p-o/p):Data buffer.
         pbcmdrespbuff:(o/p):Command responsee buffer.
         pberrrespbuff:(o/p):Command responsee buffer.
         callback:(o/p):Callback pointer to be called when data
                        transfer over.
         nnoofretries:(i/p):Number of retries in case of error.

         nflags:(i/p):Flags.
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                      Send cmd23 first.
         Handle_Data_Errors (bit4)=1; Handle data errors.
         Send_Abort (bit5)=1;
         buffer_mode  (bit6) ; =1 =>kernel mode buffer
                               =0 =>user mode buffer 
         Read_Write   (bit7)=1; Write operation. 0=Read.   

  **Command response buffer and error response buffer can be 
  passed as NULL.Then,responses will not be stored.
  **Number of retries are not implemented currently.

  *Return value : 
   0: for SUCCESS
   -EINVAL :Data bufer pointer is NULL.
   Err_Card_Busy
   Err_CMD_Failed


  *Flow:

   Sanity check:
        -Check if card no.is less than total cards connected.
         else return error.
        -Check if pointer to data buffer is not NULL.
         Else return error.
         Pointer to cmd response buffer and Error reponse buffer
         could be NULL.
        -Check if card is connected.Else return error.

   Card_Status_Check:
        -Read card status.If busy code is returned,return error.
         Call Read_card_Status(Card Number)

   Validate_command:(for given card)
         Call validate_command() to check if command is
         allowed for current--
         card type,card status,card parameters.
         Pass card no,start address,unsigned char count for this.

        -To check: If unsigned char count is in integral multiples of 
         block size.If partial block not allowed,return error.
         If (start+count) goes beyond card size.

   Form command:
        -Form command structure.Call
         Form_Data_Command(Card Number, Data size,Data Address,
                          Flags, Pointer to COMMAND_INFO);

         Fills parameters into variables for IP reegisters.

        -Set stream transfer specific command index.
         CMD17/CMD18/CMD24/CMD27.
  
   Select card :
        -Select card.Call Send_CMD7(Card_Number).

   Send_Supportive_Cmd:(if any)
        -Check if predefined transfer.
         if(nflags & Predef_Transfer)
         {
           //Multiple block transfer with Predefined block count.
           //Send CMD23 first and then send CMD18.
           Send_CMD23();
         }

   Send command:
        -Command is formed,now send it for execution.
 
         When command is executed,response will be
         copied is response buffer.

   Check response:
        -Check if response has any error.
         Call Validate_Response(Command,response,card_no)

        -Return.        
*/
//



int  read_write_blkdata(int ncardno, unsigned long dwaddr,
                        unsigned long dwdatasize,
                        unsigned char  *pbdatabuff,
                          unsigned char  *pbcmdrespbuff,
                        unsigned char  *pberrrespbuff,
                        void  (*callback)(unsigned long dwerrdata),
                        int nnoofretries,
                        int nflags)
{
  int fAlignedAddr= TRUE;
  int fMultiBlk=TRUE;
  unsigned long dwBlockSize,dwBytesRemained,dwCurrAddress,dwbytecount;
  int i;
 #ifdef DMA_ENABLE
 unsigned long dwstate;
  int ret;
 #endif
  unsigned long dwRetVal=SUCCESS,dwR1Resp;

  unsigned long ArrByteCount[4]={0,0,0,0};

  //Check if pointer to data buffer is not NULL.
  if(pbdatabuff==NULL)
    return -EINVAL;
// printk("read_write_blkdata   Validate_Command");
  //Check if card no. is valid.
  dwRetVal = Validate_Command(ncardno,dwaddr,dwdatasize,Blk_Transfer);
  if(dwRetVal != SUCCESS)
    return dwRetVal;

  /*Check and send card in transfer mode*/
  //Check if card is transfer state.
//  printk("read_write_blkdata : Check_TranState!\n");
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //Check for partial or blocksize transfer.
  //get blocksize.
  if(nflags & 0x080)//WRITE_DATA
     dwBlockSize = Card_Info[ncardno].Card_Write_BlkSize;
  else
     dwBlockSize = Card_Info[ncardno].Card_Read_BlkSize;
  
// printk("dwBlockSize =%d\n",dwBlockSize);
/* waring!!!can not open below codes 
  if(dwaddr % dwBlockSize)
     fAlignedAddr = FALSE;
  if(dwdatasize % dwBlockSize)
     fMultiBlk = FALSE;
*/
 //printk("dwaddr=%d,fAlignedAddr=%d,dwdatasize=%d,fMultiBlk=%d\n",dwaddr,fAlignedAddr,dwdatasize,fMultiBlk);

  #ifdef DMA_ENABLE
   dwstate = 0x01ffcf;   
   Read_Write_IPreg(INTMSK_off,&dwstate,WRITEFLG);
  //Write ctrl reg.  
   dwstate=ReadReg(CTRL_off);
   dwstate |= 0x20;
   WriteReg(CTRL_off,dwstate);   //enable DMA
  #endif


  //Actual data transfer.
  if(fAlignedAddr && fMultiBlk )
  {
    //single / multiple blk.xfer. : Current routine.
    dwRetVal = Blksize_Data_Transfer(ncardno,dwaddr,
                                     dwdatasize,
                                     pbdatabuff,
                                     pbcmdrespbuff,
                                     pberrrespbuff,
                                     callback,//pass callback.
                                     nnoofretries,
                                     nflags);
  }    
  else
  {
    //** Calculate unsigned char count for first block,last block and intermediate blocks.
    //First block and last block are sent as a partial transfer.
    //In between blocks are sent as multiple block transfer.
    if(fAlignedAddr)
    { 
      //eg case a: 0x532 bytes from 0x1200.Blksize=0x200.
      ArrByteCount[0] = 0;//No first partial block.
    }
    else
    {
      //eg case b : 0x600 bytes from 0x1334.Blksize=0x200.
      //eg nFirstBlkBytes = 200 - (1334 % 200) = 200 - 134 = 0xcc
      ArrByteCount[0] = dwBlockSize - (dwaddr % dwBlockSize);
      if(dwdatasize < ArrByteCount[0])
        ArrByteCount[0] = dwdatasize;
    }

    //eg case a: (532-0)  = 532. 
    //eg case b: (600-cc) = 534.         
    dwBytesRemained = (dwdatasize - ArrByteCount[0]);

    //eg case a: nMultiBlkBytes = 532 - (532 % 200) = 532 - 132 = 400
    //eg case b :nMultiBlkBytes = 534 - (534 % 200) = 400;
    ArrByteCount[1] = dwBytesRemained - (dwBytesRemained % dwBlockSize);   

    //eg case a: nLastBlkBytes  = 532 - 400 = 132.
    //eg case b: nLastBlkBytes  = 534 - 400 = 134.
    ArrByteCount[2]  = dwBytesRemained - ArrByteCount[1];
          
       
    //First block transfer from start address.
    dwCurrAddress = dwaddr;

    for(i=0;i<3;i++)
    {
      dwbytecount = ArrByteCount[i];

      if(dwbytecount) 
      {
        if(dwbytecount % dwBlockSize)
          dwRetVal = Partial_Blk_Xfer(ncardno,dwCurrAddress, 
                                      dwbytecount, 
                                      pbdatabuff,
                                      pbcmdrespbuff,
                                      pberrrespbuff,
                                      nnoofretries,
                                      nflags);
        else 
          dwRetVal = Blksize_Data_Transfer(ncardno,dwCurrAddress,
                                           dwbytecount,
                                           pbdatabuff,
                                           pbcmdrespbuff,
                                           pberrrespbuff,
                                           NULL,//No callback.
                                           nnoofretries,
                                           nflags);
                           
        if(dwRetVal != SUCCESS)
        {
  
            #ifdef DMA_ENABLE
                    Read_Write_IPreg(RINTSTS_off,&dwstate,READFLG);
                 dwstate &= 0x30;
                 Read_Write_IPreg(RINTSTS_off,&dwstate,WRITEFLG);
                 dwstate = 0x01ffff;   
                 Read_Write_IPreg(INTMSK_off,&dwstate,WRITEFLG);  
                 ret=ReadReg(CTRL_off);
                 ret &= ~(0x0020);
                 WriteReg(CTRL_off,ret);   //disable DMA
            #endif      

            return dwRetVal;
        }
        
        dwCurrAddress += dwbytecount;    
        pbdatabuff += dwbytecount;

      }

    }//end of for loop for partial xfer.

    //No use of callback for partial transfer.

  }//end of if partial transfer.

    //del by w53349 20061103
     if(dwRetVal != SUCCESS)  
         {
             printk("\n\n*******dwR1Resp & R1_EXECUTION_ERR_BITS error!\n");
             return dwRetVal;     
         }

  //Check if any error occured during execution of command.
  //Bits 19,20,21,22,23,24,26,27,29,30,31.

  if((dwRetVal = form_send_cmd(ncardno,CMD13,00,(unsigned char*)&dwR1Resp,1))
     ==SUCCESS)
  {
    //Check for error bits.
    if(dwR1Resp & R1_EXECUTION_ERR_BITS)
    {
      dwRetVal = Err_AfterExecution_check_Failed;

      if(pberrrespbuff!=NULL)
      {
        if(nflags & 0x40) 
          //Kernel mode error response buffer.
          memcpy(pberrrespbuff,(unsigned char*)&dwR1Resp,4);
        else
          //User mode buffer.
          copy_to_user(pberrrespbuff,(unsigned char*)&dwR1Resp,4);
      }
    }

  }  

    #ifdef DMA_ENABLE
          Read_Write_IPreg(RINTSTS_off,&dwstate,READFLG);
          dwstate &= 0x30;
         Read_Write_IPreg(RINTSTS_off,&dwstate,WRITEFLG);
         dwstate = 0x01ffff;   
         Read_Write_IPreg(INTMSK_off,&dwstate,WRITEFLG);  
         ret=ReadReg(CTRL_off);
         ret &= ~(0x0020);
         WriteReg(CTRL_off,ret);   //disable DMA
    #endif      

  return dwRetVal;    
}


//--------------------------------------------------------------
//Original routine renamed as BlkSizeDataXfer.

int  Blksize_Data_Transfer(int ncardno, unsigned long dwaddr,
                           unsigned long dwdatasize,
                           unsigned char  *pbdatabuff,
                           unsigned char  *pbcmdrespbuff,
                           unsigned char  *pberrrespbuff,
                           void  (*callback)(unsigned long dwerrdata),
                           int nnoofretries,
                           int nflags)
{
  unsigned long dwRetVal=SUCCESS,dwBlockSize;
    unsigned int i;
  COMMAND_INFO  tCmdInfo={0X00};
  #define nonautostop
  
  //Check if Bytecount is in integral multiples of Blocksize.
  if(nflags & 0x080)//WRITE_DATA
     dwBlockSize = Card_Info[ncardno].Card_Write_BlkSize;
  else
     dwBlockSize = Card_Info[ncardno].Card_Read_BlkSize;

// printk("Blksize_Data_Transfer : dwBlockSize=%d,dwdatasize=%d\n",dwBlockSize,dwdatasize);
  //Form command:Form command structure.
  Form_Data_Command(ncardno,dwaddr,dwdatasize,nflags,
                    &tCmdInfo,00);

  //Set block transfer specific command index.
  if(nflags & 0x080)//WRITE_DATA
  {
    if(dwdatasize<=Card_Info[ncardno].Card_Write_BlkSize)
      //Set cmd.index for Single blk transfer.
      tCmdInfo.CmdReg.Bits.cmd_index                  = CMD24;
    else
      //Set cmd.index for Multiple blk transfer.
      tCmdInfo.CmdReg.Bits.cmd_index                  = CMD25;
  }     
  else
  {
    if(dwdatasize<=Card_Info[ncardno].Card_Read_BlkSize)
      //Set cmd.index for Single blk transfer.
      tCmdInfo.CmdReg.Bits.cmd_index                 = CMD17;
    else
      //Set cmd.index for Multiple blk transfer.
      tCmdInfo.CmdReg.Bits.cmd_index                 = CMD18;
  }
        
  //Check if predefined transfer.
  //In case of SD card,STOP command is always required
  //to stop muliple data tarnsfers.IP will send AUTO_STOP
  //command,if unsigned char count !=0 is set and if card type is 
  //non-MMC.
  //In case of MMC,if predefined transfer is expected,then
  //user is expected to send CMD23 firts as IP will not
  //send AUTO STOP for MMC.
  if((nflags & PREDEF_TRANSFER) &&
     (Card_Info[ncardno].Card_type==MMC))
  {
    //Multiple block transfer with Predefined block count.
    //Send CMD23 with block count ,prior to CMD18.
    if(form_send_cmd(ncardno,CMD23,(dwdatasize/dwBlockSize),
                     pbcmdrespbuff,1) !=SUCCESS) {
      printk("Blksize_Data_Transfer: 6 \n"); 
      return Err_CMD_Failed;}
  }

  //Send data transfer command.
  //Send_Data_Command will call Host::Send_SD_MMC_Command.
  //It will poll for command done,copy response in response 
  //buffer,perform,will return SUCCESS/error code.
  //Error handling will be done by Thread2.Thread2 will not
  //invoke callback for Interface Driver/Client to BSD.
 if(tCmdInfo.CmdReg.Bits.cmd_index==CMD25 && Card_Info[ncardno].Card_type==SD)
 {
    R1_RESPONSE  tR1;
      if((dwRetVal = form_send_cmd(ncardno,CMD55,00,
          (unsigned char*)(&tR1.dwStatusReg),1)) == SUCCESS )
      {
      //  printk("dwdatasize>>9=%d\n",dwdatasize>>9);
        dwRetVal=form_send_cmd(ncardno,CMD23,dwdatasize>>9,(unsigned char*)(&tR1.dwStatusReg),00);
        if(dwRetVal)
              printk("Blksize_Data_Transfer : send CMD23 failed!\n");
      } 
 }


  //Set auto stop bit,if multiple blk.transfer.
  
 #ifdef nonautostop
    if(tCmdInfo.CmdReg.Bits.cmd_index==CMD18)
    {
        Set_Auto_stop(ncardno, &tCmdInfo);
    }
#else
    if((tCmdInfo.CmdReg.Bits.cmd_index==CMD25) ||
        (tCmdInfo.CmdReg.Bits.cmd_index==CMD18))
    {
        Set_Auto_stop(ncardno, &tCmdInfo);
    }
#endif

  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                            pberrrespbuff,pbdatabuff,
                            callback,//No callback pointer
                            nflags,nnoofretries,
                             TRUE,(nflags&0x080),
                            ((nflags & 0x40) >>6) );
  if(dwRetVal != SUCCESS){
     HI_TRACE(2,"dwRetVal52 = %lx \n",dwRetVal); 
    return dwRetVal;} 

#ifdef nonautostop
    if(tCmdInfo.CmdReg.Bits.cmd_index==CMD25){    
        dwRetVal =  sd_mmc_stop_transfer(ncardno);
        if(dwRetVal != SUCCESS){
            return dwRetVal;
        } 
       if(nflags&0x080)
       {         
           #ifdef _HI_3511_
                 i = 50000;        //set the proper value by test!
        #endif                 
           #ifdef _HI_3560_
               i = 50000;
           #endif
            do
            {
                if (--i == 0){
                    break;
                }    
                dwRetVal =0;
                Read_Write_IPreg(STATUS_off,&dwRetVal,READFLG);    
            }while(dwRetVal & 0x200);
            if(i == 0)
            {
                i = 500;
                do
                {
                    if (--i == 0)
                        break;
                    msleep(8);
                    dwRetVal =0;
                    Read_Write_IPreg(STATUS_off,&dwRetVal,READFLG);    
                }while(dwRetVal & 0x200);                

                if(i == 0)
                {
                    i = 5;
                    do
                    {
                        if (--i == 0)
                            return Err_AfterExecution_check_Failed;
                        printk("Blksize_Data_Transfer Read IP status error begin msleep!\n");
                        msleep(1000);
                        dwRetVal =0;
                        Read_Write_IPreg(STATUS_off,&dwRetVal,READFLG);    
                    }while(dwRetVal & 0x200);
                }            
            }
       }//end of if(fwrite)
        }
#endif

  //If command executed properly,Check response for any error
  //reported by card. Previous state should be Transfer state.
  dwRetVal=Validate_Response(tCmdInfo.CmdReg.Bits.cmd_index,
                             pbcmdrespbuff,ncardno,CARD_STATE_TRAN);

  return dwRetVal;        
}

//--------------------------------------------------------------
/*Partial_blk_xfer()

i/p : UserBuffer = Buffer start address from where to read/write data.
                   Caller function should update buffer pointer accordingly.
      Byteaddress= Address in a block, from where to read/write partial data.
                   
      Bytecount  = Bytes to read/write.

Function : Read single block.
           If partial read is required--
             copy the specified number of bytes,from specified address,
             in user buffer.Data is copied from the address supplied as a 
             parameter for this function.

           If partial write is required--
             write the specified number of bytes,from specified address,
             from user buffer.
             Rewrite the data block.
*/
int Partial_Blk_Xfer(int ncardno,unsigned long dwaddress, unsigned long dwTotalBytes, 
                     unsigned char  *pbdatabuff,
                     unsigned char  *pbcmdrespbuff,
                     unsigned char  *pberrrespbuff,
                     int   nnoofretries,
                     int   nflags)
{
  //Allocate data buffer of maximum size.rd_blk_len and wr_blk_len are
  //of 4 bits size.But,values of 12-15 are reserved.So,max.=11.Hence,max.
  //size is 2048 bytes.
  unsigned char  LocalDataBuff[2048];
  COMMAND_INFO  tCmdInfo={0X00};

  unsigned long dwBlkStartAddr,dwRetVal=SUCCESS;
  unsigned long dwOffset = 0;

  //printk("Entered in Partial_Blk_Xfer routine\n");

  //Card number,start address should be already validated by the caller function.
    
  //Calculate block start address.
  dwBlkStartAddr = (dwaddress & ~(Card_Info[ncardno].Card_Read_BlkSize-1));

  //Read single block.Pass-on nflags value as it is as user supplied options
  //should be followed for each block transfer.Only make it as a read command.
  //Form command structure.
  Form_Data_Command(ncardno,dwBlkStartAddr,(Card_Info[ncardno].Card_Read_BlkSize),
                    (nflags &0x7f),&tCmdInfo,00);

  //Use single block read.
  tCmdInfo.CmdReg.Bits.cmd_index = CMD17;

  //No need to check for pre-defined transfer or auto-stop setting.
  //Read,no call back,kernel address space buffer.
//  printk("Partial_Blk_Xfer : Send_cmd_to_host!11\n");
  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff ,
                            pberrrespbuff,LocalDataBuff,
                            NULL,nflags,
                            nnoofretries,TRUE,
                            00,1);
  if(dwRetVal!=SUCCESS)
    return dwRetVal;

  //Check response.
  if((dwRetVal=Validate_Response(tCmdInfo.CmdReg.Bits.cmd_index,
     pbcmdrespbuff,ncardno,CARD_STATE_TRAN)) != SUCCESS) return dwRetVal;

  //Calculate required data offset.
  dwOffset = dwaddress - dwBlkStartAddr;

  //Check if it is partial read.
  if(nflags & 0x80)
  {
    //If partial write.
    //Copy data from user buffer to local buffer.
    if(nflags & 0x40)
       memcpy((LocalDataBuff+dwOffset),pbdatabuff,dwTotalBytes); 
    else
    {
      //User mode buffer.
      if(copy_from_user((LocalDataBuff+dwOffset),pbdatabuff,dwTotalBytes)) 
        return Err_partial_write_failed;         
    }

    //Rewrite the block.tcmdInfo is already filled.
    //Read/write. bit3
    tCmdInfo.CmdReg.Bits.read_write = 1;

    //Use single block write.
    tCmdInfo.CmdReg.Bits.cmd_index  = CMD24;

    //No need to check for pre-defined transfer or auto-stop setting.
    //Read,no call back,kernel address space buffer.
  //  printk("Partial_Blk_Xfer : Send_cmd_to_host!22\n");
    dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff ,
                              pberrrespbuff,LocalDataBuff,
                              NULL,nflags,
                              nnoofretries,TRUE,
                              1,1);//write,kernel buffer.
    if(dwRetVal==SUCCESS)
      //Check response.
      dwRetVal=Validate_Response(tCmdInfo.CmdReg.Bits.cmd_index,
                                 pbcmdrespbuff,ncardno,CARD_STATE_TRAN);
  }
  else
  {
    //Copy data from local buffer to user buffer.
    if(nflags & 0x40)
       memcpy(pbdatabuff,(LocalDataBuff+dwOffset),dwTotalBytes); 
    else
    {
      //User mode buffer.
      if(copy_to_user(pbdatabuff,(LocalDataBuff+dwOffset),dwTotalBytes)) 
        return Err_partial_read_failed;         
    }
  }

  return dwRetVal;
}

//--------------------------------------------------------------

/*B.8.2.Read data stream.(MMC,SDIO only)
   Send_read_Command : Above parameters set in      LEVEL2
   structure.
   ** If data size is more than FIFO size :
   When FIFO reaches threshold,interrupt is generated.
   ISR will take care of reading/writing FIFO.

   Data buffer supplied by caller.

   int  read_write_streamdata (int ncardno, unsigned long dwaddr,
                          unsigned long dwdatasize,
                          unsigned char  *pbdatabuff,
                          unsigned char  *pbcmdrespbuff,
                          unsigned char  *pberrrespbuff,
                          void  (*callback)(unsigned long dwerrdata),
                          int nnoofretries,
                          int nflags)


  *Called from : FSD
  *Parameters :
         ncardno:(i/p):Card to be addreessed.
         dwaddr:(i/p):Starting Address
         dwdatasize:(i/p):Total size of data.
         pbdatabuff:(i/p-o/p):Data buffer.
         pbcmdrespbuff:(o/p):Command responsee buffer.
         pberrrespbuff:(o/p):Command responsee buffer.
         callback:(o/p):Callback pointer to be called when data
                        transfer over.
         nnoofretries:(i/p):Number of retries in case of error.
         nflags:(i/p):Flags.
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                      Send cmd23 first.
         Handle_Data_Errors (bit4)=1; Handle data errors.
         Send_Abort (bit5)=1;
         Read_Write   (bit7)=1; Write operation. 0=Read.   

  *Return value : 
   0: for SUCCESS
   -EINVAL :Data bufer pointer is NULL.
   Err_Card_Busy
   Err_CMD_Failed
  
  *Flow:
   Sanity check:
        -Check if card no.is less than total cards connected.
         else return error.
        -Check if pointer to data buffer is not NULL.
         Else return error.
         Pointer to cmd response buffer and Error reponse buffer
         could be NULL.
        -Check if card is connected.Else return error.

   //No need to mark set_of_commands :

   Card_Status_Check:
        -Read card status.If busy code is returned,return error.
         Call Read_card_Status(Card Number)

   Validate_command:(for given card)
        -Call Validate_Command(ptCardParams,command,card_info)
         To check :
         If (start+count) goes beyond card size.
         If card is NOT MMC card,return error.

   Form command:
        -Form command structure.Call
         Form_Data_Command(Card Number,Data size,Data Address,
                           Flags, Pointer to COMMAND_INFO);

        -Set stream transfer specific command index.
         CMD11 / CMD20.

   Select_card:
        -Select card.Call Send_CMD7(Card_Number).

   Send_Supportive_Cmd:(if any)
        -Nothing.

   sd_mmc_send_command:
        -Command is formed,now send it for execution.
 
         When command is executed,response will be
         copied is response buffer.

   Check response:
        -Check if response has any error.
         Call Validate_Response(Command,response,card_no)

        -Return.        
*/
int    read_write_streamdata (int ncardno, unsigned long dwaddr,
                          unsigned long dwdatasize,
                          unsigned char  *pbdatabuff,
                             unsigned char  *pbcmdrespbuff,
                          unsigned char  *pberrrespbuff,
                          void  (*callback)(unsigned long dwerrdata),
                          int nnoofretries,
                          int nflags)
{
  unsigned long dwRetVal,dwR1Resp;
  COMMAND_INFO  tCmdInfo={0x00};

  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(ncardno,dwaddr,dwdatasize,
     Stream_Xfer) ) != SUCCESS)
    return dwRetVal;

  //Check if pointer to data buffer is not NULL.
  //Pointer to cmd response buffer and Error reponse buffer
  //could be NULL.
  if(pbdatabuff==NULL)
    return -EINVAL;

  /*Check and send card in transfer mode*/
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;
    
  //Form command:Form command structure.
  Form_Data_Command(ncardno,dwaddr,dwdatasize,nflags,
                    &tCmdInfo,1);
    
  //For stream transfer,block size is irrelevant.
  //Program it to known value ie 00.
  tCmdInfo.dwBlkSizeReg=00;
    
  //Set stream transfer specific command index.
  if(nflags & 0x080)//WRITE_DATA
  {
    tCmdInfo.CmdReg.Bits.cmd_index          = CMD20;
    //If unsigned char count is more than 6,set auto stop bit.
    if(dwdatasize > 6)
      tCmdInfo.CmdReg.Bits.auto_stop = 1; // 1=send auto stop.
  }
  else
  {
    tCmdInfo.CmdReg.Bits.cmd_index          = CMD11;
    //If unsigned char count is more than 22,set auto stop bit.
    if(dwdatasize > 22)
      tCmdInfo.CmdReg.Bits.auto_stop = 1; // 1=send auto stop.
  }
  //Command is formed,now send it for execution.
  //Control will be returned when command is accepted
  //for execution.
  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                            pberrrespbuff,pbdatabuff,
                            callback,//No callback pointer
                            nflags,nnoofretries,
                            TRUE,(nflags&0x80),
                            FALSE );// 0= user address space buffer.
  if(dwRetVal != SUCCESS)
    return Err_CMD_Failed; 

  //If command executed properly,Check response for any error
  //reported by card.
  dwRetVal=Validate_Response(tCmdInfo.CmdReg.Bits.cmd_index,
                             pbcmdrespbuff,ncardno,CARD_STATE_TRAN);

  if(dwRetVal != SUCCESS)  return dwRetVal;

  //Check if any error occured during execution of command.
  //Bits 19,20,21,22,23,24,26,27,29,30,31.
  if((dwRetVal = form_send_cmd(ncardno,CMD13,00,(unsigned char*)&dwR1Resp,1))
      ==SUCCESS)
  {
    //Check for error bits.
    if(dwR1Resp & R1_EXECUTION_ERR_BITS)
    {
      dwRetVal = Err_AfterExecution_check_Failed;

      if(pberrrespbuff!=NULL)
      {
        if(nflags & 0x40) 
          //Kernel mode error response buffer.
          memcpy(pberrrespbuff,(unsigned char*)&dwR1Resp,4);
        else
          //User mode buffer.
          copy_to_user(pberrrespbuff,(unsigned char*)&dwR1Resp,4);
      }
    }

  }  
  else
    return Err_AfterExecution_check_Failed;

  return dwRetVal;        
}
//--------------------------------------------------------------
/*B.8.3.Form_Data_Command(int ncardno, unsigned long dwDataAddr,
  unsigned long dwdatasize, int nflags,
  COMMAND_INFO *ptCmdInfo,
  int nTransferMode);
  
  *Called from :read_write_streamdata,Read_Write_BlockData 

  *Parameters : 
         ncardno:(i/p):
         dwDataAddr:(i/p):
         dwdatasize:(i/p):
         nflags:(i/p):
         ptCmdInfo:(o/p):Required registers are formed and 
                         stored as structure elements.
         nTransferMode:(i/p): 1=Stream, 0=block transfer.

  *Flow :
        //Set command register.
        -Assign values to local COMMAND_REG structure.
         //card no.      
         ptCmdInfo->Cmdreg.card_number        = ncardno;

         //send_initialization or not (bit0)
         ptCmdInfo->Cmdreg.send_initialization  =bit0 in nflags.
         //wait till previous data complete        
         ptCmdInfo->Cmdreg.wait_prvdata_complete=bit1 in nflags.
         //respose crc to check or not 
         ptCmdInfo->Cmdreg.check_response_crc   =bit2 in nflags.

         //Set transfer_mode. 1=stream, 0=block.
         ptCmdInfo->Cmdreg.transfer_mode        = nTransferMode ; 

         //Set SD/SDIO mode,if block transfer.
         if( ! nTransferMode)
         {
           //SD Transfer or SDIO Transfer.
           if(Card_Info.Card_Type == SD)
               ptCmdInfo->Cmdreg.SD_mode = 0 //SD memory transfer
           else
              if(Card_Info.Card_Type == SDIO)
               ptCmdInfo->Cmdreg.SD_mode = 1 //SDIO memory transfer
         }

         //short response      
         ptCmdInfo->Cmdreg.response_length      = 0  ;

         //Response always expected.
         ptCmdInfo->Cmdreg.response_expect      = 1 ;

        -//Data always expected for data transfer.(RD/WR)
         ptCmdInfo->Cmdreg.data_expect          = 1  ;

        -//Read/write. bit3
         ptCmdInfo->Cmdreg.read_write        = bit3 of nflags.

        -Set command argument register. 
         ptCmdInfo->dwcmdargReg = dwDataAddr;
      
        -Set unsigned char count register value.
         ptCmdInfo->dwByteCountReg = dwdatasize;

        -Set Block size register value.This value is ignored in 
         stream transfer.
         ptCmdInfo->dwBlkSizeReg = Card_Info.BlockSize;
     
        -Return.
*/
int Form_Data_Command(int ncardno, unsigned long dwDataAddr,
                      unsigned long dwdatasize, int nflags,
                      COMMAND_INFO *ptCmdInfo,
                      int nTransferMode)
{
  //**Set command register.

  //start_cmd
  ptCmdInfo->CmdReg.Bits.start_cmd=1;

  //Update_clk_regs_only.
  ptCmdInfo->CmdReg.Bits.Update_clk_regs_only=0;

  //card no.      
  ptCmdInfo->CmdReg.Bits.card_number=ncardno;

  //send_initialization or not (bit0)
  ptCmdInfo->CmdReg.Bits.send_initialization =
                                       (nflags & 0x01);

  //Stop_abort command.
  ptCmdInfo->CmdReg.Bits.stop_abort_cmd=0;

  //wait till previous data complete        
  ptCmdInfo->CmdReg.Bits.wait_prvdata_complete =
                                       ((nflags >> 1) & 0x01);

  //This command is always used for memory transfer.
  //SDIO_IO transfer will not use this.
  ptCmdInfo->CmdReg.Bits.auto_stop = 0; //no auto_stop

  //Set transfer_mode. 1=stream, 0=block.
  ptCmdInfo->CmdReg.Bits.transfer_mode = nTransferMode ; 

  //Read/write. bit3
  ptCmdInfo->CmdReg.Bits.read_write = ( (nflags>>7) & 0x01); 


  //Data always expected for data transfer.(RD/WR)
  ptCmdInfo->CmdReg.Bits.data_expected = 1  ;

  //respose crc to check or not 
  ptCmdInfo->CmdReg.Bits.check_response_crc =
                                      ((nflags >> 2) & 0x01);

  //short response      
  ptCmdInfo->CmdReg.Bits.response_length = 0  ;

  //Response always expected.
  ptCmdInfo->CmdReg.Bits.response_expect = 1 ;


  //Set command argument register. 
  ptCmdInfo->dwCmdArg = dwDataAddr;
      
  //Set unsigned char count register value.
  ptCmdInfo->dwByteCntReg = dwdatasize;

  //Set Block size register value.This value is ignored in 
  //stream transfer.
  if(ptCmdInfo->CmdReg.Bits.read_write)
     ptCmdInfo->dwBlkSizeReg = Card_Info[ncardno].Card_Write_BlkSize;
  else
     ptCmdInfo->dwBlkSizeReg = Card_Info[ncardno].Card_Read_BlkSize;


  return SUCCESS;
}

//--------------------------------------------------------------
//Condition for auto stop:
//For STREAM transfer-READ mode  : bytecount should be > 22
//For STREAM transfer-WRITE mode : bytecount should be > 6

//For BLOCK transfer, READ  mode, 1 BIT :
//   (bytecount > 20) && (BLOCKsize > 6)
//For BLOCK transfer,WRITE  mode, 1 BIT :
//   (bytecount > 6) && (BLOCKsize > 6)

//For BLOCK transfer, READ  mode, 4 BIT :
//  (bytecount > 80) && (BLOCKsize >24)
//For BLOCK transfer, WRITE  mode,4 BIT :
//  (bytecount > 24) && (BLOCKsize >24)

//Formula for block transfers:
// 1bit ? mult=4 :1  For MMC block transfer,always 1 bit transfer. 
//Set Autostop, if (read && (blksize*mult >24) && (bytecount * mult >80))
//Set Autostop, if (write && (blksize*mult >24) && (bytecount * mult >24))


void Set_Auto_stop(int ncardno,COMMAND_INFO *ptCmdInfo)
{
  unsigned long dwBlkSize,dwBytCnt,dwCtype=0;
  int nMult=0;

  //printk("Set_auto_stop :Entry \n");

  //Set multiplier.
  //Read card type register.Check required sd width bit.
  Read_Write_IPreg(CTYPE_off,&dwCtype,READFLG);      

  dwCtype &= 0x0000ffff;

  nMult = ((dwCtype >> ncardno) & 1) ? 1:4;
  //MMC is always 1bit.So,multiplier is 4 always. 
  if(Card_Info[ncardno].Card_type==MMC)
    nMult=4;

  //Set auto stop bit.
  dwBlkSize = ptCmdInfo->dwBlkSizeReg;
  dwBytCnt = ptCmdInfo->dwByteCntReg;

  ptCmdInfo->CmdReg.Bits.auto_stop = 0;

  if( ( (ptCmdInfo->CmdReg.Bits.read_write) &&  
        ((dwBlkSize * nMult)> 24) &&
        ((dwBytCnt * nMult)> 24) 
      )  ||

      ( !(ptCmdInfo->CmdReg.Bits.read_write) &&  
        ((dwBlkSize * nMult)> 24) &&
        ((dwBytCnt * nMult)> 80) ) 
      )
      ptCmdInfo->CmdReg.Bits.auto_stop = 1;

  //printk("Set_auto_stop: CmdReg.Bits.auto_stop = %x \n", ptCmdInfo->CmdReg.Bits.SD_mode );

  return; 
}

//--------------------------------------------------------------
/*B.8.6.Stop transfer :Sends STOP-TRANSMISSION command (CMD12) to 
  card.Valid when current command is data transfer.Instructs 
  the card to stop the data transfer.

  This command will be sent by the application  whose data 
  transfer is in process.So,it is not possible that transfer
  set by one application will be stopped by another application.

  *Called From:FSD.

  *Parameters:
         ncardno:(i/p) :Card to be addressed.

  *Return value : 
   0: for SUCCESS

*/
int  sd_mmc_stop_transfer(int ncardno)
{
  unsigned long dwRetVal,dwRespBuff;
  //Validate card number.
  if((dwRetVal = Validate_Command(ncardno,00,00,
     General_cmd) ) != SUCCESS)  return dwRetVal;

  //Send STOP command ie CMD12.
  if((dwRetVal = form_send_cmd(ncardno,CMD12,00,
     (unsigned char*)(&dwRespBuff),1)) !=SUCCESS)  return dwRetVal;

  //If current state of card is not standby,return error.
  return Validate_Response(CMD12,(unsigned char*)(&dwRespBuff),ncardno,
                           DONOT_COMPARE_STATE);
}
//--------------------------------------------------------------
/*B.8.7.Read/write data from FIFO in IP.:Copy data from FIFO,to data
  buffer supplied.Note :FIFO depth is only 1k.While copying keep 
  track of page end.Data buffer is a page allocated from system 
  memory.So,pointer supplied is in the range of PgAddr to 
  PgAddr+4kb.

  Read_Write_FIFO(unsigned char *pbdatabuff,int nByteCount,int fwrite,
                  int fKerSpaceBuff)

  *Called from : BSD::Internal functions.

  *Paramemters :
         pbdatabuff :(o/p);Data read from FIFO is stored in this.
         nByteCount :(i/p);Number of bytes to be read from FIFO.
         fwrite     :(i/p): 0=Read  1=Write 
         fKerSpaceBuff:(i/p):1= kernel space buffer. 0=user space buffer.
  *Flow :
        -Check that pbdatabuff is not NULL.Else return error.
        -Check that (pbdatabuff+nByteCount) lies within page
         boundary.Else return PageBoundryError.

        -If read operation call Host::Read_multiple_data()
           Read_multiple_data( FIFO_start,nByteCount,pbdatabuff)

         else write operation call Host::Write_multiple_data()
           Write_multiple_data(FIFO_start,nByteCount,pbdatabuff)

        -Return;
*/
int Read_Write_FIFO(unsigned char *pbdatabuff,int nByteCount,int fwrite,
                    int fKerSpaceBuff)
{
  //Check that pbdatabuff is not NULL.
  if(pbdatabuff==NULL)
    return -EINVAL;

  //Check that FIFO is not full for writing or empty for 
  //reading.Read status register for this.
  //Read or Write from FIFO.

  //printk(" : From Read_Write_FIFO,going to host routine. \n");
  return   Read_Write_FIFOdata(nByteCount,pbdatabuff,fwrite,fKerSpaceBuff);

}

/* Check_TranState : Checks if the given card is in transfer 
   state or not.

   unsigned long Check_TranState(int ncardno)

  *Called from: Driver functions.

  *Parameters:
   ncardno:(i/p):Card number.

  *Return Value: 0 for SUCCESS ie if card is in transfer mode.
*/

unsigned long Check_TranState(int ncardno)
{
  unsigned long dwRetVal,dwR1Resp,dwCardStatus;

  //Check card status.
//  printk("Check_TranState : form_send_cmd!\n");
  if( (dwRetVal = form_send_cmd(ncardno,CMD13,00,(unsigned char*)&dwR1Resp,1))
    !=SUCCESS)  return dwRetVal;

  //If card state = STDBY,issue CMD7.
  //If card state = TRANS,donot issue CMD7.Card is selected.
  //For other states,return error.
  dwCardStatus = R1_CURRENT_STATE(dwR1Resp);

  if(dwCardStatus == (unsigned int)CARD_STATE_STBY) 
  {
     //Send card in transfer mode.CMD7.
     if((dwRetVal = form_send_cmd(ncardno,CMD7,00,(unsigned char*)&dwR1Resp,
        1))  !=SUCCESS)  return dwRetVal;

     //Validate response.
     if((dwRetVal=Validate_Response(CMD7,(unsigned char*)&dwR1Resp,ncardno,
        DONOT_COMPARE_STATE))!= SUCCESS)  return dwRetVal;
  }
  else if(dwCardStatus != (unsigned int)CARD_STATE_TRAN) 
         return Err_Invalid_Cardstate;

  return SUCCESS;
}


//==============================================================
/*B.9.SDIO Data transfer commands.

**** Using sdio_read_wait bit in SDIO read commands.

      sdio_abort_transfer
      sdio_suspend_transfer
      sdio_resume_transfer
      Form_SDIO_CmdReg
*/
//--------------------------------------------------------------
/*B.9.1.Abort transfer :Applicable only for SDIO type.
   **Note:Abort command is applicable for IO section only.
   For memory section of combo card,use STOP command as used
   for SD card.To abort IO transfer,write Function number in 
   ASx bits of CCCR @0x006.

   sdio_abort_transfer(int ncardno,int nfunctionno)

  *Called From:FSD

  *Parameters:
         ncardno:(i/p) :
         nfunctionno:(i/p) :

  *Return value : 
   0: for SUCCESS
   Err_CMD_Failed

  *Flow:
        -Check that card number is valid.
        -//Pior to this command,data transfer command is in 
         //process.So,already function number is selected there.
        -//Form command.CCCR Write,Regaddress = 0x06,
         //WrData=FunctionNumber,RAW enabled. 
         //ASx = Function number for ABORT.

         Call sdio_rdwr_direct(ncardno,0,0x06,
                               nfunctionno,TRUE,TRUE).

        -Response is returned by sdio_rdwr_direct().
         Return it.
*/
int    sdio_abort_transfer(int ncardno,int nfunctionno)
{
  unsigned long dwRetVal;
  unsigned char  bWriteData;
  unsigned char  pbrespbuff[4];//R5 response is one unsigned long.
  SDIO_RW_DIRECT_INFO tRWdirectInfo={0x00};
    
  //printk("Entry in sdio_abort_transfer.\n");

  /*Sanity check:*/
  //Validate card number,card type(SDIO),function number(1-7)
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
     SDIO_Command) ) != SUCCESS)  return dwRetVal;

  //Prior to this command,data transfer command might be or 
  //might be not in process.So,select function number in ASx.

  //Form command.CCCR Write,Regaddress = 0x06,
  //WrData=FunctionNumber,RAW enabled. 
  //ASx = Function number for ABORT.

  //Clear ASx bits.
  bWriteData = 00;

  //Set ASx bits as per function number.
  bWriteData |= (unsigned char)nfunctionno;

  tRWdirectInfo.Bits.WrData = bWriteData ;
  tRWdirectInfo.Bits.RegAddress = 0x06 ;
  tRWdirectInfo.Bits.RAW_Flag = TRUE  ;
  tRWdirectInfo.Bits.FunctionNo = 0 ;
  tRWdirectInfo.Bits.RW_Flag = TRUE  ;
    
  //Non data command.So,use form_send_cmd
  if((dwRetVal = form_send_cmd(ncardno,SDIO_ABORT,tRWdirectInfo.dwReg,pbrespbuff,
     0) )!= SUCCESS)   return dwRetVal; 
 
  //check response flags for error.
  dwRetVal=Validate_Response(CMD52,pbrespbuff,ncardno,
                             DONOT_COMPARE_STATE);

  //printk("Exit from sdio_abort_transfer.\n");
    
  return dwRetVal;
}
//--------------------------------------------------------------
/*B.9.2.Suspend data transfer. SDIO specific.
  Suspends the given function transfer for given card.

  1.This command will be accepted when data transfer command 
    for the given SDIO card is accepted.Then only control will 
    be passed to application.
    Already function number is selected by previous command.

    **Driver reads currently selected functio n number.If it
    mismatches with that issued by user,implies that selected
    function cannot be suspended.
    **By writing function no.at FSx,will resume the suspended
    function.

  2.This command will be treated as non data command.
    IP will stop transfer on block boundry,assuming that
    card also does same,when suspend command is received by card.
    IP internally decodes suspend command.

    On terminating data transfer(read/write),IP issues Data Done
    command.

  3.Suspend:
    Set FSx to function number which has currently grabbed the
    data bus.
    Set BR (Bus release) bit in CCCR register.
    Addressed function will temporarily halt data transfer on
    DATx lines and suspend the command that is in process.

  4.**It is applications's responsibility to keep track of
    bytes pending and accordingly send resume command.

  5.If data transfer for card is suspended,donot send any other
    data command,till that transfer is cleared.
    sdio_abort_transfer has no effect on suspended state.
    User should poll EXx flag and then,send other data command.

  int sdio_suspend_transfer(int ncardno, int nfunction,
                   unsigned long *pdwpendingbytes)

  *Called from : FSD

  *Parameters  :
         ncardno:(i/p):Card number.
         nfunction:(i/p):Function number.
         pdwpendingbytes:(o/p):Holds pending data bytes to 
                               transfer.

  *Return value : 
   0: for SUCCESS
   Err_Suspend_Not_Allowed
   Err_Function_Mismatch:Function to be suspended is currently
                         not selected in card.
   Err_Suspend_Not_Granted

  *Flow:

   Sanity check:
        -Check if given card is SDIO.
        -Check if given card supports Suspend/Resume protocol.
         //For SDIO card,after enumeration,CCCR register is read
         //and stored in card info.SBS (Bit3 at address 0x08).
         Else return error.

        -Read currently selected function.
         If(Current Function != User supplied function)
           return Err_Function_Mismatch;

   Form command:
        -Set BR (bit1 at address 0x0C) in CCCR register.
   Send command:
        -Send CMD52 (IO_RW_Direct command) in RAW format.
         Call sdio_rdwr_direct(ncardno,0,0x0C,
                               0x02,TRUE,TRUE).

   Read response.:
        -sdio_rdwr_direct returns response.
        //In response,written register is read again.
         Poll for BR=0 and BS=0 to confirm that bus has been
         released.

        -Else Read CCCR register.(IO function0 read)
         Poll for BR=0 and BS=0 to confirm that bus has been
         released.
         Call sdio_rdwr_direct(ncardno,0,0x0C,
                               00,FALSE,FALSE).

        -If (previous command is data read command)  
           -Copy data received in FIFO,in data buffer.
           -Check 
            if (suspend command is given at the end of data)
                //DF=0,indicates all data has been transferred.
               Pending_Data_Count=0;// No_Data_Pending
            else
              -Read pending bytes count.
              -Pending_Data_Count= pending bytes count.
         else //Previous write command.
           -Read pending bytes count.
           -Pending_Data_Count= pending bytes count.

        -Reset FIFO and DMA.//If DMA is still transferring data?
        -Return Pending_Data_Count;
*/
int sdio_suspend_transfer(int ncardno, int nfunction,
                     unsigned long *pdwpendingbytes)
{
  unsigned long dwRetVal;
  unsigned char  bData[2];
  unsigned char  pbrespbuff[4];//R5 response is one unsigned long.
    
  int i;
  ControlReg  tCtrlReg={0x00};
    
  /*Sanity check:*/
  //Suspend/resume NOT supported by function 0.
  if(nfunction==0)
    return Err_Invalid_Command;

  //Validate card number,card type(SDIO),function number(1-7)
  if((dwRetVal = Validate_Command(ncardno,00,nfunction,
     SDIO_Command) ) != SUCCESS)   return dwRetVal;

  //Check if given card supports Suspend/Resume protocol.
  //For SDIO card,after enumeration,CCCR register is read
  //and stored in card info.SBS (Bit3 at address 0x08).
  if(!(Card_Info[ncardno].CCCR.Card_Capability & 0x08) )        
    return Err_Suspend_Not_Allowed;

  //Read currently selected function.User function number
  //cannot be set at FSx as if that function is already
  //suspended,this action will resume it.
  //Read bits 0-3 at address 0x0D of CCCR register(Fn=0).
  //These bits are valid if BS=1. (address 0xc::bit0).
    
  if((dwRetVal= sdio_rdwr_extended(ncardno,00,0xC,0x2,
                                   bData, pbrespbuff,
                                   00,  //No err response buffer
                                   0x01,//incre,addr.(bit0), Cmdoptions
                                   NULL,//No callback.
                                   00,0x50))//unsigned char mode,no wait for prev data,read.
     != SUCCESS)  return dwRetVal;

  //Check if data bus is in use ie BS=1? 
  if(bData[0] & 0x1)
  {   
    //Check if current function is same as user supplied no.          
    if((bData[1] & 0xf) != nfunction)         //modify by w53349 20061103  bData[0]->bData[1]
      return Err_Function_Mismatch;
  }
  else
    //For BS=0,Data bus is not in use.
    return Err_DataBus_Free;        

  /*Form command:Function no.is already selected. */
  //Set BR (bit1 at address 0x0C) in CCCR register.
  bData[0]=0x2;
  //Write with RAW.
  if((dwRetVal=sdio_rdwr_direct(ncardno,0,0x0C,bData,
     pbrespbuff,TRUE,TRUE,0x02)) != SUCCESS)  return dwRetVal;
    
  //In response,written register is read again.
  if(bData[0] & 0x03)
  {
    //BS and BR are not yet cleared ie suspend not granted.
    //Poll for BR=0 and BS=0 to confirm that bus has been 
    //released.
    for(i=0;i<SUSPEND_RETRY_COUNT;i++)
    {
      dwRetVal=sdio_rdwr_direct(ncardno,0,0x0C,bData,pbrespbuff,
                                FALSE,FALSE,0x02);
      if(!(bData[0] & 0x03))
        break;
    }
        
    //If BR and BS bits not cleared,suspend is not granted.
    if(i >= SUSPEND_RETRY_COUNT)
      return Err_Suspend_Not_Granted;
  }

  //If read operation,set abort_read_data(bit8) in control reg
  tCtrlReg.Bits.abort_read_data=1;
  Set_Reset_CNTRLreg(tCtrlReg.dwReg,TRUE);
    
  //After suspend,IP will generate Data Done interrupt and
  //handler for that will copy the data from FIFO into data
  //buffer,if previous command is read command.
    
  // **** After data transfer over, read remaining bytes.
  //So,wait here for till data transfer is over.
  while(tDatCmdInfo.fCmdInProcess);
    
  //Read pending bytes count.(CIU to card).
  if(pdwpendingbytes!=NULL)
    Read_Write_IPreg(TCBCNT_off,pdwpendingbytes,READFLG);

  return SUCCESS;
}
//--------------------------------------------------------------
/*B.9.3.Resume data transfer. SDIO specific.

  1.Instructs card to resume the operation,where it has been 
    left out. 

  2.For write,host should continue data transfer from wherever 
    left and card will take care of proper destination address.

  3.For read,card takes care of sending data from wherever left
    and host should store the data at proper address.

  4.If user tried for recursive suspend and resume,it will be 
    immense complex for driver to handle this.Instead anyway,
    application has to keep track of data transfer suspend /
    resumed.So,let user pass this info.
  5.For IP this is a new data command.So,require all parameters 
    essential for data transfer command.

  sdio_resume_transfer(int ncardno, int nfunctionno,
         unsigned long dwpendingdatacount, 
         unsigned char  *pbdatabuff,
         unsigned char  *pbcmdrespbuff,
         unsigned char  *pberrrespbuff,
         void  (*callback)(unsigned long dwerrdata),
         int nnoofretries,
         int nflags)

  *Called from : FSD

  *Parameters:
         ncardno:(i/p):Card to whom resume command is to be send.
         nfunctionno:(i/p):Function number to resume.(CmdArg)
         dwpendingdatacount:(i/p):Number of bytes to transfer. 
         pbdatabuff:(i/p):Data buffer.
         pdwCmdRespBuff:(o/p):Command responsee buffer.
         pberrrespbuff:(o/p):Command responsee buffer.
         callback:(o/p):Callback pointer to be called when data
                        transfer over.
         nnoofretries:(i/p) :Number of retries in case of failure.
                      Currently not implemented.
         nflags:(i/p):Flags.
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                  Send cmd23 first.
         Handle_Data_Errors (bit4)=1; Handle data errors.
         Send_Abort (bit5)=1;
         Read_Write   (bit7)=1; Write operation. 0=Read.   

  *Return value : 
   0: for SUCCESS
   Err_Suspend_Not_Allowed
   -EINVAL:data buffer pointer is NULL.
   Err_Card_Busy
   Err_CMD_Failed
   Err_NoData_FromCard:No data avaialble with card for resume.

   *Flow:
   Sanity check:
        -Check if given card is SDIO.
        -Check if given card supports Suspend/Resume protocol.
         //For SDIO card,after enumeration,CCCR register is read
         //and stored in card info.SBS (Bit3 at address 0x08).
         Else return error.
        -Check if data buffer is not NULL.

   Command_Info:
        -Create COMMAND_INFO instance.(*ptCmdParams)

   Allocate_Resp_Buffer:
        -Allocate response buffer.(ptRespBuff)

   Form command:
         //Send CMD52 (IO_RW_Direct command) in RAW format.
        -Set BR (bit1 at address 0x0C) in CCCR register.
         (WriteData)

        //Configure IP for data transfer command.

        //command argument for CMD52. 
        ptCmdParams->dwcmdargReg=R/Wflag||FunctionNo||RAW flag||
                      Register Address||WriteData  ;
         
        //Form cmd reg.
        call Form_SDIO_CmdReg(ncardno,fwrite,CMD52,
                              fWaitForPrvData);

       -//Set Block size and unsigned char count.
        ptCmdParams->dwBlkSizeReg = dwBlkSize;
        ptCmdParams->dwByteCountReg = dwpendingdatacount;

       -Response buffer.
        ptCmdParams->ptResponse_info = ptRespBuff;

   Send command:
       -Command is formed,now add it in queue for execution.
        Call Add_command_in_queue(ptCmdParams,ptAppInfo)

       -Sleep (&ptAppInfo->Wqn)
       //Command will be processed when status poller calls
       //the switch to load next command.Sleep till command
       //is accepted.

   Read response.:
        //In response,written register is read again.
         Poll for BR=0 and BS=0 to confirm that bus has been
         released.

        -Else Read CCCR register.(IO function0 read)
         Poll for BR=0 and BS=0 to confirm that bus has been
         released.

        -Return.

   **On read data done or FIFO threshold,copy data from FIFO to 
     data buffer.
   **For write,on FIFO threshold,copy data from data buffer to 
     FIFO.
*/
//If resume given when data transfer for other function is in
//process:Then,this resume command is treated as illegal.
//Ref,AppxC,page D.

//EXx bit: fter power ON,this bit is 0 indicating Fn can accept
//command now.When any command is issued-say CMD53-then,EXx bit
//is set to 1,till that command is fully finished. eg for 
//multiple read operation,it is cleared when whole required data
//is read.For write.it is cleared when all data is written.Till 
//that it is set to 1, indicating Fn is busy in executing 
//command.

//RFx:While executing read command (EX=1),this bit is cleared 
//till the required data is read from memory and ready to be 
//transferred.Once,data gets ready for transfer,it is set to 1.
//For write command, 0 indicates data is not yet written 
//completely in memory.Once,data is written and function can 
//accept new data for writing,this bit is set to 1.

//Consider,multiple read command is issued. EX=1,RF=0
//Function is suspended.                    EX=1,RF=0
//Function completes reading data.          EX=1,RF=1
//Host cannot send another command to same 
//function as EX=1.But,can now read data from
//function as data is ready for transfer.
//Host issues resume command.               EX=1 RF=1
//Data transfer for that register is complete,
//but command is not yet finished being multiple
//read command.
//Function gets engaed in reading next data EX=1, RF=0
//for same comamnd.
//When all the required bytes are read and  EX=0, RF=x.
//transferred.

int sdio_resume_transfer(int ncardno, int nfunctionno,
                    unsigned long dwpendingdatacount, 
                    unsigned char  *pbdatabuff,
                    unsigned char  *pbcmdrespbuff,
                    unsigned char *pberrrespbuff,
                    void  (*callback)(unsigned long dwerrdata),
                    int nnoofretries,
                    int nflags)
{
  unsigned long dwRetVal;
  unsigned char bData;
  COMMAND_INFO  tCmdInfo={0x00};
  SDIO_RW_DIRECT_INFO tRWdirectInfo={0x00};

  /*Sanity check:*/
  //Suspend/resume NOT supported by function 0.
  if(nfunctionno==0)
    return Err_Invalid_Command;

  //Validate card number,card type(SDIO),function number(1-7)
  if((dwRetVal = Validate_Command(ncardno,00,nfunctionno,
     SDIO_Command) ) != SUCCESS)  return dwRetVal;

  //Check if given card supports Suspend/Resume protocol.
  //For SDIO card,after enumeration,CCCR register is read
  //and stored in card info.SBS (Bit3 at address 0x08).
  if(!(Card_Info[ncardno].CCCR.Card_Capability & 0x08) )        
    return Err_Suspend_Not_Allowed;

  if(pbdatabuff==NULL)
    return -EINVAL;

  //Return error if no bytes to be transferred.
  if(dwpendingdatacount==0)
    return Err_Invalid_ByteCount;

  //Check if card is not busy.Send dummy CMD52.
  //Read CCCR,register 00.
  dwRetVal=sdio_rdwr_direct(ncardno,0,0xF,&bData,pbcmdrespbuff,
                            FALSE,FALSE,0x02);
  //Check state of card prior to this cmd.
  if(((R5_RESPONSE*)pbcmdrespbuff)->Bits.IO_Current_state == TRN_STATE)
    return Err_Card_Busy;//Data lines engaged.

  //Check if card is deselected.
  if( ((R5_RESPONSE*)pbcmdrespbuff)->Bits.IO_Current_state == DIS_STATE)
  {
    //Send CMD7.
    if((dwRetVal=form_send_cmd(ncardno,CMD7,00,pbcmdrespbuff,1))
       !=SUCCESS)  return dwRetVal;
  }

  //Check if Ready flag for the function is set.
  if(!(bData & (1<<nfunctionno)) )
    return Err_Function_NotReady;

  //We want to resume the suspended command,which function
  //was executing.So,we will get EXx bit=1.User is supposed
  //to check RF flag and then,issue this command.
  //Here we need to go and starightway execute RESUME command.
        
  //Read FSx bit from CCCR.
  /*Form command:*/
  //Command is CMD52,but configure it alongwith registers
  //required for data command.
  //Form cmd reg.
  Form_SDIO_CmdReg(&tCmdInfo,ncardno,CMD52,nflags);
        
  //command argument.
  //Writing user function number at FSx in 0x0D register will 
  //resume the function if suspended.
  tRWdirectInfo.Bits.WrData = nfunctionno;
  tRWdirectInfo.Bits.RegAddress = 0x0D;//RegAddr.FSx bits to use.
  tRWdirectInfo.Bits.RAW_Flag = 1;
  tRWdirectInfo.Bits.FunctionNo = 0;//FunctionNo=0 for CCCR reg. ;
  tRWdirectInfo.Bits.RW_Flag = 1;

  tCmdInfo.dwCmdArg = tRWdirectInfo.dwReg;

  //Set Block size and unsigned char count.
  //For unsigned char mode transfer,it is as if a single block
  //transfer.So,it will be suspended,when whole data
  //transfer is completed.This means resume command is
  //applicable to Block mode transfer only.
  //If block size is 0,this is errorneous command.
  if((Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno])==0)
     return Err_Invalid_Command;    
  tCmdInfo.dwBlkSizeReg = 
         (unsigned long)(Card_Info[ncardno].SDIO_Fn_Blksize[nfunctionno]) ;
  tCmdInfo.dwByteCntReg= dwpendingdatacount;

  /*Send command:*/ //? Write command for IP ??
  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                            pberrrespbuff,pbdatabuff,
                            callback,//No callback pointer 
                            nflags,nnoofretries,
                            TRUE,TRUE,
                            ((nflags & 0x40) >>6) );// 0= user address space buffer. 
        

  if(dwRetVal != SUCCESS)
    return dwRetVal; 

  //Validate response.
  if((dwRetVal=Validate_Response(CMD52,pbcmdrespbuff,ncardno,
     DONOT_COMPARE_STATE)) != SUCCESS)  return dwRetVal; 

  //Check if resume started and card has more data to 
  //send/receive.Else card will resume,but will do nothing.
  //IP couldnot understand this and will wait for data 
  //transfer.
  //DF is Bit 7 in first unsigned char of response.
  if((*(pbcmdrespbuff) & 0x80)==00)
    return Err_NoData_FromCard;

  return dwRetVal;
}

//--------------------------------------------------------------
/*B.9.4.Form_SDIO_CmdReg(COMMAND_INFO *ptCmdInfo,int ncardno,
                         int nCMDIndex,int nflags)

  //Forms the command register contents for SDIO CMD52 and CMD53

  *Parameters:
         ptCmdInfo:(i/p):Command info structure pointer.
         ncardno:(i/p):Card no.
         nCMDIndex:(i/p):Command index. CMD52 or CMD53.
         nflags:(i/p):SDIO extended command options.
         nflags:(i/p):Option for errors to be responded & other 
                      conditions..
         Send_Init_Seq(bit0)=1; Send initialize sequence.;
         Wait_Prv_Data(bit1)=1; Wait till previous data 
                                transfer is over.
         Chk_Resp_CRC (bit2)=1; Check response CRC.
         Predef_Transfer(bit3)=1; Predefined data transfer.
                                Send cmd23 first.
         Handle_Data_Errors (bit4)=1; Handle data errors.
         Send_Abort (bit5)=1;
         Read_Write   (bit7)=1; Write operation. 0=Read.   

  *Flow:
*/
int Form_SDIO_CmdReg(COMMAND_INFO *ptCmdInfo,int ncardno,
                         int nCMDIndex,int nflags)
{
  //CMD53 is data command.CMD52 is ususally non data.
  //This function is called when Resume command is to be given
  //using CMD52.This is DATA OPERATION.

  //Command register parameters.
  ptCmdInfo->CmdReg.Bits.start_cmd             = 1 ;//31 Start
                                                   ;//30-22resvd. 
  ptCmdInfo->CmdReg.Bits.Update_clk_regs_only  = 0 ;//21 Normal
  ptCmdInfo->CmdReg.Bits.card_number           = ncardno;//20-16

  ptCmdInfo->CmdReg.Bits.send_initialization   = 0 ;//15 No init
  ptCmdInfo->CmdReg.Bits.stop_abort_cmd        = 0 ;//14 No stop    

  ptCmdInfo->CmdReg.Bits.wait_prvdata_complete =    //13 
                                      ((nflags >> 1) & 0x01);//bit1

  ptCmdInfo->CmdReg.Bits.auto_stop             = 0 ;//12 SDIO mode.
  ptCmdInfo->CmdReg.Bits.transfer_mode         = 0 ;//11 Block
  ptCmdInfo->CmdReg.Bits.read_write            =   //10  
                                      ((nflags >> 7)& 0x01);//bit7

  ptCmdInfo->CmdReg.Bits.data_expected         = 1 ;//9  Data cmd
 
  ptCmdInfo->CmdReg.Bits.check_response_crc    =    //8
                                      ((nflags >> 2) & 0x01);//bit2 ChkRsp CRC

  ptCmdInfo->CmdReg.Bits.response_length       = 0 ;//7  Short rsp 
  ptCmdInfo->CmdReg.Bits.response_expect       = 1 ;//6  Rsp expec.
  ptCmdInfo->CmdReg.Bits.cmd_index             = nCMDIndex;//5-0

  return SUCCESS;
}
//==============================================================
/*B.10.Card write protect/erase/lock     

      sd_mmc_wr_protect_carddata
      sd_mmc_set_clr_wrprot
      sd_mmc_get_wrprotect_status 
      sd_mmc_erase_data
      sd_mmc_card_lock
*/
//--------------------------------------------------------------
//**** Use of write protect regicter (WRTPRT) ??

/*B.10.1.Write protect the group.
   Card range is divided into number of groups.
   Length of each group is defined by CSD::WP_GRP_SIZE.
   Data address in this command is a group address in unsigned char units.
   Card ignores all LSBs below group size.

   This function will write protect or clear the group,
   starting from given address.
   
   sd_mmc_wr_protect_carddata(int ncardno,unsigned long dwaddress,unsigned char *pbrespbuff,
                       int fprotect)
  *Parameters:
          ncardno:(i/p):Card number.
          dwaddress:(i/p):Address from,where to start.
          pbrespbuff:(i/p):Response buffer.
          fprotect:(i/p):Protect or clear protection.
              =0 => clear protection.
              =1 => Implement protection. 

  *Return value : 
   0: for SUCCESS
   Err_WrgrpProtect_Not_Supported
   Err_Card_Busy

  *Flow :
   Sanity Check:
         -Check if (address) is within card size.
          Else return error.

   Validate_Command:
         -Check if card supports write protect group enable.
          CSD::WP_GRP_ENABLE.
          Else return error.

   Check_Card_Status: form_send_cmd(CMD13)
       Check response and if error,return to application.

   Select_Check_Card: form_send_cmd(CMD7)
       Check response and if error,return to application.

   Form command:
       -Select command index and add it in COMMAND_INFO
        structure.
         if(fprotect)
           Cmd_Index = CMD28 //Set Protect.
          else
           Cmd_Index = CMD29 //Clear Protect.

       -Set Command argument as "data address".
  
   sd_mmc_send_command:Send command for execution.This is 
         non data command.So,use form_send_cmd().

   Check response:
        -Check if response has any error.
         Call Validate_Response(Command,response,card_no)

       -If (Remained unsigned char count)
           repeat the loop from Form_Command.
       -Return.        
*/
int   sd_mmc_wr_protect_carddata(int ncardno,unsigned long dwaddress,unsigned char *pbrespbuff,
                          int fprotect)
{
  unsigned long dwRetVal,dwCmdIndex;
 
  //Check if card supports write protect group enable.
  //and address is within card range.
  if((dwRetVal = Validate_Command(ncardno,dwaddress,00,
     Wr_Protect_Data) ) != SUCCESS)
    return dwRetVal;

  //No need to mark set_of_commands as only one comamnd at
  //a time is in process.
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //Select command index.
  if(fprotect)
    dwCmdIndex = CMD28; //Set Protect.
  else
    dwCmdIndex = CMD29; //Clear Protect.


  //Wp_grp_size= size of write protected group in units of erase sector.
  //Erase sector size is size of erasable sector in terms of Write blocks.
  //eg WP_grp_size= m , Sector_size= n , Write_block length= 512 bytes.
  //This means, Sector_size = n x 512 bytes and 
  //            Wp_grp_size= m x Sector_size = m x n x 512 bytes.
 
   //Send command for execution.
   //Address is the address of the write protect group.
   if((dwRetVal = form_send_cmd(ncardno,dwCmdIndex,dwaddress,
      pbrespbuff,1))!=SUCCESS)
     return dwRetVal;

  //Validate response.
  dwRetVal=Validate_Response(dwCmdIndex,pbrespbuff,ncardno,
                             CARD_STATE_TRAN);

  return dwRetVal;     
}
//--------------------------------------------------------------
/*B.10.2.Set/Clear permanent write protect :Temporary/Permanent
  Setting bit 12 or 13 write protects or clears whole card.
  CSD::bit13 :permanently write protects.
  CSD::bit12 :Temporarily write protects.

  This requires sending 128bit data on data line.Read only 
  portion has to match with that part on card.Otherwise card
  will return error.

  sd_mmc_set_clr_wrprot(int ncardno,int ftemporary, int fset)

  *Parameters:
         ncardno   :(i/p):Card number.
         ftemporary:(i/p):=TRUE,for temporary wr_protect/clear
                           =FALSE,for permanent wr_protect/clear 
         fset      :(i/p):Set or clear. 1=set,  0=clear.

  *Return value : 
   0: for SUCCESS
   Err_CMD_Failed
   Err_Card_Busy

  *Flow :
   Sanity Check:
         -Check if card no. is valid.
          Else return error.

   Validate_Command:

   Allocate_Resp_Buffer:
       -Allocate and set reponse buffer.(ptResponse_info)
        to store response of the command.

   Mark set_of_commands :
       -ptAppInfo->set_of_commands=1;//Set of commands to follow
   
   Check_Card_Status: form_send_cmd(CMD13)
       Check response and if error,return to application.

   Select_Card: form_send_cmd(CMD7)
       Check response and if error,return to application.

   Form command:
       -Read CSD contents of the card from card info.
       -Set write protection bit.
         if(ftemporary)
           set bit12
         else
           set bit13
       -Select command index.
       -Copy 128 bit data in data buffer.This buffer
        will be copied in FIFO,whenever command will be accepted
        for processing.

   sd_mmc_send_command:
       -Command is formed,now add it in queue for execution.
        Call Add_command_in_queue(COMMAND_INFO *ptCmdParams,
                                 Application_Info *ptAppInfo)
       -Sleep (&App_info->Wqn)
       //Command will be processed when status poller calls
       //the switch to load next command.Sleep till command
       //is accepted.

   Read response.:When command is executed,response will be
        copied is response buffer.

   Check response:
        -Check if response has any error.
         Call Validate_Response(Command,response,card_no)

        -Return.        
*/
int sd_mmc_set_clr_wrprot(int ncardno,int ftemporary, int fset)
{
  unsigned long dwRetVal,dwR1Response,dwOrgBlkLength;
  COMMAND_INFO  tCmdInfo={0x00};
  R2_RESPONSE  tR2;
  int CRC7=0;

  //Check if card no. is valid.
  if((dwRetVal = Validate_Command(ncardno,00,00,
     General_cmd) ) != SUCCESS)
    return dwRetVal;

  //No need to mark set_of_commands as only one comamnd at
  //a time is in process.
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //Check current block length.
  dwOrgBlkLength = Card_Info[ncardno].Card_Write_BlkSize;

  //Card expects 128 bit data.It is not partial write.
  if(dwOrgBlkLength != 16)
     dwRetVal= SetBlockLength(ncardno,16,WRITEFLG,1);

  if(dwRetVal!=SUCCESS) return dwRetVal;

  //Form command:Use CMD27 with CSD data on data lines.
  Form_CSD_Write_Cmd(&tCmdInfo,ncardno);

  //Get CSD data in temporary buffer.
  memset(tR2.bResp,0,sizeof(tR2));

  if(Card_Info[ncardno].Card_type==MMC)
     Repack_csd(tR2.bResp,&Card_Info[ncardno].CSD,0);
  else
     Repack_csd(tR2.bResp,&Card_Info[ncardno].CSD,1);

  if(fset)
  {
    if(ftemporary)
      tR2.bResp[1] |=0x10;
    else
      tR2.bResp[1] |=0x20;
  }
  else
  {
    if(ftemporary)
      tR2.bResp[1] &= 0xEF;  
    else
      tR2.bResp[1] &= 0xDF;
  }

  //Change endian.
  ChangeEndian (tR2.bResp,sizeof(tR2.bResp));

  //Calculate CRC for csd data of 120 bit. 
  CRC7 = calcCrc7(tR2.bResp, 120) ;

  tR2.bResp[15] |= (CRC7 <<1);
  tR2.bResp[15] |= 1;

  //128 bit CSD data will be copied in FIFO,in 
  //Send_Cmd_to_Host().
  //No need of retries,eventhough data command.
  dwRetVal=Send_cmd_to_host(&tCmdInfo,(unsigned char*)(&dwR1Response),NULL,
                            tR2.bResp,
                            NULL,//No callback pointer
                            00,00,//No flags/retries.
                            TRUE,TRUE ,//DATA,write
                            TRUE );// 1= Kernel address space buffer. 
 
  if(dwRetVal == SUCCESS) 
  {
    //Validate response.Card state prior to cmd.should be "Transfer".
    dwRetVal=Validate_Response(CMD27,(unsigned char*)(&dwR1Response),ncardno,
                               CARD_STATE_TRAN);

    //Set write protection bit in card_info..
    if(ftemporary)
      Card_Info[ncardno].CSD.Fields.tmp_write_protect=(fset) ?1:0;
    else
      Card_Info[ncardno].CSD.Fields.perm_write_protect=(fset) ?1:0;
  }
  else
    dwRetVal=Err_CMD_Failed; 

  //Re-set the original block length.
  if(dwOrgBlkLength!=16)
    SetBlockLength(ncardno,dwOrgBlkLength,WRITEFLG,1);

  return dwRetVal;     
}

//--------------------------------------------------------------
/*    Form_CSD_Write_Cmd:Fills the command info structure for 
  CMD27.This command programs CSD register.

*/
int  Form_CSD_Write_Cmd(COMMAND_INFO *ptCmdInfo,int ncardno)
{
  //Set command register.

  ptCmdInfo->CmdReg.Bits.start_cmd             = 1 ;//31 Start
  ptCmdInfo->CmdReg.Bits.Update_clk_regs_only  = 0 ;//21 Normal cmd

  //card no.      
  ptCmdInfo->CmdReg.Bits.card_number           = ncardno;

  //No initialization,wait for prev data,check resp.CRC
  ptCmdInfo->CmdReg.Bits.send_initialization   = 0;
  ptCmdInfo->CmdReg.Bits.stop_abort_cmd        = 0 ;//14 No stop    
  ptCmdInfo->CmdReg.Bits.wait_prvdata_complete = 1;

  //Memory transfer for SDIO card.To be replaced as AUTO_STOP bit.
  //No AUTO_STOP.
  ptCmdInfo->CmdReg.Bits.auto_stop = 0;

  ptCmdInfo->CmdReg.Bits.transfer_mode         = 0; 
  ptCmdInfo->CmdReg.Bits.read_write            = 1;
  //Data command,write command,CMD27
  ptCmdInfo->CmdReg.Bits.data_expected         = 1;
  ptCmdInfo->CmdReg.Bits.check_response_crc    = 1;
  //Short response,resp.expected,block transfer      
  ptCmdInfo->CmdReg.Bits.response_length       = 0;
  ptCmdInfo->CmdReg.Bits.response_expect       = 1;

  ptCmdInfo->CmdReg.Bits.cmd_index             = CMD27;

  //command argument not required.
  ptCmdInfo->dwCmdArg                  = 00;

  //Set unsigned char count register value. CSD is 128 bits wide.
  //Set count in bytes.
  ptCmdInfo->dwByteCntReg               = 128/8;
  //This command is write block command.So,set block size same
  //as in card-CSD.
  ptCmdInfo->dwBlkSizeReg = Card_Info[ncardno].Card_Write_BlkSize;

  return SUCCESS;
}

//--------------------------------------------------------------
/*Default_CmdReg_forDataCmd:Writes default values for command 
  register.
*/
int Default_CmdReg_forDataCmd(COMMAND_INFO *ptCmdInfo,
                              int ncardno,int nCmdIndex,
                              int fwrite)
{
  ptCmdInfo->CmdReg.Bits.start_cmd             = 1 ;//31 Start
                                                    //30-22resvd. 
  ptCmdInfo->CmdReg.Bits.Update_clk_regs_only  = 0 ;//21 Normal cmd
  ptCmdInfo->CmdReg.Bits.card_number           = ncardno;//20-16
  ptCmdInfo->CmdReg.Bits.send_initialization   = 0 ;//15 No init
  ptCmdInfo->CmdReg.Bits.stop_abort_cmd        = 0 ;//14 No stop    
  ptCmdInfo->CmdReg.Bits.wait_prvdata_complete = 1 ;//13 Wait 

  //No AUTO STOP
  ptCmdInfo->CmdReg.Bits.auto_stop             = 0 ;//12 auto_stop.

  ptCmdInfo->CmdReg.Bits.transfer_mode         = 0 ;//11 Block
  ptCmdInfo->CmdReg.Bits.read_write            = fwrite;//rd/wr
  ptCmdInfo->CmdReg.Bits.data_expected         = 1 ;//9  Data cmd
  ptCmdInfo->CmdReg.Bits.check_response_crc    = 1 ;//8  ChkRsp CRC
  ptCmdInfo->CmdReg.Bits.response_length       = 0 ;//7  Short rsp 
  ptCmdInfo->CmdReg.Bits.response_expect       = 1 ;//6  Rsp expec.
  ptCmdInfo->CmdReg.Bits.cmd_index             = nCmdIndex;//5-0

  return SUCCESS;
}
//--------------------------------------------------------------
/*B.10.3.Read write_protect status:
  Reads write protection status of the 32 groups starting
  from the specified address.Status is returned on data line
  ie in FIFO.

  sd_mmc_get_wrprotect_status (int ncardno, unsigned long dwaddress, 
                        unsigned long *pdwstatus)

  *Called From:FSD
  *Parameters:
         ncardno    :(i/p):Card number.
         dwaddress  :(i/p):Starting address from where to read.
         pdwstatus  :(i/p):Pointer where unsigned long data read from
                           FIFO is to be stored. 

  *Return value : 
   0: for SUCCESS
   -EINVAL:DOWRD pointer to return status,is NULL.
   Err_Card_Busy
   Err_CMD_Failed

  *Flow :
   Sanity Check:
         -Check if card no. is valid.
          Else return error.
         -Check if pdwstatus is NULL.If yes,return error.

   Validate_Command:

   Allocate_Resp_Buffer:
       -Allocate and set reponse buffer.(ptResponse_info)
        to store response of the command.

   Mark set_of_commands :
       -ptAppInfo->set_of_commands=1;//Set of commands to follow
   
   Check_Card_Status: form_send_cmd(CMD13)
       Check response and if error,return to application.

   Select_Card: form_send_cmd(CMD7)
       Check response and if error,return to application.

   Form command:
       -Copy address supplied,as a command argument.
       -Copy pdwstatus pointer as a data buffer pointer.
        After data done,data will be copied in this data buffer.

   sd_mmc_send_command:
       -Command is formed,now add it in queue for execution.
        Call Add_command_in_queue(COMMAND_INFO *ptCmdParams,
                                 Application_Info *ptAppInfo)
       -Sleep (&App_info->Wqn)
       //Command will be processed when status poller calls
       //the switch to load next command.Sleep till command
       //is accepted.

   Read response.:When command is executed,response will be
        copied is response buffer.

   Check response:
        -Check if response has any error.
         Call Validate_Response(Command,response,card_no)

        -Return.        
*/
int sd_mmc_get_wrprotect_status (int ncardno, unsigned long dwaddress, 
                          unsigned long *pdwstatus)
{
  unsigned long dwRetVal,dwR1Response,dwOrgBlkLength,dwRet;
  COMMAND_INFO  tCmdInfo={0x00};

  //Check if card supports write protect group enable.
  if((dwRetVal = Validate_Command(ncardno,dwaddress,00,
     Rd_Protect_Data) ) != SUCCESS)  return dwRetVal;
    
  if(pdwstatus==NULL)
    return -EINVAL;

  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  //This is a block read command for 4 bytes.So,set block length
  //to 4 and after completion of this,reset it to original block length.
  dwOrgBlkLength = Card_Info[ncardno].Card_Read_BlkSize;
  //Set block length for partial reading.
  //Card sends 32 bit data.It is not partial reading.
  if(dwOrgBlkLength != 4)
     dwRetVal= SetBlockLength(ncardno,4,READFLG,1);

  //printk("SetBlockLength returns %lx \n",dwRetVal);
  if(dwRetVal!=SUCCESS) return dwRetVal;

  //Form command: CMD30.
  //Default command register parameters for data command.
  //Start+no clk update+No init+ no stop/abort+Wait for prev.+
  //SDIO mem mode+blk xfer+read cmd+data+chk rspCRC+Short rsp.

  Default_CmdReg_forDataCmd(&tCmdInfo,ncardno, CMD30,READFLG);
  //command sepcific parameters.
  tCmdInfo.dwCmdArg                   = dwaddress;
  tCmdInfo.dwByteCntReg               = 4;

  //This command is read block command.So,set block size same
  //as in card-CSD.
  tCmdInfo.dwBlkSizeReg = Card_Info[ncardno].Card_Read_BlkSize;

  //No retry.No error response buffer.
  //This comamnd will store status unsigned long in pdwstatus
  //on data done. Data command,read.
  dwRetVal=Send_cmd_to_host(&tCmdInfo,(unsigned char*)(&dwR1Response),NULL,
                            (unsigned char*)pdwstatus,
                             NULL,//No callback pointer
                             00,00,TRUE,FALSE,//No flags/retries
                             FALSE );// 0= user address space buffer.
  if(dwRetVal != SUCCESS)
    //return Err_CMD_Failed; 
    dwRetVal = Err_CMD_Failed; 
  else
  {
    //Validate response.Card state prior to cmd.should be stdby.
    dwRetVal=Validate_Response(CMD30,(unsigned char*)(&dwR1Response),ncardno,
                               CARD_STATE_TRAN);
  }
  //printk("dwRetVal before calling Setblocklength = %lx \n",dwRetVal);
  if(dwOrgBlkLength != 4)
     dwRet = SetBlockLength(ncardno,dwOrgBlkLength,READFLG,1);
  //printk("dwRetVal after calling Setblocklength = %lx \n",dwRet);

  return dwRetVal;     
}
//--------------------------------------------------------------
/*B.10.4.Erase_Card_data:Erases card data within supplied range.
  Data is erased in contiguous erase group units which are
  calculated from Erase_GRP_Size and Erase_GRP_Mult values.
  
  Note that card ignores LSBs below Erase Group unit size
  ie it rounds off erase address to Erase group unit size.

  sd_mmc_erase_data(unsigned char *pbrespbuff,int ncardno,
                  unsigned long dwerasestart,unsigned long dweraseend)

  *Called From:FSD
  *Parameters:
         pbrespbuff:(i/p):Response buffer.
         ncardno:(i/p):Card number.
         dwerasestart:(i/p):Erase start address.
         dweraseend:(i/p):Erase end address.

  *Return value : 
   0: for SUCCESS
   -EINVAL:response buffer is NULL.
   Err_Card_Busy
   Err_State_Mismatch

  *Flow:
       -Check that card number is valid.
       -Check that dwerasestart and dweraseend are within
        card range.

       -Check card status.If busy,return error.
       -Select card using CMD7.

       -//Send Erase group start command.Add it in queue.
        dwcmdarg=dwerasestart
          Call form_send_cmd(ptAppInfo,CMD35,ncardno,
                            dwcmdarg)

       -//Send Erase group End command.Add it in queue.
        dwcmdarg=dweraseend
          Call form_send_cmd(ptAppInfo,CMD36,ncardno,
                            dwcmdarg)
       -Start erase command.
          Call form_send_cmd(ptAppInfo,CMD38,ncardno,
                            dwcmdarg)
        
       -Validate response.
        Check for error such as WP_ERASE_SKIP etc.
       -Return.
*/
int   sd_mmc_erase_data(unsigned char *pbrespbuff,int ncardno,
                  unsigned long dwerasestart,unsigned long dweraseend)
{
  unsigned long dwRetVal;
  int nCMDindex;

  //printk("Entered in Erase_card_data. \n");
  /*Sanity check*/
  //Check if response buffer is NULL.
  if(pbrespbuff==NULL) return -EINVAL;

  //Check if card no.is valid and if address lies within card
  //range.
  if((dwRetVal = Validate_Command(ncardno,dwerasestart,
     dweraseend,Erase_Data)) != SUCCESS)
    return dwRetVal;

  //printk("Valid command.Going for CMD13. \n");

  /*Change card status to transfer.*/
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  /*Set erase start and end address.*/
  //For SD, erase group is of size wr_block_length while
  //for MMC,erase group size is defined in CSD.
  //Erase group START command index:  CMD32 for SD, CMD35 for MMC
  //Erase group END   command index:  CMD33 for SD, CMD36 for MMC
  //Erase             command index:  CMD38 for SD, CMD38for MMC

  //Send Erase group start command.
  if(Card_Info[ncardno].Card_type==MMC)
     nCMDindex = CMD35;
  else
     nCMDindex = CMD32;
        
  if((dwRetVal = form_send_cmd(ncardno, nCMDindex,dwerasestart,
     pbrespbuff,1)) !=SUCCESS)
    return dwRetVal;

  //If current state of card is not Transfer,return error.
  if(Validate_Response( nCMDindex ,pbrespbuff,ncardno,CARD_STATE_TRAN)
      == Err_State_Mismatch )
    return Err_State_Mismatch;
        
  //Send Erase group End command.
  if(Card_Info[ncardno].Card_type==MMC)
     nCMDindex = CMD36;
  else
     nCMDindex = CMD33;
        
  if((dwRetVal = form_send_cmd(ncardno, nCMDindex,dweraseend,
     pbrespbuff,1)) !=SUCCESS)
    return dwRetVal;

  //If current state of card is not Transfer,return error.
  if( Validate_Response( nCMDindex,pbrespbuff,ncardno,CARD_STATE_TRAN)
      == Err_State_Mismatch )
    return Err_State_Mismatch;
        
  /*Start erase command.*/
  if((dwRetVal = form_send_cmd(ncardno,CMD38,00,pbrespbuff,1))
     !=SUCCESS)  return dwRetVal;
        
  //Validate response.
  //If current state of card is not Transfer,return error.
  dwRetVal = Validate_Response(CMD38,pbrespbuff,ncardno,
                               CARD_STATE_PRG);
  return dwRetVal;
}
//--------------------------------------------------------------
/*B.10.5.sd_mmc_card_lock_Unlock_Cmd:
   Card Password protection.                        
   -Lock the card.
   -Unlock the card.
   -Change the password.
   -Clear the password.

   Donot store current password in driver as this harms the 
   security.Let user application remember this.
   
   MMC::Password register is 128 bit wide meaning password 
   size should be less that (128/8=16 bytes).
   In case of password change,old password has to be passed.

   Performs card lock/unlock related functions ie
   Set_password,clear password,lock/unlock card and forced
   erase.

   sd_mmc_card_lock(int ncardno,card_lock_info *ptcardlockinfo,
                    unsigned char *pbrespbuff)

  *Called from : FSD

  *Parameters  :
         ncardno:(i/p) ; Card whose password to be set.
         ptcardlockinfo->bCommandMode:(i/p):Command mode
                         //0 =Unlock card     ;1=Set_Pwd; 
                         //2 =CLR_PWD         ;4=Lock card;
                         //5 =Lock and set_pwd;8=Erase card
         ptcardlockinfo->bOldPwdLength:(i/p):Old password length
                                             0 if not required.
         ptcardlockinfo->pbOldPassword:(i/p):Old password unsigned char 
                                             array ptr.
         ptcardlockinfo->bNewPwdLength:(i/p):New password length
         ptcardlockinfo->pbNewPassword:(i/p):New password unsigned char 
                                             array ptr.
         pbrespbuff:(i/p):Response buffer.

  *Return value : 
   0: for SUCCESS
   -EINVAL:Pointer to response buffer and/or pointer to 
           card_lock_info are NULL.
   Err_PWDLEN_Exceeds:Password length exceeds 16.
   Err_Card_Busy
   Err_CMD_Failed

  *Flow:

   Sanity check:
        -Check if card no.is valid.Else return error.

   Validate_command:(for given card):
        -If ERASE comamnd,is given,skip following checks as only
         command unsigned char will be sent.
        -Check if array pointer pbNewPassword is not NULL.
        -Check that bOldPwdLength and bNewPwdLength is within
         0 to 16.
        -If (bOldPwdLength > 0),check that array pointer 
         pbOldPassword is not NULL.Else return error.

   Allocate_Resp_Buffer:
        -Allocate response buffer to store response for
         the command.

   Check_Card_Status: form_send_cmd(CMD13)
        -Check response and if error,return to application.

   Select_Check_Card: form_send_cmd(CMD7)
        -Check response and if error,return to application.

   Send_Supportive_Cmd:(if any)
        -Calculate the command block length.
         = (Mode unsigned char)1 + (Pwd_Len)1 + bOldPwdLength + 
           bNewPwdLength
         = bOldPwdLength + bNewPwdLength + 2
        -Set block length=Command block length. (Send CMD16) 
        -Check response and if error,return to application.

   Form command:
        -Set command mode unsigned char as per type of command.
         eg Set_pwd,clr_pwd,lock/unlock etc.
        -Copy command unsigned char in data buffer.
        -Copy PWD_len unsigned char(nPasswordLength) in data buffer.
         **Note:For PWD replacement,this field should contain
         length for old_new passwords.
        -Copy old password in data buffer.
        -Copy new password in data buffer.        
        
   Send command:
        -Send CMD42. form_send_cmd(CMD42)
         During transfer of this,data block is copied in FIFO
         and sent on data lines alongwith command on cmd line.
   Read response.:
        -response is copied in Response buffer,after command is 
         sent.
   Check response:
        -Validate response.If error,accordingly set error code 
         and return.
*/
int  sd_mmc_card_lock(int ncardno,card_lock_info *ptcardlockinfo,
               unsigned char *pbrespbuff)
{
  unsigned char         bDataBuff[34+1];//Max.34 unsigned char width of command block.
  COMMAND_INFO tCmdInfo={0x00};
  unsigned long        dwRetVal,dwOrgBlkLength,dwRet;
  int          nBuffposition; 
  unsigned char         bBlkLength, bLockCommand=0;
  unsigned char cValidOldpwLength=0,cValidNewpwLength=0;

  /*Sanity check:*/
  //Check if card no.is valid.
  if( (dwRetVal = Validate_Command(ncardno,00,00,
     General_cmd) ) != SUCCESS) return dwRetVal;

  if(pbrespbuff==NULL || ptcardlockinfo==NULL)
    return -EINVAL;

  //Check if any invalid combination of command bits.
  //First 3 cases are to maintain compatibility between type1 
  //and type2 cards as per HS-SD specifications 1.10.
  bLockCommand = (ptcardlockinfo->bCommandMode &0xf);   
  if( (bLockCommand==3) || (bLockCommand==6) || (bLockCommand==7)||
      (bLockCommand > 8) ) return Err_Invalid_Arg;

  //If ERASE comamnd,is given,skip following checks as only
  //command unsigned char will be sent.
  if(bLockCommand!=8)//Value 8 ERASE
  {
    //New password buffer :to use only for password change or set.
    //Old password buffer : holds current password.
    //buffer.
    if(Card_Info[ncardno].Card_Attrib_Info.Bits.PWD_Set)
    {
      //If password is set, only change of password is allowed.
      //Current password is required.
      if((ptcardlockinfo->pbOldPassword==NULL) ||
         (ptcardlockinfo->bOldPwdLength==0))
        return Err_Invalid_Arg;

      //Old and new password length should be upto 16 bytes each.
      if(ptcardlockinfo->bOldPwdLength>16)
        return Err_PWDLEN_Exceeds;

      cValidOldpwLength = 1;
      cValidNewpwLength = 0;
     
      //For change password,check if new password is provided.
      if(bLockCommand & 1) 
      { 
        if((ptcardlockinfo->pbNewPassword==NULL) ||
           (ptcardlockinfo->bNewPwdLength==0))
          return Err_Invalid_Arg;
        //Old and new password length should be upto 16 bytes each.
        if(ptcardlockinfo->bNewPwdLength > 16)
          return Err_PWDLEN_Exceeds;

        cValidNewpwLength = 1;
      }
    }
    else
    {
      //If password is NOT set, new pasword is to be set.So, there is
      //no current password and hence, old passowrd buffer should be NULL.
      if(ptcardlockinfo->pbOldPassword!=NULL)
        return Err_Invalid_Arg;
      //For setting password,NewPassword should be used.
      //For all other modes,password will be given in oldpassword buffer.
      if((ptcardlockinfo->pbNewPassword==NULL) ||
         (ptcardlockinfo->bNewPwdLength==0))
        return -EINVAL;
      //Old and new password length should be upto 16 bytes each.
      if(ptcardlockinfo->bNewPwdLength>16)
        return Err_PWDLEN_Exceeds;

      cValidOldpwLength = 0;
      cValidNewpwLength = 1;
    }
  }//end of if forced erase.
  else
  {
    //Check if card is permanently write protected.Read CSD.
    //If permanent write protect is set,return error.
    if(Card_Info[ncardno].CSD.Fields.perm_write_protect)
      return Err_Invalid_Command;
  }//End of if(Force Erase).

  /*Change card status to transfer.*/
  //Check if card is transfer state.
  if((dwRetVal = Check_TranState(ncardno))!=SUCCESS)  
    return dwRetVal;

  /*Send supportive command */
  /*Form command*/
  //Set command mode unsigned char as per type of command.
  //For ERASE,only ERASE bit should be set.else,erase request is rejected.
  bDataBuff[0]=bLockCommand;

  //For erase,only data unsigned char indicating ERASE mode is to be sent.
  if(bLockCommand!=8)//Value 8 ERASE
  {
    //Calculate the command block length. oldpw+newpw+2.
    //Pwd length at bDataBuff[1] is less by 2.
    bBlkLength = 2;
 
    nBuffposition = 2;
    //Copy old password in data buffer.
    if(cValidOldpwLength)
    {
      if (copy_from_user(&bDataBuff[nBuffposition],ptcardlockinfo->pbOldPassword,
          ptcardlockinfo->bOldPwdLength)) return -EFAULT;

      //Copy new password in data buffer.
      nBuffposition += ptcardlockinfo->bOldPwdLength;

      bBlkLength += ptcardlockinfo->bOldPwdLength ; 
    }
    //printk("Old password is \n "); 
    //for(i=0;i<ptcardlockinfo->bOldPwdLength;i++)  
    //printk("%c ", bDataBuff[nBuffposition]);         
    //printk(" \n "); 

    if(cValidNewpwLength)
    {
      if (copy_from_user(&bDataBuff[nBuffposition],ptcardlockinfo->pbNewPassword,
          ptcardlockinfo->bNewPwdLength)) return -EFAULT;

      bBlkLength += ptcardlockinfo->bNewPwdLength ;
    }

    //printk("New password is \n "); 
    //for(i=0;i<ptcardlockinfo->bNewPwdLength;i++)  
    //printk("%c ", bDataBuff[nBuffposition+i]);         
    //printk(" \n "); 

    //Block length should be oldpwdlength+newpwdlength.
    bDataBuff[1]=bBlkLength-2;
  }
  else
    //For erase,only data unsigned char indicating ERASE mode is to be sent.
    bBlkLength = 1;

  //Set command related registers.
  //Start+no clk update+No init+ no stop/abort+Wait for prev.+
  //SDIO mem mode+blk xfer+read cmd+data+chk rspCRC+Short rsp.
  Default_CmdReg_forDataCmd(&tCmdInfo,ncardno, CMD42,WRITEFLG);

  //command argument not required.
  tCmdInfo.dwCmdArg                   = 00;
  tCmdInfo.dwByteCntReg               =(unsigned long)bBlkLength;
  tCmdInfo.dwBlkSizeReg               =(unsigned long)bBlkLength;

    
  /*Send command: CMD42*/
  //Before sending this command,set block length equal to data size.
  // **** If not set,data will be passed in multiples of block
  // **** size as in CSD.
  dwOrgBlkLength = Card_Info[ncardno].Card_Write_BlkSize;
  if(dwOrgBlkLength != bBlkLength)
    //For lock commands,skip partial length check. 
    dwRetVal= SetBlockLength(ncardno,bBlkLength,WRITEFLG,1);

  if(dwRetVal!=SUCCESS) return dwRetVal;

  //No retry.No error response buffer. Data cmd,write
  dwRetVal=Send_cmd_to_host(&tCmdInfo,pbrespbuff,NULL,
                            bDataBuff,
                            NULL,//No callback pointer
                            00,00,TRUE,TRUE,//No flags/retries
                            TRUE );// 1= Kernel address space buffer.

  if(dwRetVal != SUCCESS)
     dwRetVal =Err_CMD_Failed;  
  else
  {
    //Validate response.Card state prior to cmd.should be stdby.
    dwRetVal=Validate_Response(CMD42,pbrespbuff,ncardno,
                               CARD_STATE_TRAN);

    //Response to CMD42 is R1.For COM_CRC and Illegal command errors,
    //response timeout will occur. No need to send CMD13 again.
    if(dwRetVal == SUCCESS)
    {
      //Set card attributes,if no error reported in response. 
      Set_CardLock_Attributes(ncardno,bLockCommand);
      //For Force Erase,to maintain compatibility with type2 SD 
      //cards,clear TMP_WR_PRT bit.
      if((bLockCommand==8) && 
         (Card_Info[ncardno].CSD.Fields.tmp_write_protect)) 
      {
        dwRetVal = sd_mmc_set_clr_wrprot(ncardno,1,0);
        if(dwRetVal==SUCCESS)
           Card_Info[ncardno].CSD.Fields.tmp_write_protect=0;
        else
           dwRetVal =Err_TmpWrProt_Failed;  
      }
    }
  }//end of if SUCCESS.

  //printk("Respbuff contents for CMD42 =  %lx \n",*((unsigned long*)pbrespbuff) );

  //printk("dwRet for CMD13 after CMD42 = %lx \n",dwRet);

  //Re-set block length to original.Donot use dwRetVal
  //as if this function fails,whatever block size is set
  //will be used for next data transfer.
  //For lock commands,skip partial length check. 
  if(dwOrgBlkLength != bBlkLength)
     dwRet = SetBlockLength(ncardno,dwOrgBlkLength,WRITEFLG,1);
  //printk("Setblocklength returns %lx \n",dwRet);

  //printk("Respbuff contents for CMD13 =  %lx \n",*((unsigned long*)pbrespbuff) );
  //printk("Pwd.Data after unsigned char swap is = %lx \n",*((unsigned long*)(bDataBuff)) );

  return dwRetVal;     
}

void Set_CardLock_Attributes(int ncardno,unsigned char bLockCmd)
{
  switch(bLockCmd)
  {
    case  0://Unlock
       Card_Info[ncardno].Card_Attrib_Info.Bits.Card_Locked=0;
       break;

    case  1://Set PWD.
       Card_Info[ncardno].Card_Attrib_Info.Bits.PWD_Set=1;
       Card_Info[ncardno].Card_Attrib_Info.Bits.Card_Locked=0;
       break;

    case  2:
       Card_Info[ncardno].Card_Attrib_Info.Bits.PWD_Set=0;
       Card_Info[ncardno].Card_Attrib_Info.Bits.Card_Locked=0;
       break;

    case  4:
       Card_Info[ncardno].Card_Attrib_Info.Bits.Card_Locked=1;
       break;

    case  5:
       Card_Info[ncardno].Card_Attrib_Info.Bits.Card_Locked=1;
       Card_Info[ncardno].Card_Attrib_Info.Bits.PWD_Set=1;
       break;

    case  8://Forced erase.
       Card_Info[ncardno].Card_Attrib_Info.Bits.PWD_Set=0;
       break;

  }//end of switch.
  return;
}
//==============================================================
/*B.11.Interrupt handlers :

      Mask_Interrupt
      ISR callback
      SDIO_ISR
*/
//--------------------------------------------------------------

/*B.11.1.Enable IRQ : Enables and sets masks for given IRQs.
  User can select the interrupts to be masked or unmasked by
  setting 1 at the appropriate bit position.Flag passed will
  decide if selected interrupts to be masked or unmasked.


  Mask_Interrupt(unsigned long dwIntrMask,int fsetMask)

  *Called from : BSD::sd_mmc_sdio_initialise()

  *Paramemters :
         dwIntrMask:(i/p):Interrupt mask register value.
                          
         fsetMask:(i/p):1=Mask and 0=Unmask.

  *Return value : 
   0: for SUCCESS
   Err_IntrMask_Not_Allowed

  *Flow :
        -Call Host::Set_IP_Parameters using SET_INTR_MASK
         or CLEAR_INTR_MASK.
        -Return;
*/
int   Mask_Interrupt(unsigned long dwIntrMask,int fsetMask)
{ 
  //Donot allow mask/unmask interrupts Response error(bit1),
  //Response CRC(bit6),Response timeout (bit8).
  if(dwIntrMask & 0x0000142)
    return Err_IntrMask_Not_Allowed;

  if(fsetMask)
     set_ip_parameters(SET_INTR_MASK, &dwIntrMask) ;
  else
     set_ip_parameters(CLEAR_INTR_MASK, &dwIntrMask) ;

  return SUCCESS;
}

void set_sdio_int(int ncardno, int flag)
{
    unsigned long dwData;

    if(ncardno != 0){
        printk("Warning! cardno must be zero, set sdio int fail!\n");
        return;
    }
    Read_Write_IPreg(INTMSK_off, &dwData, 0);
    if(flag)
       dwData |= 0x00010000;
    else
       dwData &= 0xFFFEFFFF; 
    Read_Write_IPreg(INTMSK_off, &dwData, 1);
}
EXPORT_SYMBOL(set_sdio_int);
//--------------------------------------------------------------
/*B.11.2.ISR callback:Called by Host driver ISR,on any interrupt.
  This ISR callback will just read the interrupt status 
  register and store it locally.Status poller thread will
  take care of the interrupts raised.

  ISR_Callback()
  *Called From: Hostriver::ISR().
  *Parameters:Nothing.

  *Flow:
        -Read interrupt status register.
         Call Read_Write_IPreg(Raw_Intr_Status,&dwIntrStatus,
                               TRUE);
        -Return. 
*/
void ISR_Callback(void)
{
   unsigned long dwData;
//  unsigned long rawintrstatus;
    G_dwIntrStatus=0;
   //printk("enter ISR_Callback!\n");
  //Read interrupt status register first and then disable global intr..
  //Instead of RINTSTS,read MINTSTS so that only interrupts enabled
  //by user will be handled.To clear these bits,write 1 in RINTSTS.
  Read_Write_IPreg(MINTSTS_off,(DWORD *)&G_dwIntrStatus, READFLG);
 // Read_Write_IPreg(RINTSTS_off,&rawintrstatus, READFLG);   
  HI_TRACE(3,"******current int signal is %x\n",(int)G_dwIntrStatus);
  //printk("******current int signal is %lx\n",G_dwIntrStatus);
//  printk("******current raw int signal is %lx\n",rawintrstatus);
  //Disable global intr.flag.  CNTRL_reg::bit4 = 0.
 // Set_IP_Parameters(CLEAR_CONTROLREG_BIT,&dwBitno);     //disable irq

  //printk("ISR_Callback:Calling sd_mmc_event_handler with %lx \n",G_dwIntrStatus);
  
    //here mask the INT status, weimx add on 20070915 
    dwData = 0x0;
      Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);
      
  //send_cmd_to_host will return this error.
  //Check if any of the response error bits is set
  //bits 1,6,8,13 ie 0x00002142. .Return response error only.    

  /*    weimx modify the below line code to resolve the problem:
      two interrupts come here after one send_cmd_to_host    */
  tCurrCmdInfo.dwCurrErrorSts  |= (G_dwIntrStatus & 0x00002142);        
  //Call event handler.
  //Check if any interrupt bit is set.
  if(G_dwIntrStatus & 0x0000ffff) 
  {
    if(G_CMD40_In_Process)
        Set_Interrupt_Mode(NULL,G_dwIntrStatus);
    else
       sd_mmc_event_Handler((void *)&G_dwIntrStatus);
  }

  //If response timeout,data command is not sent.
  if(G_dwIntrStatus & 0x0000100) 
     tDatCmdInfo.fCmdInProcess = 0;

  //Clear RINTSTS. All interrupts set,are being processed.
  //If during card_detect_handler,if another card_detect
  //intr.comes,then ?? not to clear RINTSTS bit?
  //This will clear MINTSTS bits also.

  if(G_dwIntrStatus & 0xFFFF0000)
  {
    if(func[4]!=NULL)
    {
        //printk("call wifi hookfunc!\n");
        func[4]();
    }
  }
  
    Read_Write_IPreg(RINTSTS_off,(DWORD *)&G_dwIntrStatus,WRITEFLG);
    G_dwIntrStatus=00;

    //here open the INT mask status ,weimx modify on 20070915
    //Enable global interrupt.  CNTRL_reg::bit4 = 1.
    //set_ip_parameters(SET_CONTROLREG_BIT,&dwBitno);   //enable irq
    dwData = 0x01ffcf;
    Read_Write_IPreg(INTMSK_off,&dwData,WRITEFLG);
    
    //For card enumeration,do it outside ISR to avoid
    //nestinf of intrs.Clear this flag first before
    //calling card_detect_handler.
#ifndef _GPIO_CARD_DETECT_            //not use gpio detect card 
    if(G_PENDING_CARD_ENUM==TRUE)
    {
        G_PENDING_CARD_ENUM=FALSE;
        printk("handle insert or remove card!\n");
        wake_up_interruptible(&ksdmmc_wait);
    }
#endif 

    return; 
}


//--------------------------------------------------------------
/*B.11.3.SDIO_ISR:Interrupt handler for SDIO interrupts.
  Calls ISR callbacks respective to SDIO card which has
  generated interrupt.
  These ISrs are registered by client of BSD.(eg File system Drv)

  SDIO_Intr_Handler(unsigned long  dwIntrStatus)

  *Called From:BSD::Status_Poller().

  *Parameters:
        dwIntrStatus:(i/p):Interrupt status 

  *Flow:
        -Invoke callback for card whose interrupt has been 
         raised.  
        -Return. 
*/

//==============================================================
/*B.12.DMA functionality. :                  LEVEL 1
      1. sd_mmc_sdio_initialise generic DMA.
      2. sd_mmc_sdio_initialise DW DMA.
      3. Start DMA
      4. Stop DMA.
*/
//==============================================================

/*B.13.Internal functions
      Thread2 functions
      General functions.
      Data analysis/Error checking
      Debug function.
*/
//-------------------------------------------------------------- 

/*B.13.1. Internal functions::Thread2 functions
*/

//-------------------------------------------------------------- 
/*Retries:If specified,command has to be retried.
If data command failed,load data command.(If data related error ?)
If non data command failed,load non data command.

1.Data error will be seen only by non data comamnd as before
  data command is done,data transfer will not start.
2.Immediate handling of data transfer errors will be done in 
  error handler.It will set the data error flag in global 
  structure.
3.Retry :If data command has to be retried,it cannot be done 
  before currently loaded non data command is over/done,
  So,wait till current command is over-either due to error
  or successful complete.
4.Send_cmd_to_host will be polling for command over flag in
  global structure.This flag will be set after command done.
5.If command over flag is set,this routine will check if there
  is any error (APART FROM DATA ERROR) occured.
  If yes,non data command will be retried.
  else check if data error is set.If yes,data command will be 
  retried.
6.If no retry for non data command or retry for data command is
  initiated, set error code and return from this routine.

7.Send_cmd_to_host will poll for command over flag in global 
  structure and not for command done in status register.This is 
  because comamnd over will be set after error handling is 
  finished and command done is recieved.So,T2 routine will check
  for errors first and then for command done.

  Instead of polling,sleep-wake up can used.
*/

/*B.13.1.4.Send_cmd_to_host:Actually sends command to host.
  All exposed APIs requiring communication with card are sent 
  through this.
  Other APIs requiring to read/write IP register,directly call
  Host driver functions.

  ptCmdInfo points to COMMAND_INFO structure.This structure 
  holds contents to write in bytecount,blockcount,cmd,cmdarg.
  For write command,this routine will write the data from data
  buffer into FIFO.
  
  int  Send_cmd_to_host(COMMAND_INFO *ptCmdInfo,
                      unsigned char        *pbcmdrespbuff,
                      unsigned char        *pberrrespbuff,
                      unsigned char        *pbdatabuff,
                      void        (*callback)(unsigned long dwerrdata),
                      int          nflags,
                      int          nnoofretries,
                      int         fDataCmd,
                      int         fwrite,
                      int         fKernDataBuff)//0=User addr.space.buffer.

  *Called from : BSD

  *Parameters  :
         ptCmdInfo     :(i/p):Pointer to COMMAND_INFO structure.
         pdwCmdRespBuff:(o/p):Buffer to store response received.
         pdwErrRespBuff:(i/p):Buffer to store response for 
                              STOP/ABORT command send after error.
         pdwDataBuff   :(i/p):Data buffer.
         callback      :(i/p):Callback for data commands.
         nflags        :(i/p):User options to whether ignore errors/send
                              ABORT/STOP on error/use retries etc.
         nnoofretries  :(i/p):Currently not implemented.
         fwrite)       :(i/p):Write/Read command.Default is FALSE.
         fDataCmd:(i/p):1 if  command is data transfer command.
         fKernDataBuff :(i/p):0=User mode buffer. 1=Kernel mode buffer.
  *Flow:
        -Copy buffer pointers and nflags in global structure.
         Clear command over flag in global structure.

          -For write command,this routine needs to write the data in 
         FIFO.

        -Load command in IP.Call Host::Send_SD_MMC_Command().

        -Poll for command over flag which will be set either 
         due to command done or error interrupt.

        -Check if any error occured.If yes,then set error code,
         if in case IP indicated error bit is set

        -Return
*/



int  Send_cmd_to_host(COMMAND_INFO *ptCmdInfo,
                      unsigned char        *pbcmdrespbuff,
                      unsigned char        *pberrrespbuff,
                      unsigned char        *pbdatabuff,
                      void        (*callback)(unsigned long dwerrdata),
                      int          nflags,
                      int          nnoofretries,
                      int         fDataCmd,
                      int         fwrite,
                      int         fKernDataBuff)//0=User addr.space.buffer.
{
  unsigned long dwRetVal,dwBytesWriten,dwDummy,dwData,ctrl_val;
  int j,dwTemp;//dwData and i are required for manual polling only.
 
#ifdef DMA_ENABLE
  unsigned long dwDMATransSize=0;
#endif
 DECLARE_WAITQUEUE(wait, current); 
  unsigned long i;

  dwBytesWriten =0;
  j=0;

// dwTemp=ptCmdInfo->CmdReg.Bits.cmd_index;
 //printk("#####cmdidex is %d\n",ptCmdInfo->CmdReg.Bits.cmd_index);
 HI_TRACE(2,"#####cmdidex is %d\n",(int)(ptCmdInfo->CmdReg.Bits.cmd_index));
  //If CMD40 is in process.Donot send any other command.MMC_ONLY mode.
  if(G_CMD40_In_Process)
    return Err_CMD40_InProcess;
      
  //If data command is in process,donot allow any command other than 
  //stop/abort.For these commands, CMDreg::stop_abort bit is set.
  if(tDatCmdInfo.fCmdInProcess && !(ptCmdInfo->CmdReg.dwReg & 0x4000))     //w53349 20061103 ptCmdInfo->dwCmdarg->ptCmdInfo->Cmdreg
    return Err_DataCmd_InProcess;

  //Initialize elements of current command structure.
  tCurrCmdInfo.pbCurrDataBuff   = pbdatabuff;
  tCurrCmdInfo.pbCurrCmdRespBuff= pbcmdrespbuff;
  tCurrCmdInfo.pbCurrErrRespBuff= pberrrespbuff;
  tCurrCmdInfo.nCurrCmdOptions   = nflags;//Option to retry/error check from user.
  tCurrCmdInfo.dwCurrErrorSts    = 00;
  tCurrCmdInfo.fCurrCmdInprocess = 0x1; 
  tCurrCmdInfo.fCurrErrorSet     = 00;
  tCurrCmdInfo.nResponsetype     = ptCmdInfo->CmdReg.Bits.response_length;


  //If data command,reset the FIFO.
 
  if(fDataCmd)
  {
    set_ip_parameters(RESET_FIFO, &dwDummy);
    //For write_data command,this routine needs to write the data in 
    //FIFO.Since, after command taken,TBBCNT is reset.
    //Use TCBCNT to have the bytes transferred with card
    //To use TBBCNT,after command taken,fill FIFO with data and then,
    //enable interrupts.
    {
    //w53349修改，当使能DMA搬移时，不填充FIFO    
        
        Read_Write_IPreg(CTRL_off,&ctrl_val,READFLG); 
        ctrl_val &= 0x20;
          //Write data in FIFO.If data size is >FIFO_SIZE,bytes 
          //equal to FIFO size will only be copied to FIFO.
          //Return value is actual number of bytes written in FIFO.        
        if(fwrite && (!ctrl_val))
        {
          //Original buff.ptr=current buff.ptr.
          dwBytesWriten=Read_Write_FIFO((unsigned char*)pbdatabuff,
                                        ptCmdInfo->dwByteCntReg,
                                        fwrite,
                                        fKernDataBuff);// 0=user space buffer. 
        }
    }
    //If data command,fill data command info structure.
    //In case of error/data done,remember to reset it.
    tDatCmdInfo.pbCurrDataBuff= pbdatabuff;
    tDatCmdInfo.fWrite = fwrite;
    tDatCmdInfo.dwByteCount=ptCmdInfo->dwByteCntReg;
    if(fwrite)
      tDatCmdInfo.dwRemainedBytes = ptCmdInfo->dwByteCntReg-dwBytesWriten;
    else
      tDatCmdInfo.dwRemainedBytes = ptCmdInfo->dwByteCntReg;
    tDatCmdInfo.nCurrCmdOptions= nflags;
    tDatCmdInfo.fCmdInProcess = 1;//data cmd.is in process.
    tDatCmdInfo.callback      = callback;
    tDatCmdInfo.DataBuffType  = fKernDataBuff;
    tDatCmdInfo.dwDataErrSts  = 00;
    //printk(" : EXiting after setting for data command. \n");
    //printk(" :Bytecount for data transfer = %lx \n", tDatCmdInfo.dwByteCount);
  }

  //Before loading command,clear RINTSTS,except card_detect and SDIO intrs.
  //Or else,these will be lost if set when driver/IP is idle.

#ifdef DMA_ENABLE
  if(fDataCmd && ctrl_val)
  {
     dwDMATransSize = tDatCmdInfo.dwRemainedBytes;
     tDatCmdInfo.dwRemainedBytes = 0;     
     //printk("the cmd to sent is a data cmd!\n");
  }
//   printk("Send_cmd_to_host : dwDMATransSize=%ld\n",dwDMATransSize);
#endif

  dwData=0;
  Read_Write_IPreg(RINTSTS_off,&dwData,READFLG);
  dwData &= 0x1fffe;
  Read_Write_IPreg(RINTSTS_off,&dwData,WRITEFLG);  //发送命令之前清楚中断标志

  //Load command in IP.
 // printk("send cmd index is %d!\n",ptCmdInfo->CmdReg.Bits.cmd_index);
  dwRetVal = Send_SD_MMC_Command(ptCmdInfo);

  //Error likely is HLE error or internal error of OS.
  if(dwRetVal!=SUCCESS)
  {
   // printk(": Send_sd_mmc_cmd returned = %lx \n", dwRetVal);
    return Err_Command_not_Accepted;
  }
//printk("cmd send sucess!wait for end!\n");



#ifdef INTR_ENABLE
  dwTemp=0;
  dwRetVal=0;
  
#if 1  
{ 
      //poll for command done
      //If command done not received,check for it first.
     add_wait_queue(&command_wait, &wait);     
	int i = 0;    
         while (1) {
             set_current_state(TASK_INTERRUPTIBLE);           
           if (!tCurrCmdInfo.fCurrCmdInprocess || i > 5)
               break;
            schedule_timeout(10); 
		i++;                
        }

        set_current_state(TASK_RUNNING);
        remove_wait_queue(&command_wait, &wait);

        if(fDataCmd && (callback==NULL))
        {

                 add_wait_queue(&data_wait, &wait);      
               while (1) {
                    set_current_state(TASK_INTERRUPTIBLE);

                    if (!tDatCmdInfo.fCmdInProcess)
                        break;                        
                    schedule();    
                }

                set_current_state(TASK_RUNNING);
                remove_wait_queue(&data_wait, &wait);
                
                       //For no callback,return data errors also.
               dwRetVal |= tDatCmdInfo.dwDataErrSts;
               //printk("Received data done. Breaking loop.\n");
                   
               if(fwrite)
               {       
                #ifdef _HI_3511_
                         i = 50000;        //set the proper value by test!
                #endif                 
                   #ifdef _HI_3560_
                       i = 50000;
                   #endif
                do
                {
                    if (--i == 0){
                        break;
                    }
                    //for(j=1000;j>0;j--);
                    dwData =0;
                    Read_Write_IPreg(STATUS_off,&dwData,READFLG);    
                }while(dwData & 0x200);
                if(i == 0)
                {
                    i = 500;
                    do
                    {
                        if (--i == 0)
                            break;
                        msleep(8);
                        dwData =0;
                        Read_Write_IPreg(STATUS_off,&dwData,READFLG);    
                    }while(dwData & 0x200);

                    if(i==0)
                    {
                        i = 5;
                        do
                        {
                            if (--i == 0)
                            return Err_AfterExecution_check_Failed;
                            printk("Send_cmd_to_host Read IP status busy, begin msleep!\n");
                            msleep(1000);
                            dwData =0;
                            Read_Write_IPreg(STATUS_off,&dwData,READFLG);    
                        }while(dwData & 0x200);
                    }
                }
               }//end of if(fwrite)
               
          }//end of else  
    }
#endif
    dwRetVal |= tCurrCmdInfo.dwCurrErrorSts; //Check for response errors and return them

#else
  //Disable interrupts and poll manually for events.
  do{
      //Read RINTSTS reg.
      dwData=0;
         
      //Without this delay,during mount command,driver hangs after
      // "FAT : Using codepage default " 
      for(i=0;i<0x0ffff;i++); 

      Read_Write_IPreg(RINTSTS_off,&dwData,READFLG);
      //printk("\n  : RINTSTS read = %lx \n",dwData);
      //printk tried to be replaced with delay. f added to 0x0fff in above delay loop.

      //Clear interrupt which are set,by writing 1 at those locations.
      Read_Write_IPreg(RINTSTS_off,&dwData,WRITEFLG);

      //Check if any interrupt bit is set.
      if(dwData & 0x0000ffff) 
      {
        G_dwIntrStatus=dwData;
        dwTemp=sd_mmc_event_Handler(&G_dwIntrStatus);
      }
      else j++;//Check for RINTSTS=0;

      if(j>30) 
      {
        tDatCmdInfo.fCmdInProcess=0;
        tCurrCmdInfo.fCurrCmdInprocess=0;
        printk("RINTSTS=0 received for maximum retries.Aborting command done_check.\n");
        return  Err_CMD_Failed ;
      }

      //Check if any of the response error bits is set
      //bits 1,6,8,13 ie 0x00002142. .Return response error only.
      //Data errors are stored in data command structure.
      //If caller of this routine wants to send this data error,
      //it can refer to this structure.
      dwRetVal = (dwData & 0x00002142);

      //If response timeout,data command is not sent.
      if(dwData & 0x0000100) 
         tDatCmdInfo.fCmdInProcess = 0;

      //If command done not received,check for it first.
      if(tCurrCmdInfo.fCurrCmdInprocess)
        continue;//poll for command done.
      else  
      {
        //printk(":Checking for data done.\n");
        //If data callback=NULL,wait for data done.
        //else return on command done.
        if(fDataCmd && (tDatCmdInfo.callback==NULL))
        {
          if(tDatCmdInfo.fCmdInProcess)
          {
           //printk("No data done. Continuing with loop.\n");
           continue;//poll for command done.
          }
          else
          {
            //For no callback,return data errors also.
            dwRetVal |= tDatCmdInfo.dwDataErrSts;

            //printk("Received data done. Breaking loop.\n");
            //Currently this in infinite loop.If card hangs,
            //driver won't return from this.Use retry count.
            //Check for Card busy status.
            if(fwrite)
            {                   
        #ifdef _HI_3511_
                 i = 50000;        //set the proper value by test!
        #endif                 
           #ifdef _HI_3560_
               i = 50000;
           #endif    
           
               do
            {
                if (--i == 0)
                    break;
                dwData =0;
                Read_Write_IPreg(STATUS_off,&dwData,READFLG);    
            }while(dwData & 0x200);
            if(i == 0)
            {
                int i = 500;
                do
                {
                    if (--i == 0)
                        return Err_AfterExecution_check_Failed;
                    //printk("Send_cmd_to_host Read IP status error begin msleep!\n");
                    msleep(8);
                    dwData =0;
                    Read_Write_IPreg(STATUS_off,&dwData,READFLG);    
                }while(dwData & 0x200);                
            }
            }
            break; 
          }
        }//Data command with no call back pointer
        //Return now as data command is with callback.
        break;
  
        //For non data command,break when command done received.
        //For data command AND no callback,return on data done. 
      }
    }while(1);

  //Clear interrupt status register.Writing 1 clears the interrupt.
  G_dwIntrStatus=00;
  Read_Write_IPreg(RINTSTS_off,&G_dwIntrStatus,READFLG);
  //printk("\n RINTSTS read = %lx \n",G_dwIntrStatus); 
  Read_Write_IPreg(RINTSTS_off,&G_dwIntrStatus,WRITEFLG);
  G_dwIntrStatus=00;
   
  //printk("\n : send_cmd_to_host returns %lx \n",dwRetVal); 
#endif
  //debug 
  //printk("\n : send_cmd_to_host returns %lx \n",dwRetVal); 
  
  return dwRetVal;
}
//-------------------------------------------------------------- 
/*
Thread2 : Interrupt handling.
    On any interrupt,Host driver will call the callback routine
    of Bus driver,which in will read the Raw interrupt status
    and will wake up this interrupt handler.

    This routine will serve interrupts and after completion of
    current comamnd,it will set Command_Over flag which will be 
    polled by Send_Cmd_To_Host()and if it is set,it will return
    control back to client.This is the only point which will
    decide if control is to be returned back to client.

    All commands to card will be routed through Send_Cmd_To_Host

  sd_mmc_event_Handler(unsigned long *pdwIntrStatus)

  *Called from : ISR

  *Parameters  :
         pdwIntrStatus:(i/p):Raw Interrupt status read when 
                            ISR is called by Host driver.

  *Assumptions:
  //ONLY PARAMETER FOR RETRY WILL BE PROVIDED.RETRIES
    WILL NOT BE IMPLEMENTED IN CURRENT VERSION.

  //Retry: only for data comamnds.
    Meaningless for non data commands as error to these
    commands are likely due to link problem,card may have
    honoured it well.Retrying will not serve great deal.

  //For retries/stop command,if current non data command is in
    process,control is sent back to client,only when retry/stop
    command is processed.Thus,when control is received by client,
    Bus driver is in position to serve next command.

  //For command done,disable interrupts for response errors.
    Thus,for command done,check if there is any error.If yes,
    set error code and return.

  //Auto command done:Set only when auto command is completed.
    Command done is set only when user sent command is done.

  //Data starvation by host:FIFO is full so IP is waiting for
    driver to read it.After timeout,IP has raised this interrupt

  //Data done:Data errors can be flashed as and when observed.
    If abort/stop is sent,donot entertain data over intr.
    If abort/stop is sent,donot entertain auto command done intr.
    If abort/stop is sent,donot entertain FIFO threshold intr.

  //No retries for abort/stop command.After command execution,
    return with error response copied in error response buffer.
    Donot serve any errors received in response to STOP/ABORT
    command initiated by driver.If responded,such errors may
    lead driver in loop

  //For data transfer related interrupts ie data errors, data
    over,FIFO threshold errors and FIFO run errors,confirm that
    data command is in process.

  //When data done error is received,no other data errors except
    End bit error, will be set as they are scanned and set 
    before data over.
  //Error response buffer required only for data transfer 
    commands.


  // ****IMP : 
  FIFO overrun :FIFO is full and host has tried to write further
  data in it.

  FIFO underrun :FIFO is empty and host has tried to read further
  data from it.

  FIFO full/empty : Reflected in status register.
  
  Data starvation: During write(read) cycle,core has informed 
  software about threshold,but software has not taken any action.
  After FIFO becomes empty(full for read),IP stops the clock
  to card and waits for software to take action and provide some 
  space in FIFO.If within timeout period,software fails to do 
  so,core will generate data starvation interrupt.

  Start Bit Error (SBE): Raised  during data read operation.If
  core doesnot receive start bit,this is raised and after data 
  timeout,data timeout condition is raised.

  End bit Error(EBE):
  During read operation,if end bit is not received by core.
  Duing write operation,core wsends data CRC and in return,card
  semds CRC status,indicating if CRC is matching or not.If it 
  indicates CRC is not matching,then this error is raised.

  //Seperate structure for data command is maintained to 
    retrieve data command parameters.

  //Sequence of interrupt servicing:
     Data errors
     FIFO threshold
     Data over
     Data starvation
     Card detect
     Auto command done.
     Command done

  //For card_detect,driver will wait till current non data and
  //data commands are over.It will call Enumerate_Card and then
  //return control back to application.

  //Flags and their meaning:
    SEND_ABORT : Data error condition has occured and ABORT/STOP
                 command is to be sent by driver as soon as 
                 possible.

    ABORT_IN_PROCESS: Driver has sent ABORT/STOP command to host
                 for execution.

    command over flag:Polled by Send_Cmd_to_Host() to check
                 if command is done and control can be returned
                 back to client.

    User options:Ignore errors and copy data.
                 Send ABORT/STOP on errors.

  //User may opt to copy data evenif error occurs,but stop 
    further data transfer.

  *Flow:
    -Disable interrupts.
    
    -//Donot entertain data transfer interrupts if 
     //SEND_ABORT flag is set => this interrupt might have been 
     //received just after ABORT command has been sent.

     //For data transfer intrs,check data command is in process.
     If(Data command in process && 
        SEND_ABORT==FALSE)
     {
       //Respond to data transfer related errors ie
       //Data timeout,CRC,End bit error,FIFO overrun/underrun.
       -If(data error) OR (FIFO errors.)//(bit9,7,15,10,11)
        { 
          Check user option.
          //Check if data command to terminate on errors.
          If (HANDLE_DATA_ERRORS == FALSE)
          {
            -If(send abort for this error)
                  set SEND_ABORT flag.
           
            -Invoke callback of client indicating data command
             is over.
            -Break out of this loop so as not to serve any other
             data interrupts.
        }
         
       -//In case of errors,check if user wants to ignore 
        //errors.If yes,then copy data evenif errors occured.

        //Default:For data errors/FIFO errors donot copy data.
        If( No data/FIFO error OR 
           (FIFO/data error AND IGNORE_ERROR) 
          )
        {
          -If(FIFO threshold reached)(bit4,5)
           {
             -If(Data read in process)
                copy bytes=THRESHOLD LEVEL from FIFO into 
                data buffer
              else
                copy bytes=THRESHOLD LEVEL from data buffer  
                into FIFO.
             -Decrement remaining counts by THRESHOLD LEVEL.
           }
               
          -If(Data Over) (bit3)
           {
             -If(Data read in process)
                copy bytes= Remaining bytes from FIFO into 
                data buffer
              else
                copy bytes= Remaining bytes from data buffer
                into FIFO.
             -Set remaining counts = 0;
             -Clear Data command in process flag.
             -Invoke callback routine of client.
           }

          -If(Data starvation interrupt)
           {
             //FIFO is full.
             -If(Data read in process)
                copy bytes= FIFO DEPTH bytes from FIFO into 
                data buffer
              else
                copy bytes= FIFO DEPTH bytes from data buffer
                into FIFO.
             -Decrement reamining counts by FIFO DEPTH bytes;
           }

        }//End of if (!send_abort)
      }//if data command interrupt.

      -If Card Detect (bit0)
       {
         -Set ENUMERATE_CARD_PENDING flag.
         //AFter non data command is over,Send_command_to_Host()
         //will get control back.
         //It will wait till current data command is over.
         //It will call enumerate card.
         //After Enumerate_card() is done,then return control back
         //to client.
       }

      //Auto comand done (bit14) is received for data command 
      //only.If ABORT/STOP command is already sent,then do 
      //nothing for this.
      -If (Auto command done AND Data command in process AND 
           ABORT_IN_PROCESS==FALSE)
       {
         //For data command,since stop is sent by IP,then donot
         //send stop again.
         -Clear SEND_ABORT.

         //If ABORT_IN_PROCESS,then do nothing for this interrupt.
         //Command done for abort command will be received soon.

           -Invoke callback of client indicating data command
          is over.
       }

      -If(Command Done) (bit2)
       {
          If(!ABORT_IN_PROCESS)//Abort command not yet sent.
          {
           -Check if any other response error set.(bits 1,6,8)
            If yes,set error code and error flag in current 
            command structure info.
           -If no response error,then copy response in 
            response buffer.

           -//Donot set command over flag,if abort command 
            //is to be sent.No STOP/ABORT to be sent,so return
            //control back to client.
            If( !SEND_ABORT )
            {
              Set command over flag.
              This flag is polled by Send_command_to_host().
            }

           -If(SEND_ABORT)
             {
               -Send abort command.
               -Clear SEND_ABORT.
               -Set ABORT_IN_PROCESS
             }

          }//If abort command is not in process.
          else
          { //Abort command done recieved.
            -Copy error response in error response buffer.
            -Clear ABORT_IN_PROCESS
            -If(NON_DATA_CMD_COMPLETE)
               Set command over flag for current command.
            -Invoke callback of client indicating data command over.
          }
        }

    -Enable interrupts.
    -Sleep till next interrupt comes.

            wake_up(&command_wait);
    
*/
int sd_mmc_event_Handler(void *pdwIntrStatus)
{
  unsigned long         dwRetVal,dwActualBytes ;
  //COMMAND_INFO  tCmdinfo;
  RawIntrStsReg   *ptRawIntrSts;
  //printk("enter sd_mmc_event_Handler!\n");

  ptRawIntrSts  = (RawIntrStsReg*)pdwIntrStatus ;
  dwActualBytes = 0; 

  //printk(" : Entering in sd_mmc_event_handler with data= %lx \n", ptRawIntrSts->dwReg);

  //For data transfer intrs,check data command is in process.
  //Is this interrupt after ABORT/STOP command sent ?

  if(tDatCmdInfo.fCmdInProcess && !G_SEND_ABORT)
  {
//    printk(" : Entering in data intr.handling \n");

    //Respond to data transfer related errors ie
    //Data timeout,CRC,End bit error,FIFO overrun/underrun.
    //(bit9,7,15,10,11)
    if((ptRawIntrSts->dwReg & DATA_OR_FIFO_ERROR))// & 0x00008E80
    { 
      //Check user option. bit4 = 1 => HANDLE_DATA_ERRORS
      if((tDatCmdInfo.nCurrCmdOptions & HANDLE_DATA_ERRORS))
      {
        //User opt to handle data errors.So,store them.
        tDatCmdInfo.dwDataErrSts|=(ptRawIntrSts->dwReg & DATA_OR_FIFO_ERROR);

        //If send abort for this error. bit5=1 => SEND_ABORT
        if((tDatCmdInfo.nCurrCmdOptions & SEND_ABORT))
           G_SEND_ABORT=TRUE;
        else
        {
          //user wants to send abort himself.So,invoke callback
          //without data done.
          //Invoke callback routine of client.
          if(tDatCmdInfo.callback!=NULL)
             tDatCmdInfo.callback(tDatCmdInfo.dwDataErrSts);
          //To clear tDatCmdInfo.dwDataErrSts to indicate that  
          //these have been handled.??
        }

        //When ABORT/STOP is sent,invoke callback for data 
        //over.Donot serve any other data interrupts.

      }//End of checking for HANDLE_DATA_ERRORS
    }//End of checking for DATA_OR_FIFO_error



    /*Check if data over/FIFO threshold interrupts*/
    //If user wants to ignore errors,then proceed evenif
    //error has occured.
    if(!(ptRawIntrSts->dwReg  & DATA_OR_FIFO_ERROR)  ||
       !(tDatCmdInfo.nCurrCmdOptions & HANDLE_DATA_ERRORS) )//bit0  HANDLE_DATA_ERRORS
    {
 //     printk(" : Data handling.No error recd.\n");
      //Check if(FIFO threshold reached)(bit4,5)
      if(ptRawIntrSts->Bits.int_status.Bits.Rx_FIFO_datareq)
      { 
   //     printk(" : Entering to handle Rx_FIFO_datareq \n");

        //If 0 bytes to be read,do nothing.Mistimed interrupt.
        if(tDatCmdInfo.dwRemainedBytes !=0)
        {
          //Read operation is in process.
          //Buffer pointer=org+(total bytes-received bytes)
          dwActualBytes = 
             Read_Write_FIFO((tDatCmdInfo.pbCurrDataBuff)+
                             (tDatCmdInfo.dwByteCount-tDatCmdInfo.dwRemainedBytes),
                             tDatCmdInfo.dwRemainedBytes,FALSE,
                             tDatCmdInfo.DataBuffType);
          //Set reamining counts.
          tDatCmdInfo.dwRemainedBytes -= dwActualBytes;
     //     printk(" : Rx_FIFO_datareq: Bytes remained = %lx \n",tDatCmdInfo.dwRemainedBytes);
         }
         else{
              
         }
      }
      else
        if(ptRawIntrSts->Bits.int_status.Bits.Tx_FIFO_datareq)
        {
     //    printk(" : Entering to handle Transmit_FIFO_datareq \n");
          //If 0 bytes to be written,do nothing.Mistimed interrupt.
          if(tDatCmdInfo.dwRemainedBytes !=0)
          {
            //Write operation is in process.
            dwActualBytes = 
                Read_Write_FIFO((tDatCmdInfo.pbCurrDataBuff)+
                                (tDatCmdInfo.dwByteCount-tDatCmdInfo.dwRemainedBytes),
                                tDatCmdInfo.dwRemainedBytes,TRUE,
                                tDatCmdInfo.DataBuffType);
            //Set reamining counts.
            tDatCmdInfo.dwRemainedBytes -= dwActualBytes;
            //printk(" : Transmit_FIFO_datareq: Bytes remained = %lx\n",tDatCmdInfo.dwRemainedBytes);
          }//end of  if(tDatCmdInfo.dwRemainedBytes !=0)

        }//End of else part for checking Tx_threshold error.
               
    }//End of if (no error || IGNORE_ERROR)
  }//if data command in progress AND no abort to be sent..


  //Data transfer over has to be entertained evenif there is any
  //error during data transfer or abort is sent.
  if(tDatCmdInfo.fCmdInProcess)
  {
    
        //If  bit7,9,11,13,15 exception error interrupt occur! 20080303 weimx add below code
    if(ptRawIntrSts->dwReg & 0xaa80)
    {
        //Clear Data command in process flag.
        printk("\n\nsd_mmc_event_Handler error interrupt when transferring data,interrupt code=%d\n", 
                    (unsigned int)(ptRawIntrSts->dwReg & 0xaa80));
          tDatCmdInfo.fCmdInProcess = FALSE;
        wake_up_interruptible(&data_wait);        
    }
    
    //Check if(Data Transfer Over) (bit3)
    if(ptRawIntrSts->Bits.int_status.Bits.Data_trans_over)
    {
    //  printk("handle DTO\n");
      //If 0 bytes to be written,do nothing.Mistimed interrupt.

      if(tDatCmdInfo.dwRemainedBytes !=0)
      {

        //Here tDatCmdInfo.dwRemainedBytes will never exceed 
        //FIFO depth.
        dwActualBytes = 
             Read_Write_FIFO((tDatCmdInfo.pbCurrDataBuff)+
                             (tDatCmdInfo.dwByteCount-tDatCmdInfo.dwRemainedBytes),
                             tDatCmdInfo.dwRemainedBytes,
                             tDatCmdInfo.fWrite,
                             tDatCmdInfo.DataBuffType);
        //Set remaining counts.
        tDatCmdInfo.dwRemainedBytes-=dwActualBytes;

        //If unsigned char count is not in multiples of FIFO_width,
        //then, one extra read/write is required.         
        //DONOT use while loop.Or else,in case of error,
        //FIFO may be empty,data transfer over and remained bytes
        //will never become 0.Driver will hang.
        if(tDatCmdInfo.dwRemainedBytes !=0)
        {
          dwActualBytes = 
            Read_Write_FIFO((tDatCmdInfo.pbCurrDataBuff)+
                               (tDatCmdInfo.dwByteCount-tDatCmdInfo.dwRemainedBytes),
                               tDatCmdInfo.dwRemainedBytes,
                               tDatCmdInfo.fWrite,
                               tDatCmdInfo.DataBuffType);
          //Again Set remaining counts.
          tDatCmdInfo.dwRemainedBytes-=dwActualBytes;
        }
      }//end of if(remainedbytes!=0)


      //Clear Data command in process flag.
      tDatCmdInfo.fCmdInProcess = FALSE;

      //To wait till write operation of card is over ? 
      //Invoke callback routine of client.
      if(tDatCmdInfo.callback!=NULL)
           tDatCmdInfo.callback(ptRawIntrSts->dwReg);
      //After invoking callback,donot call it again.SO, make callback NULL.
      tDatCmdInfo.callback=NULL;
//     printk(" : sd_mmc_event: Exiting after checking for data transfer over.\n");
      //wake_up_interruptible(&data_wait);
        wake_up_interruptible(&data_wait);
    }//End of checking for Data done.



    //In any case,handle this as clock is stooped by IP and unless
    //FIFO is cleared/filled,no command is further processed.
    //Check if(Data starvation interrupt)
    if(ptRawIntrSts->Bits.int_status.Bits.Data_starv_tmout)
    {
        //   printk("entry handle Data_starv_tmout!\n");
        //If 0 bytes to be written,do nothing.Mistimed interrupt.

        if(tDatCmdInfo.dwRemainedBytes !=0)
        {
            dwActualBytes = 
            Read_Write_FIFO((tDatCmdInfo.pbCurrDataBuff)+
                      (tDatCmdInfo.dwByteCount-tDatCmdInfo.dwRemainedBytes),
                      tDatCmdInfo.dwRemainedBytes,
                      tDatCmdInfo.fWrite,
                      tDatCmdInfo.DataBuffType);
            //Set reamining counts.
            tDatCmdInfo.dwRemainedBytes-=dwActualBytes;
            //printk(" : sd_mmc_event: Exiting after Data_starvation_intr.handling.\n");
        }//End of Data starvation check

    }//End of starvation interrupt check.
  }//End of if data command is in process check.

  //If Card Detect (bit0)
  if(ptRawIntrSts->Bits.int_status.Bits.Card_Detect)
  {
    //Set ENUMERATE_CARD_PENDING flag.
    G_PENDING_CARD_ENUM=TRUE;
    //printk("20071122add wake_up_interruptible(&data_wait)\n");
    tDatCmdInfo.fCmdInProcess = 0;        //weimx 20071122 add this line
    wake_up_interruptible(&data_wait);        //weimx 20071122add this line
    //Set card_eject and card_insert flags.
    //These will be polled by client and he will call Enumerate_Card.
 //   printk(" : Entered in Card_detect.Going for Card_Detect_Handler.\n");
    //Calling card_detect_handler here will nest the ISR.
    //Card_Detect_Handler();
  }

  //Auto comand done (bit14) is received for data command 
  //only.If ABORT/STOP command is already sent,then do 
  //nothing for this.
  if(ptRawIntrSts->Bits.int_status.Bits.ACD && tDatCmdInfo.fCmdInProcess &&
     !G_ABORT_IN_PROCESS)
  {
//   printk("entry handle ACD!\n");
    //For data command,since stop is sent by IP,then donot
    //send stop again.
    G_SEND_ABORT=FALSE;
     
    //If ABORT_IN_PROCESS,then do nothing for this interrupt.
    //Command done for abort command will be received soon.
     
    //Invoke callback of client indicating data command
    //is over.
    //OR  check for data transfer over         ??
  }

  //If any active command is not in process,then this is a false
  //trigger.Donot respond to this.Response buffer pointers invalid.
  //**** IMP : tCurrCmdInfo.fCurrCmdInprocess is set when command
  //is loaded.
  //Also,this will not respond to command_done received when 
  //clock parameters are changed.
  if(ptRawIntrSts->Bits.int_status.Bits.Cmd_Done &&
     tCurrCmdInfo.fCurrCmdInprocess) //(bit2)
  {
//    printk("sd_mmc_event: entering in command_done handling.\n");
    if(!G_ABORT_IN_PROCESS)//Abort command not yet sent.
    {
      //Check if any other response error set.(bits 1,6,8)
      if(ptRawIntrSts->Bits.int_status.Bits.Rsp_Err || 
         ptRawIntrSts->Bits.int_status.Bits.Rsp_crc_err ||
         ptRawIntrSts->Bits.int_status.Bits.Rsp_tmout)
      {
        //Set error code and error flag in current cmd.struct.
        tCurrCmdInfo.fCurrErrorSet=TRUE;
  //      printk("sd_mmc_event: Response error received.\n");
      }
      else
      {
  //      printk("sd_mmc_event: Reading response.\n");
        //Copy response. Pass if long or short response.
        //For long response all 4 resp.reg.sare copied and
        //for short response,only resp.reg.0 is copied.
        if(tCurrCmdInfo.pbCurrCmdRespBuff)
           dwRetVal = Read_Response(
                                    (RESPONSE_INFO*)tCurrCmdInfo.pbCurrCmdRespBuff,
                                    tCurrCmdInfo.nResponsetype);
      }//Check if any response error.
            
      //Donot set command over flag,if abort command 
        //is to be sent.No STOP/ABORT to be sent,so return
      //control back to client.
      if(!G_SEND_ABORT)
      {
        //printk("wakeup \n");
        //Set command over flag.
        //This flag is polled by Send_command_to_host().        
        tCurrCmdInfo.fCurrCmdInprocess=FALSE;    
       // wake_up_interruptible(&command_wait);
        wake_up_interruptible(&command_wait);


  //      printk("sd_mmc_event:Clearing tCurrCmdInfo.fCurrCmdInproces.\n");
      }
      else
      {
        //Clear SEND_ABORT.
        G_SEND_ABORT=FALSE;    
        //Set ABORT_IN_PROCESS
           G_ABORT_IN_PROCESS=TRUE;

        //Send abort command.For SDIO_IO cmd.,use ABORT.
        //For others including SDIO_MEM use STOP.
     //   Form_STOP_Cmd(&tCmdinfo);                      //no use ;del by w53349 20061103
     //   Send_SD_MMC_Command(&tCmdinfo);                //no use ;del by w53349 20061103
        //Check :If while interrupts are disabled,IP raises any
        //interrupt
        }

    }//If abort command is not in process.
    else
    { //Abort command done recieved.
      //If error response buffer provided,then copy error
      //response in it.
//      printk("lskdjflsd\n");
      if(ptRawIntrSts->Bits.int_status.Bits.Rsp_Err || 
         ptRawIntrSts->Bits.int_status.Bits.Rsp_crc_err ||
         ptRawIntrSts->Bits.int_status.Bits.Rsp_tmout)
      {
        if(tCurrCmdInfo.pbCurrErrRespBuff)
           dwRetVal = Read_Response(
                    (RESPONSE_INFO*)tCurrCmdInfo.pbCurrErrRespBuff,0);
        //Clear ABORT_IN_PROCESS
        G_ABORT_IN_PROCESS=FALSE;

        //Invoke callback of client indicating data command over.
        //OR  check for data transfer over         ??
      }
    }//End of else for ABORT in process.
  }//If command done interrupt.

    //to resovle the mmc detect problem
    if(ptRawIntrSts->Bits.int_status.Bits.Rsp_tmout &&
                 tCurrCmdInfo.fCurrCmdInprocess)
    {
        tCurrCmdInfo.dwCurrErrorSts = 0x100;
        tCurrCmdInfo.fCurrCmdInprocess=FALSE;    
           wake_up_interruptible(&command_wait);
    }
    
  //Clear interrupt status register.Writing 1 clears the interrupt.
  //This will be done by the caller of this function.
  return SUCCESS;
}


void Form_STOP_Cmd(COMMAND_INFO *ptCmdinfo)
{
  if(ptCmdinfo==NULL)
    return;
  //Form command register.This is non data command.
  return;
}

//--------------------------------------------------------------
/*B.13.2. Internal functions::General functions

      form_send_cmd
      Validate_Command 
      Add_command_in_queue 
      Validate_Response
*/
//--------------------------------------------------------------
/*B.13.2.1. form_send_cmd:  Data transfer commands are not 
  handled here.Forms the command,and calls Send_Cmd_to_Host()
  which actually transfers command to host.It will return 
  control when the command is executed by IP ie command done.
  Response will be copied in response buffer.

  -No error response buffer: As no ABORT/STOP for error in case 
   of non data commands.
  -No need of data buffer :As this routine doesnot serve data
   commands.
  -No retries and no flags :For non data commands,no retries or
   automatic handling by driver.

  form_send_cmd(int ncardno,int nCmdID,unsigned long dwcmdarg,
                unsigned long *pdwCmdRespBuff,int nflags=01)

  *Called from : BSD internal functions.

  *Parameters  :
         nCmdID:(i/p):  Command ID.
         ncardno:(i/p): Card number.
         dwcmdarg:(i/p):Command argument to be passed with 
                        command.
         pdwCmdRespBuff:(o/p):Response is stored in this buffer. 

         nflags :(i/p): Flags for user settable options.
          Send_Init_Seq(bit0)=1; Send initialize sequence.;
          Wait_Prv_Data(bit1)=1; Wait till previous data 
                                 transfer is over.
          Chk_Resp_CRC (bit2)=1; Check response CRC.
          Predef_Transfer(bit3)=1;Predefined data transfer.
                                  Send cmd23 first.
          Handle_Data_Errors (bit4)=1; Handle data errors.
          Send_Abort (bit5)=1;
             Read_Write   (bit7)=1; Write operation. 0=Read.   
  *Flow:

        -Create COMMAND_INFO instance.(*ptCmdInfo)
   Sanity check:
        -Check if card no.is valid and card is connected.
   Form command:
        -Fill command related registers as per command index
         and user supplied values/options.
        -Set default values for unreuired registers ie registers
         which are only used for data commands.

   Send command:
        -Send command for execution using 
         Send_cmd_to_host(COMMAND_INFO *ptCmdInfo,
                          unsigned long *pdwCmdRespBuff,
                          NULL,//No error    response buffer
                          NULL,//No data response buffer
                          00,  //No flags from user
                             00   //No retries.
                          );
        -Return;
*/
int  form_send_cmd(int ncardno,int nCmdID,unsigned long dwcmdarg,
                unsigned char *pbcmdrespbuff,int nflags)

{
  unsigned long dwRetVal;
  COMMAND_INFO tCmdInfo = {0x00000000};
  //printk("form_send_cmd: cardno received = %x \n", ncardno);
  //printk("form_send_cmd: CMDID received = %x \n",  nCmdID);
  //printk("form_send_cmd: arg received = %lx \n", dwcmdarg);
  //printk("form_send_cmd:  nflags received = %x \n",  nflags);

  //Check if card no.is less than total cards connected.
  //printk("Entry : form_send_cmd : input par is \nncardno=%d,\nCmdID=%d,\ndwcmdarg=%lx,\nnflags=%d\n",ncardno,nCmdID,dwcmdarg,nflags);

  
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;
  //Check if card is connected.Else return error.
  //For enumeration commands skip this check.
  switch(nCmdID)
  {
    case SDIO_RESET:
    case CMD0:
    case CMD1:
    case CMD2:
    case CMD3:
    case CMD5:
    case CMD8:
    case CMD55:
    case ACMD41:
      break;

    default:
      if(Card_Info[ncardno].Card_Connected != TRUE)
        return Err_Card_Not_Connected;
  }
  //Form command:
  tCmdInfo.dwCmdArg=00 ;//Default value.
  switch(nCmdID)
  {
    // 1. No response commands.
    case CMD0://Argument is stuff bits. 
    case CMD4://DSR Argument as supplied by user.
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=0 ;//Short resp.
      tCmdInfo.CmdReg.Bits.response_expect=0 ;//No response
      tCmdInfo.dwCmdArg = dwcmdarg;
            
      break; 
    case CMD8:
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=1 ;//R2 response
      tCmdInfo.CmdReg.Bits.response_expect=1 ;
      tCmdInfo.dwCmdArg = dwcmdarg;      
        break;

    // 2. Long response.
    case CMD2:
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=1 ;//R2 response
      tCmdInfo.CmdReg.Bits.response_expect=1 ;
             
      break; 

    // 3. Short response.
    case SDIO_RESET:
    case CMD52://SDIO RW DIRECT.Argument is command block
    case SDIO_ABORT:
    case CMD38://Argument is stuff bits. 
    case CMD40://Argument is stuff bits. 

    case CMD1://OCR Argument as supplied by user.
    case CMD3://Argument is RCA for MMC card.
    case CMD16://Block length.
    case CMD23://Number of block counts.
    case CMD28://Set write protect
    case CMD29://clear write protect.
    case CMD32://SD erase Data start address.
    case CMD33://SD erase Data end address.
    case CMD35://Data address.
    case CMD36://Data address.
    case CMD56://Bit0 = RD/WR.
    case ACMD41://Argument is OCR without busy.
    case CMD6: //HSMMC : SWITCH command. For ACMD6:Arg.is Bus Width.
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=0 ;//Short resp.
      tCmdInfo.CmdReg.Bits.response_expect=1 ;
      tCmdInfo.dwCmdArg = dwcmdarg;

      if(nCmdID==SDIO_ABORT)
         tCmdInfo.CmdReg.Bits.stop_abort_cmd = 1;
             
      break; 

    // 4. STOP/ABORT command.
    case CMD12://Stop command.
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 1;//STOP command.
      tCmdInfo.CmdReg.Bits.response_length=0 ;//Short resp.
      tCmdInfo.CmdReg.Bits.response_expect=1 ;

      break; 

    // 5. Argument is RCA.Found from the Card_Info structure.
    case CMD7:
    case CMD13:
    case CMD55:
      //Copy RCA from card info structure into 
      //argument.RCA is from 16-31 bits.
      tCmdInfo.dwCmdArg = 
             Card_Info[ncardno].Card_RCA << 16 ;
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=0 ;//Short resp.
      tCmdInfo.CmdReg.Bits.response_expect=1 ;

      break; 

    case CMD15://No response
      //Copy RCA from card info structure into 
      //argument.RCA is from 16-31 bits.
      tCmdInfo.dwCmdArg = 
             Card_Info[ncardno].Card_RCA << 16 ;
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=0 ;
      tCmdInfo.CmdReg.Bits.response_expect=0 ;//No response

      break; 

    case CMD9://Long response
    case CMD10:
      //Copy RCA from card info structure into 
      //argument.RCA is from 16-31 bits.
      tCmdInfo.dwCmdArg = 
             Card_Info[ncardno].Card_RCA << 16 ;
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=1 ;//Long response
      tCmdInfo.CmdReg.Bits.response_expect=1 ;

      break;

    case CMD5://IO_SEND_OP_COND command.
      tCmdInfo.CmdReg.Bits.stop_abort_cmd = 0;
      tCmdInfo.CmdReg.Bits.response_length=0 ;//Short resp.
      tCmdInfo.CmdReg.Bits.response_expect=1 ;//No response
      tCmdInfo.dwCmdArg = dwcmdarg;
                     
      break;

    default:
      printk("form_send_cmd:CMDID not matched. \n");
      return Err_CMD_Not_Supported;

  }//End of switch statement.
  //Default parameters.
  //printk("4444444444444444444444444\n");
  tCmdInfo.CmdReg.Bits.start_cmd =1;
  tCmdInfo.CmdReg.Bits.Update_clk_regs_only=0;//Normal command.
    
  if(nCmdID==CMD0 || nCmdID==SDIO_RESET)
    //Send initialisation sequence for reset commands.
    //These commands are after power ON.So,ramp up time 
    //requd.
    tCmdInfo.CmdReg.Bits.send_initialization =1;//init.
  else
    tCmdInfo.CmdReg.Bits.send_initialization =0;//No init.
    
  tCmdInfo.dwByteCntReg=00 ;//Only for data command.
  tCmdInfo.dwBlkSizeReg=00 ;//Only for data command.
  tCmdInfo.CmdReg.Bits.transfer_mode=0 ;//Only for data command.
  tCmdInfo.CmdReg.Bits.read_write=00 ;//Only for data command.
  tCmdInfo.CmdReg.Bits.data_expected = 0;//No data command.
  if(nCmdID==CMD52)
     tCmdInfo.CmdReg.Bits.auto_stop=1;//Only for SDIO data transfer
     //Sets Memory or IO transfer.
  else
     tCmdInfo.CmdReg.Bits.auto_stop=00;
    
  //User settable.In all modes,this field should be 
  //actual card number.
  tCmdInfo.CmdReg.Bits.card_number = ncardno;
  
  tCmdInfo.CmdReg.Bits.wait_prvdata_complete=((nflags >> 1)&
                                              0x01);
    
  tCmdInfo.CmdReg.Bits.check_response_crc = ((nflags >> 2) &
                                              0x01);
  if((nCmdID==SDIO_RESET)||(nCmdID==SDIO_ABORT))
     tCmdInfo.CmdReg.Bits.cmd_index = CMD52 ;
  else
     tCmdInfo.CmdReg.Bits.cmd_index = nCmdID ;

 // printk("form_send_cmd : Send_cmd_to_host\n");
  //Send command:
 
  dwRetVal = Send_cmd_to_host(&tCmdInfo,pbcmdrespbuff,
                              NULL,//No error    response buffer
                              NULL,//No data response buffer
                              NULL,//No callback pointer
                              00,  //No flags from user
                              00,   //No retries.
                              FALSE,//No data command
                              FALSE,
                              FALSE );// 0= user address space buffer.

 //printk("send sdio_reset cmd!return value is %lx!\n",dwRetVal);                             
    
  return dwRetVal;
}
EXPORT_SYMBOL(form_send_cmd);
//--------------------------------------------------------------
//Check if command is allowed for given card type.

/*B.13.2.2.Validate_Command : Checks validity of given command 
   for given card.

   Validate_Command (int ncardno,unsigned long dwaddress,unsigned long dwParam2,
                     int nCommandType)

  *Called from : BSD::Internal functions.

  *Parameters:
         ncardno:(i/p):
         dwaddress:(i/p):
         dwCount:(i/p):
         nCommandType:(i/p):

   *Flow: 
        -Check if card number is valid and card is connected.
         //This will avoid duplication of code for all routines.
         Return error,if failed.

        -Perform command specific validation check.

          Switch(command)
          {
             Case Blk_Transfer:
               -Check if (dataaddr+unsigned char count) < card size ie
                address is not getting out of range.
 
               -Check if Bytecount is in integral multiples of 
                Blocksize.
                NumberofBlocks=CheckByteCountSize(dwByteCountReg,
                               dwBlkSizeReg) 
   
                If (remainder) 
                  return error;//unsigned char count not in multiples of
                               //block size.

             Case Stream_Xfer:
                 If (start+count) goes beyond card size.
                 If card is NOT MMC card,return error.

             Case Wr_Protect_Data:
               -Check if card supports write protect group enable.
                CSD::WP_GRP_ENABLE.

             Case sd_mmc_card_lock_Unlock:
                -If ERASE comamnd,is given,skip following checks as only
                 command unsigned char will be sent.
                -Check if array pointer pbNewPassword is not NULL.
                -Check that bOldPwdLength and bNewPwdLength is within
                 0 to 16.
                -If (bOldPwdLength > 0),check that array pointer 
                 pbOldPassword is not NULL.Else return error.

          }//End of case statement.

    Retutn error code.

dwaddress,unsigned long dwCount,

*/
int Validate_Command (int ncardno,unsigned long dwaddress,unsigned long dwParam2,
                      int nCommandType)
{
  unsigned long dwBlockSize ;
#ifdef HSMMC_SUPPORT
  unsigned long dwData=0;
#endif

  //Check if card number is valid and card is connected.
  //Check if card no.is less than total cards connected.
  if(ncardno >= MAX_CARDS)
    return Err_Invalid_CardNo;

  //Check if card is connected.Else return error.
  if(Card_Info[ncardno].Card_Connected != TRUE)
      {
          printk("#############Validate_Command Err_Card_Not_Connected!\n");
            return Err_Card_Not_Connected;
      }

  //Perform command specific validation check.

  switch(nCommandType)
  {
    case Blk_Transfer:
      if(Card_Info[ncardno].Card_type != 3){
          if((dwaddress+dwParam2) > Card_Info[ncardno].Card_size )
              {
                  printk("#############Validate_Command Blk_Transfer00\n");
               return Err_Addr_OutOfRange;
              }
          }
      else{
              if((dwaddress*512+dwParam2) > Card_Info[ncardno].Card_size )
                  {
                      return Err_Addr_OutOfRange;
                  }
          }
      //Check if unsigned char count ie dwParam2 is 0.
      if(dwParam2==0)
      {
              printk("#############Validate_Command Blk_Transfer22\n");
              return Err_Invalid_ByteCount;
          }              
            
        break;

    case Stream_Xfer:
      dwBlockSize = Card_Info[ncardno].Card_size;
      //Check if address is not getting out of range.
      if((dwaddress+dwParam2) > dwBlockSize )
          {
              printk("#############Validate_Command Stream_Xfer 00\n");
            return Err_Addr_OutOfRange;
          }            

      if(Card_Info[ncardno].Card_type!=MMC)
          {
              printk("#############Validate_Command Stream_Xfer 11\n");
            return Err_Invalid_CardType;
          }

      //Check if unsigned char count ie dwParam2 is 0.
      if(dwParam2==0)
          {
              printk("#############Validate_Command Stream_Xfer 22\n");
           return Err_Invalid_ByteCount;
          }

   #ifdef HSMMC_SUPPORT
      //For HSMMC,stream transfer allowed only for single bit
      //data width.Read width.
      //Check if card width bit for 4 or 8 bit, is not set.
      Read_Write_IPreg(CTYPE_off,&dwData,READFLG);
      if((dwData & (1<<ncardno)) || (dwData & (1<<(ncardno+16))) )
          {
        printk("#############Validate_Command Stream_Xfer 33\n");
            return Err_Invalid_CardWidth;
          }
   #endif

      break;

    case Wr_Protect_Data:
      //Check if (address) is within card size.
      if(dwaddress > Card_Info[ncardno].Card_size)
          {
              printk("#############Validate_Command Wr_Protect_Data 00\n");
            return Err_Addr_OutOfRange;    
          }

      //Check if Wr_protect feature enabled.So,no break used.
      //Enter in next case statement.
 
    case Rd_Protect_Data:
      //Check if card supports write protect group enable.
      if(!Card_Info[ncardno].CSD.Fields.wp_grp_enable)
          {
              printk("#############Validate_Command Rd_Protect_Data 00\n");
            return Err_WrProt_Not_Supported; 
          }

      break; 

    case Erase_Data:
      //dwParam2 = dwaddress2, dwParam1= dwaddress1
      //Check if (address) is within card size.
      if((dwaddress > Card_Info[ncardno].Card_size) ||
         (dwParam2 > Card_Info[ncardno].Card_size) )
          {
              printk("#############Validate_Command Erase_Data 00\n");
            return Err_Addr_OutOfRange;    
          }

      break;

    case Card_Lock_Unlock:
      break;

    case SDIO_Command:
      //Validate card number,card type(SDIO),function number(1-7)
      //dwParam2=function number, dwaddress= Not used.
      //SDIO card types are 4,5,6,7.
      if( (Card_Info[ncardno].Card_type & 0x4) != 4)
          {
              printk("#############Validate_Command SDIO_Command 00\n");
           return Err_Invalid_CardType;
          }
           
      //Check if function number is valid.
      if(dwParam2<0 || dwParam2 > 7)
          {
              printk("#############Validate_Command SDIO_Command 11\n");
           return Err_Invalid_FunctionNo;
          }
           
      break;

    case HSMMC_Command:
      if(!(Card_Info[ncardno].HSdata & 0x80) &&
         Card_Info[ncardno].Card_type!=MMC )
          {
              printk("#############Validate_Command HSMMC_Command\n");
            return Err_Invalid_CardType;
          }
           
      break;

    case HSSD_Command:
      if(!(Card_Info[ncardno].HSdata & 0x80) ||
         Card_Info[ncardno].Card_type ==MMC || 
         Card_Info[ncardno].Card_type ==SDIO_IO )
          {
        printk("#############Validate_Command HSSD_Command\n");
            return Err_Invalid_CardType;
          }
           
      break;

    default:
      //For unhandled command,return success as no validation
      //can be applied.
      return SUCCESS; 

  }//End of case statement.

  return SUCCESS; 
}

//--------------------------------------------------------------
/*B.13.2.4.Validate_Response:Checks response for any error bits 
  set.Command index indicates response type.

  Validate_Response(int nCmdIndex, unsigned char *pbResponse,int ncardno)

  *Called From:BSD::Internal Functions.
  *Parameters:

  *Flow:

*/
// **** Note: While receiving response,card sends out most
//signifiant unsigned char first and least unsigned char,at the last.
//So,RTL receives MSByte first.RTL internally stores this 
//MSByte at bits 31-24,and so on.So,software gets response
//data properly and no need for software to rearrange this data.
//It can straight way start using response register contents as
//they are.
int  Validate_Response(int nCmdIndex, unsigned char *pbResponse,
                       int ncardno,int nCompareState)
{
  unsigned long dwRetVal;

  //If user has not passed response buffer,don't check response
  if(pbResponse==NULL)
    return -EINVAL;

  switch(nCmdIndex)
  {
    case CMD1 ://SEND_OP_COND
      //Store OCR data in card info structure.
      //Since,response is received in proper format,no need to
      //unpack and rearrange it.
      Card_Info[ncardno].Card_OCR =  *((unsigned long*)pbResponse);

      return SUCCESS;

    case CMD9 ://SEND_CSD  R2 response.
      //Store CSD data in card info structure.
      if(Card_Info[ncardno].Card_type==MMC)
        return unpack_csd(pbResponse,&Card_Info[ncardno].CSD,0);
      else
        return unpack_csd(pbResponse,&Card_Info[ncardno].CSD,1);

      //During CMD2,response holds the CID data of the card which
      //wins in sending CIDs on command line.
      //After enumeration,CID data can be obtained through CMD10.
    case CMD2  ://ALL_SEND_CID 
    case CMD10 ://SEND_CID  R2 response.
      //Store CID data in card info structure.
      dwRetVal = mmc_unpack_cid(pbResponse,
                                &Card_Info[ncardno].CID);
      return dwRetVal;

    case CMD39://FAST_IO R4 response.
      return SUCCESS;

    case CMD40://GO_IRQ_STATE R5 response.
      return SUCCESS;

    case CMD53:
    case CMD52:
      return Unpack_CMD52_Resp(pbResponse);

    default://R1,R1b Response.
      //Checks if there is any error in response.
      //Second parameter is expected state of card prior
      //to execution of the command.To by-pass this check
      //pass this value as 0xff.
      dwRetVal = mmc_unpack_r1(pbResponse,nCompareState);
      return dwRetVal;
     
  }//End of switch statement.

}
//--------------------------------------------------------------
void sdio_copy_cccr(cccrreg *ptccccr,  int ncardno)
{
    //copy sdio_card cccr to peri driver
    return ;
}

void rca_consistent(int ncardno,int rca)
{
   if(ncardno!=0){
        printk("Warning! cardno must be zero, rca consistent fail!\n");
        return;
   }
   Card_Info[ncardno].Card_RCA = rca;
}
EXPORT_SYMBOL(rca_consistent);

void sd_mmc_copy_csd(CSDreg *ptcsd,  int ncardno)
{
  ptcsd->Fields.csd_structure            = Card_Info[ncardno].CSD.Fields.csd_structure;                 
  ptcsd->Fields.spec_vers                = Card_Info[ncardno].CSD.Fields.spec_vers   ;              
  ptcsd->Fields.taac                     = Card_Info[ncardno].CSD.Fields.taac       ;               
  ptcsd->Fields.nsac                     = Card_Info[ncardno].CSD.Fields.nsac      ;                
  ptcsd->Fields.tran_speed               = Card_Info[ncardno].CSD.Fields.tran_speed;                
  ptcsd->Fields.ccc                      = Card_Info[ncardno].CSD.Fields.ccc      ;                 
  ptcsd->Fields.read_bl_len              = Card_Info[ncardno].CSD.Fields.read_bl_len;
  ptcsd->Fields.read_bl_partial          = Card_Info[ncardno].CSD.Fields.read_bl_partial;           
  ptcsd->Fields.write_blk_misalign       = Card_Info[ncardno].CSD.Fields.write_blk_misalign;        
  ptcsd->Fields.read_blk_misalign        = Card_Info[ncardno].CSD.Fields.read_blk_misalign;         
  ptcsd->Fields.dsr_imp                  = Card_Info[ncardno].CSD.Fields.dsr_imp;                   
  ptcsd->Fields.c_size                   = Card_Info[ncardno].CSD.Fields.c_size;                    
  ptcsd->Fields.vdd_r_curr_min           = Card_Info[ncardno].CSD.Fields.vdd_r_curr_min;            
  ptcsd->Fields.vdd_r_curr_max           = Card_Info[ncardno].CSD.Fields.vdd_r_curr_max;            
  ptcsd->Fields.vdd_w_curr_min           = Card_Info[ncardno].CSD.Fields.vdd_w_curr_min;            
  ptcsd->Fields.vdd_w_curr_max           = Card_Info[ncardno].CSD.Fields.vdd_w_curr_max;            
  ptcsd->Fields.c_size_mult              = Card_Info[ncardno].CSD.Fields.c_size_mult;               
  ptcsd->Fields.erase.v22.sector_size    = Card_Info[ncardno].CSD.Fields.erase.v22.sector_size;     
  ptcsd->Fields.erase.v22.erase_grp_size = Card_Info[ncardno].CSD.Fields.erase.v22.erase_grp_size;  
  ptcsd->Fields.erase.v31.erase_grp_size = Card_Info[ncardno].CSD.Fields.erase.v31.erase_grp_size;  
  ptcsd->Fields.erase.v31.erase_grp_mult = Card_Info[ncardno].CSD.Fields.erase.v31.erase_grp_mult;  
  ptcsd->Fields.wp_grp_size  = Card_Info[ncardno].CSD.Fields.wp_grp_size ;  
  ptcsd->Fields.wp_grp_enable            = Card_Info[ncardno].CSD.Fields.wp_grp_enable;             
  ptcsd->Fields.default_ecc              = Card_Info[ncardno].CSD.Fields.default_ecc;               
  ptcsd->Fields.r2w_factor               = Card_Info[ncardno].CSD.Fields.r2w_factor;                
  ptcsd->Fields.write_bl_len             = Card_Info[ncardno].CSD.Fields.write_bl_len;            
  ptcsd->Fields.write_bl_partial         = Card_Info[ncardno].CSD.Fields.write_bl_partial;          
  ptcsd->Fields.file_format_grp          = Card_Info[ncardno].CSD.Fields.file_format_grp;           
  ptcsd->Fields.copy                     = Card_Info[ncardno].CSD.Fields.copy;                      
  ptcsd->Fields.perm_write_protect       = Card_Info[ncardno].CSD.Fields.perm_write_protect;        
  ptcsd->Fields.tmp_write_protect        = Card_Info[ncardno].CSD.Fields.tmp_write_protect;         
  ptcsd->Fields.file_format              = Card_Info[ncardno].CSD.Fields.file_format     ;          
  ptcsd->Fields.ecc                      = Card_Info[ncardno].CSD.Fields.ecc;            

      return; 
}

//--------------------------------------------------------------
int sd_mmc_check_card(int nslotno)
{
  //printk(" : Card_Insert = %d \n",Card_Info[nslotno].Card_Insert);
  if(Card_Info[nslotno].Card_type < 4){
        if(Card_Info[nslotno].Card_Connected)
             return Card_Info[nslotno].Card_Insert;
        else
            return 0;
  }
  else
         return 0;
}

int sdio_check_card(int ncardno)
{
  //printk(" : Card_Insert = %d \n",Card_Info[nslotno].Card_Insert);
  if(Card_Info[ncardno].Card_type & 0x04){
        if(Card_Info[ncardno].Card_Connected)
             return Card_Info[ncardno].Card_Insert;
        else
            return 0;
  }
  else
         return 0;
}

int Check_Card_Eject(int nslotno)
{
  //printk(" : Card_Eject = %d \n",Card_Info[nslotno].Card_Eject);
  return Card_Info[nslotno].Card_Eject;
}

void read_sdio_capability(int ncardno,card_capability *capability)
{
    if(ncardno!=0){
        printk("Warning! ncardno must be zero, read sdio capability fail!\n");
        return;
    }
    capability->num_of_io_funcs = Card_Info[0].no_of_io_funcs;
    capability->memory_yes = Card_Info[0].memory_yes;
    capability->rca = Card_Info[0].Card_RCA;
    capability->ocr = Card_Info[0].Card_OCR;
}


//==============================================================
//B.14.Driver specific.:Linux specific.

//--------------------------------------------------------------


int SD_MMC_BUS_ioctl(struct inode *inode, struct file *file,
               unsigned int cmd, unsigned long arg)
{
  int val,ncardno;
  int *ptSlotNo; 
  int data_length;
  char *buf,*buf_tmp;
//  unsigned long buf_tmp_addr;

  int i;
  unsigned char response[16];

  DATA_CMD_INFO *ptDataCmd;
  DATA_CMD_INFO ptDataCmd1;
  SDIO_CMD_INFO *ptSDIOCmd;
  CARD_PROTECT_ERASE_CMD *ptProtEraseCmd;
  RD_WR_IPREG_CMD * ptRdWrIPregCmd;

  APPLICATION_CARD_LOCK_CMD  *ptcardlockcmd;
  CARD_GEN_CMD  *ptCardgencmd;

  unsigned long dwData;
  unsigned long dwRetVal=0;

  id_reg_info tIPreginfo;

  //for testing only.
  unsigned char DatatBuff[64];
  unsigned char bByteData=0;

  //for testing ends.


  val=-EINVAL; 

  switch (cmd)
  {
     unsigned long transfer_starttime,tranfer_endtime;
     case GENERAL_ONE:
       
      printk("SD MMC Driver Version : %s\n",SDMMC_DRV_VER);
    
      //Get card number passed in argument.
      ncardno= *((int*)arg);
      printk("Card no.entered is = %x \n",ncardno);
        
      //Read_total_cards_connected
      dwRetVal= Read_total_cards_connected();
      printk("Total cards connected = %lx\n ",dwRetVal);  

      //Read_Op_Mode
      dwRetVal= Read_Op_Mode();
      printk("OPerating mode of driver = %lx\n ",dwRetVal);  

      //sd_mmc_sdio_get_id 
      dwRetVal= sd_mmc_sdio_get_id(&tIPreginfo);
      printk("UserID number    = %lx\n ",tIPreginfo.dwUserID);  
      printk("VersionID number = %lx\n ",tIPreginfo.dwVerID);  

      //Read card info.
      //Read_RCA 
      dwRetVal= sd_mmc_read_rca(ncardno);
      printk("RCA of card at card number %x = %lx\n ",ncardno,dwRetVal);  

      //sd_mmc_sdio_get_card_type
      dwRetVal= sd_mmc_sdio_get_card_type(ncardno); 
      printk("Type of card number %x = %lx\n ",ncardno,dwRetVal);  

      //sd_mmc_sdio_get_currstate 
      // dwRetVal= sd_mmc_sdio_get_currstate (ncardno); 
      //printk("Current state of card number %x  = %lx\n ",ncardno,dwRetVal);  

      //sd_mmc_get_card_size
      printk("Size of card number %x  = %llu\n ",
                  ncardno,    sd_mmc_get_card_size(ncardno));  

      //Get_Card_Atributes 
      dwRetVal= Get_Card_Attributes (ncardno);
      printk("Attributes of card number %x = %lx\n ",ncardno,dwRetVal);  

      return 0;

    case GENERAL_TWO:
           set_clk_frequency(0,82,(unsigned char *)&data_length);
           set_ip_parameters(6,(unsigned long*)(&data_length));    
           set_clk_frequency(0,82,(unsigned char *)&data_length);
           set_ip_parameters(6,(unsigned long*)(&data_length));            
           set_clk_frequency(0,82,(unsigned char *)&data_length);
           set_ip_parameters(6,(unsigned long*)(&data_length));               
       if((dwRetVal = form_send_cmd(0,CMD0,00,(unsigned char*)(&data_length),1))==SUCCESS)    
       {
            printk(" send cmd0 sucess!\n");
       }
      return 0;

    case INITIALISE:
//        return Enumerate_SD_card_Stack();
      return sd_mmc_sdio_initialise();

    case SEND_RAW_COMMAND:
      ptDataCmd = (DATA_CMD_INFO*) arg;

      dwRetVal = sd_mmc_send_raw_data_cmd(ptDataCmd->dwCmdRegParams,
                                   ptDataCmd->dwCmdArgReg ,
                                   ptDataCmd->dwByteCount,
                                   ptDataCmd->dwBlockSizeReg,
                                   ptDataCmd->pbCmdRespBuff,
                                   ptDataCmd->pbErrRespBuff,
                                   ptDataCmd->pbDataBuff,
                                   ptDataCmd->callback,
                                   ptDataCmd->nNoOfRetries,
                                   ptDataCmd->nFlags);
      return dwRetVal;

    case RD_WR_BLK_DATA:
      data_length = *((int*)arg);
      data_length=2048;
#ifdef DMA_ENABLE
      data_length = 1024;
  //    buf = dma_alloc_coherent(NULL,PAGE_SIZE,&buf_phy,GFP_DMA);
      buf=(char *)kmalloc((unsigned long)(data_length*2),GFP_DMA);
  //    printk("dma_alloc_coherent alloc memory!buf=%x,buf_phy=%x\n",buf,buf_phy);
      printk("kmalloc alloc memory!buf=%x\n",(unsigned int)buf);      
#else
      buf=(char *)vmalloc((unsigned long)(data_length*2));
      printk("vmalloc alloc memory!\n");
#endif 
      
      if(!buf)
      {
        printk("memory alloc error!\n");
        return -1;
      }

  //    buf_tmp = __virt_to_bus((void *)buf);
  //    printk("buf_tmp=%x,buf_phy=%x\n",buf_tmp,buf_phy);
      
      printk("data_length=%d\n",data_length);
      buf_tmp=buf;
      memset(buf,0,data_length*2);
  
      for(i=0;i<data_length;i++)
      {
        *buf_tmp=222;
        buf_tmp++;            
      }
   //   transfer_starttime=jiffies;
        ptDataCmd1.nCardNo = 0;
        ptDataCmd1.dwAddr = 512;
        ptDataCmd1.dwByteCount = data_length;
        ptDataCmd1.pbDataBuff = buf;
        ptDataCmd1.pbCmdRespBuff = response;
        ptDataCmd1.pbErrRespBuff = NULL;
        ptDataCmd1.callback = NULL;
        ptDataCmd1.nNoOfRetries = 0;
        ptDataCmd1.nFlags= 0xd0;
      transfer_starttime=jiffies;
      dwRetVal = read_write_blkdata(ptDataCmd1.nCardNo,
                                    ptDataCmd1.dwAddr ,
                                    ptDataCmd1.dwByteCount,
                                    ptDataCmd1.pbDataBuff,
                                    ptDataCmd1.pbCmdRespBuff,
                                    ptDataCmd1.pbErrRespBuff,
                                    ptDataCmd1.callback,
                                    ptDataCmd1.nNoOfRetries,
                                    ptDataCmd1.nFlags);
      tranfer_endtime=jiffies;
    
       if(dwRetVal)
       {
            printk("data write error!\n");
            printk("the return info is 0x%x",(unsigned int)dwRetVal);
       }
      
       printk("transfer_starttime=%u\n",(unsigned int)transfer_starttime);
       printk("tranfer_endtime=%u\n",(unsigned int)tranfer_endtime);
       transfer_starttime=tranfer_endtime-transfer_starttime;
       printk("used time is %u ms\n",(unsigned int)transfer_starttime*10);

#ifdef DMA_ENABLE
   //  dma_free_coherent(NULL,PAGE_SIZE,(void *)buf,buf_phy);
     kfree(buf);
#else
      vfree(buf);
#endif
      return dwRetVal;//12;


    case RD_WR_BLK_DATA1:
      data_length=2048;
#ifdef DMA_ENABLE
      data_length = 512;
     // buf = dma_alloc_coherent(NULL,PAGE_SIZE,&buf_phy,GFP_DMA);
      buf=(char *)kmalloc((unsigned long)(data_length*2),GFP_DMA);
      printk("dma_alloc_coherent alloc memory!\n");
#else
      buf=(char *)vmalloc((unsigned long)(data_length*2));
      printk("vmalloc alloc memory!\n");
#endif 
      
      if(!buf)
      {
        printk("memory alloc error!\n");
        return -1;
      }

      
      printk("data_length=%d\n",data_length);
      buf_tmp=buf;
      memset(buf,0,data_length*2);
        ptDataCmd1.nCardNo = 0;
        ptDataCmd1.dwAddr = 512;
        ptDataCmd1.dwByteCount = data_length;
        ptDataCmd1.pbDataBuff = buf_tmp;
        ptDataCmd1.pbCmdRespBuff = response;
        ptDataCmd1.pbErrRespBuff = NULL;
        ptDataCmd1.callback = NULL;
        ptDataCmd1.nNoOfRetries = 0;
        ptDataCmd1.nFlags= 0x50;
      memset(buf_tmp,0,data_length);
      dwRetVal = read_write_blkdata(ptDataCmd1.nCardNo,
                                    ptDataCmd1.dwAddr ,
                                    ptDataCmd1.dwByteCount,
                                    ptDataCmd1.pbDataBuff,
                                    ptDataCmd1.pbCmdRespBuff,
                                    ptDataCmd1.pbErrRespBuff,
                                    ptDataCmd1.callback,
                                    ptDataCmd1.nNoOfRetries,
                                    ptDataCmd1.nFlags);
       if(dwRetVal)
       {
            printk("data read error!\n");
            printk("the return info is 0x%x",(unsigned int)dwRetVal);
       }  

      for(i=0;i<data_length;i++)
      {
        printk("data read is %d\n",buf_tmp[i]);
      }

#ifdef DMA_ENABLE
     //dma_free_coherent(NULL,PAGE_SIZE,(void *)buf,buf_phy);
      kfree(buf);
#else
      vfree(buf);
#endif
      return dwRetVal;//12;




    case RD_WR_BLK_DATA_USER_BUF:
      ptDataCmd = (DATA_CMD_INFO*) arg;

      dwRetVal = read_write_blkdata(ptDataCmd->nCardNo,
                                    ptDataCmd->dwAddr ,
                                    ptDataCmd->dwByteCount,
                                    ptDataCmd->pbDataBuff,
                                    ptDataCmd->pbCmdRespBuff,
                                    ptDataCmd->pbErrRespBuff,
                                    ptDataCmd->callback,
                                    ptDataCmd->nNoOfRetries,
                                    ptDataCmd->nFlags);

      return dwRetVal;//12;
      
    
    case RD_WR_STREAM_DATA:
        
      data_length = *((int*)arg);
      buf=(char *)vmalloc((unsigned long)(data_length*2));
      if(!buf)
      {
        printk("memory alloc error!\n");
      }
      
      buf_tmp=buf;
      memset(buf,0,data_length*2);
      
      for(i=0;i<data_length;i++)
      {
        *buf_tmp=i%256;
        buf_tmp++;            
      }
      
        ptDataCmd1.nCardNo = 0;
        ptDataCmd1.dwAddr = 0;
        ptDataCmd1.dwByteCount = data_length;
        ptDataCmd1.pbDataBuff = buf;
        ptDataCmd1.pbCmdRespBuff = response;
        ptDataCmd1.pbErrRespBuff = NULL;
        ptDataCmd1.callback = NULL;
        ptDataCmd1.nNoOfRetries = 0;
        ptDataCmd1.nFlags= 0xd0;

      dwRetVal = read_write_streamdata(ptDataCmd1.nCardNo,
                                    ptDataCmd1.dwAddr ,
                                    ptDataCmd1.dwByteCount,
                                    ptDataCmd1.pbDataBuff,
                                    ptDataCmd1.pbCmdRespBuff,
                                    ptDataCmd1.pbErrRespBuff,
                                    ptDataCmd1.callback,
                                    ptDataCmd1.nNoOfRetries,
                                    ptDataCmd1.nFlags);

       if(dwRetVal)
       {
            printk("data write error!\n");
            printk("the return info is 0x%x",(unsigned int)dwRetVal);
       }
       
        ptDataCmd1.nCardNo = 0;
        ptDataCmd1.dwAddr = 0;
        ptDataCmd1.dwByteCount = data_length;
        ptDataCmd1.pbDataBuff = buf_tmp;
        ptDataCmd1.pbCmdRespBuff = response;
        ptDataCmd1.pbErrRespBuff = NULL;
        ptDataCmd1.callback = NULL;
        ptDataCmd1.nNoOfRetries = 0;
        ptDataCmd1.nFlags= 0x50;

      dwRetVal = read_write_streamdata(ptDataCmd1.nCardNo,
                                    ptDataCmd1.dwAddr ,
                                    ptDataCmd1.dwByteCount,
                                    ptDataCmd1.pbDataBuff,
                                    ptDataCmd1.pbCmdRespBuff,
                                    ptDataCmd1.pbErrRespBuff,
                                    ptDataCmd1.callback,
                                    ptDataCmd1.nNoOfRetries,
                                    ptDataCmd1.nFlags);
       if(dwRetVal)
       {
            printk("data read error!\n");
            printk("the return info is 0x%x",(unsigned int)dwRetVal);
       }  

      for(i=0;i<data_length;i++)
      {
        if(buf[i] != buf_tmp[i])
        {
            printk("data transfer error!\n");
            printk("the %dth data; in = %d,out = %d\n",i,buf[i],buf_tmp[i]);
        }           
      }
      vfree(buf);        
/*        
      ptDataCmd = (DATA_CMD_INFO*) arg;

      dwRetVal = read_write_streamdata(ptDataCmd->nCardNo,
                                       ptDataCmd->dwAddr ,
                                       ptDataCmd->dwByteCount,
                                       ptDataCmd->pbDataBuff,
                                       ptDataCmd->pbCmdRespBuff,
                                       ptDataCmd->pbErrRespBuff,
                                       ptDataCmd->callback,
                                       ptDataCmd->nNoOfRetries,
                                       ptDataCmd->nFlags);
*/
      return  dwRetVal;//13;         

    case SDIO_RD_WR_EXTENDED:
      ptSDIOCmd = (SDIO_CMD_INFO*) arg;

      dwRetVal = sdio_rdwr_extended(ptSDIOCmd->nCardNo,
                                    ptSDIOCmd->nFunctionNo,
                                    ptSDIOCmd->nRegAddr,
                                    ptSDIOCmd->dwByteCount, 
                                    ptSDIOCmd->pbDataBuff,
                                    ptSDIOCmd->pbCmdRespBuff,
                                    ptSDIOCmd->pbErrRespBuff,
                                    ptSDIOCmd->nCmdOptions,
                                    ptSDIOCmd->callback,
                                    ptSDIOCmd->nNoOfRetries,
                                    ptSDIOCmd->nFlags);

      return dwRetVal;         

    case SDIO_RD_WR_DIRECT:
      ptSDIOCmd = (SDIO_CMD_INFO*) arg;

      dwRetVal = sdio_rdwr_direct(ptSDIOCmd->nCardNo,
                                  ptSDIOCmd->nFunctionNo,
                                  ptSDIOCmd->nRegAddr,
                                  ptSDIOCmd->pbDataBuff,
                                  ptSDIOCmd->pbCmdRespBuff,
                                  (ptSDIOCmd->nFlags&0x80)>>7,//fWrite
                                  ptSDIOCmd->fRAW,
                                  ptSDIOCmd->nFlags);
      return dwRetVal;         

    case SDIO_RESUME_TRANSFER:
      ptSDIOCmd = (SDIO_CMD_INFO*) arg;

      dwRetVal = sdio_resume_transfer(ptSDIOCmd->nCardNo,
                                 ptSDIOCmd->nFunctionNo,
                                 ptSDIOCmd->dwByteCount, 
                                 ptSDIOCmd->pbDataBuff,
                                 ptSDIOCmd->pbCmdRespBuff,
                                 ptSDIOCmd->pbErrRespBuff,
                                 ptSDIOCmd->callback,
                                 ptSDIOCmd->nNoOfRetries,
                                 ptSDIOCmd->nFlags);
      return dwRetVal;         

    case WR_PROTECT_CARD:
      ptProtEraseCmd = (CARD_PROTECT_ERASE_CMD*) arg;

      dwRetVal = sd_mmc_wr_protect_carddata(ptProtEraseCmd->nCardNo,
                                     ptProtEraseCmd->dwAddr,
                                     ptProtEraseCmd->pbCmdRespBuff,
                                     ptProtEraseCmd->fSet);
      return dwRetVal;//17;         

    case SET_CLR_WR_PROTECT://Set Temporary/permanent  wr protect bit in CSD reg.
      ptProtEraseCmd = (CARD_PROTECT_ERASE_CMD*) arg;

      dwRetVal = sd_mmc_set_clr_wrprot(ptProtEraseCmd->nCardNo,
                                ptProtEraseCmd->fflag1,
                                ptProtEraseCmd->fSet);
      return  dwRetVal;//18;         


    case GET_WR_PROTECT_STATUS:
      ptProtEraseCmd = (CARD_PROTECT_ERASE_CMD*) arg;

      dwData = 00;
      dwRetVal = sd_mmc_get_wrprotect_status(ptProtEraseCmd->nCardNo,
                                      ptProtEraseCmd->dwAddr,
                                      &dwData);

      //Directly using pointer supplied by user space 
      //application may give error.
      if(dwRetVal==SUCCESS)
        return put_user(dwData , (unsigned long *)ptProtEraseCmd->dwDataParam1);
      else 
        return dwRetVal;//19;         

    case ERASE_CARD_DATA:

      ptProtEraseCmd = (CARD_PROTECT_ERASE_CMD*) arg;

      dwRetVal = sd_mmc_erase_data(ptProtEraseCmd->pbCmdRespBuff,
                                 ptProtEraseCmd->nCardNo,
                                 ptProtEraseCmd->dwDataParam1,
                                 ptProtEraseCmd->dwDataParam2);
      return dwRetVal;//20;         

    case READ_WRITE_IP_REG:
      ptRdWrIPregCmd = (RD_WR_IPREG_CMD*) arg;

      if(get_user(dwData, (u32 *)ptRdWrIPregCmd->pdwData))
         return -EFAULT;

      dwRetVal = Read_Write_IPreg( ptRdWrIPregCmd->nOffset,
                                   &dwData,
                                   ptRdWrIPregCmd->fWrite);
      if(dwRetVal!=SUCCESS)    return dwRetVal;

      if(ptRdWrIPregCmd->fWrite)
        return SUCCESS;
      else
        return put_user(dwData , (unsigned long *)ptRdWrIPregCmd->pdwData);
         
    case STOP_TRANSFER://22
      ncardno= *((int*)arg);
      printk("Going for STOP_Transfer for Card no. = %x \n",ncardno);

      dwRetVal =  sd_mmc_stop_transfer(ncardno);
      return dwRetVal;//22;         


    case CARD_LOCK://23
      ptcardlockcmd= ( APPLICATION_CARD_LOCK_CMD*)arg;
      //printk("Going for STOP_Transfer \n");

      dwRetVal = sd_mmc_card_lock(ptcardlockcmd->nCardNo,ptcardlockcmd->ptCardLockInfo,
                           ptcardlockcmd->pbCmdRespBuff);
      return dwRetVal;//23;         

    case SET_BLK_LENGTH://24
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      printk("Going for SET_BLK_LENGTH with cardno= %x, blksiz= %lx ,writeflg= %x\n",
             ptCardgencmd->nCardNo,ptCardgencmd->dwParam1,ptCardgencmd->nFlag );
             
      dwRetVal= SetBlockLength(ptCardgencmd->nCardNo,
                               ptCardgencmd->dwParam1 , //Block size to be set.
                               ptCardgencmd->nFlag , //Write/read. 1=write.
                               0);//No skip partial checking.
      return dwRetVal;//24;    
    
    //To set SD 4bit.
    case SET_SD_4BIT_MODE://25
      //Set 4 bit SD_Mode
      printk("Setting SD mode to 4 bit,for card 0.\n ");  
      dwRetVal =  sd_set_mode(0,TRUE);
      printk("Set_SD_mode returns %lx \n ",dwRetVal); 
      return dwRetVal;//25;         

    //to set SD 4bit.
    case SET_SD_ONEBIT_MODE://26
      //Set 1 bit SD_Mode
      printk("Setting SD mode to 1 bit,for card 0.\n ");  
      dwRetVal =  sd_set_mode(0,FALSE);
      printk("Set_SD_mode returns %lx \n ",dwRetVal); 
      return dwRetVal;//26;         

  case GENERAL_TESTING://27
    return 0;
  case ENUMERATE_SINGLE_CARD: //31
    ptSlotNo = (int*)arg;
    dwRetVal = Enumerate_Single_Card(*ptSlotNo);
    return dwRetVal;
  
  case SEND_COMMAND: //28
    ptDataCmd = (DATA_CMD_INFO*) arg;      
    dwRetVal = sd_mmc_send_command(ptDataCmd->dwCmdRegParams,
                            ptDataCmd->dwCmdArgReg,
                            ptDataCmd->pbCmdRespBuff);
    return dwRetVal;

  case READ_CARD_STATUS: //29
    ptCardgencmd = (CARD_GEN_CMD*)(arg);
    dwRetVal = form_send_cmd(ptCardgencmd->nCardNo,
                             CMD13,00,
                             (unsigned char*)ptCardgencmd->dwParam1,1);
    return dwRetVal;

  case ENABLE_DISABLE_SDIOFUNCTION: //30
       
    ptSDIOCmd = (SDIO_CMD_INFO*) arg;
    dwRetVal = sdio_enable_disable_function(ptSDIOCmd->nCardNo,
                                           ptSDIOCmd->nFunctionNo,
                                           ptSDIOCmd->nFlags);
    
    return dwRetVal;

  case ENABLE_DISABLE_SDIOIRQ: //32 

    ptSDIOCmd = (SDIO_CMD_INFO*) arg;
    dwRetVal = sdio_enable_disable_irq(ptSDIOCmd->nCardNo,
                                      ptSDIOCmd->nFunctionNo,
                                      ptSDIOCmd->nFlags);

    return dwRetVal;

  case SDIO_ABORT_TRANSFER: //33 

    ptSDIOCmd = (SDIO_CMD_INFO*) arg;
    dwRetVal = sdio_abort_transfer(ptSDIOCmd->nCardNo,ptSDIOCmd->nFunctionNo);

    return dwRetVal;

  case SDIO_SUSPEND_TRANSFER: //34 
        
    dwData = 0;
    ptSDIOCmd = (SDIO_CMD_INFO*) arg;
    dwRetVal = sdio_suspend_transfer(ptSDIOCmd->nCardNo, ptSDIOCmd->nFunctionNo,
                                &dwData);

    dwRetVal |= put_user(dwData,&ptSDIOCmd->dwByteCount);
    return dwRetVal;

  case SDIO_READ_WAIT: //35 

    ptSDIOCmd = (SDIO_CMD_INFO*) arg;
    dwRetVal = sdio_read_wait(ptSDIOCmd->nCardNo,(ptSDIOCmd->nFlags & 0x40));

    return dwRetVal;

  case SET_RESET_INTR_MODE: //36 
    ptCardgencmd = (CARD_GEN_CMD*)(arg);
    if(ptCardgencmd->nFlag)
       dwRetVal = Set_Interrupt_Mode( ptCardgencmd->Gen_callback,0);
    else
       dwRetVal = Send_CMD40_response();

    return dwRetVal;

  //HSSD functions.
/*  case HSSD_SEND_CMD6: //37 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = HSSD_Send_CMD6(ptCardgencmd->ncardno,
                                ptCardgencmd->Data,    //DataBuff
                                ptCardgencmd->dwParam1,//dwFunctions,
                                ptCardgencmd->nFlag    //fMode
                           );

      return dwRetVal;
*/
    case HSSD_FUNCTION_SWITCH: //38 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      if(!ptCardgencmd->nFlag)
         dwRetVal = HSSD_Function_Switch(ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //dwFunctionNos,
                          ptCardgencmd->Data,     //pbErrGroup,
                          ptCardgencmd->nFlag     //fBuffMode
                        );
      else //kernel space buffer
      {
        bByteData=0;
        dwRetVal = HSSD_Function_Switch(ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //dwFunctionNos,
                          &bByteData,             //pbErrGroup,
                          ptCardgencmd->nFlag     //fBuffMode
                        );
        if(put_user(bByteData,(unsigned char*)&ptCardgencmd->Data))
           dwRetVal = Err_BYTECOPY_FAILED;
      }

      return dwRetVal;

    case CHECK_HSSD_TYPE: //39 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = Check_HSSD_Type (ptCardgencmd->nCardNo, &bByteData);

      if(put_user(bByteData,(unsigned char*)&ptCardgencmd->dwParam1))
         printk("CHECK_HSSD_TYPE : Failed to copy unsigned char data.\n "); 
         
      return dwRetVal;

    //for testing only.
    case READ_SD_REG: //40 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      memset(DatatBuff,00,64);

      dwRetVal = Read_SD_Reg(ptCardgencmd->nCardNo,
                             ptCardgencmd->dwParam1, //nCmdIndex,
                             DatatBuff,              //pbDataBuff,
                             ptCardgencmd->dwParam2  //nBuffSize,
                           );

      if(copy_to_user(ptCardgencmd->Data,DatatBuff,
         ptCardgencmd->dwParam2)) 
         dwRetVal = Err_BYTECOPY_FAILED;

      return dwRetVal;

    case HSSD_CHECKFN_SUPPORT: //41 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      if(!ptCardgencmd->nFlag)
         dwRetVal = HSSD_CheckFn_Support(ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //dwFunctionNos,
                          ptCardgencmd->Data,     //pbStatusData,
                          ptCardgencmd->dwParam2, //nBuffSize,
                          ptCardgencmd->nFlag     //fBuffMode
                        );
      else //kernel space buffer
      {
        memset(DatatBuff,00,64);
        dwRetVal = HSSD_CheckFn_Support(ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //dwFunctionNos,
                          DatatBuff,              //pbStatusData,
                          ptCardgencmd->dwParam2, //nBuffSize,
                          ptCardgencmd->nFlag     //fBuffMode
                        );
        if (copy_to_user(ptCardgencmd->Data,DatatBuff,
            ptCardgencmd->dwParam2)) 
            dwRetVal = Err_BYTECOPY_FAILED;
      }
      return dwRetVal;

    //SDIO functions.
    case CHANGE_SDIO_BLKSIZE: //42 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = sdio_change_blksize (ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //FunctionNo,
                          ptCardgencmd->dwParam2  //BlockSize,
                        );
      return dwRetVal;

    case GET_DATA_FROM_TUPLE: //43 
 /*   ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = Get_Data_fromTuple (int ncardno, int nfunctionno,
                          unsigned int nTupleCode, 
                          unsigned int nFieldOffset,
                          unsigned int nBytes,  
                          unsigned char* pBuffer, 
                          unsigned int nBuffsize , 
                          int fBuffMode);
*/
      return dwRetVal;

    case ENABLE_MSTRPWR_CONTROL: //44 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = sdio_enable_masterpwrcontrol (ptCardgencmd->nCardNo,
                          ptCardgencmd->nFlag     //fEnable
                        );

      return dwRetVal;

    case CHANGE_FN_PWRMODE: //45 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = sdio_change_pwrmode_forfunction (ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //nFunctionNo,
                          ptCardgencmd->nFlag     //fLowPwrMode
                        );
      return dwRetVal;

    //HSMMC routines.
    case SET_HSMODE_SETTINGS: //46 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = Set_HSmodeSettings (ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //Mode type,
                          ptCardgencmd->dwParam2  //Balue for field
                        );
      return dwRetVal;

    case SET_CLEAR_EXTCSDBITS: //46 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = Set_Clear_EXTCSDbits (ptCardgencmd->nCardNo,
                          ptCardgencmd->dwParam1, //Value,
                          ptCardgencmd->dwParam2, //Index
                          ptCardgencmd->nFlag     //Set/Clear
                        );
      return dwRetVal;

    case BUS_TESTING: //46 
      ptCardgencmd = (CARD_GEN_CMD*)(arg);

      dwRetVal = Bus_Testing (ptCardgencmd->nCardNo,
                          &bByteData             //Data line status.
                        );
      if(put_user(bByteData,(unsigned char*)&ptCardgencmd->Data))
         dwRetVal = Err_BYTECOPY_FAILED;
        return dwRetVal;

    case HDIO_GETRO: 
    {
        GPIO_CARD_DETECT_IN;
        bByteData = GPIO_CARD_DETECT_READ;

        if (bByteData == 0x1)
        {
            return -EFAULT;
        }

        GPIO_WRITE_PRT_IN;
        bByteData = GPIO_WRITE_PRT_READ;

        i = bByteData;
        return copy_to_user((void __user *)arg, &i, sizeof(i))
            ? -EFAULT : 0;
    }

    default:
      printk(" : ioctl:Default \n");
      return val;

  }
}





