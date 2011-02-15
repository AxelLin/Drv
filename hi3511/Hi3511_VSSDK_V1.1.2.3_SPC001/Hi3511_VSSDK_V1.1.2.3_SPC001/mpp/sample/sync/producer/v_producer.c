/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_venc.c
  Version       : Initial Draft
  Author        : l64467
  Created       : 2008/6/27
  Last Modified :
  Description   : this file demo four venc sample
  Function List :
              main
              SampleCreateSnapChn
              SampleDestroySnapChn
              SampleDisable1D1H264
              SampleDisable1D1Jpeg1CifH264
              SampleDisable1D1Mjpeg1CifH264
              SampleDisable4HD1H264
              SampleEnable1D1H264
              SampleEnable1D1Jpeg1CifH264
              SampleEnable1D1Mjpeg1CifH264
              SampleEnable4HD1H264
              SampleGet1D1Mjpeg1CifH264Stream
              SampleGet4HD1Stream
              SampleGetH264Stream
              SampleSaveH264Stream
              SampleSaveJpegStream
              SampleSaveSnapPic
              SampleStartSnap
              SAMPLE_1CifH264_1D1Jpeg
              SAMPLE_1CifH264_1D1Mjpeg
              SAMPLE_1D1H264_InsertUserData
              SAMPLE_1D1H264_RequestIDR
              SAMPLE_4HD1H264
  History       :
  1.Date        : 2008/6/27
    Author      : l64467
    Modification: Created file

******************************************************************************/

/*----------------------------------------------*
 * external variables                           *
 *----------------------------------------------*/

/*----------------------------------------------*
 * external routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * internal routine prototypes                  *
 *----------------------------------------------*/

/*----------------------------------------------*
 * project-wide global variables                *
 *----------------------------------------------*/

/*----------------------------------------------*
 * module-wide global variables                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * constants                                    *
 *----------------------------------------------*/

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

/*----------------------------------------------*
 * routines' implementations                    *
 *----------------------------------------------*/


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

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpp.h"
#include "hi_comm_venc.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "tw2815.h"
#include "mpi_vpp.h"
#include "mpi_venc.h"


#define VIDEVID 0
#define VICHNID 0
#define VOCHNID 0
#define VENCCHNID 0
#define SNAPCHN 1
#define TW2815_A_FILE	"/dev/misc/tw2815adev"
#define TW2815_B_FILE	"/dev/misc/tw2815bdev"

const HI_U8 g_SOI[2] = {0xFF, 0xD8};
const HI_U8 g_EOI[2] = {0xFF, 0xD9};

HI_S32 g_s32Fd2815a = -1;
HI_S32 g_s32Fd2815b = -1;

#ifndef FRAME_WITH_LAST_PTS
#define FRAME_WITH_LAST_PTS
#endif

typedef struct hiH264_FILE_HEADER_S
{
	HI_U64 u64PTS;
	HI_BOOL bIFrame;
	HI_U32 u32Len;

    #ifdef FRAME_WITH_LAST_PTS
    HI_U32 u32LastLen;
    #endif

}H264_FRAME_HEADER_S;


static HI_BOOL bVencStopFlag = HI_FALSE;

HI_S32 Sample2815Open(HI_VOID)
{
	g_s32Fd2815a = open(TW2815_A_FILE, O_RDWR);

	if (g_s32Fd2815a < 0)
	{
		printf("can't open tw2815a\n");
		return HI_FAILURE;
	}

	g_s32Fd2815b = open(TW2815_B_FILE, O_RDWR);

	if (g_s32Fd2815b < 0)
	{
		printf("can't open tw2815b\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_VOID Sample2815Close(HI_VOID)
{
	if (g_s32Fd2815a > 0)
	{
		close(g_s32Fd2815a);
	}

	if (g_s32Fd2815b > 0)
	{
		close(g_s32Fd2815b);
	}
}

HI_S32 Sample2815Set2d1(HI_VOID)
{
    HI_S32 i;
	HI_S32 s32Fd2815[] = {g_s32Fd2815a, g_s32Fd2815b};
    tw2815_set_2d1 tw2815set2d1;

    for (i=0; i<2; i++)
    {
        tw2815set2d1.ch1 = 0;
        tw2815set2d1.ch2 = 2;

        if(HI_SUCCESS != ioctl(s32Fd2815[i], TW2815_SET_2_D1, &tw2815set2d1))
        {
			printf("SET 2815 (0,2)errno %d\r\n",errno);
            return HI_FAILURE;
        }

        tw2815set2d1.ch1 = 1;
        tw2815set2d1.ch2 = 3;

        if(HI_SUCCESS != ioctl(s32Fd2815[i], TW2815_SET_2_D1, &tw2815set2d1))
        {
			printf("SET 2815 (1,3)errno %d\r\n",errno);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 SampleSysInit(MPP_SYS_CONF_S * pstSysConf,VB_CONF_S* pstVbConf)
{
	HI_S32 s32Ret;

    HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	s32Ret = HI_MPI_VB_SetConf(pstVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VB_SetConf failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VB_Init();
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VB_Init failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_SYS_SetConf(pstSysConf);
	if (HI_SUCCESS != s32Ret)
	{
		HI_MPI_VB_Exit();
		printf("conf : system config failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_SYS_Init();
	if(HI_SUCCESS != s32Ret)
	{
		HI_MPI_VB_Exit();
		printf("HI_MPI_SYS_Init err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

    return HI_SUCCESS;
}


HI_VOID SampleSysExit(HI_VOID)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_SYS_Exit();
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_SYS_Exit 0x%x!\n",s32Ret);
	}

	s32Ret = HI_MPI_VB_Exit();
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VB_Exit 0x%x!\n",s32Ret);
	}
}

HI_S32 SampleEnableVi(VI_DEV ViDev,VI_CHN ViChn,
						VI_PUB_ATTR_S *pViPubAttr,VI_CHN_ATTR_S *pViChnAttr)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VI_SetPubAttr(ViDev, pViPubAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VI attribute failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VI_SetChnAttr(ViDev, ViChn, pViChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VI Chn attribute failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VI_Enable(ViDev);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VI  enable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VI_EnableChn(ViDev, ViChn);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VI Chn enable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleEnableVo(VO_CHN VoChn, VO_PUB_ATTR_S *pVoPubAttr,
											VO_CHN_ATTR_S *pVoChnAttr)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VO_SetPubAttr(pVoPubAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO attribute failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_SetChnAttr(VoChn, pVoChnAttr);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO Chn attribute failed !\n");
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_Enable();
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO  enable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_EnableChn(VoChn);
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO Chn enable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisableVi(VI_DEV ViDev,VI_CHN ViChn)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VI_DisableChn(ViDev, ViChn);
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VI_DisableChn failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VI_Disable(ViDev);
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VI_UnBindOutput failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisableVo(VO_CHN VoChn)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VO_DisableChn(VoChn);
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VO_DisableChn failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VO_Disable();
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VO_Disable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}



HI_S32 SampleEnableEncode(VENC_GRP VeGroup, VENC_CHN VeChn,
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



HI_S32 SampleDisableEncode(VENC_GRP VeGroup, VENC_CHN VeChn)
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

    if (VeChn == 1)
    {
    	s32Ret = HI_MPI_VENC_DestroyGroup(0);
    	if (s32Ret != HI_SUCCESS)
    	{
    		printf("HI_MPI_VENC_DestroyGroup err 0x%x\n",s32Ret);
    		return HI_FAILURE;
    	}
    }
	return HI_SUCCESS;
}



HI_S32 SampleEnableViVo1D1(VI_DEV ViDev,VI_CHN ViChn,VO_CHN VoChn)
{
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;
	VO_PUB_ATTR_S VoAttr;
	VO_CHN_ATTR_S VoChnAttr;

	memset(&ViAttr, 0, sizeof(VI_PUB_ATTR_S));
	memset(&ViChnAttr, 0, sizeof(VI_CHN_ATTR_S));

	memset(&VoAttr, 0, sizeof(VO_PUB_ATTR_S));
	memset(&VoChnAttr, 0, sizeof(VO_CHN_ATTR_S));

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;
	ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 720;
	ViChnAttr.stCapRect.s32X = 0;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_FALSE;
	ViChnAttr.bChromaResample = HI_FALSE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = 10;
	VoChnAttr.bZoomEnable = HI_TRUE;
	VoChnAttr.u32Priority = 1;
	VoChnAttr.stRect.s32X = 0;
	VoChnAttr.stRect.s32Y = 0;
	VoChnAttr.stRect.u32Height = 576;
	VoChnAttr.stRect.u32Width = 720;

	if (HI_SUCCESS != SampleEnableVi(ViDev, ViChn, &ViAttr, &ViChnAttr))
	{
		return HI_FAILURE;
	}

	if (HI_SUCCESS != SampleEnableVo(VoChn, &VoAttr, &VoChnAttr))
	{
		return HI_FAILURE;
	}

    if (HI_SUCCESS != HI_MPI_VI_BindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_BindOutput failed !\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisableViVo1D1(VI_DEV ViDev,VI_CHN ViChn,VO_CHN VoChn)
{
	#if 0
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;
	#endif

	if(HI_SUCCESS != HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleDisableVi(ViDev, ViChn))
	{
		printf("SampleDisableVi faild!\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleDisableVo(VoChn))
	{
		printf("SampleDisableVo faild!\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}




HI_S32 SampleEnableVi4HD1(HI_VOID)
{
	HI_U32 i,j;

	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;

    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;

	ViChnAttr.bDownScale = HI_FALSE;
	ViChnAttr.bHighPri = 0;
	ViChnAttr.enCapSel = VI_CAPSEL_BOTTOM;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	ViChnAttr.stCapRect.s32X = 0;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.u32Height = 288;

	for(i=0;i<2;i++)
	{
		if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(i,&ViAttr))
		{
			printf("HI_MPI_VI_SetPubAttr faild!\n");
			return HI_FAILURE;
		}

		if (HI_SUCCESS!=HI_MPI_VI_Enable(i))
		{
			printf("HI_MPI_VI_Enable faild!\n");
			return HI_FAILURE;
		}

		for(j=0;j<2;j++)
		{
			if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(i,j,&ViChnAttr))
			{
				printf("HI_MPI_VI_SetChnAttr faild!\n");
				return HI_FAILURE;
			}

			if (HI_SUCCESS!=HI_MPI_VI_EnableChn(i,j))
			{
				printf("HI_MPI_VI_EnableChn faild!\n");
				return HI_FAILURE;
			}
		}
	}

    return HI_SUCCESS;
}


HI_S32 SampleSetVochnMscreen(HI_U32 u32Cnt)
{
	HI_U32 i,div;
	VO_CHN_ATTR_S VoChnAttr[9];

	div = sqrt(u32Cnt);

	for (i=0; i<u32Cnt; i++)
	{
		VoChnAttr[i].bZoomEnable = HI_TRUE;
		VoChnAttr[i].u32Priority = 1;
		if(u32Cnt!=9)
		{
		    VoChnAttr[i].stRect.s32X = (704*(i%div))/div ;
    		VoChnAttr[i].stRect.u32Width = 704/div;
		}
		else
		{
		    VoChnAttr[i].stRect.s32X = (720*(i%div))/div ;
    		VoChnAttr[i].stRect.u32Width = 720/div;
		}
		VoChnAttr[i].stRect.s32Y = (576*(i/div))/div ;
		VoChnAttr[i].stRect.u32Height = 576/div;
		VoChnAttr[i].u32Priority = 1;
		if(HI_SUCCESS!=HI_MPI_VO_SetChnAttr(i, &VoChnAttr[i]))
		{
			printf("HI_MPI_VO_SetChnAttr faild!\n");
            return HI_FAILURE;
        }
	}

	return HI_SUCCESS;
}


HI_S32 SampleEnableVo4Cif(HI_VOID)
{
	HI_U32 i;

	VO_PUB_ATTR_S  VoPubAttr;

	VoPubAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoPubAttr.u32BgColor = 0xff;

	if (0!=HI_MPI_VO_SetPubAttr(&VoPubAttr))
	{
		printf("HI_MPI_VO_SetPubAttr faild!\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS!=SampleSetVochnMscreen(4))
	{
		printf("SampleSetVochnMscreen faild!\n");
		return HI_FAILURE;
    }

	if(HI_SUCCESS!=HI_MPI_VO_Enable())
	{
		printf("HI_MPI_VO_Enable faild!\n");
		return HI_FAILURE;
    }

	for(i=0;i<4;i++)
	{
		if(HI_SUCCESS!=HI_MPI_VO_EnableChn(i))
		{
			printf("HI_MPI_VO_EnableChn faild!\n");
	        return HI_FAILURE;
        }
	}

	return HI_SUCCESS;
}


HI_S32 SamplePreview4Screen(HI_VOID)
{
	HI_U32 i;

	if(HI_SUCCESS!=SampleEnableVi4HD1())
	{
		return HI_FAILURE;
	}

	if(HI_SUCCESS!=SampleEnableVo4Cif())
	{
	    return HI_FAILURE;
	}

	for(i=0;i<4;i++)
	{
		HI_MPI_VI_BindOutput(i/2, i%2, i);
	}

	return HI_SUCCESS;
}



/*存储JPEG码流*/
HI_S32 SampleSaveJpegStream(FILE* fpJpegFile, VENC_STREAM_S *pstStream)
{
    VENC_PACK_S*  pstData;
    HI_U32  i;

    fwrite(g_SOI, 1, sizeof(g_SOI), fpJpegFile);

    for(i = 0; i < pstStream->u32PackCount; i++)
    {
        pstData= &pstStream->pstPack[i];
        fwrite(pstData->pu8Addr[0], pstData->u32Len[0], 1, fpJpegFile);
		fwrite(pstData->pu8Addr[1], pstData->u32Len[1], 1, fpJpegFile);
    }

    fwrite(g_EOI, 1, sizeof(g_EOI), fpJpegFile);

    return HI_SUCCESS;
}

/*存储H264码流*/
HI_S32 SampleSaveH264Stream(FILE* fpH264File, VENC_STREAM_S *pstStream)
{
	HI_S32 i;
	H264_FRAME_HEADER_S header;

#ifdef FRAME_WITH_LAST_PTS
    static HI_U32 lastLen = 0;
#endif

	if(pstStream->u32PackCount == 0)
	{
		return HI_FAILURE;
	}
	memset(&header,0,sizeof(H264_FRAME_HEADER_S));
	header.u64PTS = pstStream->pstPack[0].u64PTS;

	for (i=0; i< pstStream->u32PackCount; i++)
	{
		header.u32Len += pstStream->pstPack[i].u32Len[0];
		header.u32Len += pstStream->pstPack[i].u32Len[1];
		if(pstStream->pstPack[i].DataType.enH264EType == H264E_NALU_ISLICE)
		{
			header.bIFrame = HI_TRUE;
		}
	}

#ifdef FRAME_WITH_LAST_PTS
    header.u32LastLen = lastLen;
    lastLen = header.u32Len;
#endif


	/*write file header*/
	fwrite(&header,sizeof(H264_FRAME_HEADER_S),1,fpH264File);

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


HI_S32 SampleEnable1D1Mjpeg1CifH264(HI_VOID)
{
	HI_S32 i;

	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VENC_CHN_ATTR_S stAttr[2];
	VENC_ATTR_H264_S stH264Attr;
	VENC_ATTR_MJPEG_S stMjpegAttr;

	stH264Attr.u32PicWidth = 352;
	stH264Attr.u32PicHeight = 288;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.bCBR = HI_TRUE;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_FALSE;
	stH264Attr.u32Bitrate = 512;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 352*288*2;
	stH264Attr.u32Gop = 100;
	stH264Attr.u32MaxDelay = 100;
	stH264Attr.u32PicLevel = 0;

	stMjpegAttr.u32PicWidth = 704;
	stMjpegAttr.u32PicHeight = 576;
	stMjpegAttr.bMainStream = HI_TRUE;
	stMjpegAttr.bByFrame = HI_TRUE;
	stMjpegAttr.bVIField= HI_TRUE;
	stMjpegAttr.u32ViFramerate = 25;
	stMjpegAttr.u32TargetFramerate= 25;
	stMjpegAttr.u32BufSize = 704*576*2;
	stMjpegAttr.u32MCUPerECS = 0;
	stMjpegAttr.u32TargetBitrate = 8192;

	memset(&stAttr[0], 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr[0].enType = PT_H264;
	stAttr[0].pValue = (HI_VOID *)&stH264Attr;

	memset(&stAttr[1], 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr[1].enType = PT_MJPEG;
	stAttr[1].pValue = (HI_VOID *)&stMjpegAttr;

	for(i=0; i<2; i++)
	{
		if(HI_SUCCESS != SampleEnableEncode(i, i, ViDev, ViChn, &stAttr[i]))
		{
			printf("err %d\n",i);
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}

HI_S32 SampleEnable1D1H2641CifH264(HI_VOID)
{
    HI_S32 s32Ret;
    VENC_GRP VeGroup = 0;
    VENC_CHN VeChn = 0, VeChn2 = 1;

	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VENC_CHN_ATTR_S stAttr[2];
	VENC_ATTR_H264_S stH264Attr;
	VENC_ATTR_H264_S stH264Attr2;

	stH264Attr.u32PicWidth = 704;
	stH264Attr.u32PicHeight = 576;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.bCBR = HI_TRUE;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_FALSE;
	stH264Attr.u32Bitrate = 1024;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 704*576*2;
	stH264Attr.u32Gop = 100;
	stH264Attr.u32MaxDelay = 100;
	stH264Attr.u32PicLevel = 0;

	stH264Attr2.u32PicWidth = 352;
	stH264Attr2.u32PicHeight = 288;
	stH264Attr2.bMainStream = HI_FALSE;
	stH264Attr2.bByFrame = HI_TRUE;
	stH264Attr2.bCBR = HI_TRUE;
	stH264Attr2.bField = HI_FALSE;
	stH264Attr2.bVIField = HI_FALSE;
	stH264Attr2.u32Bitrate = 512;
	stH264Attr2.u32ViFramerate = 25;
	stH264Attr2.u32TargetFramerate = 25;
	stH264Attr2.u32BufSize = 352*288*2;
	stH264Attr2.u32Gop = 100;
	stH264Attr2.u32MaxDelay = 100;
	stH264Attr2.u32PicLevel = 0;


	memset(&stAttr[0], 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr[0].enType = PT_H264;
	stAttr[0].pValue = (HI_VOID *)&stH264Attr;

	memset(&stAttr[1], 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr[1].enType = PT_H264;
	stAttr[1].pValue = (HI_VOID *)&stH264Attr2;

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

	s32Ret = HI_MPI_VENC_CreateChn(VeChn, &stAttr[0], HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_CreateChn(VeChn2, &stAttr[1], HI_NULL);
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

	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn2);
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

	s32Ret = HI_MPI_VENC_StartRecvPic(VeChn2);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_VOID * SampleGet1D1Mjpeg1CifH264Stream(HI_VOID *p)
{
	HI_S32 i;
	HI_S32  maxfd = 0;
	HI_S32 s32ret;
	HI_S32 VencFd[2] = {0};
	HI_U32 u32FrameIdx = 0;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	HI_CHAR *pszStreamFile[] = {"CIFH264stream.h264","D1MJPEGstream.mjp"};

	struct timeval TimeoutVal;

	fd_set read_fds;
	FILE *pFd[2] = {0};

	for (i=0;i<2;i++)
	{
		VencFd[i] = HI_MPI_VENC_GetFd(i);
		if (VencFd[i] <= 0)
		{
			printf("HI_MPI_VENC_GetFd err 0x%x\n",s32ret);
			return NULL;
		}

		/*open file to save stream */
		pFd[i] = fopen(pszStreamFile[i], "wb");
		if (!pFd[i])
		{
			printf("open file err!\n");
			return NULL;
		}

		if(maxfd <= VencFd[i])
		{
			maxfd = VencFd[i];
		}
	}

	do{
		FD_ZERO(&read_fds);

		for (i=0; i<2;i++)
		{
			FD_SET(VencFd[i],&read_fds);
		}

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32ret = select(maxfd + 1,&read_fds,NULL,NULL,&TimeoutVal);
		if(s32ret < 0)
		{
			printf("select err\n");
			return NULL;
		}
		else if(s32ret == 0)
		{
			printf("time out\n");
			return NULL;
		}
		else
		{
			for (i=0; i<2;i++)
			{
				if(FD_ISSET(VencFd[i],&read_fds))
				{
					memset(&stStream,0,sizeof(stStream));
					s32ret = HI_MPI_VENC_Query(i, &stStat);
					if (s32ret != HI_SUCCESS)
					{
						printf("HI_MPI_VENC_Query:0x%x\n",s32ret);
						fflush(stdout);
						return NULL;
					}

					stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
					if (NULL == stStream.pstPack)
					{
						printf("malloc stream pack faild!\n");
						return NULL;
					}

					stStream.u32PackCount = stStat.u32CurPacks;

					s32ret = HI_MPI_VENC_GetStream(i, &stStream, HI_IO_NOBLOCK);
					if (s32ret != HI_SUCCESS)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_GetStream err 0x%x\n",s32ret);
						return NULL;
					}

					if(0 == i)
					{
						SampleSaveH264Stream(pFd[i], &stStream);
					}
					else if(1 == i)
					{
						SampleSaveJpegStream(pFd[i],&stStream);
					}

					s32ret = HI_MPI_VENC_ReleaseStream(i,&stStream);
					if (s32ret)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_ReleaseStream err 0x%x\n",s32ret);
						return NULL;
					}

					free(stStream.pstPack);
					stStream.pstPack = NULL;
				}
			}
		}

		u32FrameIdx ++;

	}while(u32FrameIdx <= 1000);

	for (i=0;i<2;i++)
	{
		fclose(pFd[i]);
	}

	return HI_SUCCESS;
}

HI_VOID* SampleGet1D1H2641CifH264Stream(HI_VOID *p)
{
	HI_S32 i;
	HI_S32  maxfd = 0;
	HI_S32 s32ret;
	HI_S32 VencFd[2] = {0};
	HI_U32 u32FrameIdx = 0;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	HI_CHAR *pszStreamFile[] = {"D1H264stream.h264","stream0.h264"};

	struct timeval TimeoutVal;

	fd_set read_fds;
	FILE *pFd[2] = {0};

	for (i=0;i<2;i++)
	{
		VencFd[i] = HI_MPI_VENC_GetFd(i);
		if (VencFd[i] <= 0)
		{
			printf("HI_MPI_VENC_GetFd err 0x%x\n",s32ret);
			return NULL;
		}

        if (i == 1)
        {
    		/*open file to save stream */
    		pFd[i] = fopen(pszStreamFile[i], "wb");
    		if (!pFd[i])
    		{
    			printf("open file err!\n");
    			return NULL;
    		}
        }

		if(maxfd <= VencFd[i])
		{
			maxfd = VencFd[i];
		}
	}

	do{
		FD_ZERO(&read_fds);

		for (i=0; i<2;i++)
		{
			FD_SET(VencFd[i],&read_fds);
		}

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32ret = select(maxfd + 1,&read_fds,NULL,NULL,&TimeoutVal);
		if(s32ret < 0)
		{
			printf("select err\n");
			return NULL;
		}
		else if(s32ret == 0)
		{
			printf("time out\n");
			return NULL;
		}
		else
		{
			for (i=0; i<2;i++)
			{
				if(FD_ISSET(VencFd[i],&read_fds))
				{
					memset(&stStream,0,sizeof(stStream));
					s32ret = HI_MPI_VENC_Query(i, &stStat);
					if (s32ret != HI_SUCCESS)
					{
						printf("HI_MPI_VENC_Query:0x%x\n",s32ret);
						fflush(stdout);
						return NULL;
					}

					stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
					if (NULL == stStream.pstPack)
					{
						printf("malloc stream pack faild!\n");
						return NULL;
					}

					stStream.u32PackCount = stStat.u32CurPacks;

					s32ret = HI_MPI_VENC_GetStream(i, &stStream, HI_IO_NOBLOCK);
					if (s32ret != HI_SUCCESS)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_GetStream err 0x%x\n",s32ret);
						return NULL;
					}

                    if(1 == i)
					{
						SampleSaveH264Stream(pFd[i],&stStream);
					}

					s32ret = HI_MPI_VENC_ReleaseStream(i,&stStream);
					if (s32ret)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_ReleaseStream err 0x%x\n",s32ret);
						return NULL;
					}

					free(stStream.pstPack);
					stStream.pstPack = NULL;
				}
			}
		}

		u32FrameIdx ++;

	}while(bVencStopFlag != HI_TRUE);

	for (i=1;i<2;i++)
	{
		fclose(pFd[i]);
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisable1D1Mjpeg1CifH264(HI_VOID)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VENC_StopRecvPic(1);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_StopRecvPic(0);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(1);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(0);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyChn(1);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyChn(0);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyGroup(0);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 SampleCreateSnapChn(VENC_GRP VeGroup,VENC_CHN SnapChn)
{
	HI_S32 s32Ret;
	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_JPEG_S stJpegAttr;

	stJpegAttr.u32BufSize = 704*576*2;
	stJpegAttr.u32PicWidth = 704;
	stJpegAttr.u32PicHeight = 576;
	stJpegAttr.bVIField = HI_TRUE;
	stJpegAttr.bByFrame = HI_TRUE;
	stJpegAttr.u32MCUPerECS = 1620;
	stJpegAttr.u32ImageQuality = 0;

	memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr.enType = PT_JPEG;
	stAttr.pValue = (HI_VOID *)&stJpegAttr;

	/*创建抓拍通道*/
	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_CreateChn(SnapChn, &stAttr, HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleSaveSnapPic(VENC_CHN SnapChn,FILE *pFile)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;

	s32VencFd = HI_MPI_VENC_GetFd(SnapChn);
	if(s32VencFd < 0)
	{
		printf("HI_MPI_VENC_GetFd err \n");
		return HI_FAILURE;
	}

	FD_ZERO(&read_fds);
	FD_SET(s32VencFd,&read_fds);

	s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);

	if (s32Ret < 0)
	{
		printf("select err\n");
		return HI_FAILURE;
	}
	else if (0 == s32Ret)
	{
		printf("time out\n");
		return HI_FAILURE;
	}
	else
	{
		if (FD_ISSET(s32VencFd, &read_fds))
		{
			s32Ret = HI_MPI_VENC_Query(SnapChn, &stStat);

			if (s32Ret != HI_SUCCESS)
			{
				printf("HI_MPI_VENC_Query:0x%x\n",s32Ret);
				fflush(stdout);
				return HI_FAILURE;
			}

			stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);

			if (NULL == stStream.pstPack)
			{
				printf("malloc memory err!\n");
				return HI_FAILURE;
			}

			stStream.u32PackCount = stStat.u32CurPacks;

			s32Ret = HI_MPI_VENC_GetStream(SnapChn, &stStream, HI_TRUE);

			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
				fflush(stdout);
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return HI_FAILURE;
			}

			s32Ret = SampleSaveJpegStream(pFile, &stStream);
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
				fflush(stdout);
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return HI_FAILURE;
			}

			s32Ret = HI_MPI_VENC_ReleaseStream(SnapChn,&stStream);
			if (s32Ret)
			{
				printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
				free(stStream.pstPack);
				stStream.pstPack = NULL;
				return HI_FAILURE;
			}

			free(stStream.pstPack);
			stStream.pstPack = NULL;
		}

	}

	return HI_SUCCESS;
}

HI_S32 SampleStartSnap(VENC_GRP VeGroup,VENC_CHN SnapChn,
										VI_DEV ViDev,VI_CHN ViChn,FILE *pFile)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_StartRecvPic(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	/*save jpeg picture*/
	s32Ret = SampleSaveSnapPic(SnapChn,pFile);
	if(s32Ret != HI_SUCCESS)
	{
		printf("SampleSaveSnapPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_StopRecvPic(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StopRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnbindInput(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnbindInput err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleDestroySnapChn(VENC_GRP VeGroup,VENC_CHN SnapChn)
{
	HI_S32 s32Ret;
	s32Ret = HI_MPI_VENC_DestroyChn(SnapChn);
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


HI_S32 SampleEnable1D1Jpeg1CifH264(HI_VOID)
{
	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_H264_S stH264Attr;


	stH264Attr.u32PicWidth = 352;
	stH264Attr.u32PicHeight = 288;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.bCBR = HI_TRUE;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_FALSE;
	stH264Attr.u32Bitrate = 512;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 352*288*2;
	stH264Attr.u32Gop = 40;
	stH264Attr.u32MaxDelay = 10;
	stH264Attr.u32PicLevel = 3;

	memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr.enType = PT_H264;
	stAttr.pValue = (HI_VOID *)&stH264Attr;

	if(HI_SUCCESS != SampleEnableEncode(VENCCHNID, VENCCHNID,
												VIDEVID, VICHNID, &stAttr))
	{
		return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleCreateSnapChn(SNAPCHN,SNAPCHN))
	{
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisable1D1Jpeg1CifH264(HI_VOID)
{
	if(HI_SUCCESS != SampleDisableEncode(VENCCHNID, VENCCHNID))
	{
		return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleDestroySnapChn(SNAPCHN,SNAPCHN))
	{
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 SampleEnable4HD1H264(HI_VOID)
{
	HI_S32 i;

	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_H264_S stH264Attr;

	stH264Attr.u32PicWidth = 704;
	stH264Attr.u32PicHeight = 288;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.bCBR = HI_TRUE;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_FALSE;
	stH264Attr.u32Bitrate = 2048;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 704*288*2;
	stH264Attr.u32Gop = 20;
	stH264Attr.u32MaxDelay = 10;
	stH264Attr.u32PicLevel = 3;


	memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr.enType = PT_H264;
	stAttr.pValue = (HI_VOID *)&stH264Attr;


	for(i=0; i<4; i++)
	{
		if(HI_SUCCESS != SampleEnableEncode(i, i, i/2, i%2, &stAttr))
		{
			printf("err %d\n",i);
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}


HI_VOID * SampleGet4HD1Stream(HI_VOID *p)
{
	HI_S32 i;
	HI_S32  maxfd = 0;
	HI_S32 s32ret;
	HI_S32 VencFd[4] = {0};
	HI_U32 u32FrameIdx = 0;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	HI_CHAR *pszStreamFile[] = {"HD1stream0.h264","HD1stream1.h264",
										"HD1stream2.h264","HD1stream3.h264"};

	struct timeval TimeoutVal;

	fd_set read_fds;
	FILE *pFd[4] = {0};

	for (i=0;i<4;i++)
	{
		VencFd[i] = HI_MPI_VENC_GetFd(i);
		if (VencFd[i] <= 0)
		{
			printf("HI_MPI_VENC_GetFd err 0x%x\n",s32ret);
			return NULL;
		}

		/*open file to save stream */
		pFd[i] = fopen(pszStreamFile[i], "wb");
		if (!pFd[i])
		{
			printf("open file err!\n");
			return NULL;
		}

		if(maxfd <= VencFd[i]) maxfd = VencFd[i] ;
	}

	do{
		FD_ZERO(&read_fds);

		for (i=0; i<4;i++)
		{
			FD_SET(VencFd[i],&read_fds);
		}

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32ret = select(maxfd + 1,&read_fds,NULL,NULL,&TimeoutVal);
		if(s32ret < 0)
		{
			printf("select err\n");
			return NULL;
		}
		else if(s32ret == 0)
		{
			printf("time out\n");
			continue;
		}
		else
		{
			for (i=0; i<4;i++)
			{
				if(FD_ISSET(VencFd[i],&read_fds))
				{
					memset(&stStream,0,sizeof(stStream));

					s32ret = HI_MPI_VENC_Query(i, &stStat);
					if (s32ret != HI_SUCCESS)
					{
						printf("HI_MPI_VENC_Query:0x%x\n",s32ret);
						return NULL;
					}

					stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
					if (NULL == stStream.pstPack)
					{
						printf("malloc stream pack err!\n");
						return NULL;
					}

					stStream.u32PackCount = stStat.u32CurPacks;

					s32ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
					if (s32ret != HI_SUCCESS)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_GetStream err 0x%x\n",s32ret);
						return NULL;
					}

					SampleSaveH264Stream(pFd[i], &stStream);

					s32ret = HI_MPI_VENC_ReleaseStream(i,&stStream);
					if (s32ret != HI_SUCCESS)
					{
						free(stStream.pstPack);
						stStream.pstPack = NULL;
						printf("HI_MPI_VENC_ReleaseStream err 0x%x\n",s32ret);
						return NULL;
					}

					free(stStream.pstPack);
					stStream.pstPack = NULL;
				}
			}
		}

		u32FrameIdx ++;

	}while(u32FrameIdx <= 0xfff);

	for (i=0;i<4;i++)
	{
		fclose(pFd[i]);
	}

	return HI_SUCCESS;
}


HI_S32 SampleDisable4HD1H264(HI_VOID)
{
	HI_S32 i;

	for(i=0; i<4; i++)
	{
		if(HI_SUCCESS != SampleDisableEncode(i, i))
		{
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}


HI_S32 SampleEnable1D1H264(HI_VOID)
{
	VENC_GRP VeGrpChn = VENCCHNID;
	VENC_CHN VeChn = VENCCHNID;
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_H264_S stH264Attr;

	stH264Attr.u32PicWidth = 352;
	stH264Attr.u32PicHeight = 288;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.bCBR = HI_TRUE;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_TRUE;
	stH264Attr.u32Bitrate = 512;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 352*288*2;
	stH264Attr.u32Gop = 40;
	stH264Attr.u32MaxDelay = 10;
	stH264Attr.u32PicLevel = 3;

	memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
	stAttr.enType = PT_H264;
	stAttr.pValue = (HI_VOID *)&stH264Attr;

	if(HI_SUCCESS != SampleEnableEncode(VeGrpChn, VeChn, ViDev, ViChn, &stAttr))
	{
		printf(" SampleEnableEncode err\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 SampleDisable1D1H264(HI_VOID)
{
	VENC_GRP VeGroup = VENCCHNID;
	VENC_CHN VeChn = VENCCHNID;

	if(HI_SUCCESS != SampleDisableEncode(VeGroup, VeChn))
	{
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


/*获取单路H264码流*/
HI_VOID* SampleGetH264Stream(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	HI_U32 u32FrameIdx = 0;
	VENC_CHN VeChn;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	FILE *pFile  = NULL;

	pFile = fopen("stream.h264","wb");

	if(pFile == NULL)
	{
		HI_ASSERT(0);
		return NULL;
	}

	VeChn = (HI_S32)p;

	s32VencFd = HI_MPI_VENC_GetFd(VeChn);

	do{
		FD_ZERO(&read_fds);
		FD_SET(s32VencFd,&read_fds);

		s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);

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

				s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_TRUE);

				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				SampleSaveH264Stream(pFile, &stStream);

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

		u32FrameIdx ++;
	}while (bVencStopFlag != HI_TRUE);

	fclose(pFile);
	return HI_SUCCESS;
}





/*双码流业务（一路D1的MJPEG，一路CIF的H264）*/
HI_S32 SAMPLE_1CifH264_1D1Mjpeg(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	pthread_t VencGetStreamPid;

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 8;

	stVbConf.astCommPool[1].u32BlkSize = 352*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 8;

	stSysConf.u32AlignWidth = 64;

	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableViVo1D1(ViDev,ViChn,VoChn))
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnable1D1Mjpeg1CifH264())
	{
		Sample2815Close();
		SampleDisableViVo1D1(ViDev,ViChn,VoChn);
		SampleSysExit();
		printf("SampleEnable1D1Mjpeg1CifH264 faild\n");
	    return HI_FAILURE;
	}

    pthread_create(&VencGetStreamPid, 0, SampleGet1D1Mjpeg1CifH264Stream, NULL);

	pthread_join(VencGetStreamPid,0);


	Sample2815Close();
	SampleDisableViVo1D1(ViDev,ViChn,VoChn);
	SampleDisable1D1Mjpeg1CifH264();
	SampleSysExit();
	return HI_SUCCESS;
}

/*双码流业务（一路D1的H264，一路CIF的H264）*/
HI_S32 SAMPLE_1CIFH264_1CIFH264(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	pthread_t VencGetStreamPid;

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 8;

	stVbConf.astCommPool[1].u32BlkSize = 352*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 20;

	stSysConf.u32AlignWidth = 64;

	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableViVo1D1(ViDev,ViChn,VoChn))
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnable1D1H2641CifH264())
	{
		Sample2815Close();
		SampleDisableViVo1D1(ViDev,ViChn,VoChn);
		SampleSysExit();
		printf("SampleEnable1D1Mjpeg1CifH264 faild\n");
	    return HI_FAILURE;
	}

    bVencStopFlag = HI_FALSE;

    pthread_create(&VencGetStreamPid, 0, SampleGet1D1H2641CifH264Stream, NULL);

    printf("\033[0;32mdouble press 'Enter' for stop!\033[0;39m\n");

    getchar();
    getchar();

    bVencStopFlag = HI_TRUE;

    printf("\033[0;32mstream produce OK!\033[0;39m\n");


	pthread_join(VencGetStreamPid,0);


	Sample2815Close();
	SampleDisableViVo1D1(ViDev,ViChn,VoChn);
	SampleDisable1D1Mjpeg1CifH264();
	SampleSysExit();
	return HI_SUCCESS;
}


/*抓拍业务（一路CIF的H264编码，D1抓拍）*/
HI_S32 SAMPLE_1CifH264_1D1Jpeg(HI_VOID)
{
	VENC_CHN VeChn;
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	FILE *pFile  = NULL;
	char acFile[36];
	HI_S32 s32SnapNum = 0;

	pthread_t VencH264Pid;

	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	/*config system*/
	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 8;

	stVbConf.astCommPool[1].u32BlkSize = 352*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 8;

	stSysConf.u32AlignWidth = 64;

	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	/*config 2815*/
	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	/*config and enable VI,VO*/
	if(HI_SUCCESS != SampleEnableViVo1D1(ViDev,ViChn,VoChn))
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	/*config and enable h264 and jpeg*/
	if(HI_SUCCESS != SampleEnable1D1Jpeg1CifH264())
	{
		Sample2815Close();
		SampleDisableViVo1D1(ViDev,ViChn,VoChn);
		SampleSysExit();
		printf("SampleEnable1D1Mjpeg1CifH264 faild\n");
	    return HI_FAILURE;
	}

	/*get h264 stream*/
	VeChn = 0;
	pthread_create(&VencH264Pid, 0, SampleGetH264Stream, (HI_VOID *)VeChn);


	/*enable jpeg*/
	do
	{
		sleep(2);
		sprintf(acFile, "jpegstream%d.jpg", s32SnapNum);

		pFile = fopen(acFile,"wb");

		if(pFile == NULL)
		{
			continue;
		}

		if(HI_SUCCESS != SampleStartSnap(SNAPCHN, SNAPCHN,VIDEVID,VICHNID, pFile))
		{
			continue;
		}

		s32SnapNum++;

		fclose(pFile);

	}while(s32SnapNum < 0xf);


	pthread_join(VencH264Pid,0);
	Sample2815Close();
	SampleDisableViVo1D1(ViDev,ViChn,VoChn);
	SampleDisable1D1Jpeg1CifH264();
	SampleSysExit();
	return HI_SUCCESS;
}


/*四路HD1业务*/
HI_S32 SAMPLE_4HD1H264(HI_VOID)
{
	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	pthread_t VencGetStreamPid;

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*288*1.5;
	stVbConf.astCommPool[0].u32BlkCnt = 32;

	stSysConf.u32AlignWidth = 64;

	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SamplePreview4Screen())
	{
		Sample2815Close();
		SampleSysExit();
		printf("SamplePreview4Screen faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnable4HD1H264())
	{
		Sample2815Close();
		SampleSysExit();
		printf("SampleEnable4HD1H264 faild\n");
	    return HI_FAILURE;
	}

    pthread_create(&VencGetStreamPid, 0, SampleGet4HD1Stream, NULL);

	pthread_join(VencGetStreamPid,0);


	Sample2815Close();
	SampleDisable4HD1H264();
	SampleSysExit();
	return HI_SUCCESS;
}

/*I帧请求业务（一路H264编码）*/
HI_S32 SAMPLE_1D1H264_RequestIDR(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = VENCCHNID;
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	HI_U32 u32IDR = 0;
	pthread_t VencH264Pid;

	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 8;

	stSysConf.u32AlignWidth = 64;

	/*system*/
	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableViVo1D1(ViDev,ViChn,VoChn))
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnable1D1H264())
	{
		Sample2815Close();
		SampleDisableViVo1D1(ViDev,ViChn,VoChn);
		SampleSysExit();
		printf("SampleEnable1D1Mjpeg1CifH264 faild\n");
	    return HI_FAILURE;
	}

	pthread_create(&VencH264Pid, 0, SampleGetH264Stream, (HI_VOID *)VeChn);

	while(u32IDR < 0xf)
	{
		sleep(1);
		u32IDR++;
		s32Ret = HI_MPI_VENC_RequestIDR(VeChn);
		if(HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_VENC_RequestIDR err 0x%xn",s32Ret);
			continue;
		}

		printf("HI_MPI_VENC_RequestIDR success %d\n",u32IDR);
	}

	pthread_join(VencH264Pid,0);

	Sample2815Close();
	SampleDisableViVo1D1(ViDev,ViChn,VoChn);
	SampleDisable1D1H264();
	SampleSysExit();
	return HI_SUCCESS;
}


/*用户数据业务（一路H264的编码，加用户数据）*/
HI_S32 SAMPLE_1D1H264_InsertUserData(HI_VOID)
{
	VENC_CHN VeChn = VENCCHNID;
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};

	pthread_t VencH264Pid;

    bVencStopFlag = HI_FALSE;

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 10;

	stVbConf.astCommPool[1].u32BlkSize = 352*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 40;

	stSysConf.u32AlignWidth = 64;

	if(HI_SUCCESS != SampleSysInit(&stSysConf,&stVbConf))
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Open())
	{
		SampleSysExit();
		printf("open 2815 file faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != Sample2815Set2d1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("set 2815 2D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableViVo1D1(ViDev,ViChn,VoChn))
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnable1D1H264())
	{
		Sample2815Close();
		SampleDisableViVo1D1(ViDev,ViChn,VoChn);
		SampleSysExit();
		printf("SampleEnable1D1Mjpeg1CifH264 faild\n");
	    return HI_FAILURE;
	}

	pthread_create(&VencH264Pid, 0, SampleGetH264Stream, (HI_VOID *)VeChn);


    while (getchar() != 'q')
    {
        puts("do you want to end this stream, q for terminating ?");
    }

    bVencStopFlag = HI_TRUE;

	pthread_join(VencH264Pid,0);

	Sample2815Close();
	SampleDisableViVo1D1(ViDev,ViChn,VoChn);
	SampleDisable1D1H264();
	SampleSysExit();
	return HI_SUCCESS;
}

HI_S32 main(int argc, char *argv[])
{
    printf("\033[0;32mPress 'Enter' to start Recode!\033[0;39m\n");

    getchar();

    printf("start recording ...\n");

    SAMPLE_1CIFH264_1CIFH264();

	return HI_SUCCESS;
}


