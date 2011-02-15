/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_sync.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/09/12
  Description   : This function show you how to implement play contrl.
                  It contains preview, multi-channel synchronization and
                  play contrl of video stream and av-stream, etc.
  History       :
  1.Date        : 2008/09/12
    Author      : x100808
    Modification: Created file

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vdec.h"
#include "mpi_venc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"

#include "tw2815.h"
#include "adv7179.h"
#include "tlv320aic31.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef STREAM_WITH_PTS
#define STREAM_WITH_PTS
#endif

#ifndef FRAME_WITH_LAST_PTS
#define FRAME_WITH_LAST_PTS
#endif

#ifndef AV_STREAM_TOGETHER
#define AV_STREAM_TOGETHER
#endif

/* fd for vi/vo config  (tw2815/adv7179) */
HI_S32 g_s32Fd2815a = -1;
HI_S32 g_s32Fd2815b = -1;
HI_S32 g_s32Fdadv7179 = -1;

static HI_U32 s_u32TvMode = VIDEO_ENCODING_MODE_PAL;
static HI_U32 s_u32ScreenWidth = 720;
static HI_U32 s_u32ScreenHeight = 576;
static HI_U32 s_u32NormFrameRate = 25;

static HI_BOOL s_bVdecStopFlag = HI_FALSE;
static HI_BOOL s_bStartFlag = HI_FALSE;
static HI_U32  s_u32UltraSpeed = 0;

typedef unsigned char   byte;

typedef enum hiVDEC_SIZE_E
{
    VDEC_SIZE_D1 = 0,
    VDEC_SIZE_CIF,
    VDEC_SIZE_BUTT
} VDEC_SIZE_E;

typedef struct hiVDEC_SEND_STREAM_S
{
    HI_S32 VdecChn;
    HI_CHAR fileName[40];
} VDEC_SEND_STREAM_S;

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

typedef struct hiNALU_t
{
  int startcodeprefix_len;
  unsigned len;
  unsigned max_size;
  int nal_unit_type;
  int nal_reference_idc;
  int forbidden_bit;
  HI_U8 *buf;
} NALU_t;

/* av play speed list                                                       */
typedef enum hiAV_PLAY_SPEED_E
{
    AV_PLAY_SPEED_NORMAL = 1,
    AV_PLAY_SPEED_SLOW   = 2,
    AV_PLAY_SPEED_2X     = 3,
    AV_PLAY_SPEED_4X     = 4,
    AV_PLAY_SPEED_8X     = 8,
    AV_PLAY_SPEED_BACK   = 9,
    AV_PLAY_SPEED_BUTT
} AV_PLAY_SPEED_E;

static AV_PLAY_SPEED_E s_enAVspeed = AV_PLAY_SPEED_NORMAL;
static HI_BOOL s_bNeedSilence = HI_FALSE;

#define CHECK(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d !\n", name, __FUNCTION__, __LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

#ifdef SYNC_DEBUG
#define POSITION printf("line: %d func: %s \n", __LINE__, __FUNCTION__)
#else
#define POSITION
#endif

#define PAUSE_G_RESUME()\
    do\
    {\
        while (getchar() != 'g')\
        {\
            puts("press g for go on!");\
        }\
    } while(0)

/*****************************************************************************
 Prototype       : do_Set2815_2d1 & do_Set7179
 Description     : The following two functions are used
                    to set AD and DA for VI and VO.

*****************************************************************************/
/* set A/D encoder's work mode                                                    */
static void do_Set2815_2d1(void)
{
	tw2815_set_2d1 tw2815set2d1;
    HI_U32 tmp;

	if(g_s32Fd2815a <0)
	{
		g_s32Fd2815a = open("/dev/misc/tw2815adev",O_RDWR);
		if(g_s32Fd2815a <0)
		{
			printf("can't open tw2815\n");
		}
	}

	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(g_s32Fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(g_s32Fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

    tmp = 0x88;
	if(0!=ioctl(g_s32Fd2815a,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);
		return;
	}

	if(g_s32Fd2815b <0)
	{
		g_s32Fd2815b = open("/dev/misc/tw2815bdev",O_RDWR);
		if(g_s32Fd2815b <0)
		{
			printf("can't open tw2815\n");
		}
	}

	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(g_s32Fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(g_s32Fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

    tmp = 0x88;
	if(0!=ioctl(g_s32Fd2815b,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);
		return;
	}

	return;
}

/* set D/A decoder's work mode                                              */
static void do_Set7179(VIDEO_NORM_E stComposeMode)
{
	if(g_s32Fdadv7179 <0)
	{
		g_s32Fdadv7179 = open("/dev/misc/adv7179",O_RDWR);
		if(g_s32Fdadv7179 <0)
		{
			printf("can't open 7179\n");
			return;
		}
	}

    if(stComposeMode==VIDEO_ENCODING_MODE_PAL)
    {
        if(0!=ioctl(g_s32Fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_PAL))
    	{
    		printf("7179 errno %d\r\n",errno);
    	}
	}
	else if(stComposeMode==VIDEO_ENCODING_MODE_NTSC)
	{
        if(0!=ioctl(g_s32Fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_NTSC))

    	{
    		printf("7179 errno %d\r\n",errno);
    	}
	}
	else
	{
        return;
	}
	return;
}

/*****************************************************************************
 Prototype       :  SampleInitViMScreen
                    SampleSetVochnMscreen
                    SampleInitVoMScreen
                    SampleViBindVo
                    SampleViUnBindVo
                    SampleDisableVo
                    SampleDisableVi

 Description     : The following seven functions are used
                    to set VI and VO, configure,enable,disable,etc.

*****************************************************************************/

HI_S32 SampleInitViMScreen(HI_U32 ScreenCnt)
{
    HI_U32 i,j,tmp;
    ScreenCnt = (ScreenCnt == 9 ? 8 : ScreenCnt);

    VI_PUB_ATTR_S ViAttr;
    VI_CHN_ATTR_S ViChnAttr[8];

    memset(&ViAttr, 0, sizeof(VI_PUB_ATTR_S));
    memset(ViChnAttr, 0, 8 * sizeof(VI_CHN_ATTR_S));

    ViAttr.enInputMode = VI_MODE_BT656;
    ViAttr.enWorkMode = VI_WORK_MODE_2D1;

    switch (ScreenCnt)
    {
        case 1 :
        {
            /* our 1 vi2vo is cif size */
            for (i = 0; i < 2; i++)
            {
                ViChnAttr[i].bDownScale = HI_TRUE;
                ViChnAttr[i].bHighPri = 0;
                ViChnAttr[i].enCapSel = VI_CAPSEL_BOTTOM;
                ViChnAttr[i].enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                ViChnAttr[i].stCapRect.s32X = 0;
                ViChnAttr[i].stCapRect.s32Y = 0;
                ViChnAttr[i].stCapRect.u32Width = s_u32ScreenWidth;
                ViChnAttr[i].stCapRect.u32Height = s_u32ScreenHeight/2;
            }
            break;
        }
        case 4 :
            /* fall through                                                 */
        case 8 :
        {
            for (i = 0; i < ScreenCnt; i++)
            {
                ViChnAttr[i].bDownScale = HI_TRUE;
                ViChnAttr[i].bHighPri = 0;
                ViChnAttr[i].enCapSel = VI_CAPSEL_BOTTOM;
                ViChnAttr[i].enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                ViChnAttr[i].stCapRect.s32X = 0;
                ViChnAttr[i].stCapRect.s32Y = 0;
                ViChnAttr[i].stCapRect.u32Width = s_u32ScreenWidth;
                ViChnAttr[i].stCapRect.u32Height = s_u32ScreenHeight/2;
            }
            break;
        }
        default:
            printf("Unsupport screen num %d!\n", ScreenCnt);
            return HI_FAILURE;
    }

    if (1 == ScreenCnt)
    {
        tmp = 1;
    }else
    {
        tmp = ScreenCnt/2;
    }

    for (i = 0; i < tmp; i++)
    {
        if (HI_SUCCESS != HI_MPI_VI_SetPubAttr(i, &ViAttr))
        {
            printf("HI_MPI_VI_SetPubAttr @ %d\n!", __LINE__);
            return HI_FAILURE;
        }

        if (HI_SUCCESS != HI_MPI_VI_Enable(i))
        {
            printf("HI_MPI_VI_Enable @ %d\n!", __LINE__);
            return HI_FAILURE;
        }

        for(j = 0; j < 2; j++)
        {
            if (HI_SUCCESS != HI_MPI_VI_SetChnAttr(i,j,&ViChnAttr[i*2+j]))
            {
                printf("HI_MPI_VI_SetChnAttr @ %d\n!", __LINE__);
                return HI_FAILURE;
            }

            if (HI_SUCCESS != HI_MPI_VI_EnableChn(i,j))
            {
                printf("HI_MPI_VI_EnableChn @ %d\n!", __LINE__);
                return HI_FAILURE;
            }
        }
    }

    return HI_SUCCESS;
}


HI_S32 SampleSetVochnMscreen(HI_U32 u32Cnt)
{
    HI_U32 i,div,offset;
    VO_CHN_ATTR_S VoChnAttr[u32Cnt];

    div = sqrt(u32Cnt);
    if (9 == u32Cnt)
    {
        offset = 16;
    }
    else
    {
        offset = 0;
    }

    for (i = 0; i < u32Cnt; i++)
    {
        VoChnAttr[i].bZoomEnable = HI_TRUE;
        VoChnAttr[i].u32Priority = 1;

        VoChnAttr[i].stRect.s32X = ((s_u32ScreenWidth + offset)*(i%div))/div ;
        VoChnAttr[i].stRect.u32Width = (s_u32ScreenWidth + offset)/div;

        VoChnAttr[i].stRect.s32Y = (s_u32ScreenHeight*(i/div))/div ;
        VoChnAttr[i].stRect.u32Height = s_u32ScreenHeight/div;

        CHECK(HI_MPI_VO_SetChnAttr(i, &VoChnAttr[i]),"HI_MPI_VO_SetChnAttr");
    }

    return HI_SUCCESS;
}

HI_S32 SampleInitVoMScreen(HI_U32 ScreenCnt, HI_U32 EnableNum)
{
    HI_U32 i;
    VO_PUB_ATTR_S  VoPubAttr;

    VoPubAttr.stTvConfig.stComposeMode = s_u32TvMode;
    VoPubAttr.u32BgColor = 0xff;

    if (HI_SUCCESS != HI_MPI_VO_SetPubAttr(&VoPubAttr))
    {
        POSITION;
        return HI_FAILURE;
    }

    if (HI_SUCCESS != SampleSetVochnMscreen(ScreenCnt))
    {
        POSITION;
        return HI_FAILURE;
    }

    if (HI_SUCCESS != HI_MPI_VO_Enable())
    {
        POSITION;
        return HI_FAILURE;
    }

    for (i = 0; i < EnableNum; i++)
    {
        CHECK(HI_MPI_VO_EnableChn(i), "HI_MPI_VO_EnableChn");
    }

    return HI_SUCCESS;
}

HI_S32 SampleViBindVo(HI_U32 ScreenCnt)
{
    HI_U32 i;

    for (i = 0; i < ScreenCnt; i++)
    {
        CHECK(HI_MPI_VI_BindOutput(i/2, i%2, i), "HI_MPI_VI_BindOutput");
    }

    return HI_SUCCESS;
}

HI_S32 SampleViUnBindVo(HI_U32 ScreenCnt)
{
    HI_U32 i;

    for (i = 0; i < ScreenCnt; i++)
    {
        CHECK(HI_MPI_VI_UnBindOutput(i/2, i%2, i), "HI_MPI_VI_UnBindOutput");
    }

    return HI_SUCCESS;
}

HI_S32 SampleDisableVo(HI_U32 ScreenCnt, HI_U32 DisableNum)
{
    HI_U32 i;

    for (i = 0; i < DisableNum; i++)
    {
        CHECK(HI_MPI_VO_DisableChn(i), "HI_MPI_VO_DisableChn");
    }

    CHECK(HI_MPI_VO_Disable(), "HI_MPI_VO_Disable");

    return HI_SUCCESS;
}

HI_S32 SampleDisableVi(HI_U32 ScreenCnt)
{
    HI_U32 i;

    ScreenCnt = (ScreenCnt == 1 ? 2 : ScreenCnt);
    ScreenCnt = (ScreenCnt == 9 ? 8 : ScreenCnt);

    for (i = 0; i < ScreenCnt; i++)
    {
        CHECK(HI_MPI_VI_DisableChn(i/2, i%2), "HI_MPI_VI_DisableChn");
    }

    for (i = 0; i < ScreenCnt/2; i++)
    {
        CHECK(HI_MPI_VI_Disable(i), "HI_MPI_VI_Disable");
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :
 Description     : The following function is used to decode audio stream.
                   currently, we only support PT_ADPCMA.

*****************************************************************************/
/*
 *   @ create audio channel and prepare for audio decode
 */
HI_S32 Sample_Enable_Audio(HI_VOID)
{
    HI_S32 s32ret;
    HI_U32 i;
    AIO_ATTR_S stAttr;
    ADEC_CHN_ATTR_S stAdecAttr;
    AUDIO_DEV AoDevId = 0;
    AO_CHN AoChn = 0;

    stAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAttr.enWorkmode = AIO_MODE_I2S_MASTER;
    stAttr.u32EXFlag = 1;
    stAttr.u32FrmNum = 10;
    stAttr.u32PtNumPerFrm = 160;

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

    stAdecAttr.enType = PT_ADPCMA;
    stAdecAttr.u32BufSize = 28;
    stAdecAttr.enMode = ADEC_MODE_PACK;/* pack mode or stream mode ? */

    ADEC_ATTR_ADPCM_S stAdpcm;
    stAdecAttr.pValue = &stAdpcm;
    stAdpcm.enADPCMType = ADPCM_TYPE_DVI4;

    /* create adec chn*/
    s32ret = HI_MPI_ADEC_CreateChn(0, &stAdecAttr);
    if (s32ret)
    {
        printf("create adec chn %d err:0x%x\n", i,s32ret);
        return s32ret;
    }

    return HI_SUCCESS;
}

/*
 *   @ disable audio channel and device
 */
HI_S32 Sample_Disable_Audio(HI_VOID)
{
    CHECK(HI_MPI_AO_DisableChn(0, 0), "HI_MPI_AO_DisableChn");
    CHECK(HI_MPI_AO_Disable(0) ,"HI_MPI_AO_Disable");
    CHECK(HI_MPI_ADEC_DestroyChn(0), "HI_MPI_ADEC_DestroyChn");

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :
 Description     : The following two functions are used to decode.

*****************************************************************************/
static HI_VOID SampleDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }

    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }

    return ;
}

static HI_S32 SampleStartVdecMChn(HI_S32 s32VdecChn,HI_S32 s32VoChn, VDEC_SIZE_E vdecSize)
{
    HI_S32 s32ret;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_ATTR_H264_S stH264Attr;

	stH264Attr.u32Priority = 0;
	stH264Attr.enMode = H264D_MODE_STREAM;
	stH264Attr.u32RefFrameNum = 2;

    /* Picture width must be no less than the vedio width                    */
    switch (vdecSize)
    {
        case VDEC_SIZE_D1 :
        {
            stH264Attr.u32PicWidth = s_u32ScreenWidth;
            stH264Attr.u32PicHeight = s_u32ScreenHeight;
            break;
        }
        case VDEC_SIZE_CIF :
        {
            stH264Attr.u32PicWidth = 352;
            stH264Attr.u32PicHeight = 288;
            break;
        }
        default:
        {
            printf("Unsupport decode method!\n");
            return HI_FAILURE;
        }
    }

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    stAttr.enType = PT_H264;
    stAttr.u32BufSize = (((stH264Attr.u32PicWidth) * (stH264Attr.u32PicHeight))<<1);
    stAttr.pValue = (void*)&stH264Attr;

    s32ret = HI_MPI_VDEC_CreateChn(s32VdecChn, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_CreateChn %d failed errno 0x%x \n", s32VdecChn, s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VDEC_StartRecvStream(s32VdecChn);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_StartRecvStream %d failed errno 0x%x \n", s32VdecChn, s32ret);
        return s32ret;
    }

	if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(s32VdecChn, s32VoChn))
	{
        printf("bind vdec %d to vo %d failed\n", s32VdecChn, s32VoChn);
		SampleDestroyVdecChn(s32VdecChn);
		return HI_FAILURE;
	}

    return HI_SUCCESS;
}

HI_S32 ClearVdecAndVouBuffer(VO_CHN VoChn)
{
    CHECK(HI_MPI_VDEC_StopRecvStream(0),"Stop Vdec");
    CHECK(HI_MPI_VDEC_ResetChn(0),"Clear buffer");
    CHECK(HI_MPI_VDEC_StartRecvStream(0),"Start Vdec");

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : SampleSendH264StreamToDecode

 Description     : This is the thread function for general decoding.

*****************************************************************************/
void* SampleSendH264StreamToDecode(void* p)
{
	VDEC_STREAM_S stStream;
    FILE* file = NULL;
    HI_S32 s32ret;

#if 0
    static HI_U64 chnPts[4];
#endif

    NALU_t nalu;

    VDEC_SEND_STREAM_S *pChnAndFile = (VDEC_SEND_STREAM_S *)p;

    HI_ASSERT(p != NULL);

    #if 0
        printf("decChn = %d, filename = %s\n", pChnAndFile->VdecChn, pChnAndFile->fileName);
    #endif

	/*open the stream file*/
    file = fopen(pChnAndFile->fileName, "r");
    if (!file)
    {
        printf("open file %s err\n",pChnAndFile->fileName);
		return NULL;
    }

    /*申请存放 NALU 的内存*/
    nalu.max_size = ONE_SLICE_SIZE_MAX;
    nalu.buf = malloc(nalu.max_size);
    if (HI_NULL == nalu.buf)
    {
        return HI_NULL;
    }

    while(s_bVdecStopFlag != HI_TRUE)
    {
        H264_FRAME_HEADER_S header;
#if 1
        /*read file header*/
        s32ret = fread(&header,1,sizeof(H264_FRAME_HEADER_S),file);
        if(s32ret <=0)
        {
            fseek(file, 0L, SEEK_SET);
            continue;
        }

        s32ret = fread(nalu.buf,1,header.u32Len,file) ;
#else
		s32ret = fread(nalu.buf,1,ONE_SLICE_SIZE_MAX,file);
#endif
		if(s32ret <=0)
		{
            /* we can contrl file wrap here                                 */
            #if 0
                fseek(file,0,SEEK_SET);
                continue;
            #else
                puts("fread over!");
    		    break;
            #endif
		}

        nalu.len = s32ret;
        stStream.pu8Addr = nalu.buf;
        stStream.u32Len = nalu.len;
        stStream.u64PTS = header.u64PTS;

		s32ret = HI_MPI_VDEC_SendStream(pChnAndFile->VdecChn, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {
			printf("send to vdec 0x %x \n", s32ret);
            break;
        }
    }

    free(nalu.buf);

    fclose(file);
    return NULL;
}

/*****************************************************************************
 Prototype       : SampleSendAVStreamToDec

 Description     : This is the thread function for av-stream play contrl.

*****************************************************************************/
void* SampleSendAVStreamToDec(void* p)
{
	VDEC_STREAM_S stStream;
    AUDIO_STREAM_S stAudioStream;
    AUDIO_FRAME_INFO_S stAudioFrameInfo;
    FILE* file = NULL;
    HI_S32 s32ret;
    HI_U32 i;
    HI_BOOL lastIframe = HI_FALSE;
    static HI_S32 iFrameLen = 0;
    long offset;

    NALU_t nalu;
    HI_U8 *pu8AudioStream = NULL;

    VDEC_SEND_STREAM_S *pChnAndFile = (VDEC_SEND_STREAM_S *)p;

    /* av stream relative variable definition                               */
    H264_FRAME_HEADER_S  vheader;
    AUDIO_FRAME_HEADER_S aheader;
    AV_STREAM_HEADER_S   avheader;

    memset(&vheader, 0,sizeof(H264_FRAME_HEADER_S));
    memset(&aheader, 0,sizeof(AUDIO_FRAME_HEADER_S));
    memset(&avheader,0,sizeof(AV_STREAM_HEADER_S));

    HI_ASSERT(p != NULL);

	/*open the stream file*/
    file = fopen(pChnAndFile->fileName, "r");
    if (!file)
    {
        printf("open file %s err\n",pChnAndFile->fileName);
		return NULL;
    }

    /* apply memory space for store video stream */
    nalu.max_size = ONE_SLICE_SIZE_MAX;
    nalu.buf = malloc(nalu.max_size);
    if (HI_NULL == nalu.buf)
    {
        return HI_NULL;
    }

    pu8AudioStream = (HI_U8*)malloc(MAX_AUDIO_STREAM_LEN * sizeof(HI_U8));
    if (HI_NULL == pu8AudioStream)
    {
        return HI_NULL;
    }

    while(s_bVdecStopFlag != HI_TRUE)
    {
        /* 0. local definition                                              */

        /* 1.parse av frame header                                          */

        s32ret = fread(&avheader, sizeof(AV_STREAM_HEADER_S), 1, file);
        if (s32ret <= 0)
        {
            /* rewind to start of the file                                  */
            fseek(file, 0L, SEEK_SET);
            POSITION;

            continue;
        }

        /* 2. send video to decode                                          */

        if (s_enAVspeed == AV_PLAY_SPEED_BACK)
        {
            HI_U32 retType = 0;
            HI_S32 ioffset = 0;

            while (1)
            {
                if (lastIframe == HI_TRUE)
                {
                    offset = ftell(file);
                    if (offset == -1 || offset == 0 || offset < iFrameLen)
                    {
                        s32ret = fseek(file, 0L, SEEK_SET);
                        if (s32ret != 0)
                        {
                            perror("seek error!");
                            return NULL;
                        }

                        POSITION;

                        /* exit processing                                  */
                        lastIframe = HI_FALSE;
                        retType = 1;
                        break;
                    }

                    s32ret = fseek(file, -1*iFrameLen, SEEK_CUR);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        return NULL;
                    }

                    s32ret = fread(&avheader, sizeof(AV_STREAM_HEADER_S), 1, file);
                    if (s32ret <= 0)
                    {
                        /* rewind to start of the file                                  */
                        fseek(file, 0L, SEEK_SET);
                        POSITION;
                        lastIframe = HI_FALSE;
                        retType = 1;
                        break;
                    }
                }

                /* read one frame first                                         */
                s32ret = fread(&vheader,1,sizeof(H264_FRAME_HEADER_S),file);
                if (s32ret <=0)
                {
                    s32ret = fseek(file, 0L, SEEK_SET);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        return NULL;
                    }

                    POSITION;
                    lastIframe = HI_FALSE;
                    retType = 1;
                    break;
                }

                if (vheader.bIFrame == HI_TRUE)
                {
                    lastIframe = HI_TRUE;
                    iFrameLen = (HI_S32)(3 * sizeof(AV_STREAM_HEADER_S)
                                            + avheader.u32Len
                                            + avheader.u32LastLen
                                          );
                    break;
                }

                lastIframe = HI_FALSE;

                /* get the length of last frame structure                       */
                ioffset =(HI_S32)(2 * sizeof(AV_STREAM_HEADER_S)
                                  + sizeof(H264_FRAME_HEADER_S)
                                  + avheader.u32LastLen);

                /* if current offset is little than last frame length, go to start
                  of the file.                                                  */
                offset = ftell(file);
                if (offset == -1 || offset == 0 || offset < ioffset)
                {
                    s32ret = fseek(file, 0L, SEEK_SET);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        return NULL;
                    }

                    POSITION;

                    /* exit processing                                  */
                    lastIframe = HI_FALSE;
                    retType = 1;
                    break;
                }

                /* locate file pointer to previous frame                    */
                s32ret = fseek(file, -1*ioffset, SEEK_CUR);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    return NULL;
                }

                s32ret = fread(&avheader, sizeof(AV_STREAM_HEADER_S), 1, file);
                if (s32ret <= 0)
                {
                    /* rewind to start of the file                                  */
                    fseek(file, 0L, SEEK_SET);
                    POSITION;

                    lastIframe = HI_FALSE;
                    retType = 1;
                    break;
                }
            }

            if (retType == 1)
            {
                /* restart normal decode from start of the file             */
                vheader.u64PTS = 0;
                continue;
            }
        }
        else if (s_enAVspeed == AV_PLAY_SPEED_4X || s_enAVspeed == AV_PLAY_SPEED_8X)
        {
            HI_S32 leapLen = 0;
            lastIframe = HI_FALSE;

            s32ret = fread(&vheader,1,sizeof(H264_FRAME_HEADER_S),file);
            if (s32ret <=0)
            {
                s32ret = fseek(file, 0L, SEEK_SET);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    return NULL;
                }
                continue;
            }

            if (HI_TRUE == vheader.bIFrame)
            {
              #if 0
                printf("spd=%d,pts=%llu\n",
                    s_enAVspeed,
                    vheader.u64PTS
                    );
              #endif
            }
            else
            {
                s32ret = fseek(file, vheader.u32Len, SEEK_CUR);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    return NULL;
                }

                /* leap to next av frame header                             */
                {
                    leapLen = avheader.u32Len - sizeof(H264_FRAME_HEADER_S) - vheader.u32Len;
                    if (leapLen < 0)
                    {
                        puts("leaplen error!");
                        return NULL;
                    }
                    s32ret = fseek(file, (long)leapLen, SEEK_CUR);
                    if (s32ret != 0)
                    {
                        perror("fseek failed!");
                        return NULL;
                    }
                }
                continue;
            }
        }
        else
        {
            lastIframe = HI_FALSE;

            s32ret = fread(&vheader, sizeof(H264_FRAME_HEADER_S), 1, file);
            if (s32ret <= 0)
            {
                fseek(file, 0L, SEEK_SET);
                POSITION;
                continue;
            }
        }

#if 0
        printf("avheader=%d,vheader=%d,iframe=%d,vlen=%d,offset=%d\n"
            "avheader(cnt=%d,off=%d,last=%d,len=%d)\n"
            "vheaer(lst=%d,pts=%llu)\n",
            sizeof(AV_STREAM_HEADER_S),
            sizeof(H264_FRAME_HEADER_S),
            vheader.bIFrame,
            vheader.u32Len,
            ftell(file),
            avheader.u32AuCnt,
            avheader.u32AuOffset,
            avheader.u32LastLen,
            avheader.u32Len,
            vheader.u32LastLen,
            vheader.u64PTS
            );
#endif

        /* read video data to decoder                                      */
        s32ret = fread(nalu.buf,1,vheader.u32Len,file);
        if (s32ret <= 0)
        {
            puts("fread over!");
            POSITION;
		    break;
        }

        nalu.len = s32ret;
        stStream.pu8Addr = nalu.buf;
        stStream.u32Len = nalu.len;
        stStream.u64PTS = vheader.u64PTS;

		s32ret = HI_MPI_VDEC_SendStream(pChnAndFile->VdecChn, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {
			printf("send to vdec %#x \n", s32ret);

            /* leap to next av frame header                             */
            {
                HI_S32 leapLen = 0;
                leapLen = avheader.u32Len - sizeof(H264_FRAME_HEADER_S) - vheader.u32Len;
                if (leapLen < 0)
                {
                    puts("leaplen error!");
                    return NULL;
                }
                s32ret = fseek(file, (long)leapLen, SEEK_CUR);
                if (s32ret != 0)
                {
                    perror("fseek failed!");
                    return NULL;
                }
            }

            continue;
        }

        /* while in specail play speed, we needn't to decode audio.         */
        if (s_enAVspeed != AV_PLAY_SPEED_NORMAL
            || s_bNeedSilence == HI_TRUE)
        {
            HI_S32 leapLen = 0;
            leapLen = avheader.u32Len - sizeof(H264_FRAME_HEADER_S) - vheader.u32Len;
            if (leapLen < 0)
            {
                puts("leaplen error!");
                fseek(file, 0L, SEEK_SET);
                continue;
            }
            s32ret = fseek(file, (long)leapLen, SEEK_CUR);
            if (s32ret != 0)
            {
                perror("fseek failed!");
                return NULL;
            }
            continue;
        }

        /* 3. send audio stream to decode in while                          */

        for (i = 0 ; i < avheader.u32AuCnt; i++)
        {
            s32ret = fread(&aheader, sizeof(AUDIO_FRAME_HEADER_S), 1, file);
            if (s32ret <= 0)
            {
                printf("read audio header failed!\n");
                POSITION;
                break;
            }
#if 0
            printf("aheader(lst=%d,len=%d,seq=%d,pts=%llu,offset(%d))\n",
                aheader.u32LastLen,
                aheader.u32Len,
                aheader.u32Seq,
                aheader.u64PTS,
                ftell(file)
            );
#endif

            /* read raw data from file */
            s32ret = fread(pu8AudioStream, 1, aheader.u32Len, file);
            if (s32ret <= 0)
            {
                perror("adec fread raw data");
                POSITION;
                break;
            }

            stAudioStream.pStream = pu8AudioStream;
            stAudioStream.u32Len = aheader.u32Len;
            stAudioStream.u32Seq = aheader.u32Seq;
            stAudioStream.u64TimeStamp = aheader.u64PTS;

            s32ret = HI_MPI_ADEC_SendStream(0, &stAudioStream);
            if (s32ret)
            {
                printf("send stream to adec fail,err:%x\n", s32ret);
                break;
            }

            /* get audio frme from adec after decoding*/
            s32ret = HI_MPI_ADEC_GetData(0, &stAudioFrameInfo);
            if (HI_SUCCESS != s32ret)
            {
                printf("adec get data err\n");
                break;
            }

            /* send audio frme to ao */
            s32ret = HI_MPI_AO_SendFrame(0, 0, stAudioFrameInfo.pstFrame, HI_IO_BLOCK);
            if (HI_SUCCESS != s32ret)
            {
                printf("ao send frame err:0x%x\n",s32ret);
                return NULL;
            }

            s32ret = HI_MPI_ADEC_ReleaseData(0, &stAudioFrameInfo);
            if (HI_SUCCESS != s32ret)
            {
                printf("adec release data err\n");
                break;
            }
        }
    }

    free(nalu.buf);
    free(pu8AudioStream);
    nalu.buf = HI_NULL;
    pu8AudioStream = HI_NULL;

    fclose(file);
    return NULL;
}


/*****************************************************************************
 Prototype       : SampleSendStreamToDecode4Contrl

 Description     : This is the thread function for video play contrl.

*****************************************************************************/
void* SampleSendStreamToDecode4Contrl(void* p)
{
	VDEC_STREAM_S stStream;
    FILE* file = NULL;
    HI_S32 s32ret;
    long offset;
    HI_BOOL lastIframe = HI_FALSE;
    HI_S32 iFrameLen = 0;

#if 0
    static HI_U64 chnPts[4];
#endif

    NALU_t nalu;

    VDEC_SEND_STREAM_S *pChnAndFile = (VDEC_SEND_STREAM_S *)p;

    HI_ASSERT(p != NULL);

    #if 0
        printf("decChn = %d, filename = %s\n", pChnAndFile->VdecChn, pChnAndFile->fileName);
    #endif

	/*open the stream file*/
    file = fopen(pChnAndFile->fileName, "r");
    if (!file)
    {
        printf("open file %s err\n",pChnAndFile->fileName);
		return NULL;
    }

    /*申请存放 NALU 的内存*/
    nalu.max_size = ONE_SLICE_SIZE_MAX;
    nalu.buf = malloc(nalu.max_size);
    if (HI_NULL == nalu.buf)
    {
        return HI_NULL;
    }

    while(s_bVdecStopFlag != HI_TRUE)
    {
#ifdef STREAM_WITH_PTS

        H264_FRAME_HEADER_S header;
        HI_S32 ioffset;

        if (s_u32UltraSpeed == 1)
        {
            HI_U32 retType = 0;

    #ifdef FRAME_WITH_LAST_PTS
            while (1)
            {

                if (lastIframe == HI_TRUE)
                {
                    offset = ftell(file);
                    if (offset == -1 || offset == 0 || offset < iFrameLen)
                    {
                        s32ret = fseek(file, 0L, SEEK_SET);
                        if (s32ret != 0)
                        {
                            perror("seek error!");
                            return NULL;
                        }

                        /* exit processing                                  */
                        lastIframe = HI_FALSE;
                        fseek(file, sizeof(H264_FRAME_HEADER_S), SEEK_SET);
                        s_u32UltraSpeed = 0;
                        break;
                    }

                    s32ret = fseek(file, -1*iFrameLen, SEEK_CUR);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        lastIframe = HI_FALSE;
                        return NULL;
                    }
                }

                /* read one frame first                                         */
                s32ret = fread(&header,1,sizeof(H264_FRAME_HEADER_S),file);
                if (s32ret <=0)
                {
                    s32ret = fseek(file, 0L, SEEK_SET);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        return NULL;
                    }
                    retType = 1;
                    break;
                }

                if (header.bIFrame == HI_TRUE)
                {
                    lastIframe = HI_TRUE;
                    iFrameLen = (HI_S32)(2 * sizeof(H264_FRAME_HEADER_S) + header.u32Len
                                                                         + header.u32LastLen);
                    break;
                }

                lastIframe = HI_FALSE;

                /* get the length of last frame structure                       */
                ioffset =(HI_S32)(2 * sizeof(H264_FRAME_HEADER_S) + header.u32LastLen);
#if 1
                /* if current offset is little than last frame length, go to start
                  of the file.                                                  */
                offset = ftell(file);
                if (offset == -1 || offset == 0 || offset < ioffset)
                {
                    s32ret = fseek(file, 0L, SEEK_SET);
                    if (s32ret != 0)
                    {
                        perror("seek error!");
                        return NULL;
                    }

                    /* exit processing                                  */
                    lastIframe = HI_FALSE;
                    fseek(file, sizeof(H264_FRAME_HEADER_S), SEEK_SET);
                    s_u32UltraSpeed = 0;
                    break;
                }
#endif
                /* locate file pointer to previous frame                    */
                s32ret = fseek(file, -1*ioffset, SEEK_CUR);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    lastIframe = HI_FALSE;
                    return NULL;
                }
            }

            if (retType == 1)
            {
                /* restart normal decode from start of the file             */
                header.u64PTS = 0;
                continue;
            }

    #endif /* End of FRAME_WITH_LAST_PTS*/
        }
        else if (s_u32UltraSpeed != 0)
        {
            lastIframe = HI_FALSE;
            s32ret = fread(&header, 1, sizeof(H264_FRAME_HEADER_S), file);
            if(s32ret <= 0)
            {
                s32ret = fseek(file, 0L, SEEK_SET);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    return NULL;
                }
                continue;
            }

            if (HI_TRUE == header.bIFrame)
            {
            }
            else
            {
                s32ret = fseek(file, header.u32Len, SEEK_CUR);
                if (s32ret != 0)
                {
                    perror("seek error!");
                    return NULL;
                }
                continue;
            }
        }
        else
        {
            lastIframe = HI_FALSE;

            /* read file header */
            s32ret = fread(&header,1,sizeof(H264_FRAME_HEADER_S),file);
            if(s32ret <=0)
            {
                fseek(file, 0L, SEEK_SET);
                continue;
            }
        }

        s32ret = fread(nalu.buf,1,header.u32Len,file) ;
#else
		s32ret = fread(nalu.buf,1,ONE_SLICE_SIZE_MAX,file);
#endif
		if(s32ret <=0)
		{
            /* we can contrl file wrap here                                 */
            #if 0
                fseek(file,0,SEEK_SET);
                continue;
            #else
                puts("fread over!");
    		    break;
            #endif
		}

        nalu.len = s32ret;
        stStream.pu8Addr = nalu.buf;
        stStream.u32Len = nalu.len;
        stStream.u64PTS = header.u64PTS;

		s32ret = HI_MPI_VDEC_SendStream(pChnAndFile->VdecChn, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32ret)
        {
			printf("send to vdec 0x %x \n", s32ret);
            break;
        }
    }

    free(nalu.buf);
    fclose(file);
    return NULL;
}

/*****************************************************************************
 Prototype       : SampleSyncDec

 Description     : This function is used to show how to implement synchronous
                   playback with multi-channel decoding.

*****************************************************************************/
HI_S32 SampleSyncDec(HI_U32 NumChn)
{
    #define MAX_VO_SYNC_CHN 4

    HI_CHAR ch = '\0';
    HI_U32 ChnNum = NumChn;
    HI_U32 i,j;
    VO_GRP VoGroup = 0;

    pthread_t pidSndStream[MAX_VO_SYNC_CHN];

    VDEC_SEND_STREAM_S stChnAndFile[MAX_VO_SYNC_CHN];
    VO_SYNC_GROUP_ATTR_S stSyncGrpAttr;

    s_bVdecStopFlag = HI_FALSE;
    s_bStartFlag = HI_FALSE;

    memset(stChnAndFile, 0, MAX_VO_SYNC_CHN * sizeof(VDEC_SEND_STREAM_S));
    memset(&stSyncGrpAttr, 0 , sizeof (VO_SYNC_GROUP_ATTR_S));

    for (j = 0; j < MAX_VO_SYNC_CHN; j++)
    {
        stChnAndFile[j].VdecChn = j;
        sprintf(stChnAndFile[j].fileName, "stream%d.h264", j);
    }

    i = (ChnNum == 3) ? 1 : 0;
    for (; i < MAX_VO_SYNC_CHN; i++)
    {
        /* create vdec channel and bind vo channel                          */
        CHECK(SampleStartVdecMChn(i, i, VDEC_SIZE_CIF), "SampleStartVdecChn");

        /* send stream from file to vdec                                    */
        if (0 != pthread_create(&pidSndStream[i], NULL,
                        SampleSendH264StreamToDecode, (void*)&stChnAndFile[i]))
        {
            SampleDestroyVdecChn(i);
            printf("create send stream from file(%d) failed!\n", i);
            return HI_FAILURE;
        }
    }

#if 1

    /* synchronize operation                                            */

    /* create synchronous group                                             */
    stSyncGrpAttr.u32SynFrameRate = s_u32NormFrameRate;
    stSyncGrpAttr.enSyncMode = VO_SYNC_MODE_MIN_CHN;
    stSyncGrpAttr.u32UsrBaseChn = 3;

    CHECK(HI_MPI_VO_CreateSyncGroup(VoGroup, &stSyncGrpAttr), "HI_MPI_VO_CreateSyncGroup");

    /* register vo channel to synchronous group                             */
    i = (ChnNum == 3) ? 1 : 0;
    for (; i < MAX_VO_SYNC_CHN; i++)
    {
        if (HI_SUCCESS != HI_MPI_VO_RegisterSyncGroup(i, VoGroup))
        {
            printf("vo(%d) register group(%d) failed!\n", i, VoGroup);
            return HI_FAILURE;
        }
    }

    /* start/stop synchronous group                                         */
    do
    {
        if (ch == '\n')
            continue;

        puts("-menu-------------------------");
        puts("'q' for exit decode!");
        puts("'s' for starting synchronize!");
        puts("'t' for stop synchronize!");
        puts("'p' for pause synchronize!");
        puts("'e' for step synchronize!");
        puts("'r' for resume synchronize!");
        puts("'l' for slow playing of synchronize!");
        puts("'f' for fast playing of synchronize!");
        puts("'n' for normal playing of synchronize!");
        puts("'y' for set base pts!");
        puts("'a' for non-sync chn pause!");
        puts("'b' for non-sync chn step!");
        puts("'c' for non-sync chn resume!");
        puts("------------------------------");

        if ('s' == ch)
        {
            CHECK(HI_MPI_VO_SyncGroupStart(VoGroup), "HI_MPI_VO_SyncGroupStart");
            s_bStartFlag = HI_TRUE;
        }
        else if ('t' == ch)
        {
            if (HI_TRUE == s_bStartFlag)
            {
                CHECK(HI_MPI_VO_SyncGroupStop(VoGroup), "HI_MPI_VO_SyncGroupStop");
                s_bStartFlag = HI_FALSE;
            }
            else
            {
                puts("group not start!");
            }
        }
        else if ('p' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }
            puts("PAUSE group!");
            CHECK(HI_MPI_VO_PauseSyncGroup(VoGroup), "HI_MPI_VO_PauseSyncGroup");
        }
        else if ('e' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }

            for (i = 0; i < 10; i++)
            {
                puts("STEP group!");
                CHECK(HI_MPI_VO_StepSyncGroup(VoGroup), "HI_MPI_VO_StepSyncGroup");
                sleep(2);
            }
            CHECK(HI_MPI_VO_ResumeSyncGroup(VoGroup), "HI_MPI_VO_ResumeSyncGroup");
        }
        else if ('r' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }

            puts("RESUME group!");
            CHECK(HI_MPI_VO_ResumeSyncGroup(VoGroup), "HI_MPI_VO_ResumeSyncGroup");
        }
        else if ('l' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }

            puts("SLOW down!");
            CHECK(HI_MPI_VO_SetSyncGroupFrameRate(VoGroup, 12),"SLOW DOWN");
        }
        else if ('f' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }

            puts("FAST play");
            CHECK(HI_MPI_VO_SetSyncGroupFrameRate(VoGroup, 50),"FAST FORWARD");
        }
        else if ('n' == ch)
        {
            if (HI_TRUE != s_bStartFlag)
            {
                puts("synchronize doesn't start!");
                continue;
            }

            puts("NORMAL speed");
            CHECK(HI_MPI_VO_SetSyncGroupFrameRate(VoGroup, 25), "NORMAL SPEED");
        }
        else if ('a' == ch)
        {
            if (HI_TRUE == s_bStartFlag)
            {
                puts("you should use synchronous method!");
            }

            puts("non-sync pause");
            CHECK(HI_MPI_VO_ChnPause(1), "HI_MPI_VO_ChnPause");
        }
        else if ('y' == ch)
        {
            VO_SYNC_BASE_S stSyncBase;
            stSyncBase.VoChn = 0;
            stSyncBase.u64BasePts = 0;

            if (HI_TRUE == s_bStartFlag)
            {
                puts("you should use synchronous method!");
            }

            printf("please input pts:");
            scanf("%llu",&stSyncBase.u64BasePts);

            puts("set base pts");
            CHECK(HI_MPI_VO_SetSyncGroupBase(VoGroup, &stSyncBase),"RESET BASE");
        }
        else if ('b' == ch)
        {
            if (HI_TRUE == s_bStartFlag)
            {
                puts("you should use synchronous method!");
            }

            puts("non-sync step");
            CHECK(HI_MPI_VO_ChnStep(1), "HI_MPI_VO_ChnStep");
        }
        else if ('c' == ch)
        {
            if (HI_TRUE == s_bStartFlag)
            {
                puts("you should use synchronous method!");
            }

            puts("non-sync resume");
            CHECK(HI_MPI_VO_ChnResume(1), "HI_MPI_VO_ChnResume");
        }

    } while ( (ch = getchar()) != 'q');

    /* inorder to prevent exit while step                                   */
    CHECK(HI_MPI_VO_ChnResume(1), "HI_MPI_VO_ChnResume");

    if (HI_TRUE == s_bStartFlag)
    {
        CHECK(HI_MPI_VO_SyncGroupStop(VoGroup), "HI_MPI_VO_SyncGroupStop");
    }

    /* unregister vo channel from synchronous group                         */
    i = (ChnNum == 3) ? 1 : 0;
    for (; i < MAX_VO_SYNC_CHN; i++)
    {
        if (HI_SUCCESS != HI_MPI_VO_UnRegisterSyncGroup(i, VoGroup))
        {
            printf("vo(%d) unregister group(%d) failed!\n", i, VoGroup);
            return HI_FAILURE;
        }
    }

    /* destroy synchronous group                                            */
    CHECK(HI_MPI_VO_DestroySyncGroup(VoGroup), "HI_MPI_VO_DestroySyncGroup");

#endif

    s_bVdecStopFlag = HI_TRUE;

    i = (ChnNum == 3) ? 1 : 0;
    for (; i < MAX_VO_SYNC_CHN; i++ )
    {
        /* inorder to prevent exit while step                                   */
        CHECK(HI_MPI_VO_ChnResume(i),"Resume");

        pthread_join(pidSndStream[i], NULL);
        CHECK(HI_MPI_VDEC_UnbindOutput(i),"HI_MPI_VDEC_UnbindOutput");
        SampleDestroyVdecChn(i);
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : Sample2CifDec

 Description     : This function is used to show how to implement play contrl
                   of video stream.

                   In order to compare with the normal preview, there are two
                   channel of pictures, one for decoding directly with normal
                   speed, another for your playing contrl freely.

                   Its contrl contains 1x,2x,4x,8x,1/2x,backforward,pause,
                   resume,step forward.

*****************************************************************************/
HI_S32 Sample2CifDec(HI_VOID)
{
    HI_U32 i;
    HI_CHAR ch;
    VO_CHN VoChn = 1;
    pthread_t pidSndStream[2];

    VDEC_SEND_STREAM_S stChnAndFile[2];

    s_bVdecStopFlag = HI_FALSE;

    for (i = 0; i < 2; i++)
    {
        /* set vdec channel and file name                                   */
        stChnAndFile[i].VdecChn = i;
        sprintf(stChnAndFile[i].fileName, "stream%d.h264", 0);

        /* create vdec channel and bind vo channel                          */
        CHECK(SampleStartVdecMChn(i, i, VDEC_SIZE_CIF), "SampleStartVdecChn");
    }

    /* send stream from file to vdec                                    */
    if (0 != pthread_create(&pidSndStream[0], NULL,
                    SampleSendH264StreamToDecode, (void*)&stChnAndFile[0]))
    {
        SampleDestroyVdecChn(0);
        printf("create send stream from file(%d) failed!\n", 0);
        return HI_FAILURE;
    }

    /* send stream from file to vdec                                    */
    if (0 != pthread_create(&pidSndStream[1], NULL,
                    SampleSendStreamToDecode4Contrl, (void*)&stChnAndFile[1]))
    {
        SampleDestroyVdecChn(1);
        printf("create send stream from file(%d) failed!\n", 1);
        return HI_FAILURE;
    }

    /* start channel 2's play contrl                                        */
    do
    {
        if (ch == '\n')
            continue;

        puts("-menu-------------------------");
        puts("'1' for 1X!");
        puts("'2' for 2X!");
        puts("'4' for 4X!");
        puts("'8' for 8X!");
        puts("'s' for slow play!");
        puts("'i' for inverse play!");
        puts("'p' for pause!");
        puts("'r' for resume!");
        puts("'t' for step forward!");
        puts("'q' for exit!");
        puts("------------------------------");

        s_u32UltraSpeed = 0;

        if ('1' == ch)
        {
            puts("1X speed");
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"1X");
        }
        else if ('2' == ch)
        {
            puts("2X speed");
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 2*s_u32NormFrameRate),"2X");
        }
        else if ('4' == ch)
        {
            puts("4X speed");

            s_u32UltraSpeed = 0;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"1X");
            usleep(1000);

            ClearVdecAndVouBuffer(0);
            s_u32UltraSpeed = 4;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 4*s_u32NormFrameRate),"4X");
        }
        else if ('8' == ch)
        {
            puts("8X speed");

            s_u32UltraSpeed = 0;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"1X");
            usleep(1000);

            ClearVdecAndVouBuffer(0);
            s_u32UltraSpeed = 8;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 8*s_u32NormFrameRate),"8X");
        }
        else if ('s' == ch)
        {
            puts("half speed");
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate/2),"Half");
        }
        else if ('i' == ch)
        {
            puts("inverse play");
            ClearVdecAndVouBuffer(0);
            s_u32UltraSpeed = 1;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, -1*s_u32NormFrameRate),"inverse");
        }
        else if ('p' == ch)
        {
            puts("pause");
            CHECK(HI_MPI_VO_ChnPause(VoChn),"Pause");
        }
        else if ('r' == ch)
        {
            puts("resume");
            CHECK(HI_MPI_VO_ChnResume(VoChn),"Resume");
        }
        else if ('t' == ch)
        {
            puts("step");
            CHECK(HI_MPI_VO_ChnStep(VoChn),"Step");
        }
        else
        {
            #if 0
            puts("unKnow Command!");
            #endif
        }

    }while ((ch = getchar()) != 'q');

    /* inorder to prevent exit while step                                   */
    CHECK(HI_MPI_VO_ChnResume(VoChn),"Resume");

    s_bVdecStopFlag = HI_TRUE;

    for (i = 0; i < 2; i++ )
    {
        pthread_join(pidSndStream[i], NULL);
        CHECK(HI_MPI_VDEC_UnbindOutput(i),"HI_MPI_VDEC_UnbindOutput");
        SampleDestroyVdecChn(i);
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : SampleAVContrl

 Description     : This function is used to show how to implement play contrl
                   of audio and video complex stream.
                   Its contrl contains 1x,2x,4x,8x,1/2x,backforward,pause,
                   resume,step forward.

*****************************************************************************/
HI_S32 SampleAVContrl(HI_VOID)
{
    HI_CHAR ch;
    VO_CHN VoChn = 0;
    pthread_t pidSndStream;

    VDEC_SEND_STREAM_S stChnAndFile;

    s_bVdecStopFlag = HI_FALSE;

    /* set vdec channel and file name                                   */
    stChnAndFile.VdecChn = 0;
    sprintf(stChnAndFile.fileName, "stream%d.av", 0);

    /* create vdec channel and bind vo channel                          */
    CHECK(SampleStartVdecMChn(0, 0, VDEC_SIZE_CIF), "SampleStartVdecChn");

    /* create audio decode channel and start it                             */
    CHECK(Sample_Enable_Audio(),"Sample_Enable_Audio");

    /* send stream from file to vdec                                    */
    if (0 != pthread_create(&pidSndStream, NULL,
                    SampleSendAVStreamToDec, (void*)&stChnAndFile))
    {
        SampleDestroyVdecChn(0);
        printf("create send stream from file(%d) failed!\n", 0);
        return HI_FAILURE;
    }

    /* start channel 2's play contrl                                        */
    do
    {
        if (ch == '\n')
            continue;

        puts("-menu-------------------------");
        puts("'1' for 1X!");
        puts("'2' for 2X!");
        puts("'4' for 4X!");
        puts("'8' for 8X!");
        puts("'s' for slow play!");
        puts("'i' for inverse play!");
        puts("'b' for back forward 4X!");
        puts("'c' for back forward 8X!");
        puts("'p' for pause!");
        puts("'r' for resume!");
        puts("'t' for step forward!");
        puts("'q' for exit!");
        puts("------------------------------");

        if ('1' == ch && s_enAVspeed != AV_PLAY_SPEED_NORMAL)
        {
            puts("1X speed");
            s_enAVspeed = AV_PLAY_SPEED_NORMAL;

            ClearVdecAndVouBuffer(0);

            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"1X");
        }
        else if ('2' == ch && s_enAVspeed != AV_PLAY_SPEED_2X)
        {
            puts("2X speed");
            ClearVdecAndVouBuffer(0);

            s_enAVspeed = AV_PLAY_SPEED_2X;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 2*s_u32NormFrameRate),"2X");
        }
        else if ('4' == ch && s_enAVspeed != AV_PLAY_SPEED_4X)
        {
            puts("4X speed");
            ClearVdecAndVouBuffer(0);
            s_enAVspeed = AV_PLAY_SPEED_4X;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 4*s_u32NormFrameRate),"4X");
        }
        else if ('8' == ch && s_enAVspeed != AV_PLAY_SPEED_8X)
        {
            puts("8X speed");
            ClearVdecAndVouBuffer(0);

            s_enAVspeed = AV_PLAY_SPEED_8X;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, 8*s_u32NormFrameRate),"8X");
        }
        else if ('s' == ch && s_enAVspeed != AV_PLAY_SPEED_SLOW)
        {
            puts("half speed");
            s_enAVspeed = AV_PLAY_SPEED_SLOW;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate/2),"Half");
        }
        else if ('i' == ch && s_enAVspeed != AV_PLAY_SPEED_BACK)
        {
            puts("inverse play");

            /* we need to return to normal play first                       */
            s_enAVspeed = AV_PLAY_SPEED_NORMAL;
            s_bNeedSilence = HI_TRUE;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"inverse");
            usleep(1000);

            ClearVdecAndVouBuffer(0);
            s_bNeedSilence = HI_FALSE;
            s_enAVspeed = AV_PLAY_SPEED_BACK;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, -1 * s_u32NormFrameRate),"inverse");

        }
        else if ('b' == ch && s_enAVspeed != AV_PLAY_SPEED_BACK)
        {
            /* back forward                                                 */
            puts("back forward 4X");

            /* we need to return to normal play first                       */
            s_enAVspeed = AV_PLAY_SPEED_NORMAL;
            s_bNeedSilence = HI_TRUE;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"inverse");
            usleep(1000);

            ClearVdecAndVouBuffer(0);
            s_bNeedSilence = HI_FALSE;
            s_enAVspeed = AV_PLAY_SPEED_BACK;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, -4 * s_u32NormFrameRate),"inverse");
        }
        else if ('c' == ch && s_enAVspeed != AV_PLAY_SPEED_BACK)
        {
            /* back forward                                                 */
            puts("back forward 8X");

            /* we need to return to normal play first                       */
            s_enAVspeed = AV_PLAY_SPEED_NORMAL;
            s_bNeedSilence = HI_TRUE;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, s_u32NormFrameRate),"inverse");
            usleep(1000);

            ClearVdecAndVouBuffer(0);
            s_bNeedSilence = HI_FALSE;
            s_enAVspeed = AV_PLAY_SPEED_BACK;
            CHECK(HI_MPI_VO_SetFrameRate(VoChn, -8 * s_u32NormFrameRate),"inverse");
        }
        else if ('p' == ch)
        {
            puts("pause");
            CHECK(HI_MPI_VO_ChnPause(VoChn),"Pause");
        }
        else if ('r' == ch)
        {
            puts("resume");
            CHECK(HI_MPI_VO_ChnResume(VoChn),"Resume");
        }
        else if ('t' == ch)
        {
            puts("step");
            CHECK(HI_MPI_VO_ChnStep(VoChn),"Step");
        }
        else
        {
            #if 0
            puts("unKnow Command!");
            #endif
        }

    }while ((ch = getchar()) != 'q');

    /* inorder to prevent exit while step                                   */
    CHECK(HI_MPI_VO_ChnResume(VoChn),"Resume");

    s_enAVspeed = AV_PLAY_SPEED_NORMAL;
    s_bVdecStopFlag = HI_TRUE;

    pthread_join(pidSndStream, NULL);

    CHECK(HI_MPI_VDEC_UnbindOutput(0),"HI_MPI_VDEC_UnbindOutput");
    CHECK(Sample_Disable_Audio(),"Sample_Disable_Audio");

    SampleDestroyVdecChn(0);

    return HI_SUCCESS;
}


/*****************************************************************************
 Prototype       :  1CIF_VI2VO_3CIF_SYNC_DEC
                    4CIF_SYNC_DEC
                    4CIF_VI2VO

 Description     : This section is where the main function will implement.

*****************************************************************************/
HI_S32 SAMPLE_4CIF_VI2VO(HI_VOID)
{
    CHECK(SampleInitViMScreen(4), "SampleInitViMScreen");
    CHECK(SampleInitVoMScreen(4,4), "SampleInitVoMScreen");
    CHECK(SampleViBindVo(4), "SampleViBindVo");

    while ( getchar() != 'q')
    {
        printf("\033[0;32mpress 'q' for exit vi2vo preview!\033[0;39m\n");
    }

    CHECK(SampleViUnBindVo(4),"SampleViUnBindVo(4)");
    CHECK(SampleDisableVi(4),"SampleDisableVi(4)");
    CHECK(SampleDisableVo(4,4),"SampleDisableVo(4)");

    return HI_SUCCESS;
}

HI_S32 SAMPLE_1CIF_VI2VO_3CIF_SYNC_DEC(HI_VOID)
{
    HI_U32 ChnNum = 3;

    /* start 1 channel preview and make vo as 4 split screen */
    CHECK(SampleInitViMScreen(1), "SampleInitViMScreen");
    CHECK(SampleInitVoMScreen(4,4), "SampleInitVoMScreen");
    CHECK(SampleViBindVo(1), "SampleViBindVo");

    CHECK(SampleSyncDec(ChnNum),"SampleSyncDec");

    /* disable vi2vo and disable vo                                         */
    CHECK(SampleViUnBindVo(1),"SampleViUnBindVo");
    CHECK(SampleDisableVi(1),"SampleDisableVi");
    CHECK(SampleDisableVo(4,4),"SampleDisableVo");

    return HI_SUCCESS;
}

HI_S32 SAMPLE_4CIF_SYNC_DEC(HI_VOID)
{
    HI_U32 ChnNum = 4;

    CHECK(SampleInitVoMScreen(4,4), "SampleInitVoMScreen");

    CHECK(SampleSyncDec(ChnNum),"SampleSyncDec");

    CHECK(SampleDisableVo(4,4),"SampleDisableVo");

    return HI_SUCCESS;
}

HI_S32 SAMPLE_2CIF_DEC_CONTRL(HI_VOID)
{
    CHECK(SampleInitVoMScreen(4,2), "SampleInitVoMScreen");

    CHECK(Sample2CifDec(),"Sample2CifDec");

    CHECK(SampleDisableVo(4,2),"SampleDisableVo");

    return HI_SUCCESS;
}

HI_S32 SAMPLE_AV_STREAM_CONTRL(HI_VOID)
{
    CHECK(SampleInitVoMScreen(4,1), "SampleInitVoMScreen");

    CHECK(SampleAVContrl(),"SampleAVContrl");

    CHECK(SampleDisableVo(4,1),"SampleDisableVo");

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :  HandleSig & Usage
 Description     : The following two functions are used
                    to process abnormal case and provide help respectively.

*****************************************************************************/

/* function to process abnormal case                                        */
HI_VOID HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
    	(HI_VOID)HI_MPI_SYS_Exit();
    	(HI_VOID)HI_MPI_VB_Exit();
    	printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}

/* usage for user                                                           */
HI_VOID Usage(HI_VOID)
{
    printf("\n/************************************/\n");
    printf("1  --> 1CIF_VI2VO_3CIF_SYNC_DEC\n");
    printf("2  --> 4CIF_SYNC_DEC\n");
    printf("3  --> 4CIF_VI2VO\n");
    printf("4  --> 2CIF_DEC_CONTRL\n");
    printf("5  --> AV_STREAM_CONTRL\n");
    printf("q  --> quit\n");
    printf("\033[0;32mplease enter order:\033[0;39m\n");
}

/*****************************************************************************
 Prototype       : main
 Description     : main entry
 Input           : argc    : "reset", "N", "n" is optional.
                   argv[]  : 0 or 1 is optional
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/8/30
    Author       : x00100808
    Modification : Created function

*****************************************************************************/
int main(int argc, char *argv[])
{
	char ch;

    /* configure for vi2vo preview                                         */
	MPP_SYS_CONF_S stSysConf = {0};
	VB_CONF_S stVbConf ={0};

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 20;
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 60;
	stSysConf.u32AlignWidth = 64;

    if (argc > 1)
    {
        /* if program can not run anymore, you may try 'reset' adhere command  */
        if (!strcmp(argv[1],"reset"))
        {
           	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
        	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
        	return HI_SUCCESS;
        }
        else if ((!strcmp(argv[1],"N") || (!strcmp(argv[1],"n"))))
        {
            /* if you live in Japan or North America, you may need adhere 'N/n' */
            s_u32TvMode = VIDEO_ENCODING_MODE_NTSC;
            s_u32ScreenHeight = 480;
        }
        else
        {
            /* you can expand command freely here                           */
        }
    }

    /* process abnormal case                                                */
    signal(SIGINT, HandleSig);
    signal(SIGTERM, HandleSig);

    /* configure video buffer and initial system                            */
	CHECK(HI_MPI_VB_SetConf(&stVbConf),"HI_MPI_VB_SetConf");
	CHECK(HI_MPI_VB_Init(),"HI_MPI_VB_Init");
	CHECK(HI_MPI_SYS_SetConf(&stSysConf),"HI_MPI_SYS_SetConf");
	CHECK(HI_MPI_SYS_Init(),"HI_MPI_SYS_Init");


    /* set AD and DA for VI and VO                                          */
    do_Set2815_2d1();
    if (VIDEO_ENCODING_MODE_NTSC == s_u32TvMode)
    {
        do_Set7179(VIDEO_ENCODING_MODE_NTSC);
        s_u32NormFrameRate = 30;
    }
    else
    {
        do_Set7179(VIDEO_ENCODING_MODE_PAL);
        s_u32NormFrameRate = 25;
    }

	Usage();
	while((ch = getchar())!= 'q')
	{

		if('\n' == ch)
		{
			continue;
		}

		switch(ch)
		{
			case '1':
			{
			    printf("\033[0;33m1CIF_VI2VO_3CIF_SYNC_DEC!\033[0;39m\n");
				CHECK(SAMPLE_1CIF_VI2VO_3CIF_SYNC_DEC(),"1CIF_VI2VO_3CIF_SYNC_DEC");
				break;
			}

			case '2':
			{
    			printf("\033[0;33m4CIF_SYNC_DEC!\033[0;39m\n");
				CHECK(SAMPLE_4CIF_SYNC_DEC(),"4CIF_SYNC_DEC");
				break;
			}

            case '3':
            {
    			printf("\033[0;33m4CIF_VI2VO!\033[0;39m\n");
				CHECK(SAMPLE_4CIF_VI2VO(),"4CIF_VI2VO");
				break;
            }

            case '4':
            {
                printf("\033[0;33m2CIF_DEC_CONTRL!\033[0;39m\n");
                CHECK(SAMPLE_2CIF_DEC_CONTRL(), "2CIF_DEC_CONTRL");
                break;
            }

            case '5' :
            {
                printf("\033[0;33mAV_STREAM_CONTRL!\033[0;39m\n");
                CHECK(SAMPLE_AV_STREAM_CONTRL(), "AV_STREAM_CONTRL");
                break;
            }

			default:
				printf("\033[0;31mno order!\033[0;39m\n");
		}

		Usage();
	}

    /* de-init sys and vb */
	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");

	return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

