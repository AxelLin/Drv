/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : av_stream_producer.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/10/10
  Description   : This sample is mainly used to produce AV stream which is in
                  a special format, figure as:

                    @ structure of av frame package @

                          |<-AN AV FRAME--------------------------->|
                          |  |<-VIDEO---->|<--AUDIO->|...|<-AUDIO-->|
                          |                                         |
                          |  |v-header    |a-header  |...|a-header  |
                    ------|--|-|----------|-|--------|...|-|--------|--------
                          |  |                                      |
                           ^
                           |
                           av-header

                  Any problem you can contact Kevin Hsu @ Hisilicon
                  Email: xujilin@huawei.com
  History       :
  1.Date        : 2008/10/10
    Author      : Hi3511MPP
    Modification: Created file
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_common.h"
#include "hi_comm_vb.h"
#include "hi_comm_sys.h"
#include "hi_comm_video.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_aenc.h"
#include "hi_comm_venc.h"
#include "hi_comm_aio.h"
#include "hi_comm_venc.h"
#include "hi_comm_avenc.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_ai.h"
#include "mpi_vb.h"
#include "mpi_aenc.h"
#include "mpi_venc.h"
#include "mpi_avenc.h"
#include "tw2815.h"
#include "tlv320aic31.h"

#define avenc_trace(fmt...) \
do{\
    printf("[%s]:%d ", __FUNCTION__, __LINE__);\
    printf(fmt);\
} while(0)
#define TW2815_A_FILE   "/dev/misc/tw2815adev"
#define TW2815_B_FILE   "/dev/misc/tw2815bdev"

#ifndef FRAME_WITH_LAST_PTS
#define FRAME_WITH_LAST_PTS
#endif

#ifndef AV_STREAM_TOGETHER
#define AV_STREAM_TOGETHER
#endif

typedef enum hiFRAME_TYPE_E
{
    FRAME_TYPE_VIDEO = 0,
    FRAME_TYPE_AUDIO = 1,
    FRAME_TYPE_BUTT

} FRAME_TYPE_E;

typedef struct hiH264_FILE_HEADER_S
{
	HI_U64 u64PTS;
	HI_BOOL bIFrame;
	HI_U32 u32Len;

    #ifdef FRAME_WITH_LAST_PTS
    HI_U32 u32LastLen;
    #endif

}H264_FRAME_HEADER_S, *H264_FRAME_HEADER_S_PTR;

typedef struct hiAUDIO_FRAME_HEADER_S
{
    FRAME_TYPE_E frameType;

    HI_U64 u64PTS;
    HI_U32 u32Seq;
    HI_U32 u32Len;

    #ifdef FRAME_WITH_LAST_PTS
    HI_U32 u32LastLen;
    #endif

} AUDIO_FRAME_HEADER_S, *AUDIO_FRAME_HEADER_S_PTR;

/*******
    @ structure of av frame package @

          |<-AN AV FRAME--------------------------->|
          |  |<-VIDEO---->|<--AUDIO->|...|<-AUDIO-->|
          |                                         |
          |  |v-header    |a-header  |...|a-header  |
    ------|--|-|----------|-|--------|...|-|--------|--------
          |  |                                      |
           ^
           |
           av-header
 *
 *  @ designed by Kanglin Chen & Kevin Hsu @
 *  @ 9/10/2008 @
 */

typedef struct hiAV_STREAM_HEADER_S
{
    H264_FRAME_HEADER_S_PTR pVHeader;
    AUDIO_FRAME_HEADER_S_PTR pAHeader;
    HI_U32 u32AuOffset;
    HI_U32 u32AuCnt;
    HI_U32 u32Len;
    HI_U32 u32LastLen;

} AV_STREAM_HEADER_S;

typedef struct
{
    AVENC_CHN AvencChn;
    HI_BOOL bStart;
    HI_S32 s32AiDev;
    HI_S32 s32AiChn;
    HI_S32 s32AencChn;
    pthread_t stAvencPid;
    int TspErr;
} SAMPLE_AVENC_S;


static  SAMPLE_AVENC_S s_stSampleAvenc[8];
int s_fd2815a = -1;
int s_fd2815b = -1;

HI_S32 SAMPLE_AVENC_InitMpp()
{
    HI_S32 s32ret;
    MPP_SYS_CONF_S struSysConf;
    VB_CONF_S struVbConf;

    memset(&struVbConf, 0, sizeof(struVbConf));
    memset(&struSysConf, 0, sizeof(struSysConf));

    struVbConf.u32MaxPoolCnt = 128;
    struVbConf.astCommPool[0].u32BlkSize = 768*576*2;
    struVbConf.astCommPool[0].u32BlkCnt = 10;
    struVbConf.astCommPool[1].u32BlkSize = 352*288*1.5;
    struVbConf.astCommPool[1].u32BlkCnt = 50;
    struVbConf.astCommPool[2].u32BlkSize = 192*144*1.5;
    struVbConf.astCommPool[2].u32BlkCnt = 50;

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    s32ret = HI_MPI_VB_SetConf(&struVbConf);
    if (s32ret)
    {
        avenc_trace("HI_MPI_VB_SetConf ERR 0x%x\n",s32ret);
        return s32ret;
    }
    s32ret = HI_MPI_VB_Init();
    if (s32ret)
    {
        avenc_trace("HI_MPI_VB_Init ERR 0x%x\n", s32ret);
        return s32ret;
    }

    struSysConf.u32AlignWidth = 64;
    s32ret = HI_MPI_SYS_SetConf(&struSysConf);
    if (HI_SUCCESS != s32ret)
    {
        avenc_trace("Set mpi sys config failed!\n");
        return s32ret;
    }
    s32ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32ret)
    {
        avenc_trace("Mpi init failed!\n");
        return s32ret;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AVENC_ExitMpp()
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    return 0;
}


int SAMPLE_tw2815_open()
{
    s_fd2815a = open(TW2815_A_FILE,O_RDWR);
    if (s_fd2815a < 0)
    {
        avenc_trace("can't open tw2815a\n");
        return -1;
    }
    s_fd2815b = open(TW2815_B_FILE,O_RDWR);
    if (s_fd2815b < 0)
    {
        avenc_trace("can't open tw2815b\n");
        return -1;
    }
    return 0;
}

void SAMPLE_tw2815_close()
{
    if (s_fd2815a > 0)
    {
        close(s_fd2815a);
    }
    if (s_fd2815b > 0)
    {
        close(s_fd2815b);
    }
}

int SAMPLE_tw2815_set_mode()
{
    int i;
    tw2815_set_2d1 tw2815set2d1;
    int fd2815[] = {s_fd2815a, s_fd2815b};

    for (i=0; i<2; i++)
    {
        tw2815set2d1.ch1 = 0;
        tw2815set2d1.ch2 = 2;

        if(0!=ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1))
        {
            avenc_trace("2815 errno %d\r\n",errno);
            return -1;
        }

        tw2815set2d1.ch1 = 1;
        tw2815set2d1.ch2 = 3;

        if(0!=ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1))
        {
            avenc_trace("2815 errno %d\r\n",errno);
            return-1;
        }
    }
    return 0;
}



/*  s32Seq: 0£º×ó0123£¬ÓÒ89ab   1£º×ó0246£¬ÓÒ1357
            2£º×ó0123£¬ÓÒ4567   3£º×ó0145£¬ÓÒ2367   */
/* s32Samplerate:[0:8k,1:16k]  s32Bitwidth:[0:16bit, 1:8bit]*/
int SAMPLE_tw2815_set_audio(HI_S32 s32Samplerate,HI_S32 s32Bitwidth,
        HI_S32 s32Chnnum,HI_U32 s32Seq)
{
    HI_S32 value;
    HI_S32 ret;
    tw2815_w_reg set_reg;
    HI_U32 i;

    if(s32Samplerate<0||s32Samplerate>1)
    {
        avenc_trace("s32Samplerate error\n");
        return -1;
    }
    if(s32Bitwidth<0||s32Bitwidth>1)
    {
        avenc_trace("s32Bitwidth error\n");
        return -1;
    }

    value = s32Samplerate;
    ret=ioctl(s_fd2815a,TW2815_SET_ADA_SAMPLERATE,&value);
    if(ret <0)
    {
         avenc_trace("2815 sample errno:%d\n",errno);
         return -1;
    }

    value = s32Bitwidth;
    if(0!=ioctl(s_fd2815a,TW2815_SET_ADA_BITWIDTH,&value))
    {
        avenc_trace("2815 s32Bitwidth errno\n");
        return -1;
    }

    value = s32Chnnum;/* 0~16*/
    if(0!=ioctl(s_fd2815a,TW2815_SET_AUDIO_OUTPUT,&value))
    {
        avenc_trace("2815 output errno\n");
        return -1;
    }
    value = s32Seq;
    if(0!=ioctl(s_fd2815a,TW2815_SET_CHANNEL_SEQUENCE,&value))
    {
        avenc_trace("2815 output errno\n");
        return -1;
    }

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

    return 0;
}


HI_S32 SAMPLE_AVENC_CheckTsp(AVENC_STREAM_S *pAVStream)
{
    AUDIO_STREAM_S  *pAStream = NULL;
    VENC_STREAM_S  *pVStream = NULL;
    AVENC_LIST_S   *pNode    = NULL;
    HI_U64          u64TSA=0, u64TSTmp=0, u64TSV=0;
    HI_U64          u64AU;
    HI_S32          s32Num;
    HI_S32          s32Ret;

    pNode = pAVStream->pAudioListHead;
    s32Num = 0;
    u64AU  = 0;
    while(NULL != pNode)
    {
        pAStream = (AUDIO_STREAM_S *)(pNode->pData);

        if(0 != s32Num)
        {
            u64AU += pAStream->u64TimeStamp - u64TSTmp;
        }
        else
        {
            u64TSA = pAStream->u64TimeStamp;
        }

        u64TSTmp = pAStream->u64TimeStamp;
        pNode = pNode->pNext;
        s32Num++;
    }

    if(s32Num > 2)
    {
        u64AU = u64AU/s32Num;
    }
    else
    {
        u64AU = 20*1000;
    }

    pNode = pAVStream->pVideoListHead;
    while(NULL != pNode)
    {
        pVStream = (VENC_STREAM_S *)(pNode->pData);
        u64TSV = pVStream->pstPack->u64PTS;
        pNode = pNode->pNext;
    }

    s32Ret = HI_SUCCESS;
    if((u64TSA != 0) && (u64TSV != 0))
    {
        if(((u64TSA - u64TSV) < u64AU) || ((u64TSV - u64TSA) < u64AU))
        {
            s32Ret = HI_SUCCESS;
        }
        else
        {
            s32Ret = HI_FAILURE;
        }
    }


    return s32Ret;
}

int SAMPLE_AVENC_StartVi(HI_U32 u32ChnCnt)
{
    //HI_S32 s32ret;
    VI_PUB_ATTR_S ViAttr;
    VI_CHN_ATTR_S ViChnAttr;
    int i;

    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));

    ViAttr.enInputMode = VI_MODE_BT656;
    ViAttr.enWorkMode = VI_WORK_MODE_2D1;

    ViChnAttr.enCapSel = VI_CAPSEL_BOTTOM;
    ViChnAttr.stCapRect.u32Width = 704;
    ViChnAttr.stCapRect.u32Height = 288;
    ViChnAttr.stCapRect.s32X = 0;
    ViChnAttr.stCapRect.s32Y = 0;
    ViChnAttr.bDownScale = HI_TRUE;
    ViChnAttr.bChromaResample = HI_FALSE;
    ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;

    for (i=0; i <= (u32ChnCnt-1)/2; i++)
    {
        if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(i,&ViAttr))
        {
            avenc_trace("set VI attribute failed !\n");
            return -1;
        }
        if (HI_SUCCESS!=HI_MPI_VI_Enable(i))
        {
            avenc_trace("set VI  enable failed !\n");
            return -1;
        }
    }

    for (i = 0; i < u32ChnCnt; i++)
    {
        if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(i/2,i%2,&ViChnAttr))
        {
            avenc_trace("set VI Chn attribute failed !\n");
            return -1;
        }
        if (HI_SUCCESS!=HI_MPI_VI_EnableChn(i/2,i%2))
        {
            avenc_trace("set VI Chn enable failed !\n");
            return -1;
        }
    }

    return 0;
}

HI_S32 SAMPLE_AVENC_StartVo(HI_VOID)
{
    HI_S32 ret;

    VO_PUB_ATTR_S pubAttr;
    VO_CHN_ATTR_S chnAttr;

    memset(&pubAttr, 0, sizeof(VO_PUB_ATTR_S));
    memset(&chnAttr, 0, sizeof(VO_CHN_ATTR_S));

    pubAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
    pubAttr.u32BgColor = 0x0000ff;

    chnAttr.bZoomEnable = HI_FALSE;
    chnAttr.stRect.s32X = 0;
    chnAttr.stRect.s32Y = 0;
    chnAttr.stRect.u32Width = 352;
    chnAttr.stRect.u32Height = 288;
    chnAttr.u32Priority = 1;

    ret = HI_MPI_VO_SetPubAttr(&pubAttr);
    assert(ret == 0);

    ret = HI_MPI_VO_SetChnAttr(0, &chnAttr);
    assert(ret == 0);

    ret = HI_MPI_VO_Enable();
    assert(ret == 0);

    ret = HI_MPI_VO_EnableChn(0);
    assert(ret == 0);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AVENC_ViBindVo(HI_VOID)
{
    HI_S32 ret;

    ret = HI_MPI_VI_BindOutput(0,0,0);
    assert(ret == 0);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AVENC_ViUnBindVo(HI_VOID)
{
    HI_S32 ret;

    ret = HI_MPI_VI_UnBindOutput(0,0,0);
    assert(ret == 0);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AVENC_StopVo(HI_VOID)
{
    HI_S32 ret;

    ret = HI_MPI_VO_DisableChn(0);
    assert(ret == 0);

    ret = HI_MPI_VO_Disable();
    assert(ret == 0);

    return HI_SUCCESS;
}

int SAMPLE_AVENC_StartVenc(HI_U32 u32ChnCnt)
{
    HI_S32 s32ret;
    //VENC_CHN VeChn = 0;
    VENC_CHN_ATTR_S stAttr;
    VENC_ATTR_H264_S stH264Attr;
    int i;

    for (i = 0; i < u32ChnCnt; i++)
    {
        s32ret = HI_MPI_VENC_CreateGroup(i);
        if (s32ret != HI_SUCCESS)
        {
            avenc_trace("HI_MPI_VENC_CreateGroup err 0x%x \n", s32ret);
            return -1;
        }

        memset(&stH264Attr, 0, sizeof(stH264Attr));
        stH264Attr.u32PicWidth = 352;
        stH264Attr.u32PicHeight = 288;
        stH264Attr.bMainStream = 1;
        stH264Attr.bByFrame = 1;/* °´Ö¡»ñÈ¡*/
        stH264Attr.bCBR = 0;
        stH264Attr.bField = 0;
        stH264Attr.bVIField= 1;
        stH264Attr.u32Bitrate = 512;
        stH264Attr.u32ViFramerate = 25;
        stH264Attr.u32TargetFramerate= 25;
        stH264Attr.u32BufSize = 228096;
        stH264Attr.u32Gop= 20;
        stH264Attr.u32MaxDelay = 10;
        stH264Attr.u32PicLevel= 3;
        stAttr.pValue = &stH264Attr;
        stAttr.enType = PT_H264;
        s32ret = HI_MPI_VENC_CreateChn(i, &stAttr, NULL);
        if (s32ret != HI_SUCCESS)
        {
            avenc_trace("HI_MPI_VENC_CreateChn err 0x%x \n", s32ret);
            return -1;
        }

        s32ret = HI_MPI_VENC_RegisterChn(i, i);
        if (s32ret != HI_SUCCESS)
        {
            avenc_trace("HI_MPI_VENC_RegisterChn err 0x%x \n", s32ret);
            return -1;
        }

        s32ret = HI_MPI_VENC_BindInput(i, i/2, i%2);
        if (s32ret != HI_SUCCESS)
        {
            avenc_trace("HI_MPI_VENC_BindInput err 0x%x \n", s32ret);
            return -1;
        }
    }

    return 0;
}

int SAMPLE_AVENC_StartAi(HI_U32 u32ChnCnt)
{
    HI_S32 s32ret, i;
    AUDIO_DEV AiDevId = 1;
//  AI_CHN AiChn = 0;
    AIO_ATTR_S stAttr;

    stAttr.enBitwidth = AUDIO_BIT_WIDTH_8;
    stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAttr.u32EXFlag = 1;
    stAttr.u32FrmNum = 10;
    stAttr.u32PtNumPerFrm = 160;
    /* set ai public attr*/
    s32ret = HI_MPI_AI_SetPubAttr(AiDevId, &stAttr);
    if(HI_SUCCESS != s32ret)
    {
        avenc_trace("set ai %d attr err:0x%x\n", AiDevId,s32ret);
        return s32ret;
    }
    /* enable ai device*/
    s32ret = HI_MPI_AI_Enable(AiDevId);
    if(HI_SUCCESS != s32ret)
    {
        avenc_trace("enable ai dev %d err:0x%x\n", AiDevId, s32ret);
        return s32ret;
    }

    for (i=0; i<u32ChnCnt; i++)
    {
        s32ret = HI_MPI_AI_EnableChn(AiDevId, i);
        if(HI_SUCCESS != s32ret)
        {
            avenc_trace("enable ai chn %d err:0x%x\n", i, s32ret);
            return s32ret;
        }
    }

    return 0;
}

int SAMPLE_AVENC_StartAenc(HI_U32 u32ChnCnt)
{
    HI_S32 s32ret;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    int i;

    stAencAttr.enType = PT_ADPCMA;
    stAencAttr.u32BufSize = 10;
    stAencAttr.pValue = &stAdpcmAenc;
    stAdpcmAenc.enADPCMType = ADPCM_TYPE_DVI4;

    /* create aenc chn*/
    for (i=0; i<u32ChnCnt; i++)
    {
        s32ret = HI_MPI_AENC_CreateChn(i, &stAencAttr);
        if (s32ret)
        {
            avenc_trace("create aenc chn err:0x%x\n", s32ret);
            return s32ret;
        }
    }

    return 0;
}

void *SAMPLE_AVENC_proc(void *pArgs)
{
    size_t writeByte;
    HI_S32 s32ret;
    HI_U32 i;
    AVENC_CHN AVChnId = 0;
    AVENC_STREAM_S stAVStream;

#if 0
    FILE *pVfd = NULL;
    FILE *pAfd = NULL;
#else
    FILE *pAVfd = NULL;
#endif

    HI_CHAR aszFileName[128];
    SAMPLE_AVENC_S *pstSampleAvenc;

    AVENC_LIST_S   *pNode    = NULL;
    AUDIO_STREAM_S  *pAStream = NULL;
    VENC_STREAM_S  *pVStream = NULL;

    pstSampleAvenc = (SAMPLE_AVENC_S*)pArgs;
    AVChnId = pstSampleAvenc->AvencChn;

#if 0
    sprintf(aszFileName, "video_chn%d.h264", AVChnId);
    pVfd = fopen(aszFileName, "w+");
    if (NULL == pVfd)
    {
        avenc_trace("open file %s err\n", aszFileName);
        return NULL;
    }
    sprintf(aszFileName, "audio_chn%d.data", AVChnId);
    pAfd = fopen(aszFileName, "w+");
    if (NULL == pVfd)
    {
        avenc_trace("open file %s err\n", aszFileName);
        return NULL;
    }
#else
    /* we save audio and video in one file, it's a av stream.               */
    sprintf(aszFileName, "stream%d.av",AVChnId);
    pAVfd = fopen(aszFileName, "w+");
    if (NULL == pAVfd)
    {
        avenc_trace("open file %s error!\n", aszFileName);
        return NULL;
    }
#endif

    while (pstSampleAvenc->bStart)
    {
        /* variables definition                                             */
        static HI_U32 vlastLen = 0;
        static HI_U32 alastLen = 0;

        HI_U32 AvLen = 0;
        HI_U32 AudioCounter = 0;
        static HI_U32 AvLastLen = 0;

        /* saved file format */
        H264_FRAME_HEADER_S vheader;
        AUDIO_FRAME_HEADER_S aheader;
        AV_STREAM_HEADER_S avheader;

        memset(&vheader, 0,sizeof(H264_FRAME_HEADER_S));
        memset(&aheader, 0,sizeof(AUDIO_FRAME_HEADER_S));
        memset(&avheader,0,sizeof(AV_STREAM_HEADER_S));

        avheader.pVHeader = NULL;
        avheader.pAHeader = NULL;

        s32ret = HI_MPI_AVENC_GetStream(AVChnId, &stAVStream, HI_IO_BLOCK);
        if (s32ret)
        {
            avenc_trace("get avenc chn %d streamerr ,%x\n", AVChnId,s32ret);
            return NULL;
        }

#if 1
 		/*check the video and audio not synchronization number*/
        s32ret = SAMPLE_AVENC_CheckTsp(&stAVStream);
        if (s32ret)
        {
            pstSampleAvenc->TspErr ++;
        }

#endif
        /*  -----------------------------------------
         *  this section is used to save stream to file.
         *  -----------------------------------------
         */

        /* 1. config video frame header                                      */

        pNode = stAVStream.pVideoListHead;
        if (pNode == NULL)
        {
            //puts("video lost!");
            continue;
        }
        while (NULL != pNode)
        {
            pVStream = (VENC_STREAM_S *)(pNode->pData);
            if (0 == pVStream->u32PackCount)
            {
                printf("[error]video stream empty!\n");
                return NULL;
            }

            vheader.u64PTS = pVStream->pstPack[0].u64PTS;

            for ( i = 0; i < pVStream->u32PackCount; i++ )
            {
                vheader.u32Len += pVStream->pstPack[i].u32Len[0];
                vheader.u32Len += pVStream->pstPack[i].u32Len[1];
                if (pVStream->pstPack[i].DataType.enH264EType
                    == H264E_NALU_ISLICE)
                {
                    vheader.bIFrame = HI_TRUE;
                }
            }

            vheader.u32LastLen = vlastLen;
            vlastLen = vheader.u32Len;

            AvLen += vheader.u32Len;
            avheader.pVHeader = &vheader;

            pNode = pNode->pNext;
            if (pNode != NULL)
            {
                printf("[unexpected]:more than one video frame!\n");
                return NULL;
            }
        }

        /* 2. config audio frame header                                      */

        pNode = stAVStream.pAudioListHead;
        if (NULL != pNode)
        {
            avheader.pAHeader = &aheader;
        }
        while (NULL != pNode)
        {
            pAStream = (AUDIO_STREAM_S *)(pNode->pData);
            aheader.frameType = FRAME_TYPE_AUDIO;
            aheader.u64PTS = pAStream->u64TimeStamp;
            aheader.u32Seq = pAStream->u32Seq;
            aheader.u32Len = pAStream->u32Len;

            aheader.u32LastLen = alastLen;
            alastLen = aheader.u32Len;

            AudioCounter++;
            AvLen += aheader.u32Len;
            pNode = pNode->pNext;
        }

        /* 3. config av frame header                                         */

        avheader.u32AuCnt = AudioCounter;
        avheader.u32AuOffset = sizeof(H264_FRAME_HEADER_S) + vheader.u32Len;
        avheader.u32Len =  sizeof(H264_FRAME_HEADER_S)
                            + AudioCounter * sizeof(AUDIO_FRAME_HEADER_S)
                            + AvLen;

        avheader.u32LastLen = AvLastLen;
        AvLastLen = avheader.u32Len;

        /* 4. write av frame header                                         */

        writeByte = fwrite(&avheader, sizeof(AV_STREAM_HEADER_S), 1, pAVfd);
        assert(writeByte != 0);

        /* 5. write video frame header & video data                         */

        writeByte = fwrite(&vheader, sizeof(H264_FRAME_HEADER_S), 1, pAVfd);
        assert(writeByte != 0);

        pNode = stAVStream.pVideoListHead;
        while (NULL != pNode)
        {
            pVStream = (VENC_STREAM_S *)(pNode->pData);
        	for (i=0; i< pVStream->u32PackCount; i++)
        	{
        		fwrite(pVStream->pstPack[i].pu8Addr[0],
        				pVStream->pstPack[i].u32Len[0], 1, pAVfd);

        		fflush(pAVfd);

        		if (pVStream->pstPack[i].u32Len[1] > 0)
        		{

        			fwrite(pVStream->pstPack[i].pu8Addr[1],
        					pVStream->pstPack[i].u32Len[1], 1, pAVfd);

        			fflush(pAVfd);
        		}
        	}
            pNode = pNode->pNext;
            if (pNode != NULL)
            {
                printf("[unexpected]:more than one video frame!\n");
                return NULL;
            }
        }


        /* 6. write audio frame header & data                               */

        pNode = stAVStream.pAudioListHead;
        while(NULL != pNode)
        {
            pAStream = (AUDIO_STREAM_S *)(pNode->pData);
            writeByte = fwrite(&aheader, sizeof(AUDIO_FRAME_HEADER_S), 1, pAVfd);
            assert(writeByte != 0);

            fwrite(pAStream->pStream, pAStream->u32Len, 1,  pAVfd);

            pNode = pNode->pNext;
        }

        s32ret = HI_MPI_AVENC_ReleaseStream(AVChnId, &stAVStream);
        if (s32ret)
        {
            avenc_trace("release avenc chn %d streamerr ,%x\n", AVChnId,s32ret);
            return NULL;
        }
    }

    printf("==================================================================\n");

#if 0
    fclose(pVfd);
    fclose(pAfd);
#else
    fclose(pAVfd);
#endif
    return NULL;
}

int SAMPLE_AVENC_main(int chncnt)
{
    HI_S32 s32ret;
    int i;
    AVENC_OPERATE_OBJECT_E enObject;
    HI_U32 u32ChnCnt = chncnt;

    AUDIO_DEV AiDevId = 1;
    VENC_CHN VChnId = 0;
    AENC_CHN AChnId = 0;
    AVENC_CHN AVChnId;

    /* init MPP */
    s32ret = SAMPLE_AVENC_InitMpp();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }

    /* open AD tw2815*/
    s32ret = SAMPLE_tw2815_open();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }

    /* set tw2815, audio samlerate:8k, bitwidth:16bit*/
    s32ret = SAMPLE_tw2815_set_audio(0,1,8,1);
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }

    /* set tw2815 video mode*/
    s32ret = SAMPLE_tw2815_set_mode();
    if (s32ret)
    {
        avenc_trace("SAMPLE_tw2815_set_mode err \n");
        return s32ret;
    }

    /* enable VI device and VI channel*/
    s32ret = SAMPLE_AVENC_StartVi(u32ChnCnt);
    if (s32ret)
    {
        return -1;
    }

    /* enable VO device and VO channel                                      */
    s32ret = SAMPLE_AVENC_StartVo();
    if (s32ret)
    {
        puts("SAMPLE_AVENC_StartVo failed!");
        return -1;
    }

    /* vi bind vo                                                           */
    s32ret = SAMPLE_AVENC_ViBindVo();
    if (s32ret)
    {
        puts("SAMPLE_AVENC_ViBindVo failed!");
        return -1;
    }

    /* create GROUP and VENC channel, bind to VI */
    s32ret = SAMPLE_AVENC_StartVenc(u32ChnCnt);
    if (s32ret)
    {
        return s32ret;
    }

    /* enable AI device */
    s32ret = SAMPLE_AVENC_StartAi(u32ChnCnt);
    if (s32ret)
    {
        return s32ret;
    }

    /* create AENC channel*/
    s32ret = SAMPLE_AVENC_StartAenc(u32ChnCnt);
    if (s32ret)
    {
        return s32ret;
    }

    /* create AVENC channel and start them*/
    for (i = 0; i < u32ChnCnt; i++)
    {
        VChnId = i;
        AChnId = i;
        //AChnId = -1;
        s32ret = HI_MPI_AVENC_CreatCH(AiDevId, i, VChnId, AChnId, &AVChnId);
        if (s32ret)
        {
            avenc_trace("create avenc chn(%d,%d) err:0x%x\n", VChnId, AChnId,s32ret);
            return s32ret;
        }
        if (VChnId >= 0 && AChnId >= 0)
        {
            enObject = AVENC_OPERATION_BOTH;
        }
        else if (VChnId >= 0 && AChnId < 0)
        {
            enObject = AVENC_OPERATION_VIDEO;
        }
        else if (AChnId >= 0 && VChnId < 0)
        {
            enObject = AVENC_OPERATION_AUDIO;
        }
        else
        {
            return s32ret;
        }

        s32ret = HI_MPI_AVENC_StartCH(AVChnId, enObject, 0);
        if (s32ret)
        {
            avenc_trace("start avenc chn err:0x%x\n", s32ret);
            return s32ret;
        }
        avenc_trace("start avenc chn id %d ok\n", AVChnId);
        s_stSampleAvenc[i].AvencChn = AVChnId;

        s_stSampleAvenc[i].bStart = HI_TRUE;
        s_stSampleAvenc[i].TspErr = 0;
        /* create pthread getting video and audio stream */
        pthread_create(&(s_stSampleAvenc[i].stAvencPid), NULL, SAMPLE_AVENC_proc, &s_stSampleAvenc[i]);
    }

    avenc_trace("start get avenc stream, input twice Enter to stop sample ... ... \n");

    printf("\033[0;32mdouble press 'Enter' for stop!\033[0;39m\n");

    getchar();
    getchar();

    printf("\033[0;32mAV stream produce OK!\033[0;39m\n");

    /* vi unbind vo                                                         */
    s32ret = SAMPLE_AVENC_ViUnBindVo();
    if (s32ret)
    {
        puts("SAMPLE_AVENC_ViUnBindVo failed!");
        return -1;
    }

    /* disable vo                                                           */
    s32ret = SAMPLE_AVENC_StopVo();
    if (s32ret)
    {
        puts("SAMPLE_AVENC_StopVo failed!");
        return -1;
    }

    /* clean up ...*/
    for (i = 0; i < u32ChnCnt; i++)
    {
        s_stSampleAvenc[i].bStart = HI_FALSE;
        pthread_join(s_stSampleAvenc[i].stAvencPid , NULL);
        HI_MPI_AVENC_StopCH(s_stSampleAvenc[i].AvencChn, enObject);
        HI_MPI_AVENC_DestroyCH(s_stSampleAvenc[i].AvencChn, HI_TRUE);
    }

    SAMPLE_AVENC_ExitMpp();

    return 0;
}

int main(int argc, char *argv[])
{
    printf("\033[0;32mPress 'Enter' to start Recode!\033[0;39m\n");

    getchar();

    SAMPLE_AVENC_main(1);
    SAMPLE_tw2815_close();
    SAMPLE_AVENC_ExitMpp();
    return 0;
}

