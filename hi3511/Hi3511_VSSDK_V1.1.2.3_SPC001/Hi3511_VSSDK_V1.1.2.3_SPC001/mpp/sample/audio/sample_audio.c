/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_audio.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/03/15
  Description   : 
  History       :
  1.Date        : 2008/03/15
    Author      : Hi3511MPP
    Modification: Created file    
******************************************************************************/    

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"
#include "tw2815.h"
#include "tlv320aic31.h"

#define SIO0_WORK_MODE   AIO_MODE_I2S_MASTER
#define SIO1_WORK_MODE   AIO_MODE_I2S_SLAVE


#define AUDIO_PAYLOAD_TYPE PT_ADPCMA    /* encoder type, PT_ADPCMA,PT_G711A,PT_AAC...*/

#define AUDIO_ADPCM_TYPE ADPCM_TYPE_IMA /* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define AUDIO_AAC_TYPE AAC_TYPE_AACLC   /* AAC_TYPE_AACLC, AAC_TYPE_EAAC, AAC_TYPE_EAACPLUS*/
#define G726_BPS MEDIA_G726_16K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */
#define AMR_FORMAT AMR_FORMAT_MMS       /* AMR_FORMAT_MMS, AMR_FORMAT_IF1, AMR_FORMAT_IF2*/
#define AMR_MODE AMR_MODE_MR122         /* AMR_MODE_MR122, AMR_MODE_MR102 ... */
#define AMR_DTX 1

#define AUDIO_POINT_NUM 320             /* point num of one frame 80,160,320,480,1024,2048*/


typedef struct
{
    HI_BOOL bStart;
    HI_BOOL bUseAEC;
    HI_S32 s32AiDev;
    HI_S32 s32AiChn;
    HI_S32 s32AencChn; 
    FILE *pfd;
    pthread_t stAencPid;
} SAMPLE_AENC_S;

typedef struct
{
    HI_BOOL bStart;
    HI_S32 s32AoDev;
    HI_S32 s32AoChn;
    FILE *pfd;
    pthread_t stAdecPid;
    pthread_t stAdecPid2;
} SAMPLE_ADEC_S;

typedef struct
{
    HI_BOOL bStart;
    HI_BOOL bUseAEC;
    HI_S32 s32AiDev;
    HI_S32 s32AiChn;
    HI_S32 s32AoDev;
    HI_S32 s32AoChn;
    pthread_t stPid;
} SAMPLE_AIAO_S;



#define TW2815_A_FILE   "/dev/misc/tw2815adev"
#define TW2815_B_FILE   "/dev/misc/tw2815bdev"
#define TLV320_FILE    "/dev/misc/tlv320aic31"


static SAMPLE_AENC_S s_stSampleAenc[8];
static SAMPLE_ADEC_S s_stSampleAdec;
static SAMPLE_AIAO_S s_stSampleAiAo;



HI_S32 SAMPLE_Audio_InitMpp()
{
    HI_S32 s32ret;
    MPP_SYS_CONF_S struSysConf;

    HI_MPI_SYS_Exit();
    
    struSysConf.u32AlignWidth = 64;
    s32ret = HI_MPI_SYS_SetConf(&struSysConf);
    if (HI_SUCCESS != s32ret)
    {
        printf("Set mpi sys config failed!\n");
        return s32ret;
    }
    s32ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32ret)
    {
        printf("Mpi init failed!\n");
        return s32ret;
    }
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_Audio_ExitMpp()
{
    HI_MPI_SYS_Exit();
    return 0;
}


/*  s32Seq:
    0：左0123，右89ab
    1：左0246，右1357   
    2：左0123，右4567 (8通道时使用)
    3：左0145，右2367 (4通道时使用)  */
/* s32Samplerate:[0:8k,1:16k]  s32Bitwidth:[0:16bit, 1:8bit]*/  
int SAMPLE_TW2815_CfgAudio(AIO_ATTR_S *pstAttr)
{   
    HI_S32 value;
    HI_S32 ret;
    tw2815_w_reg set_reg;
    HI_U32 i;
    int s_fd2815a = -1;
    int s_fd2815b = -1;

    HI_S32 s32Bitwidth,s32Chnnum,s32Seq,s32Samplerate;
        
    s32Bitwidth = (AUDIO_BIT_WIDTH_16==pstAttr->enBitwidth)?0:1;
    s32Chnnum = (AUDIO_BIT_WIDTH_16==pstAttr->enBitwidth)?4:8;
    s32Seq = (AUDIO_BIT_WIDTH_16==pstAttr->enBitwidth)?3:2;
    
    /* TW2815 only support 8kbps and 16kbps */
    s32Samplerate = (AUDIO_SAMPLE_RATE_8000==pstAttr->enSamplerate)?0:1;
    
    /* open TW2815 device */    
    s_fd2815a = open(TW2815_A_FILE,O_RDWR);
    s_fd2815b = open(TW2815_B_FILE,O_RDWR);
    if (s_fd2815a < 0 || s_fd2815a < 0)
    {
        printf("Warning:can't open tw2815\n");
        return -1;
    }
    
    value = s32Samplerate;
    ret=ioctl(s_fd2815a,TW2815_SET_ADA_SAMPLERATE,&value);
    if(ret <0)
    {
         printf("2815 sample errno:%d\n",errno);
         return -1;
    }

    value = s32Bitwidth;
    if(0!=ioctl(s_fd2815a,TW2815_SET_ADA_BITWIDTH,&value))
    {
        printf("2815 s32Bitwidth errno\n");
        return -1;
    }

    value = s32Chnnum;
    if(0!=ioctl(s_fd2815a,TW2815_SET_AUDIO_OUTPUT,&value))
    {
        printf("2815 output errno\n");
        return -1;
    }
    value = s32Seq;
    if(0!=ioctl(s_fd2815a,TW2815_SET_CHANNEL_SEQUENCE,&value))
    {
        printf("2815 output errno\n");
        return -1;
    }


//set 2815b    
    set_reg.value = 0x42;
    set_reg.addr = 0x6c;
    
    ioctl(s_fd2815b, TW2815_WRITE_REG,&set_reg);
    
    value = s32Samplerate;
    ioctl(s_fd2815b,TW2815_SET_ADA_PLAYBACK_SAMPLERATE,&value);
    
    value = s32Bitwidth;
    ioctl(s_fd2815b, TW2815_SET_ADA_PLAYBACK_BITWIDTH,&value);
    
    for(i =0; i < 4;i++)
    {
        value = i;
        ioctl(s_fd2815b,TW2815_SET_MIX_MUTE,&value);
    }
    
    ioctl(s_fd2815b, TW2815_REG_DUMP,&value);
    
    close(s_fd2815a);
    close(s_fd2815b);
    return 0;
}


/* s32Samplerate(0:8k, 1:16k )*/
HI_S32 SAMPLE_Tlv320_CfgAudio(HI_BOOL bPCMMode, HI_BOOL bMaster, HI_S32 s32Samplerate)
{
    //HI_S32 ret;
    int vol = 0x100;
    Audio_Ctrl audio_ctrl;
    int s_fdTlv = -1;
    
    audio_ctrl.chip_num = 0;

    s_fdTlv = open(TLV320_FILE,O_RDWR);
    if(s_fdTlv < 0)
    {
        printf("can't open tlv320,%s\n", TLV320_FILE);
        return -1;   
    }

    if(s32Samplerate == 0)
    {
        s32Samplerate = AC31_SET_8K_SAMPLERATE;
    }
    else if(s32Samplerate == 1)
    {
        s32Samplerate = AC31_SET_16K_SAMPLERATE;
    }
    else if(s32Samplerate == 2)
    {
        s32Samplerate = AC31_SET_24K_SAMPLERATE;
    }
    else if(s32Samplerate == 3)
    {
        s32Samplerate = AC31_SET_32K_SAMPLERATE;
    }
    else if(s32Samplerate == 4)
    {
        s32Samplerate = AC31_SET_44_1K_SAMPLERATE;
    }
    else 
    {
        return -1;
    }

    /* set transfer mode 0:I2S 1:PCM */
    audio_ctrl.trans_mode = bPCMMode;
    if (ioctl(s_fdTlv,SET_TRANSFER_MODE,&audio_ctrl))
    {
        printf("set tlv320aic31 trans_mode err\n");
        return -1;
    }

    if (HI_TRUE == bMaster)
    {
        /*set sample of DAC and ADC */
        audio_ctrl.sample = s32Samplerate;
        if (ioctl(s_fdTlv,SET_DAC_SAMPLE,&audio_ctrl))
        {
            printf("ioctl err1\n");
            return -1;
        }
        
        if (ioctl(s_fdTlv,SET_ADC_SAMPLE,&audio_ctrl))
        {
            printf("ioctl err2\n");
            return -1;
        }   
        printf("set tlv320aic31 sample 0x%x\n",s32Samplerate);
    }
        
    /*set volume control of left and right DAC */
    audio_ctrl.chip_num = 0;
    audio_ctrl.if_mute_route = 0;
    audio_ctrl.input_level = 0;
    ioctl(s_fdTlv,LEFT_DAC_VOL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_DAC_VOL_CTRL,&audio_ctrl);

    /*Right/Left DAC Datapath Control */
    audio_ctrl.if_powerup = 1;/*Left/Right DAC datapath plays left/right channel input data*/
    ioctl(s_fdTlv,LEFT_DAC_POWER_SETUP,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_DAC_POWER_SETUP,&audio_ctrl);
     
    audio_ctrl.ctrl_mode = bMaster;
    ioctl(s_fdTlv,SET_CTRL_MODE,&audio_ctrl); 
    printf("set tlv320aic31 master:%d\n",bMaster);
    
    /* (0:16bit 1:20bit 2:24bit 3:32bit) */
    audio_ctrl.data_length = 0;
    ioctl(s_fdTlv,SET_DATA_LENGTH,&audio_ctrl);
  
    
    /*DACL1 TO LEFT_LOP/RIGHT_LOP VOLUME CONTROL 82 92*/
    audio_ctrl.if_mute_route = 1;/* route*/
    audio_ctrl.input_level = vol; /*level control*/
    ioctl(s_fdTlv,DACL1_2_LEFT_LOP_VOL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,DACR1_2_RIGHT_LOP_VOL_CTRL,&audio_ctrl);
    
#if 0
    if (HI_TRUE == bMaster)
    {
        /* DAC OUTPUT SWITCHING CONTROL 41*/
        audio_ctrl.dac_path = 0;
        ioctl(s_fdTlv,DAC_OUT_SWITCH_CTRL,&audio_ctrl);
    }
#endif

    /* LEFT_LOP/RIGHT_LOP OUTPUT LEVEL CONTROL 86 93*/
    audio_ctrl.if_mute_route = 1;
    audio_ctrl.if_powerup = 1;
    audio_ctrl.input_level = 0;
    ioctl(s_fdTlv,LEFT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);
    ioctl(s_fdTlv,RIGHT_LOP_OUTPUT_LEVEL_CTRL,&audio_ctrl);

    /* LEFT/RIGHT ADC PGA GAIN CONTROL 15 16*/    
    audio_ctrl.if_mute_route =0;    
    audio_ctrl.input_level = 0;    
    ioctl(s_fdTlv,LEFT_ADC_PGA_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,RIGHT_ADC_PGA_CTRL,&audio_ctrl);        
    /*INT2L TO LEFT/RIGTH ADCCONTROL 17 18*/    
    audio_ctrl.input_level = 0;    
    ioctl(s_fdTlv,IN2LR_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,IN2LR_2_RIGTH_ADC_CTRL,&audio_ctrl);    
    /*IN1L_2_LEFT/RIGTH_ADC_CTRL 19 22*/    
    audio_ctrl.input_level = 0xf;    
    audio_ctrl.if_powerup = 1;    
    ioctl(s_fdTlv,IN1L_2_LEFT_ADC_CTRL,&audio_ctrl);    
    ioctl(s_fdTlv,IN1R_2_RIGHT_ADC_CTRL,&audio_ctrl); 

    close(s_fdTlv);
    return 0;
}

FILE *pfd_ai = NULL;
int SaveAiFrame(int chn, AUDIO_FRAME_S *pstFrame)
{
    if (!pfd_ai)
    {
        char file[32];
        sprintf(file, "ai_chn%d.data", chn);
        pfd_ai = fopen(file, "w+");
        if (!pfd_ai)
        {
            printf("open file %s for save ai data err\n", file);
            return -1;
        }
    }
    fwrite(pstFrame->aData, 1, pstFrame->u32Len, pfd_ai);
    return 0;
}


void *Aenc_proc(void *parg)
{
    HI_S32 s32ret;
    SAMPLE_AENC_S *pstAencCtl = (SAMPLE_AENC_S *)parg;
    AUDIO_FRAME_S stFrame;
    AUDIO_STREAM_S stStream;    

    while(pstAencCtl->bStart)
    {   
#if 1   /* don't use AEC */        
        /* get audio frame form ai chn */
        s32ret = HI_MPI_AI_GetFrame(pstAencCtl->s32AiDev,pstAencCtl->s32AiChn,&stFrame,NULL,HI_IO_BLOCK);
        if(HI_SUCCESS != s32ret)
        {   
            printf("get ai frame err:0x%x ai(%d,%d)\n",s32ret,pstAencCtl->s32AiDev,pstAencCtl->s32AiChn);       
            break;
        }
        /* send audio frame to aenc chn */
        s32ret = HI_MPI_AENC_SendFrame(pstAencCtl->s32AencChn,&stFrame,NULL);
        if (s32ret)
        {
            printf("send audio frame to aenc chn %d err:%x\n", pstAencCtl->s32AencChn, s32ret); 
            break;
        }
#else   /*  if you use AEC */       
{
        AEC_FRAME_S stRefFrame;
        s32ret = HI_MPI_AI_GetFrame(pstAencCtl->s32AiDev,pstAencCtl->s32AiChn,&stFrame,&stRefFrame,HI_IO_BLOCK);
        if(HI_SUCCESS != s32ret)
        {   
            printf("get ai frame err:0x%x ai(%d,%d)\n",s32ret,pstAencCtl->s32AiDev,pstAencCtl->s32AiChn);       
            break;
        }
        
        if (HI_TRUE == stRefFrame.bValid)/* you should already enable AEC*/
        {
            s32ret = HI_MPI_AENC_SendFrame(pstAencCtl->s32AencChn,&stFrame,&stRefFrame.stRefFrame);
        }
        else/* even AEC has enabled, sometimes RefFrame maybe invalid(eg. AO is not enable...)*/
        {
            s32ret = HI_MPI_AENC_SendFrame(pstAencCtl->s32AencChn,&stFrame,NULL);
        }
        if (s32ret)
        {
            printf("send audio frame to aenc chn %d err:%x\n", pstAencCtl->s32AencChn, s32ret); 
            break;
        }
}
#endif
        /* get stream from aenc chn */
        s32ret = HI_MPI_AENC_GetStream(pstAencCtl->s32AencChn, &stStream);
        if (HI_SUCCESS != s32ret )
        {
            printf("get stream from aenc chn %d fail \n", pstAencCtl->s32AencChn);  
            break;
        }
        
        /* save audio stream to file */
        fwrite(stStream.pStream,1,stStream.u32Len, pstAencCtl->pfd);
        
        HI_MPI_AENC_ReleaseStream(pstAencCtl->s32AencChn, &stStream);
    }
    
    fclose(pstAencCtl->pfd);
    return NULL;
}

void *Adec_proc(void *parg)
{
    HI_S32 s32ret;
    AUDIO_DEV AoDev;
    AO_CHN AoChn;
    HI_S32 s32AdecChn = 0;
    AUDIO_STREAM_S stAudioStream;
    AUDIO_FRAME_INFO_S stAudioFrameInfo;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;
    FILE *pfd = NULL;
    HI_U8 *pu8AudioStream = NULL;
    
    AoDev = pstAdecCtl->s32AoDev;    
    AoChn = pstAdecCtl->s32AoChn;    
    pfd   = pstAdecCtl->pfd;
    
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        return NULL;
    }
    
    /* read stream of one whole frame from file, then send to decoder by PACK_MODE,
    here we use HISI voice frame header to parse every frame(AMR/AAC not support),
    of course, you can parse by your own package format */
    
    while (HI_TRUE == pstAdecCtl->bStart)
    {
        HI_U32 u32RawDataLen;
        
        /* 
            Hisi Voice frame header(exclude AMR and AAC):
                 reseve          frame type       data len         count     
            |-----8bit[0]----|-----8bit[1]----|----8bit[2]---|----8bit[3]----|
        */
        
        /* read HISI audio frame header (4 bytes) */
        if (fread(&pu8AudioStream[0], 1, 4, pfd) != 4)
        {
            break;
        }
        u32RawDataLen = pu8AudioStream[2]*2;/* Notice: 'data len' is by u16 */
        
        /* read raw data from file */
        if (fread(&pu8AudioStream[4], 1, u32RawDataLen, pfd) != u32RawDataLen)
        {
            perror("adec fread raw data");
            break;
        }
        
        /* send to ADEC should contain frame header*/
        stAudioStream.pStream = &pu8AudioStream[0];
        stAudioStream.u32Len = u32RawDataLen + 4;     
        
        s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream);
        if (s32ret)
        {
            printf("send stream to adec fail,err:%x\n", s32ret);
            break;
        }

        /* get audio frme from adec after decoding*/
        s32ret = HI_MPI_ADEC_GetData(s32AdecChn, &stAudioFrameInfo);
        if (HI_SUCCESS != s32ret)
        {
            printf("adec get data err\n");    
            break;
        }

        /* send audio frme to ao */
        s32ret = HI_MPI_AO_SendFrame(AoDev, AoChn, stAudioFrameInfo.pstFrame, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {   
            //printf("ao send frame err:0x%x\n",s32ret);        
            return NULL;
        }

        s32ret = HI_MPI_ADEC_ReleaseData(s32AdecChn, &stAudioFrameInfo);     
        if (HI_SUCCESS != s32ret)
        {
            //printf("adec release data err\n");    
            break;
        }
    }
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;

    return NULL;
}

void *Adec_sendao_proc(void *parg)
{
    HI_S32 s32ret;
    AUDIO_DEV AoDev;
    AO_CHN AoChn;
    HI_S32 s32AdecChn = 0;
    AUDIO_FRAME_INFO_S stAudioFrameInfo;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;
    AoDev = pstAdecCtl->s32AoDev;    
    AoChn = pstAdecCtl->s32AoChn;
        
    while(HI_TRUE == pstAdecCtl->bStart)
    {
        /* get audio frme form adec */
        s32ret = HI_MPI_ADEC_GetData(s32AdecChn, &stAudioFrameInfo);
        if (HI_SUCCESS != s32ret)
        {
            //printf("adec get data err\n");    
            break;
        }
        //printf("start send ao \n");
        /* send audio frme to ao */
        s32ret = HI_MPI_AO_SendFrame(AoDev, AoChn, stAudioFrameInfo.pstFrame, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {   
            //printf("ao send frame err:0x%x\n",s32ret);        
            return NULL;
        }
        //printf("over send ao \n");
        s32ret = HI_MPI_ADEC_ReleaseData(s32AdecChn, &stAudioFrameInfo);     
        if (HI_SUCCESS != s32ret)
        {
            //printf("adec release data err\n");    
            break;
        }
    }
    
    return NULL;
}

void * Adec_readfile_proc(void *parg)
{
    HI_S32 s32ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 128;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn = 0;
    HI_U8 *pu8AudioStream = NULL;
    SAMPLE_ADEC_S *pstAdecCtl = (SAMPLE_ADEC_S *)parg;    
    FILE *pfd = pstAdecCtl->pfd;
    
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        return NULL;
    }

    while (HI_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);          
        if (u32ReadLen <= 0)
        {            
            perror("adec fread");
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }
        stAudioStream.u32Len = u32ReadLen;

        s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream);
        if (s32ret)
        {
            break;
        }
    }
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;
    return NULL;
}

void *AiAo_proc(void *parg)
{
    HI_S32 s32ret;
    SAMPLE_AIAO_S *pstAiAoCtl = (SAMPLE_AIAO_S *)parg;
    AUDIO_FRAME_S stFrame;

    while(HI_TRUE == pstAiAoCtl->bStart)
    {
        /* get audio frame form ai chn */
        s32ret = HI_MPI_AI_GetFrame(pstAiAoCtl->s32AiDev,pstAiAoCtl->s32AiChn,&stFrame,NULL,HI_IO_BLOCK);
        if(HI_SUCCESS != s32ret)
        {   
            printf("get ai frame err:0x%x ai(%d,%d)\n",s32ret,pstAiAoCtl->s32AiDev,pstAiAoCtl->s32AiChn);       
            break;
        }

        /* save ai frame for debug */
        //SaveAiFrame(pstAiAoCtl->s32AiChn,&stFrame);
        
        /* send audio frme to ao */
        s32ret = HI_MPI_AO_SendFrame(pstAiAoCtl->s32AoDev, pstAiAoCtl->s32AoChn, &stFrame, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {   
            printf("ao send frame err:0x%x\n",s32ret);
            break;
        }         
    }
    
    return NULL;
}

HI_S32 SAMPLE_Audio_Aenc()
{
    HI_S32 s32ret;
    int i;
    HI_U32 u32ChnCnt = 1;
    AIO_ATTR_S stAttr;
    AENC_CHN_ATTR_S stAencAttr;    
    AUDIO_DEV AiDevId;
    SAMPLE_AENC_S *pstSampleAenc;
    AIO_MODE_E aenSIOMode[2] ;
    
    aenSIOMode[0] = SIO0_WORK_MODE;/* work mode of SIO 0 */
    aenSIOMode[1] = SIO1_WORK_MODE;/* work mode of SIO 1 */

    /* init MPP sys */
    s32ret = SAMPLE_Audio_InitMpp();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }                  

    AiDevId = 0;
    stAttr.enWorkmode = aenSIOMode[AiDevId];
    stAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;        
    stAttr.u32EXFlag = 1;/* should set 1, ext to 16bit */
    stAttr.u32FrmNum = 40;
    stAttr.u32PtNumPerFrm = AUDIO_POINT_NUM;
    if (PT_ADPCMA == AUDIO_PAYLOAD_TYPE && AUDIO_ADPCM_TYPE == ADPCM_TYPE_IMA) 
    {
        stAttr.u32PtNumPerFrm++;
    }
    else if (PT_AMR == AUDIO_PAYLOAD_TYPE)
    {
        stAttr.u32PtNumPerFrm = 160;
    }
    else if (PT_AAC == AUDIO_PAYLOAD_TYPE)
    {
        stAttr.u32PtNumPerFrm = (AUDIO_AAC_TYPE==AAC_TYPE_AACLC)?\
            AACLC_SAMPLES_PER_FRAME:AACPLUS_SAMPLES_PER_FRAME;
        stAttr.enSamplerate = AUDIO_SAMPLE_RATE_16000;
    }

    if (1 == AiDevId)
    {
        SAMPLE_TW2815_CfgAudio(&stAttr);
    }
    else
    {
        SAMPLE_Tlv320_CfgAudio((aenSIOMode[0]==AIO_MODE_PCM_SLAVE_NSTD||aenSIOMode[0]==AIO_MODE_PCM_SLAVE_STD)?1:0,
            (SIO0_WORK_MODE==AIO_MODE_I2S_MASTER)?0:1,
            (stAttr.enSamplerate==AUDIO_SAMPLE_RATE_8000)?0:1 );
    }    
    
    /* set ai public attr*/
    s32ret = HI_MPI_AI_SetPubAttr(AiDevId, &stAttr);
    if (HI_SUCCESS != s32ret)
    {
        printf("set ai %d attr err:0x%x\n", AiDevId,s32ret);
        return s32ret;
    }
    /* enable ai device*/
    s32ret = HI_MPI_AI_Enable(AiDevId);
    if (HI_SUCCESS != s32ret)
    {
        printf("enable ai dev %d err:0x%x\n", AiDevId, s32ret);
        return s32ret;
    }
    
    /* enable ai chnnel*/
    for (i=0; i<u32ChnCnt; i++)
    {        
        s32ret = HI_MPI_AI_EnableChn(AiDevId, i);
        if (HI_SUCCESS != s32ret)
        {
            printf("enable ai chn %d err:0x%x\n", i, s32ret);
            return s32ret;
        }
    }        

    /* set AENC channle attr */
    stAencAttr.enType = AUDIO_PAYLOAD_TYPE;
    stAencAttr.u32BufSize = 8;    
    if (PT_ADPCMA == stAencAttr.enType)
    {
        AENC_ATTR_ADPCM_S stAdpcmAenc;
        stAencAttr.pValue = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        AENC_ATTR_G711_S stAencG711;
        stAencAttr.pValue = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        AENC_ATTR_G726_S stAencG726;
        stAencAttr.pValue = &stAencG726;
        stAencG726.enG726bps = G726_BPS  ;      
    }
    else if (PT_AMR == stAencAttr.enType)
    {
        AENC_ATTR_AMR_S stAencAmr;
        stAencAttr.pValue = &stAencAmr;
        stAencAmr.enFormat = AMR_FORMAT ;
        stAencAmr.enMode = AMR_MODE ;
        stAencAmr.s32Dtx = AMR_DTX ;
    }
    else if (PT_AAC == stAencAttr.enType)
    {
        AENC_ATTR_AAC_S stAencAac;
        stAencAttr.pValue = &stAencAac;
        stAencAac.enAACType = AUDIO_AAC_TYPE;        
        stAencAac.enBitRate = AAC_BPS_128K;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;   
        stAencAac.enSmpRate = stAttr.enSamplerate;/*should equel to ADC */
        stAencAac.enSoundMode = stAttr.enSoundmode;
    }     
    else
    {
        printf("not supported type\n");
        return -1;
    }

    for (i=0; i<u32ChnCnt; i++)
    {
        HI_CHAR aszFileName[128];
        pstSampleAenc = &s_stSampleAenc[i];
        
        /* create aenc chn*/
        s32ret = HI_MPI_AENC_CreateChn(i, &stAencAttr);
        if (s32ret)
        {
            printf("create aenc chn %d err:0x%x\n", i,s32ret);
            return s32ret;
        }
        
        /* create file for save stream*/        
        sprintf(aszFileName, "audio_chn%d%s", i, (PT_AAC==stAencAttr.enType)?".aac":".data");
        pstSampleAenc->pfd = fopen(aszFileName, "w+");
        if (NULL == pstSampleAenc->pfd)
        {
            printf("open file %s err\n", aszFileName);
            return -1;
        }
        printf("\nstart save audio stream to \"%s\" ... ... \r\n",aszFileName);
        
        pstSampleAenc->bStart   = HI_TRUE;
        pstSampleAenc->s32AiDev = AiDevId;
        pstSampleAenc->s32AiChn = pstSampleAenc->s32AencChn = i;
        
        /* create pthread for encode audio data and get stream */
        s32ret = pthread_create(&pstSampleAenc->stAencPid,0,Aenc_proc,pstSampleAenc);
    }
    
    printf("start save AENC stream , input twice Enter to stop sample ... ... \n");    
    getchar();
    getchar();

    /* clear------------------------------------------*/        
    for (i=0; i<u32ChnCnt; i++)
    {       
        pstSampleAenc = &s_stSampleAenc[i];        
        pstSampleAenc->bStart = HI_FALSE;
        pthread_join(pstSampleAenc->stAencPid, 0);
        HI_MPI_AENC_DestroyChn(pstSampleAenc->s32AencChn);
    }
    
    for (i=0; i<u32ChnCnt; i++)
    {
        s32ret = HI_MPI_AI_DisableChn(AiDevId, i);
    }  
    s32ret = HI_MPI_AI_Disable(AiDevId);
    
    SAMPLE_Audio_ExitMpp();
    return HI_SUCCESS;          
}


HI_S32 SAMPLE_Audio_Adec()
{
    HI_S32 s32ret, i;
    HI_CHAR aszFileName[64];
    AIO_ATTR_S stAttr;
    ADEC_CHN_ATTR_S stAdecAttr;    
    AUDIO_DEV AoDevId = 0;
    AO_CHN AoChn = 0;
    AIO_MODE_E enSIO0Mode = SIO0_WORK_MODE;

    /* init mpp sys*/
    s32ret = SAMPLE_Audio_InitMpp();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }          

    stAttr.enBitwidth = AUDIO_BIT_WIDTH_16;/* should equal to DA TLV320*/
    stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAttr.enWorkmode = enSIO0Mode;
    stAttr.u32EXFlag = 0;
    stAttr.u32FrmNum = 40;
    stAttr.u32PtNumPerFrm = AUDIO_POINT_NUM;
    if (PT_ADPCMA == AUDIO_PAYLOAD_TYPE && AUDIO_ADPCM_TYPE == ADPCM_TYPE_IMA) 
    {
        stAttr.u32PtNumPerFrm++;/*adpcm_ima point number must add 1 */
    }
    else if (PT_AMR == AUDIO_PAYLOAD_TYPE)
    {
        stAttr.u32PtNumPerFrm = 160;/* amr only support 160 point*/
    }
    else if (PT_AAC == AUDIO_PAYLOAD_TYPE)
    {
        stAttr.u32PtNumPerFrm = (AUDIO_AAC_TYPE==AAC_TYPE_AACLC)?\
            AACLC_SAMPLES_PER_FRAME:AACPLUS_SAMPLES_PER_FRAME;
        stAttr.enSamplerate = AUDIO_SAMPLE_RATE_16000;
    }
    
    /*config DA TLV320*/
    SAMPLE_Tlv320_CfgAudio((enSIO0Mode==AIO_MODE_PCM_SLAVE_NSTD||enSIO0Mode==AIO_MODE_PCM_SLAVE_STD)?1:0,
        (enSIO0Mode==AIO_MODE_I2S_MASTER)?0:1,
        (stAttr.enSamplerate==AUDIO_SAMPLE_RATE_8000)?0:1 );
    
    /* set ao public attr*/
    s32ret = HI_MPI_AO_SetPubAttr(AoDevId, &stAttr);
    if(HI_SUCCESS != s32ret)
    {
        printf("set ao %d attr err:0x%x\n", AoDevId,s32ret);
        return s32ret;
    }
    /* enable ao device*/
    s32ret = HI_MPI_AO_Enable(AoDevId);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao dev %d err:0x%x\n", AoDevId, s32ret);
        return s32ret;
    }
    /* enable ao chnnel*/
    s32ret = HI_MPI_AO_EnableChn(AoDevId, AoChn);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao chn %d err:0x%x\n", AoChn, s32ret);
        return s32ret;
    }
    
    stAdecAttr.enType = AUDIO_PAYLOAD_TYPE;
    stAdecAttr.u32BufSize = 5;
    stAdecAttr.enMode = ADEC_MODE_PACK;/* pack mode or stream mode ? */
        
    if (PT_ADPCMA == stAdecAttr.enType)
    {
        ADEC_ATTR_ADPCM_S stAdpcm;
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        ADEC_ATTR_G711_S stAdecG711;
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        ADEC_ATTR_G726_S stAdecG726;
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_BPS ;      
    }
    else if (PT_AMR == stAdecAttr.enType)
    {
        ADEC_ATTR_AMR_S stAdecAmr;
        stAdecAttr.pValue = &stAdecAmr;
        stAdecAmr.enFormat = AMR_FORMAT;
    }
    else if (PT_AAC == stAdecAttr.enType)
    {
        ADEC_ATTR_AAC_S stAdecAac;
        stAdecAttr.pValue = &stAdecAac;
        stAdecAttr.enMode = ADEC_MODE_STREAM;
    }
    else
    {
        return -1;
    }     
    
    /* create adec chn*/
    s32ret = HI_MPI_ADEC_CreateChn(0, &stAdecAttr);
    if (s32ret)
    {
        printf("create adec chn %d err:0x%x\n", i,s32ret);
        return s32ret;
    }
    
    /* open audio stream file */
    sprintf(aszFileName, "audio_chn0%s", (PT_AAC==stAdecAttr.enType)?".aac":".data");
    s_stSampleAdec.pfd = fopen(aszFileName, "r");
    if (!s_stSampleAdec.pfd)
    {
        printf("open adev file err,%s\n", aszFileName);
        return -1;
    }   
    printf("start read audio stream from file \"%s\",send adec and output ... \r\n", aszFileName);
    
    s_stSampleAdec.bStart = HI_TRUE;
    s_stSampleAdec.s32AoDev = AoDevId;
    s_stSampleAdec.s32AoChn = AoChn;

    if (ADEC_MODE_PACK == stAdecAttr.enMode)/* send stream by pack(recommend) */
    {
        /* create adec pthread (only create one thread)*/
        s32ret = pthread_create(&s_stSampleAdec.stAdecPid,0,Adec_proc,&s_stSampleAdec);
        if (s32ret)
        {
            return s32ret;
        }  
    }   
    else /* send stream by stream (must create two thread) */
    {
        /* create pthread for get stream from file*/
        s32ret = pthread_create(&s_stSampleAdec.stAdecPid2,0,Adec_readfile_proc,&s_stSampleAdec);
        if (s32ret)
        {
            return s32ret;
        }
        
        /* create adec pthread for send audio frame to AO */
        s32ret = pthread_create(&s_stSampleAdec.stAdecPid,0,Adec_sendao_proc,&s_stSampleAdec);
        if (s32ret)
        {
            return s32ret;
        }    
    }
             
    printf("start play ADEC stream, input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();
    
    s_stSampleAdec.bStart = HI_FALSE; 
    HI_MPI_AO_DisableChn(AoDevId, AoChn);
    HI_MPI_AO_Disable(AoDevId);
    HI_MPI_ADEC_DestroyChn(0);
    pthread_join(s_stSampleAdec.stAdecPid, 0);
    pthread_join(s_stSampleAdec.stAdecPid2, 0);
    fclose(s_stSampleAdec.pfd);
    s_stSampleAdec.pfd = NULL;
    
    SAMPLE_Audio_ExitMpp();    
    return HI_SUCCESS;          
}

HI_S32 SAMPLE_Audio_AiAo()
{
    HI_S32 s32ret;
    AIO_ATTR_S stAiAttr, stAoAttr;
    AUDIO_DEV AiDevId;
    AI_CHN AiChn = 0;
    AUDIO_DEV AoDevId = 0;    
    AO_CHN AoChn = 0;
    AIO_MODE_E aenSIOMode[2] ;

    aenSIOMode[0] = SIO0_WORK_MODE;/* work mode of SIO 0 */
    aenSIOMode[1] = SIO1_WORK_MODE;/* work mode of SIO 1 */

    /*------------------------- SYS  ------------------------------*/
    /* init MPP sys */
    s32ret = SAMPLE_Audio_InitMpp();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }
    
    /*--------------------------  AI ---------------------------------------*/
    AiDevId = 0;
    stAiAttr.enWorkmode = aenSIOMode[AiDevId];
    stAiAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAiAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;/* invalid factly when SLAVE mode*/
    stAiAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;    
    stAiAttr.u32EXFlag = 1;/* recommend set 1 when 8 bitwidth, ext 8bit to 16bit */
    stAiAttr.u32FrmNum = 30;
    stAiAttr.u32PtNumPerFrm = 320;
    
    /* config TW2815 */
    SAMPLE_TW2815_CfgAudio(&stAiAttr);
    
    /* set ai public attr*/
    s32ret = HI_MPI_AI_SetPubAttr(AiDevId, &stAiAttr);
    if(HI_SUCCESS != s32ret)
    {
        printf("set ai %d attr err:0x%x\n", AiDevId,s32ret);
        return s32ret;
    }
    /* enable ai device*/
    s32ret = HI_MPI_AI_Enable(AiDevId);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ai dev %d err:0x%x\n", AiDevId, s32ret);
        return s32ret;
    }    
    /* enable ai chnnel*/
    s32ret = HI_MPI_AI_EnableChn(AiDevId, AiChn);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ai chn %d err:0x%x\n", AiChn, s32ret);
        return s32ret;
    }
    
    /*---------------------------   AO　----------------------------------*/
    AoDevId = 0;
    stAoAttr.enWorkmode = aenSIOMode[AoDevId];;    
    stAoAttr.enSamplerate = stAiAttr.enSamplerate;
    stAoAttr.enSoundmode = stAiAttr.enSoundmode;  
    stAoAttr.u32PtNumPerFrm = stAiAttr.u32PtNumPerFrm;
    stAoAttr.u32EXFlag = 0;
    stAoAttr.u32FrmNum = 30; 
    stAoAttr.enBitwidth = AUDIO_BIT_WIDTH_16;/* should equal to DA */
    
    /*config TLV320 (link SIO_0)*/
    SAMPLE_Tlv320_CfgAudio((aenSIOMode[0]==AIO_MODE_PCM_SLAVE_NSTD||aenSIOMode[0]==AIO_MODE_PCM_SLAVE_STD)?1:0,
        (aenSIOMode[0]==AIO_MODE_I2S_MASTER)?0:1,
        (stAoAttr.enSamplerate==AUDIO_SAMPLE_RATE_8000)?0:1 );
    
    /* set ao public attr*/
    s32ret = HI_MPI_AO_SetPubAttr(AoDevId, &stAoAttr);
    if(HI_SUCCESS != s32ret)
    {
        printf("set ao %d attr err:0x%x\n", AoDevId,s32ret);
        return s32ret;
    }
    /* enable ao device*/
    s32ret = HI_MPI_AO_Enable(AoDevId);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao dev %d err:0x%x\n", AoDevId, s32ret);
        return s32ret;
    }
    /* enable ao chnnel*/
    s32ret = HI_MPI_AO_EnableChn(AoDevId, AoChn);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao chn %d err:0x%x\n", AoChn, s32ret);
        return s32ret;
    }
    
    /*------------------------------------------------------------------------*/
    /* create ai-ao pthread*/
    s_stSampleAiAo.bStart = HI_TRUE;
    s_stSampleAiAo.s32AiDev = AiDevId;
    s_stSampleAiAo.s32AiChn = AiChn;
    s_stSampleAiAo.s32AoDev = AoDevId;
    s_stSampleAiAo.s32AoChn = AoChn;
    s32ret = pthread_create(&s_stSampleAiAo.stPid, 0, AiAo_proc, &s_stSampleAiAo);
    if (s32ret)
    {
        return s32ret;
    }
    
    printf("start AI to AO , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();
    
    /*clear----------------------------------------------------------------*/
    s_stSampleAiAo.bStart = HI_FALSE; 
    sleep(1);
    HI_MPI_AI_DisableChn(AiDevId, AiChn);
    HI_MPI_AI_Disable(AiDevId);
    HI_MPI_AO_DisableChn(AoDevId, AoChn);
    HI_MPI_AO_Disable(AoDevId);
    pthread_join(s_stSampleAiAo.stPid, 0);
    SAMPLE_Audio_ExitMpp();

    return 0;
}


#define SAMPLE_AUDIO_HELP(void)\
{\
    printf("usage : %s 1|2|3\n", argv[0]);\
    printf("1:  send audio frame to AENC channel form AI, save them\n");\
    printf("2:  read audio stream from file,decode and send AO\n");\
    printf("3:  start AI to AO loop\n\n");\
}

HI_S32 main(int argc, char *argv[])
{    
    int index;

    if (2 != argc)
    {
        SAMPLE_AUDIO_HELP();
        return HI_FAILURE;
    }
    
    index = atoi(argv[1]);
    
    if (1 != index && 2 != index && 3 != index)
    {
        SAMPLE_AUDIO_HELP();
        return HI_FAILURE;
    }
    
    if (1 == index)/* send audio frame to AENC channel form AI, save them*/
    {
        SAMPLE_Audio_Aenc();
    }
    else if (2 == index)/* read audio stream from file,decode and send AO*/
    {
        SAMPLE_Audio_Adec();
    }
    else if (3 == index)/* AI to AO*/
    {
        SAMPLE_Audio_AiAo();
    }

    SAMPLE_Audio_ExitMpp();

    return HI_SUCCESS;
}

