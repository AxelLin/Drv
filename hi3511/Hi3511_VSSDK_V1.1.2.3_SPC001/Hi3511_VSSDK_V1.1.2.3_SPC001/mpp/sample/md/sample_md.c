/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_md.c
  Version       : Initial Draft
  Author        : l64467
  Created       : 2008/6/27
  Last Modified :
  Description   : this file demo that get MD date and print it to screen
  Function List :
              main
              SampleDisableMd
              SampleGetMdData
              SamplePrintfResult
              SAMPLE_Md_GetData
  History       :
  1.Date        : 2008/6/27
    Author      : l64467
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

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_md.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "tw2815.h"
#include "mpi_venc.h"
#include "mpi_md.h"


#define VIDEVID 0
#define VICHNID 0
#define VOCHNID 0
#define VENCCHNID 0
#define TW2815_A_FILE	"/dev/misc/tw2815adev"
#define TW2815_B_FILE	"/dev/misc/tw2815bdev"

HI_S32 g_s32Fd2815a = -1;
HI_S32 g_s32Fd2815b = -1;

HI_BOOL g_bIsQuit = HI_FALSE;

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

HI_S32 SampleSysInit(HI_VOID)
{
	HI_S32 s32Ret;
	MPP_SYS_CONF_S stSysConf = {0};
	VB_CONF_S stVbConf ={0};
    
    HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
	
	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 20;	
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 40;	

	s32Ret = HI_MPI_VB_SetConf(&stVbConf);
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

	stSysConf.u32AlignWidth = 64;

	s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
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
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
}

/*enable vi,vo and bind them*/
HI_S32 SampleEnableViVo1D1(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;
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
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
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
	
	if (HI_SUCCESS != HI_MPI_VI_SetPubAttr(ViDev, &ViAttr))
	{
		printf("set VI attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VI_SetChnAttr(ViDev, ViChn, &ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}	

	if (HI_SUCCESS != HI_MPI_VI_Enable(ViDev))
	{
		printf("set VI  enable failed !\n");
		return HI_FAILURE;		
	}
	
	if (HI_SUCCESS != HI_MPI_VI_EnableChn(ViDev, ViChn))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;	
	}
	
 	if (HI_SUCCESS != HI_MPI_VO_SetPubAttr(&VoAttr))
	{
		printf("set VO attribute failed !\n");
		return HI_FAILURE;		
	}
	
	if (HI_SUCCESS != HI_MPI_VO_SetChnAttr(VoChn, &VoChnAttr))
	{
		printf("set VO Chn attribute failed !\n");
		return HI_FAILURE;		
	}

	if (HI_SUCCESS != HI_MPI_VO_Enable())
	{
		printf("set VO  enable failed !\n");
		return HI_FAILURE;	
	}
	
	if (HI_SUCCESS != HI_MPI_VO_EnableChn(VoChn))
	{
		printf("set VO Chn enable failed !\n");
		return HI_FAILURE;		
	}

    if (HI_SUCCESS != HI_MPI_VI_BindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_BindOutput failed !\n");
		return HI_FAILURE;		
	}
   
	return HI_SUCCESS;
}


HI_S32 SampleDisableViVo1D1(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	if(HI_SUCCESS != HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VI_DisableChn(ViDev, ViChn))
	{
		printf("HI_MPI_VI_DisableChn failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VI_Disable(ViDev))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VO_DisableChn(VoChn))
	{
		printf("HI_MPI_VO_DisableChn failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VO_Disable())
	{
		printf("HI_MPI_VO_Disable failed !\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
	
}


HI_S32 SampleEnableVenc1Cif(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_GRP VeGroup = VENCCHNID;
	VENC_CHN VeChn = VENCCHNID;
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
	stH264Attr.u32Gop = 100;
	stH264Attr.u32MaxDelay = 100;
	stH264Attr.u32PicLevel = 0;

	stAttr.enType = PT_H264;
	stAttr.pValue = (HI_VOID *)&stH264Attr;
	
	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_BindInput(VeGroup, VIDEVID, VICHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_CreateChn(VeChn, &stAttr, HI_NULL);
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


HI_VOID* SampleGetH264Stream(HI_VOID *p)
{
	HI_S32 i;
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	VENC_CHN VeChn;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	FILE *pFile  = NULL;
	struct timeval TimeoutVal; 
	
	pFile = fopen("stream.h264","wb");
	
	if(pFile == NULL)
	{
		HI_ASSERT(0);
		return NULL;
	}

	VeChn = (HI_S32)p;
	
	s32VencFd = HI_MPI_VENC_GetFd(VeChn);

	/*get 1000 frame*/
	do{
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
				
				s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_TRUE);
				
				if (HI_SUCCESS != s32Ret) 
				{
					printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
					fflush(stdout);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				/*storage the stream to file*/
				for (i=0; i< stStream.u32PackCount; i++)
				{
					fwrite(stStream.pstPack[i].pu8Addr[0], 
							stStream.pstPack[i].u32Len[0], 1, pFile);
					
					fflush(pFile);
	
					if (stStream.pstPack[i].u32Len[1] > 0)
					{
		
						fwrite(stStream.pstPack[i].pu8Addr[1], 
								stStream.pstPack[i].u32Len[1], 1, pFile);
						
						fflush(pFile);
					}	
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
	
	}while (HI_FALSE == g_bIsQuit);

	fclose(pFile);
	return HI_SUCCESS;
}



HI_VOID SampleDisableVenc1D1(HI_VOID)
{		
	HI_MPI_VENC_StopRecvPic(VENCCHNID);
	HI_MPI_VENC_UnRegisterChn(VENCCHNID);
	HI_MPI_VENC_DestroyChn(VENCCHNID);
	HI_MPI_VENC_DestroyGroup(VENCCHNID);
}


HI_S32 SampleEnableMd(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = VENCCHNID;
    MD_CHN_ATTR_S stMdAttr;
    MD_REF_ATTR_S  stRefAttr;

	/*set MD attribute*/
    stMdAttr.stMBMode.bMBSADMode =HI_TRUE;
    stMdAttr.stMBMode.bMBMVMode = HI_FALSE;
	stMdAttr.stMBMode.bMBPelNumMode = HI_FALSE;
	stMdAttr.stMBMode.bMBALARMMode = HI_FALSE;
	stMdAttr.u16MBALSADTh = 1000;
	stMdAttr.u8MBPelALTh = 20;
	stMdAttr.u8MBPerALNumTh = 20;
    stMdAttr.enSADBits = MD_SAD_8BIT;
    stMdAttr.stDlight.bEnable = HI_FALSE;
    stMdAttr.u32MDInternal = 0;
    stMdAttr.u32MDBufNum = 16;

	/*set MD frame*/
    stRefAttr.enRefFrameMode = MD_REF_AUTO;
	stRefAttr.enRefFrameStat = MD_REF_DYNAMIC;
    
    s32Ret =  HI_MPI_MD_SetChnAttr(VeChn, &stMdAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_SetChnAttr Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }
	
    s32Ret = HI_MPI_MD_SetRefFrame(VeChn, &stRefAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_SetRefFrame Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_MD_EnableChn(VeChn);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_EnableChn Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

	return HI_SUCCESS;
}



HI_S32 SampleDisableMd(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = VENCCHNID;
	
	s32Ret = HI_MPI_MD_DisableChn(VeChn);
	
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_MD_DisableChn Err 0x%x\n",s32Ret);
        return HI_FAILURE;
    } 

	return HI_SUCCESS;
}



HI_VOID SamplePrintfResult(const MD_DATA_S *pstMdData, FILE *pfd)
{
	/*get MD SAD data*/
	if(pstMdData->stMBMode.bMBSADMode)
	{
		HI_S32 i,j;
		HI_U16* pTmp = NULL;
		
		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U16 *)((HI_U32)pstMdData->stMBSAD.pu32Addr + 
										i*pstMdData->stMBSAD.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}

	/*get MD MV data*/
	if(pstMdData->stMBMode.bMBMVMode)
	{
		HI_S32 i,j;
		HI_U16* pTmp = NULL;
		
		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U16 *)((HI_U32)pstMdData->stMBMV.pu32Addr + 
										i*pstMdData->stMBMV.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}

	/*get MD MB alarm data*/
	if(pstMdData->stMBMode.bMBALARMMode)
	{
		HI_S32 i,j,k;
		HI_U8* pTmp = NULL;

		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U8 *)((HI_U32)pstMdData->stMBAlarm.pu32Addr + 
										i*pstMdData->stMBAlarm.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				k = j%8;

				if(j != 0 && k==0)
				{
					pTmp++;
				}
				
				fprintf(pfd, "%2d",((*pTmp)>>k)&0x1);		
			}
			
			fprintf(pfd, "\n");
		}
	}


	/*get MD MB alarm pels number data*/
	if(pstMdData->stMBMode.bMBPelNumMode)
	{
		HI_S32 i,j;
		HI_U8* pTmp = NULL;

		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U8 *)((HI_U32)pstMdData->stMBPelAlarmNum.pu32Addr + 
								(i*pstMdData->stMBPelAlarmNum.u32Stride));
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}
	
}



HI_VOID *SampleGetMdData(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32MdFd;
	MD_DATA_S stMdData;
	VENC_CHN VeChn = VENCCHNID;
	fd_set read_fds;
	struct timeval TimeoutVal; 
    FILE *pfd;

    pfd = fopen("md_result.data", "wb");
    if (!pfd)
    {
        return NULL;
    }

	s32MdFd = HI_MPI_MD_GetFd(VeChn);

	/*get 0xfff MD data*/
	do{
		FD_ZERO(&read_fds);
		FD_SET(s32MdFd,&read_fds);

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select(s32MdFd+1, &read_fds, NULL, NULL, &TimeoutVal);

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
			memset(&stMdData, 0, sizeof(MD_DATA_S));
			
			if (FD_ISSET(s32MdFd, &read_fds))
			{
				s32Ret = HI_MPI_MD_GetData(VeChn, &stMdData, HI_IO_NOBLOCK);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_MD_GetData err 0x%x\n",s32Ret);
					return NULL;
				}
			}
	
			SamplePrintfResult(&stMdData, pfd);

			s32Ret = HI_MPI_MD_ReleaseData(VeChn, &stMdData);
			if(s32Ret != HI_SUCCESS)
			{
		    	printf("Md Release Data Err 0x%x\n",s32Ret);
				return NULL;
			}

		}
	}while (HI_FALSE == g_bIsQuit);

    if (pfd)
    {
        fclose(pfd);
    }
	return NULL;
}



/* one chn h264 venc (cif), get SAD ,output them to terminal*/
HI_S32 SAMPLE_Md_GetData(HI_VOID)
{
    pthread_t MdPid;
	pthread_t VencPid;

	/*system init*/
	if(HI_SUCCESS != SampleSysInit())
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

	/*enable vi vo*/
	if(HI_SUCCESS != SampleEnableViVo1D1())
	{
		Sample2815Close();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	/*enable 1 cif coding*/
	if(HI_SUCCESS != SampleEnableVenc1Cif())
	{
		Sample2815Close();
		SampleDisableViVo1D1();
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}


	/*enable MD*/
	if(HI_SUCCESS != SampleEnableMd())
	{
		Sample2815Close();
		SampleDisableViVo1D1();
		SampleDisableVenc1D1();
		SampleSysExit();
	    return HI_FAILURE;
	}

	g_bIsQuit = HI_FALSE;

	/*enable get H264 stream thread*/
	pthread_create(&VencPid, 0, SampleGetH264Stream, (HI_VOID *)VENCCHNID);

	/*enable get MD data thread*/
    pthread_create(&MdPid, 0, SampleGetMdData, NULL);
    
    printf("start get md data , input twice Enter to stop sample ... ... \n");
	getchar();
	getchar();

	g_bIsQuit = HI_TRUE;
	sleep(1);

	SampleDisableMd();
	Sample2815Close();
	SampleDisableViVo1D1();
	SampleDisableVenc1D1();
	SampleSysExit();
	
	return HI_SUCCESS;
}

#define SAMPLE_MD_HELP(void)\
{\
    printf("usage : %s 1 \n", argv[0]);\
    printf("1:  SAMPLE_Md_GetData\n");\
}

HI_S32 main(int argc, char *argv[])
{    
    int index;

    if (2 != argc)
    {
        SAMPLE_MD_HELP();
        return HI_FAILURE;
    }
    
    index = atoi(argv[1]);
    
    if (1 != index)
    {
        SAMPLE_MD_HELP();
        return HI_FAILURE;
    }
    
    if (1 == index)
    {
        SAMPLE_Md_GetData();
    }

    return HI_SUCCESS;
}




