#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>
#include "hi_comm_video.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "pciv_test_comm.h"
#include "pci_trans.h"
#include "pciv_msg.h"

#include "hi_comm_vo.h"
#include "mpi_vo.h"
#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "tw2815.h"
#include "pciv_stream.h"

#define isxdigit(c)	(('0' <= (c) && (c) <= '9') \
			 || ('a' <= (c) && (c) <= 'f') \
			 || ('A' <= (c) && (c) <= 'F'))

#define isdigit(c)	('0' <= (c) && (c) <= '9')
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define toupper(c)	(islower(c) ? ((c) - 'a' + 'A') : (c))

#define TW2815_A_FILE	"/dev/misc/tw2815adev"
#define TW2815_B_FILE	"/dev/misc/tw2815bdev"

int g_fd2815a = -1;
int g_fd2815b = -1;

static VI_CHN_ATTR_S g_viChnAttr[VIU_MAX_DEV_NUM][VIU_MAX_CHN_NUM_PER_DEV];
static VO_CHN_ATTR_S g_voChnAttr[VO_MAX_CHN_NUM];

HI_S32 SAMPLE_2815_open()
{
	g_fd2815a = open(TW2815_A_FILE,O_RDWR);
	if (g_fd2815a < 0)
	{
		printf("can't open tw2815a\n");
		return HI_FAILURE;
	}
	g_fd2815b = open(TW2815_B_FILE,O_RDWR);
	if (g_fd2815b < 0)
	{
		printf("can't open tw2815b\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

void SAMPLE_2815_close()
{
	if (g_fd2815a > 0)
	{
		close(g_fd2815a);
	}
	if (g_fd2815b > 0)
	{
		close(g_fd2815b);
	}
}

HI_S32 SAMPLE_2815_set_2d1()
{
    int i;
    tw2815_set_2d1 tw2815set2d1;
    int fd2815[] = {g_fd2815a, g_fd2815b};

    for (i=0; i<2; i++)
    {
        tw2815set2d1.ch1 = 0;
        tw2815set2d1.ch2 = 2;

        if(0!=ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1))
        {
            return HI_FAILURE;
        }

        tw2815set2d1.ch1 = 1;
        tw2815set2d1.ch2 = 3;

        if(0!=ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1))
        {
            return HI_FAILURE;
        }
    }
    return HI_SUCCESS;
}

HI_S32 PCIV_VoConfig1Chn()
{
    VO_CHN_ATTR_S *pAttr = &g_voChnAttr[0];
    
    pAttr->bZoomEnable = HI_TRUE;
    pAttr->u32Priority = 1;
    pAttr->stRect.s32X = 0;
    pAttr->stRect.s32Y = 0;
    pAttr->stRect.u32Width = 704;
    pAttr->stRect.u32Height = 576;

    return HI_SUCCESS;
}

HI_S32 PCIV_VoConfig4Chn()
{
    HI_S32 i, j;
    VO_CHN_ATTR_S *pAttr = &g_voChnAttr[0];

    for(i=0; i<2; i++)
    {
        for(j=0; j<2; j++)
        {
            pAttr = &(g_voChnAttr[i*2+j]);
            pAttr->bZoomEnable = HI_TRUE;
            pAttr->u32Priority = 1;
            pAttr->stRect.s32X = 8 + i*352;
            pAttr->stRect.s32Y = j*288;
            pAttr->stRect.u32Width  = 352;
            pAttr->stRect.u32Height = 288;
        }
    }
    return HI_SUCCESS;
}

HI_S32 PCIV_VoConfig6Chn()
{
    HI_S32 i, j, k;
    VO_CHN_ATTR_S *pAttr = &g_voChnAttr[0];

    pAttr->bZoomEnable = HI_TRUE;
    pAttr->u32Priority = 1;
    pAttr->stRect.s32X = 0;
    pAttr->stRect.s32Y = 0;
    pAttr->stRect.u32Width  = 480;
    pAttr->stRect.u32Height = 384;

    k=1;    
    for(i=0; i<3; i++)
    {
        for(j=0; j<3; j++)
        {
            if( i<2 && j<2 )
            {
                continue;
            }
            pAttr = &(g_voChnAttr[k++]);
            pAttr->bZoomEnable = HI_TRUE;
            pAttr->u32Priority = 1;
            pAttr->stRect.s32X = i*240;
            pAttr->stRect.s32Y = j*192;
            pAttr->stRect.u32Width  = 240;
            pAttr->stRect.u32Height = 192;
        }
    }
}

HI_S32 PCIV_VoConfig16Chn()
{
    HI_S32 i, j, k;
    VO_CHN_ATTR_S *pAttr = &g_voChnAttr[0];

    k=0;    
    for(i=0; i<4; i++)
    {
        for(j=0; j<4; j++)
        {
            pAttr = &(g_voChnAttr[k++]);
            pAttr->bZoomEnable = HI_TRUE;
            pAttr->u32Priority = 1;
            pAttr->stRect.s32X = 8 + i*176;
            pAttr->stRect.s32Y = j*144;
            pAttr->stRect.u32Width  = 176;
            pAttr->stRect.u32Height = 144;
        }
    }
    return HI_SUCCESS;
}

HI_S32 PCIV_VoConfigChn(HI_S32 s32ChnNum)
{
    /* Get the config of VO channel */
    if(s32ChnNum <= 1)
    {
        PCIV_VoConfig1Chn();
    }
    else if(s32ChnNum <= 4)
    {
        PCIV_VoConfig4Chn();
    }
    else if(s32ChnNum <= 6)
    {
        PCIV_VoConfig6Chn();
    }
    else
    {
        PCIV_VoConfig16Chn();
    }

    return HI_SUCCESS;
}


HI_S32 PCIV_ViConfigD1(VI_DEV viDev, VI_CHN viChn)
{
    VI_CHN_ATTR_S *pAttr = &g_viChnAttr[viDev][viChn];
    
    memset(pAttr,0,sizeof(VI_CHN_ATTR_S));

	pAttr->stCapRect.s32X      = 8;
	pAttr->stCapRect.s32Y      = 0;
    pAttr->stCapRect.u32Width  = 704;
	pAttr->stCapRect.u32Height = 288;	

    pAttr->enCapSel        = VI_CAPSEL_BOTH;
	pAttr->bDownScale      = HI_FALSE;
	pAttr->bChromaResample = HI_FALSE;
	pAttr->enViPixFormat   = PCIV_TEST_VI_PIXEL_FORMAT;
	return HI_SUCCESS;
}

HI_S32 PCIV_ViConfigCif(VI_DEV viDev, VI_CHN viChn)
{
    VI_CHN_ATTR_S *pAttr = &g_viChnAttr[viDev][viChn];
    
    memset(pAttr,0,sizeof(VI_CHN_ATTR_S));

	pAttr->stCapRect.s32X      = 8;
	pAttr->stCapRect.s32Y      = 0;
    pAttr->stCapRect.u32Width  = 704;
	pAttr->stCapRect.u32Height = 288;	

    pAttr->enCapSel        = VI_CAPSEL_BOTTOM;
	pAttr->bDownScale      = HI_TRUE;
	pAttr->bChromaResample = HI_FALSE;
	pAttr->enViPixFormat   = PCIV_TEST_VI_PIXEL_FORMAT;
	return HI_SUCCESS;
}

HI_S32 PCIV_ViConfigDefault(HI_VOID)
{
    HI_S32 i, j;
    
    /* viDev 0 configed as D1 */
    for(j=0; j<2; j++)
    {
        PCIV_ViConfigD1(0, j);
    }

    /* Other device is configed as CIF */
    for(i=1; i<VIU_MAX_DEV_NUM; i++)
    {
        for(j=0; j<2; j++)
        {
            PCIV_ViConfigCif(i, j);
        }
    }
 
    return HI_SUCCESS;
}

HI_S32 PCIV_VencConfigChn(HI_BOOL bMain, PAYLOAD_TYPE_E enPtType, 
			   HI_U32 u32PicWidth,HI_U32  u32PicHeight, 
			   VENC_CHN_ATTR_S *pAttr, HI_U32 *pu32AttrLen)
{
	HI_S32 s32Ret;
	VENC_ATTR_MJPEG_S stMjpegAttr;
	VENC_ATTR_H264_S  stH264Attr;
	VENC_ATTR_JPEG_S  stJpegAttr;
	VENC_ATTR_MPEG4_S stMpeg4Attr;

    /* If not main stream then downscale the size */
    if( !bMain)
    {
        u32PicWidth  = EDGE_ALIGN(u32PicWidth/2, 16);
        u32PicHeight = EDGE_ALIGN(u32PicHeight/2, 16);
    }
    
	switch(enPtType)
	{
		case PT_H264:
		{
            stH264Attr.bCBR       = HI_TRUE;
            stH264Attr.bField     = HI_FALSE;
			stH264Attr.bByFrame   = HI_TRUE;
            stH264Attr.bVIField   = HI_TRUE;
            if(u32PicWidth >= 704)
            {
                stH264Attr.u32Bitrate = 1024*2;
            }
            else if(u32PicWidth >= 352)
            {
                stH264Attr.u32Bitrate = 512;
            }
            else
            {
                stH264Attr.u32Bitrate = 192;
            }
            stH264Attr.u32ViFramerate     = 25;
            stH264Attr.u32TargetFramerate = 25;
            stH264Attr.u32BufSize   = u32PicWidth*u32PicHeight*2;
            stH264Attr.u32Gop       = 50;
            stH264Attr.u32MaxDelay  = 10;
            stH264Attr.u32PicLevel  = 0;
			stH264Attr.bMainStream  = bMain;
			stH264Attr.u32PicWidth  = u32PicWidth;
			stH264Attr.u32PicHeight = u32PicHeight;
			memcpy(pAttr->pValue, &stH264Attr, sizeof(stH264Attr));
			*pu32AttrLen = sizeof(stH264Attr);
		}
		break;
		case PT_MJPEG:
		{
			stMjpegAttr.bMainStream = bMain;
			stMjpegAttr.u32PicWidth =  u32PicWidth;
			stMjpegAttr.u32PicHeight = u32PicHeight;
            stMjpegAttr.u32MCUPerECS = (u32PicWidth*u32PicHeight)>>8;
            stMjpegAttr.u32BufSize = 8*(u32PicWidth*u32PicHeight);
			memcpy(pAttr->pValue, &stMjpegAttr, sizeof(stMjpegAttr));
			*pu32AttrLen = sizeof(stMjpegAttr);
		}
		break;
		case PT_JPEG:
		{
			stJpegAttr.u32PicWidth  = u32PicWidth;
			stJpegAttr.u32PicHeight = u32PicHeight;
			stJpegAttr.u32MCUPerECS = (u32PicWidth*u32PicHeight)>>8;
			stJpegAttr.u32BufSize   = 8*(u32PicWidth*u32PicHeight);
			memcpy(pAttr->pValue, &stJpegAttr, sizeof(stJpegAttr));
			*pu32AttrLen = sizeof(stJpegAttr);
		}
		break;
		case PT_MP4VIDEO:
		{
            stMpeg4Attr.u32PicWidth  = u32PicWidth;
            stMpeg4Attr.u32PicHeight = u32PicHeight;
            stMpeg4Attr.bByFrame     = HI_TRUE;
            stMpeg4Attr.bVIField     = HI_TRUE;
            stMpeg4Attr.u32Gop       = 23;
            stMpeg4Attr.u32MaxDelay  = 5;
            stMpeg4Attr.u32ViFramerate     = 25;
            stMpeg4Attr.u32TargetFramerate = 25;
            stMpeg4Attr.u32TargetBitrate   = 40000;
            stMpeg4Attr.u32BufSize         = u32PicWidth*u32PicHeight;
            stMpeg4Attr.enQuantType        = MPEG4E_QUANT_H263;
			memcpy(pAttr->pValue, &stMpeg4Attr, sizeof(stMpeg4Attr));            
			*pu32AttrLen = sizeof(stMpeg4Attr);
		}
		break;
		default:
			return HI_FAILURE;
	}

    pAttr->enType = enPtType;
	return HI_SUCCESS;
}



HI_S32 PCIV_Host_SysInit()
{
    HI_S32         s32Ret, i;
    MPP_SYS_CONF_S stSysConf = {0};
    VB_CONF_S      stVbConf ={0};
	VI_PUB_ATTR_S  stviAttr;
	VO_PUB_ATTR_S  stvoAttr;
	
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    
    stVbConf.u32MaxPoolCnt = 128;
    stVbConf.astCommPool[0].u32BlkSize = 884736;/* 768*576*1.5*/
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    stVbConf.astCommPool[1].u32BlkSize = 221184;/* 384*288*1.5*/
    stVbConf.astCommPool[1].u32BlkCnt = 60;    
    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    s32Ret = HI_MPI_VB_Init();
    HI_ASSERT((HI_SUCCESS == s32Ret));

    stSysConf.u32AlignWidth = 64;
    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    s32Ret = HI_MPI_SYS_Init();
    HI_ASSERT((HI_SUCCESS == s32Ret));

	/* Enable VI device*/
    memset(&stviAttr,0,sizeof(VI_PUB_ATTR_S));
	stviAttr.enInputMode = VI_MODE_BT656;
	stviAttr.enWorkMode  = VI_WORK_MODE_2D1;
	stviAttr.u32AdType   = AD_2815;
	stviAttr.enViNorm    = VIDEO_ENCODING_MODE_PAL;
    for (i = 0; i < VIU_MAX_DEV_NUM; i++)
    {
        s32Ret = HI_MPI_VI_SetPubAttr(i, &stviAttr);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        s32Ret = HI_MPI_VI_Enable(i);
        HI_ASSERT((HI_SUCCESS == s32Ret));
    }

    /* Enable VO device*/
    stvoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
    stvoAttr.u32BgColor = 0x000000;    
    s32Ret = HI_MPI_VO_SetPubAttr(&stvoAttr);
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    s32Ret = HI_MPI_VO_Enable();
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    return HI_SUCCESS;
}

/* The vi and vo should be configed first */
HI_S32 PCIV_Host_Vi2Vo(VO_CHN voStart, HI_S32 s32Num, HI_BOOL bEnable)
{
    HI_S32 i, s32Ret;

    if(HI_FALSE == bEnable)
    {
        for (i = 0; i < s32Num; i++)
        {
            VO_CHN voChn   = i + voStart;
            VI_DEV viDevId = i/2;
            VI_CHN viChn   = i%2;

            s32Ret = HI_MPI_VO_DisableChn(voChn);
            HI_ASSERT((HI_SUCCESS == s32Ret));

            s32Ret = HI_MPI_VI_DisableChn(viDevId, viChn);
            HI_ASSERT((HI_SUCCESS == s32Ret));
        }
        return s32Ret;
    }

    for (i = 0; i < s32Num; i++)
    {
        VO_CHN voChn   = i + voStart;
        VI_DEV viDevId = i/2;
        VI_CHN viChn   = i%2;

        s32Ret= HI_MPI_VO_SetChnAttr(voChn, &g_voChnAttr[voChn]);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        s32Ret = HI_MPI_VO_EnableChn(voChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        s32Ret = HI_MPI_VI_SetChnAttr(viDevId, viChn, &g_viChnAttr[viDevId][viChn]);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        s32Ret = HI_MPI_VI_EnableChn(viDevId, viChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        s32Ret = HI_MPI_VI_BindOutput(viDevId, viChn, voChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));
    }
    
    return HI_SUCCESS;
}

#define DEMO_MAX_ARGS 20
char args[DEMO_MAX_ARGS][255];
static int adjust_str(char *ptr)
{
	int i;
	while(*ptr==' ' && *ptr++ != '\0');

	for(i=strlen(ptr);i>0;i--)
	{
		if(*(ptr+i-1) == 0x0a || *(ptr+i-1) == ' ')
			*(ptr+i-1) = '\0';
		else
			break;
	}

	for(i=0;i<DEMO_MAX_ARGS;i++)
	{
		int j = 0;
		while(*ptr==' ' && *ptr++ != '\0');
		
		while((*ptr !=' ') && (*ptr!='\0'))
		{
			args[i][j++] = *ptr++;
		}

		args[i][j] = '\0';
		if('\0' == *ptr)
		{
			i++;
			break;
		}
		args[i][j] = '\0';
	}

	return i;
}

static PCIV_PORTMAP_S g_stPortMapVo[6][VO_MAX_CHN_NUM];
static PCIV_PORTMAP_S g_stPortMapVenc[6][VENC_MAX_CHN_NUM];

static HI_U32 g_u32VoChnNum[6]   = {0};
static HI_U32 g_u32ViChnNum[6]   = {0};
static HI_U32 g_u32VencChnNum[6] = {0};
static HI_U32 g_u32VencRevNum[6] = {0};
static HI_U32 g_u32PfWinBase[6]  = {0};

HI_S32 PCIV_GetPcivAttr(HI_S32 s32Target, PCIV_PCIVCMD_ATTR_S *pCmd)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S      stMsg;
    PCIV_DEVICE_S       *pPcivDev = NULL;
    PCIV_PCIVCMD_ECHO_S *pCmdEcho = NULL; 

    if(0 == s32Target)
    {
        s32Ret = HI_MPI_PCIV_GetAttr(&pCmd->stDevAttr);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    memcpy(&(stMsg.cMsgBody), pCmd, sizeof(PCIV_PCIVCMD_ATTR_S));
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_GETATTR;
    stMsg.enDevType = PCIV_DEVTYPE_PCIV;
    stMsg.u32MsgLen = sizeof(PCIV_PCIVCMD_ATTR_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /*
    ** Wait for the echo message. stMsg will be changed
    ** The pCmd->stDevAttr.stPCIDevice and u32AddrArray will be filled. 
    ** Attation: u32AddrArray[x] is the offset from PCI shm_phys_addr
    */
    pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)stMsg.cMsgBody;
    s32Ret   = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_PCIV); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);
    memcpy(&pCmd->stDevAttr, &pCmdEcho->stDevAttr, sizeof(PCIV_DEVICE_ATTR_S));

    return s32Ret;
}

HI_S32 PCIV_SetPcivAttr(HI_S32 s32Target, PCIV_PCIVCMD_ATTR_S *pCmd)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S      stMsg;
    PCIV_DEVICE_S       *pPcivDev = NULL;
    PCIV_PCIVCMD_ECHO_S *pCmdEcho = NULL; 

    if(0 == s32Target)
    {
        s32Ret = HI_MPI_PCIV_SetAttr(&pCmd->stDevAttr);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        s32Ret = PCIV_InitReceiver(&pCmd->stDevAttr, PCIV_DefaultRecFun);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        return s32Ret;
    }

    memcpy(&(stMsg.cMsgBody), pCmd, sizeof(PCIV_PCIVCMD_ATTR_S));
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_SETATTR;
    stMsg.enDevType = PCIV_DEVTYPE_PCIV;
    stMsg.u32MsgLen = sizeof(PCIV_PCIVCMD_ATTR_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /*
    ** Wait for the echo message. stMsg will be changed
    ** The pCmd->stDevAttr.stPCIDevice and u32AddrArray will be filled. 
    ** Attation: u32AddrArray[x] is the offset from PCI shm_phys_addr
    */
    pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)stMsg.cMsgBody;
    s32Ret   = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_PCIV); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}


HI_S32 PCIV_PcivCreate(HI_S32 s32Target, PCIV_PCIVCMD_CREATE_S *pCmd)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S      stMsg;
    PCIV_DEVICE_S       *pPcivDev = NULL;
    PCIV_PCIVCMD_ECHO_S *pCmdEcho = NULL; 

    if(0 == s32Target)
    {
        pPcivDev = &(pCmd->stDevAttr.stPCIDevice);
        s32Ret = HI_MPI_PCIV_Create(pPcivDev);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        if(pCmd->bMalloc)
        {
            s32Ret = HI_MPI_PCIV_Malloc(&pCmd->stDevAttr);
            if(HI_SUCCESS != s32Ret)
            {
                /* May be buffer is used by vo, wait a minute and try again. */
                usleep(40000);
                s32Ret = HI_MPI_PCIV_Malloc(&pCmd->stDevAttr);
            }
            HI_ASSERT((HI_SUCCESS == s32Ret));
        }
        else
        {   
            s32Ret = HI_MPI_PCIV_SetAttr(&pCmd->stDevAttr);
            HI_ASSERT((HI_SUCCESS == s32Ret));

            switch(pCmd->stBindObj.stBindObj.enType)
            {
                /* through flow. No Break */
                case PCIV_BIND_VENC:
                {
                    s32Ret = PCIV_InitCreater(&pCmd->stDevAttr, &pCmd->stBindObj.stBindObj);
                    break;
                }
                case PCIV_BIND_VSTREAMSND:
                {
                    s32Ret = PCIV_InitCreater(&pCmd->stDevAttr, NULL);
                    break;
                }
                case PCIV_BIND_VSTREAMREV:
                {
                    printf("The type is PCIV_BIND_VSTREAMREV. Something may be wrong!");
                    s32Ret = HI_FAILURE;
                    break;
                }
                default:
                {
                    // Do nothing!
                    break;
                }
            }
            HI_ASSERT((HI_SUCCESS == s32Ret));
        }

        pPcivDev = &(pCmd->stBindObj.stPCIDevice);
        memcpy(pPcivDev, &(pCmd->stDevAttr.stPCIDevice), sizeof(PCIV_DEVICE_S));
        s32Ret = HI_MPI_PCIV_Bind(&pCmd->stBindObj);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    memcpy(&(stMsg.cMsgBody), pCmd, sizeof(PCIV_PCIVCMD_CREATE_S));
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_CREATE;
    stMsg.enDevType = PCIV_DEVTYPE_PCIV;
    stMsg.u32MsgLen = sizeof(PCIV_PCIVCMD_CREATE_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 
    
    /*
    ** Wait for the echo message. stMsg will be changed
    ** The pCmd->stDevAttr.stPCIDevice and u32AddrArray will be filled. 
    ** Attation: u32AddrArray[x] is the offset from PCI shm_phys_addr
    */
    pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)stMsg.cMsgBody;
    s32Ret   = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_PCIV); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);
    memcpy(&pCmd->stDevAttr, &pCmdEcho->stDevAttr, sizeof(PCIV_DEVICE_ATTR_S));
    
    return HI_SUCCESS;
}

HI_S32 PCIV_PcivStart(HI_S32 s32Target, HI_S32 s32Dev, HI_S32 s32Port)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_PCIVCMD_START_S *pCmd     = (PCIV_PCIVCMD_START_S *)stMsg.cMsgBody;
    PCIV_PCIVCMD_ECHO_S  *pCmdEcho = (PCIV_PCIVCMD_ECHO_S  *)stMsg.cMsgBody;

    if(0 == s32Target)
    {
        PCIV_DEVICE_S      stPciDev;
        stPciDev.s32PciDev = s32Dev;
        stPciDev.s32Port   = s32Port;

        s32Ret = HI_MPI_PCIV_Start(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        return s32Ret;
    }

    /* Send Message to stop remote PCIV */
    pCmd->stPciDev.s32PciDev = s32Dev;
    pCmd->stPciDev.s32Port   = s32Port;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_START;
    stMsg.enDevType = PCIV_DEVTYPE_PCIV;
    stMsg.u32MsgLen = sizeof(PCIV_PCIVCMD_START_S);
    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_PCIV); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return HI_SUCCESS;
}


HI_S32 PCIV_PcivDestroy(HI_S32 s32Target, HI_S32 s32Dev, HI_S32 s32Port)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_PCIVCMD_DESTROY_S *pCmd     = (PCIV_PCIVCMD_DESTROY_S *)stMsg.cMsgBody;
    PCIV_PCIVCMD_ECHO_S    *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)stMsg.cMsgBody;

    if(0 == s32Target)
    {
        PCIV_DEVICE_S      stPciDev;
        stPciDev.s32PciDev = s32Dev;
        stPciDev.s32Port   = s32Port;

        s32Ret = HI_MPI_PCIV_Stop(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
    
        s32Ret = HI_MPI_PCIV_Free(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
    
        s32Ret = HI_MPI_PCIV_Destroy(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret);
        
        s32Ret  = PCIV_DeInitReceiver(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        s32Ret  = PCIV_DeInitCreater(&stPciDev);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        return s32Ret;
    }

    /* Send Message to stop remote PCIV */
    pCmd->stPciDev.s32PciDev = s32Dev;
    pCmd->stPciDev.s32Port   = s32Port;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_DESTROY;
    stMsg.enDevType = PCIV_DEVTYPE_PCIV;
    stMsg.u32MsgLen = sizeof(PCIV_PCIVCMD_DESTROY_S);
    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_PCIV); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return HI_SUCCESS;
}

HI_S32 PCIV_VoStart(HI_S32 s32Target, HI_S32 s32Cnt, VO_CHN voChn[])
{
    HI_S32 s32Ret, i, s32MsgLen;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VOCMD_CREATE_S *pCmd     = (PCIV_VOCMD_CREATE_S *)stMsg.cMsgBody;
    PCIV_VOCMD_ECHO_S   *pCmdEcho = (PCIV_VOCMD_ECHO_S   *)stMsg.cMsgBody;

    if(s32Target == 0)
    {
        for(i=0; i<s32Cnt; i++)
        {
            s32Ret = HI_MPI_VO_SetChnAttr(voChn[i], &g_voChnAttr[voChn[i]]);
            HI_ASSERT(HI_SUCCESS == s32Ret); 
            
            s32Ret = HI_MPI_VO_EnableChn(voChn[i]);
            HI_ASSERT(HI_SUCCESS == s32Ret); 

            if(HI_SUCCESS != s32Ret) break;
        }

        return s32Ret;
    }

    /* If start the remote vo then use the message */
    for(i=0; i<s32Cnt; i++)
    {
        memcpy(&(pCmd->stAttr), &g_voChnAttr[voChn[i]], sizeof(VO_CHN_ATTR_S));
        pCmd->voChn     = voChn[i];

        pCmd++;
    }
    
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_CREATE;
    stMsg.enDevType = PCIV_DEVTYPE_VOCHN;
    stMsg.u32MsgLen = sizeof(PCIV_VOCMD_CREATE_S) * s32Cnt;

    HI_ASSERT(stMsg.u32MsgLen < PCIV_MSG_MAXLEN); 

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VOCHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);
    return s32Ret;
}

HI_S32 PCIV_VoStop(HI_S32 s32Target, HI_S32 s32Cnt, VO_CHN voChn[])
{
    HI_S32 s32Ret, i;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VOCMD_DESTROY_S *pCmd    = (PCIV_VOCMD_DESTROY_S *)stMsg.cMsgBody;
    PCIV_VOCMD_ECHO_S   *pCmdEcho = (PCIV_VOCMD_ECHO_S   *)stMsg.cMsgBody;

    if(s32Target == 0)
    {
        for(i=0; i<s32Cnt; i++)
        {
            s32Ret = HI_MPI_VO_DisableChn(voChn[i]);
            HI_ASSERT(HI_SUCCESS == s32Ret); 

            if(HI_SUCCESS != s32Ret) break;
        }

        return s32Ret;
    }

    /* If start the remote vo then use the message */
    for(i=0; i< s32Cnt; i++)
    {
        pCmd->voChn = voChn[i];
        
        pCmd++;
    }
    
    /* If start the remote vo then use the message */
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_DESTROY;
    stMsg.enDevType = PCIV_DEVTYPE_VOCHN;
    stMsg.u32MsgLen = sizeof(PCIV_VOCMD_DESTROY_S)*s32Cnt;
    
    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VOCHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);
    return s32Ret;
}


/* Send Message to create remote VI device */
HI_S32 PCIV_ViStart(HI_S32 s32Target, VI_DEV viDev, VI_CHN viChn)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VICMD_CREATE_S *pCmd     = (PCIV_VICMD_CREATE_S *)stMsg.cMsgBody;
    PCIV_VICMD_ECHO_S   *pCmdEcho = (PCIV_VICMD_ECHO_S   *)stMsg.cMsgBody;

    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        /* Start the VI first */
        s32Ret = HI_MPI_VI_SetChnAttr(viDev, viChn, &g_viChnAttr[viDev][viChn]);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        s32Ret = HI_MPI_VI_EnableChn(viDev, viChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    /* If start the remote vi then use the message */
    pCmd->viDev = viDev;
    pCmd->viChn = viChn;
    memcpy(&(pCmd->stAttr), &g_viChnAttr[viDev][viChn], sizeof(VI_CHN_ATTR_S));
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_CREATE;
    stMsg.enDevType = PCIV_DEVTYPE_VICHN;
    stMsg.u32MsgLen = sizeof(PCIV_VICMD_CREATE_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VICHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}

/* Send Message to create remote VI device */
HI_S32 PCIV_ViStop(HI_S32 s32Target, VI_DEV viDev, VI_CHN viChn)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VICMD_DESTROY_S *pCmd     = (PCIV_VICMD_DESTROY_S *)stMsg.cMsgBody;
    PCIV_VICMD_ECHO_S    *pCmdEcho = (PCIV_VICMD_ECHO_S   *)stMsg.cMsgBody;

    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        s32Ret = HI_MPI_VI_DisableChn(viDev, viChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    /* If start the remote vi then use the message */
    pCmd->viDev = viDev;
    pCmd->viChn = viChn;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_DESTROY;
    stMsg.enDevType = PCIV_DEVTYPE_VICHN;
    stMsg.u32MsgLen = sizeof(PCIV_VICMD_DESTROY_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VICHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}


/* Send Message to create remote VENC device */
HI_S32 PCIV_GrpStart(HI_S32 s32Target, VENC_GRP vencGrp)
{
    HI_S32 s32Ret;
    VI_DEV viDev;
    VI_CHN viChn;
    PCIV_MSGHEAD_S stMsg;
    PCIV_GRPCMD_CREATE_S *pCmd     = (PCIV_GRPCMD_CREATE_S *)stMsg.cMsgBody;
    PCIV_GRPCMD_ECHO_S   *pCmdEcho = (PCIV_GRPCMD_ECHO_S   *)stMsg.cMsgBody;

    viDev = vencGrp / 2;
    viChn = vencGrp % 2;
    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        /* Create new group first */
        s32Ret = HI_MPI_VENC_CreateGroup(vencGrp);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        s32Ret = HI_MPI_VENC_BindInput(vencGrp, viDev, viChn);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    /* If start the remote vi then use the message */
    pCmd->vencGrp = vencGrp;
    pCmd->viDev   = viDev;
    pCmd->viChn   = viChn;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_CREATE;
    stMsg.enDevType = PCIV_DEVTYPE_GROUP;
    stMsg.u32MsgLen = sizeof(PCIV_GRPCMD_CREATE_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_GROUP); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}

/* Send Message to create remote VENC device */
HI_S32 PCIV_GrpStop(HI_S32 s32Target, VENC_GRP vencGrp)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_GRPCMD_DESTROY_S *pCmd     = (PCIV_GRPCMD_DESTROY_S *)stMsg.cMsgBody;
    PCIV_GRPCMD_ECHO_S    *pCmdEcho = (PCIV_GRPCMD_ECHO_S   *)stMsg.cMsgBody;

    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        s32Ret = HI_MPI_VENC_UnbindInput(vencGrp);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        
        /* Create new group first */
        s32Ret = HI_MPI_VENC_DestroyGroup(vencGrp);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        return s32Ret;
    }

    /* If start the remote vi then use the message */
    pCmd->vencGrp = vencGrp;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_DESTROY;
    stMsg.enDevType = PCIV_DEVTYPE_GROUP;
    stMsg.u32MsgLen = sizeof(PCIV_GRPCMD_DESTROY_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_GROUP); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}

/* Send Message to create remote VENC device */
HI_S32 PCIV_VencStart(HI_S32 s32Target, VENC_GRP vencGrp, VENC_CHN vencChn, HI_BOOL bMain)
{
    HI_S32   s32Ret;
    HI_U32   u32PicWidth, u32PicHeight, u32AttrLen;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VENCCMD_CREATE_S *pCmd     = (PCIV_VENCCMD_CREATE_S *)stMsg.cMsgBody;
    PCIV_VENCCMD_ECHO_S   *pCmdEcho = (PCIV_VENCCMD_ECHO_S   *)stMsg.cMsgBody;
    VENC_CHN_ATTR_S       *pAttr    = NULL;

    /* Get the venc config */
    u32PicWidth   = g_viChnAttr[vencGrp/2][vencGrp%2].stCapRect.u32Width;
    u32PicHeight  = g_viChnAttr[vencGrp/2][vencGrp%2].stCapRect.u32Height;
    if(g_viChnAttr[vencGrp/2][vencGrp%2].bDownScale)
    {
        u32PicWidth >>= 1;
    }
    
    if(g_viChnAttr[vencGrp/2][vencGrp%2].enCapSel == VI_CAPSEL_BOTH)
    {
        u32PicHeight <<= 1;
    }

    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        pAttr         = &(pCmd->stAttr);
        pAttr->pValue = pCmd + 1;
        s32Ret = PCIV_VencConfigChn(bMain, PT_H264, u32PicWidth, u32PicHeight,
                                    pAttr, &u32AttrLen);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        s32Ret  = HI_MPI_VENC_CreateChn(vencChn, pAttr, NULL);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        s32Ret = HI_MPI_VENC_RegisterChn(vencGrp,vencChn);/*register to group 0 */
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        
        s32Ret = HI_MPI_VENC_StartRecvPic(vencChn);/*main chn start rev */
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        return s32Ret;
    }

    /* If start the remote venc then use the message */
    pAttr         = &(pCmd->stAttr);
    pAttr->pValue = pCmd + 1;
    s32Ret = PCIV_VencConfigChn(bMain, PT_H264, u32PicWidth, u32PicHeight,
                                pAttr, &u32AttrLen);
    HI_ASSERT(HI_SUCCESS == s32Ret); 

    pCmd->vencGrp = vencGrp;
    pCmd->vencChn = vencChn;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_CREATE;
    stMsg.enDevType = PCIV_DEVTYPE_VENCCHN;
    stMsg.u32MsgLen = sizeof(PCIV_VENCCMD_CREATE_S) + u32AttrLen;

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VENCCHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}

/* Send Message to create remote VENC device */
HI_S32 PCIV_VencStop(HI_S32 s32Target, VENC_CHN vencChn)
{
    HI_S32   s32Ret;
    PCIV_MSGHEAD_S stMsg;
    PCIV_VENCCMD_DESTROY_S *pCmd     = (PCIV_VENCCMD_DESTROY_S *)stMsg.cMsgBody;
    PCIV_VENCCMD_ECHO_S    *pCmdEcho = (PCIV_VENCCMD_ECHO_S   *)stMsg.cMsgBody;

    /* If start the local vi then just do it */
    if(s32Target == 0)
    {
        s32Ret = HI_MPI_VENC_StopRecvPic(vencChn);/*main chn start rev */
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        
        s32Ret = HI_MPI_VENC_UnRegisterChn(vencChn);/*register to group 0 */
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        
        s32Ret  = HI_MPI_VENC_DestroyChn(vencChn);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        return s32Ret;
    }

    pCmd->vencChn   = vencChn;
    stMsg.u32Target = s32Target;
    stMsg.enMsgType = PCIV_MSGTYPE_DESTROY;
    stMsg.enDevType = PCIV_DEVTYPE_VENCCHN;
    stMsg.u32MsgLen = sizeof(PCIV_VENCCMD_DESTROY_S);

    s32Ret = PCIV_SendMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 

    /* Wait for the echo message. stMsg will be changed */
    s32Ret = PCIV_ReadMsg(s32Target, PCIV_MSGPORT_USERCMD, &stMsg);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    HI_ASSERT(stMsg.enMsgType == PCIV_MSGTYPE_CMDECHO);
    HI_ASSERT(stMsg.enDevType == PCIV_DEVTYPE_VENCCHN); 
    HI_ASSERT(pCmdEcho->s32Echo == HI_SUCCESS);

    return s32Ret;
}


/* 
** T1 is VI , T2 is VO. Preview change to s32Num channels.
** The Flow is:
** 1. Backup the connect object of all the vo. And get the last preview number.
** 2. Check whether the total vo or vi channel is too much
** 3. Start all the vi channel on s32T1 first. The data source ready first.
** 4. Destroy all the pciv connection.
** 5. Start all the new preview pciv for VO.
** 6. Start the new preview pciv for vi on s32T1.
** 7. Then start the other pciv for another board's vo or VDEC.
** 8. Reconfig the VO channel, and disable the unused vo.
** ATTATION: From step 4 to step 6 will no new picture come.
*/
HI_S32 PCIV_DoPreview(HI_S32 s32T1, HI_S32 s32T2, HI_S32 s32Num, HI_S32 s32BufNum)
{
    HI_S32 s32Ret, i, j, k, s32NewVoNum, s32NewViNum, s32LastPre, s32LastOth;
    PCIV_PIC_ATTR_S      *pPicAttr = NULL;
    PCIV_BIND_OBJ_S      *pBindObj = NULL;
    PCIV_DEVICE_S        *pDev     = NULL;
    PCIV_REMOTE_OBJ_S    *pRemtObj = NULL;
    PCIV_DEVICE_ATTR_S   *pDevAttr = NULL;
    PCIV_PCIVCMD_CREATE_S stPcivCmd[16];
    PCIV_PORTMAP_S        stMapTmpPre[VO_MAX_CHN_NUM];
    PCIV_PORTMAP_S        stMapTmpOth[VO_MAX_CHN_NUM];    
    
    /* Backup the connect object of all the vo. And get the last preview number. */
    s32LastPre = s32LastOth = 0;
    for(i=0; i<g_u32VoChnNum[s32T2]; i++)
    {
        pBindObj = &g_stPortMapVo[s32T2][i].stRmtObj;
        
        if(  (pBindObj->enType == PCIV_BIND_VI)
           &&(g_stPortMapVo[s32T2][i].s32RmtTgt == s32T1))
        {
            memcpy(&stMapTmpPre[s32LastPre], &g_stPortMapVo[s32T2][i], sizeof(PCIV_PORTMAP_S));
            s32LastPre++;
        }
        else
        {
            memcpy(&stMapTmpOth[s32LastOth], &g_stPortMapVo[s32T2][i], sizeof(PCIV_PORTMAP_S));
            s32LastOth++;
        }
    }

    /* Check whether the total vo or vi channel is too much */
    s32NewVoNum  = s32LastOth + s32Num;
    if( (s32NewVoNum > VO_MAX_CHN_NUM) )
    {
        printf("Target %d has no much(%d) vo channel\n", s32T2, s32NewVoNum);
        return HI_SUCCESS;
    }

    s32NewViNum = g_u32ViChnNum[s32T1] - s32LastPre + s32Num;
    if(s32NewViNum > VIU_MAX_CHN_NUM)
    {
        printf("Target %d has no much(%d) vi channel\n", s32T1, s32NewViNum);
        return HI_SUCCESS;
    }

    /* Start all the vi channel on s32T1 first. The data source ready first. */
    for(i=0; i < s32NewViNum; i++)
    {
        s32Ret = PCIV_ViStart(s32T1, i/2, i%2);
        HI_ASSERT(HI_SUCCESS == s32Ret);  
    }

    /* Destroy all the pciv connection. */
    for(i=0; i<g_u32VoChnNum[s32T2]; i++)
    {
        PCIV_MSGHEAD_S stMsg;
        PCIV_PCIVCMD_DESTROY_S *pCmd     = (PCIV_PCIVCMD_DESTROY_S *)stMsg.cMsgBody;
        PCIV_PCIVCMD_ECHO_S    *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)stMsg.cMsgBody;
        PCIV_PORTMAP_S         *pMap     = NULL;

        pMap = &(g_stPortMapVo[s32T2][i]);
        
        /* Stop the remote PCIV */
        s32Ret = PCIV_PcivDestroy(pMap->s32RmtTgt, pMap->stRmtDev.s32PciDev, pMap->stRmtDev.s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        /* Stop the local PCIV */
        s32Ret = PCIV_PcivDestroy(s32T2, pMap->stLocalDev.s32PciDev, pMap->stLocalDev.s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
    }

    /* Start all the new preview pciv for VO. Preview channle is 0~s32Num */
    s32Ret = PCIV_VoConfigChn(s32NewVoNum);
    HI_ASSERT(HI_SUCCESS == s32Ret);  
    for(i=0; i < s32NewVoNum; i++)
    {
        /* Start PCIV for VO. stPcivCmd will be filled */
        stPcivCmd[i].bMalloc = HI_TRUE;
        stPcivCmd[i].stDevAttr.u32Count = s32BufNum;

        pDev = &stPcivCmd[i].stDevAttr.stPCIDevice;
        pDev->s32PciDev = PCIV_DEFAULT_DEVID;
        pDev->s32Port   = -1;

        pBindObj = &stPcivCmd[i].stBindObj.stBindObj;
        pBindObj->enType      = PCIV_BIND_VO;
        pBindObj->unAttachObj.voDevice.voChn = i;

        pRemtObj = &stPcivCmd[i].stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId = s32T1;
        
        pPicAttr = &stPcivCmd[i].stDevAttr.stPicAttr;
        pPicAttr->enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        pPicAttr->u32Height     = g_voChnAttr[i].stRect.u32Height;
        pPicAttr->u32Width      = g_voChnAttr[i].stRect.u32Width;
        pPicAttr->u32Stride     = EDGE_ALIGN(pPicAttr->u32Width, 64);
        if(pPicAttr->u32Width > 352)
        {
            pPicAttr->u32Field      = VIDEO_FIELD_INTERLACED;
        }
        else
        {
            pPicAttr->u32Field      = VIDEO_FIELD_FRAME;
        }
        
        s32Ret  = PCIV_PcivCreate(s32T2, &stPcivCmd[i]);
        s32Ret |= PCIV_PcivStart(s32T2, pDev->s32PciDev, pDev->s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret);  

        g_stPortMapVo[s32T2][i].stLocalDev.s32PciDev = pDev->s32PciDev;
        g_stPortMapVo[s32T2][i].stLocalDev.s32Port   = pDev->s32Port;
    }

    /* Start the new preview pciv for vi on s32T1. */
    for(i=0; i<s32Num; i++)
    {
        /* Start pciv for VI. stPcivCmd will be used here */
        /* Modify some device information */
        stPcivCmd[i].bMalloc = HI_FALSE;
        
        /* Set the remote object as this vo's pci device */
        pDev     = &stPcivCmd[i].stDevAttr.stPCIDevice;
        pRemtObj = &stPcivCmd[i].stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId        = s32T2;
        pRemtObj->stTargetDev.s32PciDev = pDev->s32PciDev;
        pRemtObj->stTargetDev.s32Port   = pDev->s32Port;
        
        /* Now the vo's pci device is no use. We create a new for VI */
        pDev->s32PciDev = PCIV_DEFAULT_DEVID;
        pDev->s32Port   = -1;

        pBindObj = &stPcivCmd[i].stBindObj.stBindObj;
        if(i < s32LastPre)
        {
            memcpy(pBindObj, &stMapTmpPre[i].stRmtObj, sizeof(PCIV_BIND_OBJ_S));
        }
        else
        {
            VI_DEV viDev = g_u32ViChnNum[s32T1] / 2;
            VI_CHN viChn = g_u32ViChnNum[s32T1] % 2;
            pBindObj->enType      = PCIV_BIND_VI;
            pBindObj->unAttachObj.viDevice.viDevId = viDev;
            pBindObj->unAttachObj.viDevice.viChn   = viChn;

            g_u32ViChnNum[s32T1]++;
        }
        
        pDevAttr = &stPcivCmd[i].stDevAttr;
        for(j=0; j<pDevAttr->u32Count; j++)
        {
            /* Get the PCI Bus Address. g_u32PfWinBase of host is 0 */
            pDevAttr->u32AddrArray[j] += g_u32PfWinBase[s32T2];
        }
        
        s32Ret = PCIV_PcivCreate(s32T1, &stPcivCmd[i]);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        memcpy(&g_stPortMapVo[s32T2][i].stRmtDev, pDev, sizeof(*pDev));
        memcpy(&g_stPortMapVo[s32T2][i].stRmtObj, pBindObj, sizeof(*pBindObj));
        g_stPortMapVo[s32T2][i].s32RmtTgt = s32T1;
    }

    /* Then start the other pciv for another board's vo or VDEC. */
    for(i=s32Num, k=0; i<s32NewVoNum; i++, k++)
    {
        /* Modify some device information */
        stPcivCmd[i].bMalloc = HI_FALSE;
        
        /* Set the remote object as this vo's pci device */
        pDev     = &stPcivCmd[i].stDevAttr.stPCIDevice;
        pRemtObj = &stPcivCmd[i].stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId        = s32T2;
        pRemtObj->stTargetDev.s32PciDev = pDev->s32PciDev;
        pRemtObj->stTargetDev.s32Port   = pDev->s32Port;
        
        /* Now the vo's pci device is no use. We create a new for VI */
        pDev->s32PciDev = PCIV_DEFAULT_DEVID;
        pDev->s32Port   = -1;

        pBindObj = &stPcivCmd[i].stBindObj.stBindObj;
        memcpy(pBindObj, &stMapTmpOth[k].stRmtObj, sizeof(PCIV_BIND_OBJ_S));

        pDevAttr = &stPcivCmd[i].stDevAttr;
        for(j=0; j<pDevAttr->u32Count; j++)
        {
            /* Get the PCI Bus Address. g_u32PfWinBase of host is 0 */
            pDevAttr->u32AddrArray[j] += g_u32PfWinBase[s32T2];
        }
        
        s32Ret = PCIV_PcivCreate(stMapTmpOth[k].s32RmtTgt, &stPcivCmd[i]);
        HI_ASSERT(HI_SUCCESS == s32Ret);  

        memcpy(&g_stPortMapVo[s32T2][i].stRmtDev, pDev, sizeof(*pDev));
        memcpy(&g_stPortMapVo[s32T2][i].stRmtObj, pBindObj, sizeof(*pBindObj));
        g_stPortMapVo[s32T2][i].s32RmtTgt = stMapTmpOth[k].s32RmtTgt;

    }

    /* 
    ** Config the remote device of pciv which is binded with VO on board s32T2.
    ** And Start the pciv device which is binded with VI on remote board.
    */
    for(i=0; i<s32NewVoNum; i++)
    {
        PCIV_PCIVCMD_ATTR_S   stPcivAttrCmd;

        mempcpy(&stPcivAttrCmd.stDevAttr.stPCIDevice,
                &g_stPortMapVo[s32T2][i].stLocalDev, sizeof(PCIV_DEVICE_S));
        s32Ret = PCIV_GetPcivAttr(s32T2, &stPcivAttrCmd);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        pRemtObj = &stPcivAttrCmd.stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId        = g_stPortMapVo[s32T2][i].s32RmtTgt;
        pRemtObj->stTargetDev.s32PciDev = g_stPortMapVo[s32T2][i].stRmtDev.s32PciDev;
        pRemtObj->stTargetDev.s32Port   = g_stPortMapVo[s32T2][i].stRmtDev.s32Port;
        s32Ret = PCIV_SetPcivAttr(s32T2, &stPcivAttrCmd);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        s32Ret = PCIV_PcivStart(pRemtObj->u32MsgTargetId, 
                                pRemtObj->stTargetDev.s32PciDev,
                                pRemtObj->stTargetDev.s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    /* Reconfig the VO channel, and disable the unused vo. */
    {
        VO_CHN voChn[VO_MAX_CHN_NUM];
        if(s32NewVoNum > 0)
        {
            for(i=0; i < s32NewVoNum; i++)
            {
                voChn[i] = i;
            }
            
            s32Ret = PCIV_VoStart(s32T2, s32NewVoNum, voChn);
            HI_ASSERT(HI_SUCCESS == s32Ret);
        }
        
        if(s32NewVoNum < g_u32VoChnNum[s32T2])
        {
            HI_S32 s32StopNum = g_u32VoChnNum[s32T2] - s32NewVoNum;
            for(i=0; i < s32StopNum; i++)
            {
                voChn[i] = s32NewVoNum + i;
            }

            s32Ret = PCIV_VoStop(s32T2, s32StopNum, voChn);
            HI_ASSERT(HI_SUCCESS == s32Ret);
        }
    }

    g_u32VoChnNum[s32T2] = s32NewVoNum;
    g_u32ViChnNum[s32T1] = s32NewViNum;
    return HI_SUCCESS;
}

/* 
** T1 is the creater, T2 is the reciver, s32Num is the channel number
** One channel involve two stream: Main and sub Stream
*/
HI_S32 PCIV_StartVencTrans(HI_S32 s32T1, HI_S32 s32T2, VENC_GRP vencGrp)
{
    HI_S32 s32Ret, j, k, s32TotalChn;
    PCIV_PIC_ATTR_S      *pPicAttr = NULL;
    PCIV_BIND_OBJ_S      *pBindObj = NULL;
    PCIV_DEVICE_S        *pDev     = NULL;
    PCIV_REMOTE_OBJ_S    *pRemtObj = NULL;
    PCIV_DEVICE_ATTR_S   *pDevAttr = NULL;
    PCIV_PCIVCMD_CREATE_S stPcivCmd[16];
    PCIV_PCIVCMD_ATTR_S   stPcivAttrCmd;
    VI_DEV viDev = (vencGrp) / 2;
    VI_CHN viChn = (vencGrp) % 2;

    if(g_stPortMapVenc[s32T1][vencGrp*2].s32RmtTgt != -1)
    {
        printf("The VENC channel %d on target %d is runing.\n",vencGrp, s32T1);
        return HI_SUCCESS;
    }

    /* If VI not be configed then config it */
    s32Ret = PCIV_ViStart(s32T1, viDev, viChn);
    if(s32Ret != HI_SUCCESS)
    {
        printf("Vi Device %d channel %d not started\n", viDev, viChn);
        return HI_SUCCESS;
    }

    /* Start the Group. Only the new group need to be configed. */
    s32Ret = PCIV_GrpStart(s32T1, vencGrp);
    HI_ASSERT(HI_SUCCESS == s32Ret);  

    /* Start the transfer path first */    
    for(j=0; j<2; j++)
    {
        PCIV_DEVICE_S stRevDev, stSndDev;
        VENC_CHN vencChn = vencGrp * 2 + j;

        /*
        ** Step 1, Start PCIV for reciver. Config the attribute first.
        */
        stPcivCmd[vencChn].bMalloc = HI_TRUE;
        stPcivCmd[vencChn].stDevAttr.u32Count = 1;

        pDev = &stPcivCmd[vencChn].stDevAttr.stPCIDevice;
        pDev->s32PciDev = PCIV_DEFAULT_DEVID;
        pDev->s32Port   = -1;

        pBindObj = &stPcivCmd[vencChn].stBindObj.stBindObj;
        pBindObj->enType = PCIV_BIND_VSTREAMREV;

        /* Caculate the buffer size */
        pPicAttr = &stPcivCmd[vencChn].stDevAttr.stPicAttr;
        pPicAttr->enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        pPicAttr->u32Field      = VIDEO_FIELD_FRAME;
        pPicAttr->u32Width      = g_viChnAttr[viDev][viChn].stCapRect.u32Width;
        pPicAttr->u32Height     = g_viChnAttr[viDev][viChn].stCapRect.u32Height;
        if(g_viChnAttr[viDev][viChn].enCapSel == VI_CAPSEL_BOTH)
        {
            pPicAttr->u32Height <<= 1;
        }
        
        if(g_viChnAttr[viDev][viChn].bDownScale)
        {
            pPicAttr->u32Width >>= 1;
        }
        
        /* If sub stream, then downscale the size */
        if(j == 1)
        {
            pPicAttr->u32Width  >>= 1;
            pPicAttr->u32Height >>= 1;
        }
        pPicAttr->u32Stride = EDGE_ALIGN(pPicAttr->u32Width, 16);

        /* Create a new PCIV and get the share memory */
        s32Ret  = PCIV_PcivCreate(s32T2, &stPcivCmd[vencChn]);
        s32Ret |= PCIV_PcivStart(s32T2, pDev->s32PciDev, pDev->s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        g_stPortMapVenc[s32T1][vencChn].s32RmtTgt = s32T2;
        memcpy(&g_stPortMapVenc[s32T1][vencChn].stRmtDev, pDev, sizeof(*pDev));
        memcpy(&g_stPortMapVenc[s32T1][vencChn].stRmtObj, pBindObj, sizeof(*pBindObj));
        
        /*
        ** OK, The new pci device is created, we save the device ID for use
        */
        stRevDev.s32PciDev = pDev->s32PciDev;
        stRevDev.s32Port   = pDev->s32Port;

        
        /*
        ** Step 2, Start pciv for sender. stPcivCmd will be used here
        **         we need not to malloc memory
        */
        stPcivCmd[vencChn].bMalloc = HI_FALSE;
        
        pDev     = &stPcivCmd[vencChn].stDevAttr.stPCIDevice;
        pDev->s32PciDev = PCIV_DEFAULT_DEVID;
        pDev->s32Port   = -1;

        pBindObj = &stPcivCmd[vencChn].stBindObj.stBindObj;
        pBindObj->enType = PCIV_BIND_VENC; //PCIV_BIND_VSTREAMSND;
        pBindObj->unAttachObj.vencDevice.vencChn = vencChn;
        
        /* Set the remote object as this vo's pci device */
        pRemtObj = &stPcivCmd[vencChn].stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId        = s32T2;
        pRemtObj->stTargetDev.s32PciDev = stRevDev.s32PciDev;
        pRemtObj->stTargetDev.s32Port   = stRevDev.s32Port;

        /* Config the share memory */
        pDevAttr = &stPcivCmd[vencChn].stDevAttr;
        for(k=0; k<pDevAttr->u32Count; k++)
        {
            /* Get the PCI Bus Address. g_u32PfWinBase of host is 0 */
            pDevAttr->u32AddrArray[k] += g_u32PfWinBase[s32T2];
        }

        /* Create a new PCIV and set the share memory */
        s32Ret  = PCIV_PcivCreate(s32T1, &stPcivCmd[vencChn]);
        s32Ret |= PCIV_PcivStart(s32T1, pDev->s32PciDev, pDev->s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        
        memcpy(&g_stPortMapVenc[s32T1][vencChn].stLocalDev, pDev, sizeof(*pDev));
        
        stSndDev.s32PciDev = pDev->s32PciDev;
        stSndDev.s32Port   = pDev->s32Port;
        
        /*
        ** Step 3, All the pci device is created. 
        **         We need to update the receiver's remote object.
        */
        stPcivAttrCmd.stDevAttr.stPCIDevice.s32PciDev = stRevDev.s32PciDev;
        stPcivAttrCmd.stDevAttr.stPCIDevice.s32Port   = stRevDev.s32Port;
        s32Ret = PCIV_GetPcivAttr(s32T2, &stPcivAttrCmd);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        pRemtObj = &stPcivAttrCmd.stDevAttr.stRemoteObj;
        pRemtObj->u32MsgTargetId        = s32T1;
        pRemtObj->stTargetDev.s32PciDev = stSndDev.s32PciDev;
        pRemtObj->stTargetDev.s32Port   = stSndDev.s32Port;
        s32Ret = PCIV_SetPcivAttr(s32T2, &stPcivAttrCmd);
        HI_ASSERT(HI_SUCCESS == s32Ret); 
        
        /*
        ** Step 4, Start the VENC channel on s32T1 board.
        */
        {
            HI_BOOL bMain = (j == 0) ? HI_TRUE : HI_FALSE;
            s32Ret = PCIV_VencStart(s32T1, vencGrp, vencChn, bMain);
            HI_ASSERT(HI_SUCCESS == s32Ret); 
        }
    }

    g_u32VencChnNum[s32T1]++;
    return HI_SUCCESS;
}

/* 
** T1 is the creater, T2 is the reciver, s32Num is the channel number
** One channel involve two stream: Main and sub Stream
*/
HI_S32 PCIV_StopVencTrans(HI_S32 s32T1, HI_S32 s32T2, VENC_GRP vencGrp)
{
    HI_S32 s32Ret, i;

    if(g_stPortMapVenc[s32T1][vencGrp*2].s32RmtTgt == -1)
    {
        printf("The VENC channel %d on target %d has stoped.\n",vencGrp, s32T1);
        return HI_SUCCESS;
    }
    
    /* Stop the VENC channel first */
    for(i = 1; i >= 0; i--)
    {
        VENC_CHN vencChn    = vencGrp * 2 + i;
        PCIV_DEVICE_S *pDev = NULL;
        /* Stop the VENC channel first */
        s32Ret = PCIV_VencStop(s32T1, vencChn);
        HI_ASSERT(HI_SUCCESS == s32Ret);  

        /* Stop the VENC creator PCIV */
        pDev = &(g_stPortMapVenc[s32T1][vencChn].stLocalDev);
        s32Ret = PCIV_PcivDestroy(s32T1, pDev->s32PciDev, pDev->s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret);  
        g_stPortMapVenc[s32T1][vencChn].stLocalDev.s32PciDev = -1;
        g_stPortMapVenc[s32T1][vencChn].stLocalDev.s32PciDev = -1;
        
        /* Stop the reciver PCIV */
        s32T2  = g_stPortMapVenc[s32T1][vencChn].s32RmtTgt;
        pDev   = &(g_stPortMapVenc[s32T1][vencChn].stRmtDev);
        s32Ret = PCIV_PcivDestroy(s32T2, pDev->s32PciDev, pDev->s32Port);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        g_stPortMapVenc[s32T1][vencChn].s32RmtTgt = -1;
    }

    /* Stop the VENC group */
    s32Ret = PCIV_GrpStop(s32T1, vencGrp);
    HI_ASSERT(HI_SUCCESS == s32Ret);  

    g_u32VencChnNum[s32T1]--;
    return HI_SUCCESS;
}

static void parsearg(int argc)
{
    HI_S32 s32Ret, s32MsgTarget;
    
    if( (0 == strcmp("vi", args[0])) && (argc > 4))
    {
        VI_DEV viDev = atoi(args[2]);
        VI_CHN viChn = atoi(args[3]);

        if(0 == strcmp("d1", args[4]))
        {
            PCIV_ViConfigD1(viDev, viChn);
        }
        else
        {
            PCIV_ViConfigCif(viDev, viChn);
        }
        
        s32Ret = PCIV_ViStart(atoi(args[1]),viDev, viChn);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else if( (0 == strcmp("pre", args[0])) && (argc > 3))
    {
        HI_S32 s32BufNum=4;

        if(argc > 4)
        {
            s32BufNum = atoi(args[4]);
            if(s32BufNum > 4) s32BufNum = 4;
        }
        
        s32Ret = PCIV_DoPreview(atoi(args[1]),atoi(args[2]), atoi(args[3]), s32BufNum);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else if( (0 == strcmp("cvenc", args[0])) && (argc > 3))
    {
        s32Ret = PCIV_StartVencTrans(atoi(args[1]),atoi(args[2]), atoi(args[3]));
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else if( (0 == strcmp("dvenc", args[0])) && (argc > 3))
    {
        s32Ret = PCIV_StopVencTrans(atoi(args[1]),atoi(args[2]), atoi(args[3]));
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    else
    {
        printf("vi t1 dev chn size: Start vi on [t1], size can be 'd1' or 'cif'\n");
        printf("pre t1 t2 num  : Create preview from borad [t1] to [t2]\n");
        printf("                 with [num] channel. [tx]=0 means host\n");
        printf("                 [num]=0 means do not preview this [t1]\n");
        printf("cvenc t1 t2 grp: Create VENC Group on [t1] send to [t2]. \n");
        printf("dvenc t1 t2 grp: Destroy the VENC Group from [t1] to [t2] \n");
    }
    
}

#define PCIV_CHIP_NUM 4
int main(int argc, char *argv[])
{
    HI_S32 s32Ret, i, j, k, s32ChipNum = 1;
    pthread_t           stThread[PCIV_CHIP_NUM];
    PCIV_THREAD_PARAM_S stThreadParam[PCIV_CHIP_NUM];
    if(argc >= 2)
    {
        s32ChipNum = atoi(argv[1]);
    }

    if(s32ChipNum<1 || (s32ChipNum+1) > PCIV_CHIP_NUM)
    {
        s32ChipNum = 1;
    }
    printf("Salve chip number is %d \n", s32ChipNum);
    
    /*set AD tw2815*/
    s32Ret = SAMPLE_2815_open();
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
	s32Ret = SAMPLE_2815_set_2d1(); 
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    /*mpp sys init*/
    s32Ret = PCIV_Host_SysInit();
    HI_ASSERT((HI_SUCCESS == s32Ret));

    PCIV_ViConfigDefault();

    for(i=0; i<6; i++)
    {
        g_u32VoChnNum[i]   = 0;
        g_u32ViChnNum[i]   = 0;
        g_u32VencChnNum[i] = 0;
        g_u32VencRevNum[i] = 0;
        g_u32PfWinBase[i]  = 0;

        for(j=0; j<VENC_MAX_CHN_NUM; j++)
        {
            g_stPortMapVenc[i][j].stLocalDev.s32PciDev = -1;
            g_stPortMapVenc[i][j].stLocalDev.s32PciDev = -1;
            g_stPortMapVenc[i][j].s32RmtTgt = -1;
        }

        for(j=0; j<VO_MAX_CHN_NUM; j++)
        {
            g_stPortMapVo[i][j].stLocalDev.s32PciDev = -1;
            g_stPortMapVo[i][j].stLocalDev.s32PciDev = -1;
            g_stPortMapVo[i][j].s32RmtTgt = -1;
        }
        
    }
    
    /* Open the message port. We only use slave 1 and 2 now. */
    for(i=1; i<=s32ChipNum; i++)
    {   
        PCIV_BASEWINDOW_S stBase;
        s32Ret = PCIV_OpenMsgPort(i);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        stBase.s32SlotIndex = i;
        s32Ret = HI_MPI_PCIV_GetBaseWindow(&stBase);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        g_u32PfWinBase[i] = stBase.u32PfWinBase;
        printf("Slot %d PF window base is 0x%08x\n", i, g_u32PfWinBase[i]);

        stThreadParam[i].s32DstTarget = i;
        stThreadParam[i].s32LocalId   = HI_MPI_PCIV_GetLocalId();
        s32Ret = pthread_create(&stThread[i], NULL, PCIV_ThreadRev, &stThreadParam[i]);
        HI_ASSERT((HI_SUCCESS == s32Ret));
    }

    PCIV_InitBuffer();
    
    stThreadParam[0].s32DstTarget = 0;
    stThreadParam[0].s32LocalId   = HI_MPI_PCIV_GetLocalId();
    s32Ret = pthread_create(&stThread[0], NULL, PCIV_ThreadSend, &stThreadParam[0]);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    
    for(;;)
    {
        HI_CHAR *ptr;
        char     cbuf[256];
        
        printf("> ");
        ptr = fgets(cbuf, 255, stdin);

        memset(args, 0, sizeof(args));
        argc = adjust_str(ptr);
        if(0 == strcmp("q", args[0]) || 0 == strcmp("Q", args[0]))
        {
            break;
        }
        
        parsearg(argc);
    }

    for(i=0; i<PCIV_CHIP_NUM; i++)
    {
        for(j=0; j<PCIV_CHIP_NUM; j++)
        {
            if(i == j) continue;
            
            PCIV_DoPreview(i, j, 0, 2);

            for(k=0; k<VENC_MAX_GRP_NUM; k++)
            {
                if(g_stPortMapVenc[i][k*2].s32RmtTgt == -1) continue;
                PCIV_StopVencTrans(i, j, k);
            }
        }
    }
    
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    
    return 0;
}

