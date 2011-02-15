/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_codec.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/08/30
  Description   : sample_codec.c implement file
  History       :
  1.Date        : 2008/08/30
    Author      : x00100808
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

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vdec.h"
#include "mpi_venc.h"

#include "tw2815.h"
#include "adv7179.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/* fd for vi/vo config  (tw2815/adv7179)                                                       */
int fd2815a = -1;
int fd2815b = -1;
int fdadv7179 = -1;

static HI_U32 g_u32TvMode = VIDEO_ENCODING_MODE_PAL;
static HI_U32 g_u32ScreenWidth = 704;
static HI_U32 g_u32ScreenHeight = 576;

#define CHECK(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d !\n", name, __FUNCTION__, __LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

typedef enum hiVDEC_SIZE_E
{
    VDEC_SIZE_D1 = 0,
    VDEC_SIZE_CIF,
    VDEC_SIZE_BUTT
} VDEC_SIZE_E;

/* if you want to save stream, open this definition                         */
/* #define SAVE_STREAM_SWITCH */

static HI_BOOL g_bPreviewOnly = HI_FALSE;
static HI_BOOL g_bVencStopFlag = HI_FALSE;
static HI_BOOL g_bVdecStartFlag = HI_FALSE;

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

	if(fd2815a <0)
	{
		fd2815a = open("/dev/misc/tw2815adev",O_RDWR);
		if(fd2815a <0)
		{
			printf("can't open tw2815\n");
		}
	}

	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

    tmp = 0x88;
	if(0!=ioctl(fd2815a,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);
		return;
	}

	if(fd2815b <0)
	{
		fd2815b = open("/dev/misc/tw2815bdev",O_RDWR);
		if(fd2815b <0)
		{
			printf("can't open tw2815\n");
		}
	}

	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);
	}

    tmp = 0x88;
	if(0!=ioctl(fd2815b,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);
		return;
	}

	return;
}

/* set D/A decoder's work mode                                              */
static void do_Set7179(VIDEO_NORM_E stComposeMode)
{
	if(fdadv7179 <0)
	{
		fdadv7179 = open("/dev/misc/adv7179",O_RDWR);
		if(fdadv7179 <0)
		{
			printf("can't open 7179\n");
			return;
		}
	}

    if(stComposeMode==VIDEO_ENCODING_MODE_PAL)
    {
        if(0!=ioctl(fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_PAL))
    	{
    		printf("7179 errno %d\r\n",errno);
    	}
	}
	else if(stComposeMode==VIDEO_ENCODING_MODE_NTSC)
	{
        if(0!=ioctl(fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_NTSC))

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
    VI_CHN_ATTR_S ViChnAttr[ScreenCnt];

    memset(&ViAttr, 0, sizeof(VI_PUB_ATTR_S));
    memset(ViChnAttr, 0, ScreenCnt * sizeof(VI_CHN_ATTR_S));



    ViAttr.enInputMode = VI_MODE_BT656;
    ViAttr.enWorkMode = VI_WORK_MODE_2D1;

    switch (ScreenCnt)
    {
        case 1 :
        {
            ViChnAttr[0].bDownScale = HI_FALSE;
            ViChnAttr[0].bHighPri = 0;
            ViChnAttr[0].enCapSel = VI_CAPSEL_BOTH;
            ViChnAttr[0].enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            ViChnAttr[0].stCapRect.s32X = 0;
            ViChnAttr[0].stCapRect.s32Y = 0;
            ViChnAttr[0].stCapRect.u32Width = g_u32ScreenWidth;
            ViChnAttr[0].stCapRect.u32Height = g_u32ScreenHeight/2;
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
                ViChnAttr[i].stCapRect.u32Width = g_u32ScreenWidth;
                ViChnAttr[i].stCapRect.u32Height = g_u32ScreenHeight/2;
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
            return HI_FAILURE;
        }

        if (HI_SUCCESS != HI_MPI_VI_Enable(i))
        {
            return HI_FAILURE;
        }

        for(j = 0; j < 2; j++)
        {
            if (HI_SUCCESS != HI_MPI_VI_SetChnAttr(i,j,&ViChnAttr[i*2+j]))
            {
                return HI_FAILURE;
            }

            if (HI_SUCCESS != HI_MPI_VI_EnableChn(i,j))
            {
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

        VoChnAttr[i].stRect.s32X = ((g_u32ScreenWidth + offset)*(i%div))/div ;
        VoChnAttr[i].stRect.u32Width = (g_u32ScreenWidth + offset)/div;

        VoChnAttr[i].stRect.s32Y = (g_u32ScreenHeight*(i/div))/div ;
        VoChnAttr[i].stRect.u32Height = g_u32ScreenHeight/div;

        CHECK(HI_MPI_VO_SetChnAttr(i, &VoChnAttr[i]),"HI_MPI_VO_SetChnAttr");
    }

    return HI_SUCCESS;
}

HI_S32 SampleInitVoMScreen(HI_U32 ScreenCnt)
{
    HI_U32 i;
    VO_PUB_ATTR_S  VoPubAttr;

    VoPubAttr.stTvConfig.stComposeMode = g_u32TvMode;
    VoPubAttr.u32BgColor = 0xff;

    if (HI_SUCCESS != HI_MPI_VO_SetPubAttr(&VoPubAttr))
    {
        return HI_FAILURE;
    }

    if (HI_SUCCESS != SampleSetVochnMscreen(ScreenCnt))
    {
        return HI_FAILURE;
    }

    if (HI_SUCCESS != HI_MPI_VO_Enable())
    {
        return HI_FAILURE;
    }

    for (i = 0; i < ScreenCnt; i++)
    {
        CHECK(HI_MPI_VO_EnableChn(i), "HI_MPI_VO_EnableChn");
    }

    return HI_SUCCESS;
}

HI_S32 SampleViBindVo(HI_U32 ScreenCnt)
{
    HI_U32 i;

    if (1 == ScreenCnt)
    {
        printf("We have no vi2vo D1 preview!\n");
        return HI_SUCCESS;
    }

    for (i = 0; i < ScreenCnt - 1; i++)
    {
        CHECK(HI_MPI_VI_BindOutput(i/2, i%2, i), "HI_MPI_VI_BindOutput");
    }

    return HI_SUCCESS;
}

HI_S32 SampleViUnBindVo(HI_U32 ScreenCnt)
{
    HI_U32 i;

    if (1 == ScreenCnt)
    {
        printf("We have no vi2vo D1 preview!\n");
        return HI_SUCCESS;
    }

    for (i = 0; i < ScreenCnt - 1; i++)
    {
        CHECK(HI_MPI_VI_UnBindOutput(i/2, i%2, i), "HI_MPI_VI_UnBindOutput");
    }

    return HI_SUCCESS;
}

HI_S32 SampleDisableVo(HI_U32 ScreenCnt)
{
    HI_U32 i;

    for (i = 0; i < ScreenCnt; i++)
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
 Description     : The following function is used to encode.

*****************************************************************************/
HI_S32 SampleSaveH264Stream(FILE* fpH264File, VENC_STREAM_S *pstStream)
{
    HI_S32 i;

    for (i=0; i< pstStream->u32PackCount; i++)
    {
        fwrite(pstStream->pstPack[i].pu8Addr[0],
                pstStream->pstPack[i].u32Len[0], 1, fpH264File);

        fflush(fpH264File);

        if (pstStream->pstPack[i].u32Len[1] > 0)
        {

            fwrite(pstStream->pstPack[i].pu8Addr[1],
                    pstStream->pstPack[i].u32Len[1], 1, fpH264File);

            fflush(fpH264File);
        }
    }
    return HI_SUCCESS;
}

static HI_S32 SampleEnableEncodeH264(VENC_GRP VeGroup, VENC_CHN VeChn,
                        VI_DEV ViDev, VI_CHN ViChn, VENC_CHN_ATTR_S *pstAttr)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 SampleDisableEncodeH264(VENC_GRP VeGroup, VENC_CHN VeChn)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VENC_StopRecvPic(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyChn(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

static HI_S32 SendStreamToVdec(HI_S32 s32VencChn, VENC_STREAM_S *pstStream)
{
	HI_S32 VdChn = s32VencChn;
	VDEC_STREAM_S stStream;
	HI_S32 s32ret;
	int i;
	HI_U8 *pu8PackAddr = NULL;
	HI_S32 s32PackLen;

	if (HI_FALSE == g_bVdecStartFlag)
	{
	    printf("vdec doesn't start!\n");
		return HI_FAILURE;
	}

    /* Currently, we only support PT_H264  */
	if(1)
	{
    	for (i = 0; i< pstStream->u32PackCount; i ++)
    	{
    		if (0 == pstStream->pstPack[i].u32Len[1])
    		{
    			s32PackLen = pstStream->pstPack[i].u32Len[0];
    			pu8PackAddr = pstStream->pstPack[i].pu8Addr[0];
    		}
    		else
    		{
    			s32PackLen = pstStream->pstPack[i].u32Len[0] + pstStream->pstPack[i].u32Len[1];
    			pu8PackAddr = (HI_U8*)malloc(s32PackLen);
    			if (!pu8PackAddr)
    			{
    				return HI_FAILURE;
    			}
    			memcpy(pu8PackAddr, pstStream->pstPack[i].pu8Addr[0], pstStream->pstPack[i].u32Len[0]);
    			memcpy(pu8PackAddr+pstStream->pstPack[i].u32Len[0], pstStream->pstPack[i].pu8Addr[1],
    				pstStream->pstPack[i].u32Len[1]);
    		}

    		stStream.pu8Addr = pu8PackAddr;
    		stStream.u32Len = s32PackLen;
    		stStream.u64PTS = pstStream->pstPack[i].u64PTS;
    		s32ret = HI_MPI_VDEC_SendStream(VdChn, &stStream, HI_IO_BLOCK);
    		if (s32ret)
    		{
    			printf("send stream to vdec chn %d fail,err:%x\n", VdChn, s32ret);
    		}

    		if (0 != pstStream->pstPack[i].u32Len[1])
    		{
    			free(pu8PackAddr);
    			pu8PackAddr = NULL;
    		}
    	}
	}
    else
    {
        printf("Unsupport encode class!\n");
        return HI_FAILURE;
    }

	return HI_SUCCESS;
}

static HI_VOID* SampleGetH264StreamAndSend(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	VENC_CHN VeChn;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	FILE *pFile  = NULL;

    struct timeval TimeoutVal;

	VeChn = (HI_S32)p;
	pFile = fopen("stream.h264","wb");

	if(pFile == NULL)
	{
		HI_ASSERT(0);
		return NULL;
	}

	s32VencFd = HI_MPI_VENC_GetFd(VeChn);

    while (HI_TRUE != g_bVencStopFlag)
	{
		FD_ZERO(&read_fds);
		FD_SET(s32VencFd,&read_fds);

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);

		if (s32Ret < 0)
		{
			printf("select err\n");
			return NULL;
		}
		else if (0 == s32Ret)
		{
			printf("time out\n");
			return NULL;
		}
		else
		{
			if (FD_ISSET(s32VencFd, &read_fds))
			{
				s32Ret = HI_MPI_VENC_Query(VeChn, &stStat);

				if (s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_VENC_Query:0x%x err\n",s32Ret);
					fflush(stdout);
					return NULL;
				}

				stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);

				if (NULL == stStream.pstPack)
				{
					printf("malloc memory err!\n");
					return NULL;
				}

				stStream.u32PackCount = stStat.u32CurPacks;

				s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_IO_NOBLOCK);
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

                /* if you need to save current stream, open this switch.    */
            #ifdef SAVE_STREAM_SWITCH
				SampleSaveH264Stream(pFile, &stStream);
    		#endif

                /* send stream to VDEC while encoding...                    */
                if(HI_SUCCESS != SendStreamToVdec(VeChn, &stStream))
                {
                    printf("SendStreamToVdec failed!\n");
                    return NULL;
                }

				s32Ret = HI_MPI_VENC_ReleaseStream(VeChn,&stStream);
				if (s32Ret)
				{
					printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				free(stStream.pstPack);
				stStream.pstPack = NULL;
			}
		}
	}

	fclose(pFile);
	return NULL;
}


static HI_S32 SampleEncodeH264(VI_DEV ViDev, VI_CHN ViChn, VDEC_SIZE_E vdecSize)
{
    VENC_GRP VeGrpChn = 0;
    VENC_CHN VeChn = 0;
    VENC_CHN_ATTR_S stAttr;
    VENC_ATTR_H264_S stH264Attr;

    pthread_t VencH264Pid;

    switch (vdecSize)
    {
        case VDEC_SIZE_D1 :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth;
            stH264Attr.u32PicHeight = g_u32ScreenHeight;
            stH264Attr.bVIField = HI_TRUE;
            stH264Attr.u32Bitrate = 1024;
            stH264Attr.u32Gop = 100;

            break;
        }
        case VDEC_SIZE_CIF :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth/2;
            stH264Attr.u32PicHeight = g_u32ScreenHeight/2;
            stH264Attr.bVIField = HI_FALSE;
            stH264Attr.u32Bitrate = 512;
            stH264Attr.u32Gop = 100;

            break;
        }
        default:
        {
            printf("Unsupport decode method!\n");
            return HI_FAILURE;
        }
    }

    if (VIDEO_ENCODING_MODE_NTSC == g_u32TvMode)
    {
        stH264Attr.u32ViFramerate = 30;
        stH264Attr.u32TargetFramerate = 30;
    }
    else
    {
        stH264Attr.u32ViFramerate = 25;
        stH264Attr.u32TargetFramerate = 25;
    }

    stH264Attr.bMainStream = HI_TRUE;
    stH264Attr.bByFrame = HI_TRUE;
    stH264Attr.bCBR = HI_TRUE;
    stH264Attr.bField = HI_FALSE;
    stH264Attr.u32BufSize = stH264Attr.u32PicWidth * stH264Attr.u32PicHeight * 2;
    stH264Attr.u32MaxDelay = 100;
    stH264Attr.u32PicLevel = 0;

    memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
    stAttr.enType = PT_H264;
    stAttr.pValue = (HI_VOID *)&stH264Attr;

    if(HI_SUCCESS != SampleEnableEncodeH264(VeGrpChn, VeChn, ViDev, ViChn, &stAttr))
    {
        printf(" SampleEnableEncode err\n");
        return HI_FAILURE;
    }

    if(HI_SUCCESS != pthread_create(&VencH264Pid, NULL, SampleGetH264StreamAndSend, (HI_VOID *)VeChn))
    {
        printf("create SampleGetH264Stream thread failed!\n");
        SampleDisableEncodeH264(VeGrpChn, VeChn);
        return HI_FAILURE;
    }

    while (getchar() != 'q')
    {
        puts("i am encoding...  (q for exit!)");
    }

    g_bVencStopFlag = HI_TRUE;

	pthread_join(VencH264Pid,NULL);

	CHECK(SampleDisableEncodeH264(VeGrpChn, VeChn), "SampleDisableEncodeH264");

    return HI_SUCCESS;
}


/*****************************************************************************
 Prototype       :
 Description     : The following function is used to decode.

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

    g_bVdecStartFlag = HI_FALSE;

    return ;
}

static HI_S32 SampleStartVdecChn(HI_S32 s32VoChn, VDEC_SIZE_E vdecSize)
{
    HI_S32 s32ret;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_ATTR_H264_S stH264Attr;

	stH264Attr.u32Priority = 0;
	stH264Attr.enMode = H264D_MODE_STREAM;
	stH264Attr.u32RefFrameNum = 2;

    switch (vdecSize)
    {
        case VDEC_SIZE_D1 :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth;
            stH264Attr.u32PicHeight = g_u32ScreenHeight;
            break;
        }
        case VDEC_SIZE_CIF :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth/2;
            stH264Attr.u32PicHeight = g_u32ScreenHeight/2;
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

    s32ret = HI_MPI_VDEC_CreateChn(0, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VDEC_StartRecvStream(0);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32ret);
        return s32ret;
    }

	if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(0, s32VoChn))
	{
        printf("bind vdec to vo failed\n");
		SampleDestroyVdecChn(0);
		return HI_FAILURE;
	}

    g_bVdecStartFlag = HI_TRUE;


    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :  CODEC_1D1_H264
                    VI2VO_3CIF_CODEC_1CIF_H264
                    VI2VO_8CIF_CODEC_1CIF_H264

 Description     : The following three functions are used
                    to implement different function in main branch.

*****************************************************************************/

HI_S32 CODEC_1D1_H264(HI_VOID)
{
    SampleInitViMScreen(1);
    SampleInitVoMScreen(1);

    if (HI_TRUE == g_bPreviewOnly)
    {
        CHECK(HI_MPI_VI_BindOutput(0, 0, 0),"HI_MPI_VI_BindOutput");
        while (getchar() != 'q')
        {
        }
        CHECK(HI_MPI_VI_UnBindOutput(0, 0, 0),"HI_MPI_VI_UnBindOutput");
    }
    else
    {
        g_bVencStopFlag = HI_FALSE;

        /* we use VOCHN 0 as decode destination                                 */
        CHECK(SampleStartVdecChn(0, VDEC_SIZE_D1),"SampleStartVdecChn D1 failed!");

        /* we use VIDEV 0, VICHN 0 as encode source                              */
        CHECK(SampleEncodeH264(0, 0, VDEC_SIZE_D1),"SampleEncodeH264 D1 failed!");

        SampleDestroyVdecChn(0);
    }

    CHECK(SampleDisableVi(1),"SampleDisableVi(1)");
    CHECK(SampleDisableVo(1),"SampleDisableVo(1)");

    return HI_SUCCESS;
}

HI_S32 VI2VO_3CIF_CODEC_1CIF_H264(HI_VOID)
{
    SampleInitViMScreen(4);
    SampleInitVoMScreen(4);
    SampleViBindVo(4);

    if (HI_TRUE == g_bPreviewOnly)
    {
        while (getchar() != 'q')
        {
        }
    }
    else
    {
        g_bVencStopFlag = HI_FALSE;

        /* we use VOCHN 3 as decode destination                                 */
        CHECK(SampleStartVdecChn(3, VDEC_SIZE_CIF),"SampleStartVdecChn CIF failed!");

        /* we use VIDEV 1, VICHN 1 as encode source                              */
        CHECK(SampleEncodeH264(1, 1, VDEC_SIZE_CIF),"SampleEncodeH264 CIF failed!");

        SampleDestroyVdecChn(0);
    }

    CHECK(SampleViUnBindVo(4),"SampleViUnBindVo(4)");
    CHECK(SampleDisableVi(4),"SampleDisableVi(4)");
    CHECK(SampleDisableVo(4),"SampleDisableVo(4)");

    return HI_SUCCESS;
}

HI_S32 VI2VO_8CIF_CODEC_1CIF_H264(HI_VOID)
{
    SampleInitViMScreen(9);
    SampleInitVoMScreen(9);
    SampleViBindVo(9);

    if (HI_TRUE == g_bPreviewOnly)
    {
        while (getchar() != 'q')
        {
        }
    }
    else
    {
        g_bVencStopFlag = HI_FALSE;

        /* we use VOCHN 8 as decode destination                                 */
        CHECK(SampleStartVdecChn(8, VDEC_SIZE_CIF),"SampleStartVdecChn CIF failed!");

       /* we use VIDEV 0, VICHN 0 as encode source                              */
        CHECK(SampleEncodeH264(0, 0, VDEC_SIZE_CIF),"SampleEncodeH264 CIF failed!");

        SampleDestroyVdecChn(0);
    }

    CHECK(SampleViUnBindVo(9),"SampleViUnBindVo(9)");
    CHECK(SampleDisableVi(9),"SampleDisableVi(9)");
    CHECK(SampleDisableVo(9),"SampleDisableVo(9)");

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
    printf("1  --> CODEC_1D1_H264\n");
    printf("2  --> VI2VO_3CIF_CODEC_1CIF_H264\n");
    printf("3  --> VI2VO_8CIF_CODEC_1CIF_H264\n");
    printf("q  --> quit\n");
    printf("\033[0;32mplease enter order:\033[0;39m\n");
    printf("\033[0;36m(if you're codecing, type 'q' first!):\033[0;39m\n");
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
            g_u32TvMode = VIDEO_ENCODING_MODE_NTSC;
            g_u32ScreenHeight = 480;
        }
        else if (!strcmp(argv[1],"vi2vo"))
        {
            /* you can open vi2vo preview only                              */
            g_bPreviewOnly = HI_TRUE;
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
    if (VIDEO_ENCODING_MODE_NTSC == g_u32TvMode)
    {
        do_Set7179(VIDEO_ENCODING_MODE_NTSC);
    }
    else
    {
        do_Set7179(VIDEO_ENCODING_MODE_PAL);;
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
			    printf("\033[0;33mwhat you see is codecing!\033[0;39m\n");
				CHECK(CODEC_1D1_H264(),"CODEC_1D1_H264");
				break;
			}

			case '2':
			{
    			printf("\033[0;33mthe last window is codecing!\033[0;39m\n");
				CHECK(VI2VO_3CIF_CODEC_1CIF_H264(),"VI2VO_3CIF_CODEC_1CIF_H264");
				break;
			}

			case '3':
			{
    			printf("\033[0;33mthe last window is codecing!\033[0;39m\n");
				CHECK(VI2VO_8CIF_CODEC_1CIF_H264(),"VI2VO_8CIF_CODEC_1CIF_H264");
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

