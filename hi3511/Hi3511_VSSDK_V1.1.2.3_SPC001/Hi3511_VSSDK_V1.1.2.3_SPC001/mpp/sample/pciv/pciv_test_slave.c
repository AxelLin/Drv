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
#include <sys/mman.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_pciv.h"
#include "tw2815.h"

#include "hi_comm_video.h"
#include "pci_trans.h"
#include "pciv_test_comm.h"
#include "pciv_msg.h"
#include "pciv_stream.h"

#define TW2815_A_FILE	"/dev/misc/tw2815adev"
#define TW2815_B_FILE	"/dev/misc/tw2815bdev"

int g_fd2815a = -1;
int g_fd2815b = -1;

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

void PCIV_Test_Slave_StopVi(VI_DEV viDev, VI_CHN viChn)
{
	HI_MPI_VI_DisableChn(viDev, viChn);
}

HI_S32 PCIV_Test_Slave_StartVi(VI_DEV viDev, VI_CHN viChn)
{
    HI_S32        s32Ret;
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;

    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));
    
	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode  = VI_WORK_MODE_2D1;
	ViAttr.u32AdType   = AD_2815;
	ViAttr.enViNorm    = VIDEO_ENCODING_MODE_PAL;
    
    ViChnAttr.stCapRect.u32Width  = 704;
	ViChnAttr.stCapRect.u32Height = 288;	
	ViChnAttr.stCapRect.s32X  = 8;
	ViChnAttr.stCapRect.s32Y  = 0;
    ViChnAttr.enCapSel        = VI_CAPSEL_BOTTOM;
	ViChnAttr.bDownScale      = HI_TRUE;
	ViChnAttr.bChromaResample = HI_FALSE;
	ViChnAttr.enViPixFormat   = PCIV_TEST_VI_PIXEL_FORMAT;

    /* VI device may be enabled, so we don't care the result */
	HI_MPI_VI_SetPubAttr(viDev, &ViAttr);
    HI_MPI_VI_Enable(viDev);
    
    s32Ret = HI_MPI_VI_SetChnAttr(viDev, viChn, &ViChnAttr);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    
    s32Ret = HI_MPI_VI_EnableChn(viDev, viChn);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    
	return 0;		

}


/*send h264 nalu to vdec pthread*/
#define MAX_READ_LEN 1024
HI_BOOL g_bVdecRun  = HI_FALSE;
HI_S32  g_fileIndex = 0;
void* proc_sendh264stream(void* p)
{
    const VDEC_CHN vdecChn = (VDEC_CHN)p;
	VDEC_STREAM_S  stStream;
    FILE*  file = NULL;
    HI_S32 s32Ret;
	char   buf[MAX_READ_LEN];

	/*open the stream file*/
	sprintf(buf, "stream%d.h264", g_fileIndex++);
    file = fopen(buf, "r");
    if (!file)
    {
        printf("vdec %d open file %s err\n", vdecChn, buf);
		return NULL;
    }
	
    while(g_bVdecRun)
    {
		s32Ret = fread(buf, 1, MAX_READ_LEN, file);
		if(s32Ret <=0)
		{
		    fseek(file, 0, SEEK_SET);
		    continue;
		}

        stStream.pu8Addr = buf;
        stStream.u32Len  = s32Ret;
        stStream.u64PTS  = 0;

		s32Ret = HI_MPI_VDEC_SendStream(vdecChn, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32Ret)
        {
			printf("send to vdec error 0x%x \n", s32Ret);
            break;
        }
    }
    
    fclose(file);
    return NULL;
}

HI_S32 PCIV_Slave_StopVdec(VDEC_CHN vdecChn)
{
    HI_S32 s32Ret;
    g_bVdecRun = HI_FALSE;

    HI_MPI_VDEC_StopRecvStream(vdecChn);
    HI_MPI_VDEC_DestroyChn(vdecChn);
    return s32Ret;
}

HI_S32 PCIV_Slave_StartVdec(VDEC_CHN vdecChn)
{
    HI_S32 s32Ret;
    VDEC_ATTR_H264_S stH264Attr;
    VDEC_CHN_ATTR_S  stAttr;
    pthread_t        thread;

    stH264Attr.u32Priority    = 0;
    stH264Attr.u32PicHeight   = 576;
    stH264Attr.u32PicWidth    = 720;
    stH264Attr.enMode         = H264D_MODE_STREAM;
    stH264Attr.u32RefFrameNum = 2;

    stAttr.enType     = PT_H264;
    stAttr.u32BufSize = (((stH264Attr.u32PicWidth) * (stH264Attr.u32PicHeight))<<1);
    stAttr.pValue     = (void*)&stH264Attr;

    s32Ret = HI_MPI_VDEC_CreateChn(vdecChn, &stAttr, NULL);
    HI_ASSERT(HI_SUCCESS == s32Ret);
	
    s32Ret = HI_MPI_VDEC_StartRecvStream(vdecChn);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    g_bVdecRun = HI_TRUE;
    pthread_create(&thread, NULL, proc_sendh264stream, (void *)vdecChn);

    return s32Ret;
}

static HI_U32 g_u32PfAhbBase = 0;

HI_S32 PCIV_Slave_SysInit()
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
    stVbConf.astCommPool[0].u32BlkCnt  = 20;
    stVbConf.astCommPool[1].u32BlkSize = 221184;/* 384*288*1.5*/
    stVbConf.astCommPool[1].u32BlkCnt  = 60;    
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

HI_S32 PCIV_Slave_StartVenc(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32   s32Ret;
    VENC_CHN vencChn;
    VENC_CHN_ATTR_S     *pAttr    = NULL;
    PCIV_VENCCMD_CREATE_S *pCmd     = (PCIV_VENCCMD_CREATE_S *)pMsg->cMsgBody;
    PCIV_VENCCMD_ECHO_S   *pCmdEcho = (PCIV_VENCCMD_ECHO_S   *)pMsg->cMsgBody;

    /* If start the local vi then just do it */
    pAttr         = &(pCmd->stAttr);
    pAttr->pValue = pCmd + 1;

    vencChn = pCmd->vencChn;
    s32Ret  = HI_MPI_VENC_CreateChn(vencChn, pAttr, NULL);
    HI_ASSERT(HI_SUCCESS == s32Ret); 

    /* register to group */
    s32Ret = HI_MPI_VENC_RegisterChn(pCmd->vencGrp,vencChn);
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    
    s32Ret = HI_MPI_VENC_StartRecvPic(vencChn);/*main chn start rev */
    HI_ASSERT(HI_SUCCESS == s32Ret);

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VENCCHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VENCCMD_ECHO_S);
    pCmdEcho->vencChn = vencChn;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StopVenc(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32   s32Ret;
    VENC_CHN vencChn;
    VENC_CHN_ATTR_S     *pAttr    = NULL;
    PCIV_VENCCMD_DESTROY_S *pCmd     = (PCIV_VENCCMD_DESTROY_S *)pMsg->cMsgBody;
    PCIV_VENCCMD_ECHO_S    *pCmdEcho = (PCIV_VENCCMD_ECHO_S   *)pMsg->cMsgBody;

    vencChn = pCmd->vencChn;
    s32Ret = HI_MPI_VENC_StopRecvPic(vencChn);/*main chn start rev */
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    
    s32Ret = HI_MPI_VENC_UnRegisterChn(vencChn);/*register to group 0 */
    HI_ASSERT(HI_SUCCESS == s32Ret); 
    
    s32Ret  = HI_MPI_VENC_DestroyChn(vencChn);
    HI_ASSERT(HI_SUCCESS == s32Ret); 

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VENCCHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VENCCMD_ECHO_S);
    pCmdEcho->vencChn = vencChn;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}


HI_S32 PCIV_Slave_StartVi(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret;
    VI_DEV viDev;
    VI_CHN viChn;
    PCIV_VICMD_CREATE_S *pCmd     = (PCIV_VICMD_CREATE_S *)pMsg->cMsgBody;
    PCIV_VICMD_ECHO_S   *pCmdEcho = (PCIV_VICMD_ECHO_S   *)pMsg->cMsgBody;

    /* Start the VI first */
    viDev = pCmd->viDev;
    viChn = pCmd->viChn;
    s32Ret = HI_MPI_VI_SetChnAttr(viDev, viChn, &pCmd->stAttr);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    s32Ret = HI_MPI_VI_EnableChn(viDev, viChn);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VICHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VICMD_ECHO_S);
    pCmdEcho->viDev   = viDev;
    pCmdEcho->viChn   = viChn;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StopVi(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret;
    VI_DEV viDev;
    VI_CHN viChn;
    PCIV_VICMD_DESTROY_S *pCmd     = (PCIV_VICMD_DESTROY_S *)pMsg->cMsgBody;
    PCIV_VICMD_ECHO_S    *pCmdEcho = (PCIV_VICMD_ECHO_S   *)pMsg->cMsgBody;
#if 1
    /* Start the VI first */
    viDev = pCmd->viDev;
    viChn = pCmd->viChn;
    s32Ret = HI_MPI_VI_DisableChn(viDev, viChn);
    HI_ASSERT((HI_SUCCESS == s32Ret));
#endif
    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VICHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VICMD_ECHO_S);
    pCmdEcho->viDev   = viDev;
    pCmdEcho->viChn   = viChn;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StartGrp(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32   s32Ret;
    VENC_GRP vencGrp;
    PCIV_GRPCMD_CREATE_S *pCmd     = (PCIV_GRPCMD_CREATE_S *)pMsg->cMsgBody;
    PCIV_GRPCMD_ECHO_S   *pCmdEcho = (PCIV_GRPCMD_ECHO_S   *)pMsg->cMsgBody;

    /* Start the VI first */
    vencGrp = pCmd->vencGrp;
    /* Create new group first */
    s32Ret = HI_MPI_VENC_CreateGroup(vencGrp);
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    s32Ret = HI_MPI_VENC_BindInput(vencGrp, pCmd->viDev, pCmd->viChn);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_GROUP;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_GRPCMD_ECHO_S);
    pCmdEcho->vencGrp = vencGrp;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StopGrp(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32   s32Ret;
    VENC_GRP vencGrp;
    PCIV_GRPCMD_DESTROY_S *pCmd     = (PCIV_GRPCMD_DESTROY_S *)pMsg->cMsgBody;
    PCIV_GRPCMD_ECHO_S    *pCmdEcho = (PCIV_GRPCMD_ECHO_S   *)pMsg->cMsgBody;

    /* Start the VI first */
    vencGrp = pCmd->vencGrp;
    s32Ret = HI_MPI_VENC_UnbindInput(vencGrp);
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    /* Create new group first */
    s32Ret = HI_MPI_VENC_DestroyGroup(vencGrp);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_GROUP;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_GRPCMD_ECHO_S);
    pCmdEcho->vencGrp = vencGrp;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}


HI_S32 PCIV_Slave_StartVo(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret, s32CurPos;
    VO_CHN voChn = -1;
    PCIV_VOCMD_CREATE_S *pCmd     = (PCIV_VOCMD_CREATE_S *)pMsg->cMsgBody;
    PCIV_VOCMD_ECHO_S   *pCmdEcho = (PCIV_VOCMD_ECHO_S   *)pMsg->cMsgBody;

    s32CurPos = 0;
    while(s32CurPos < pMsg->u32MsgLen)
    {
        voChn  = pCmd->voChn;
        s32Ret = HI_MPI_VO_SetChnAttr(voChn, &pCmd->stAttr);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        s32Ret = HI_MPI_VO_EnableChn(voChn);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        pCmd++;
        s32CurPos += sizeof(*pCmd);
    }
    
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VOCHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VOCMD_ECHO_S);
    pCmdEcho->s32Echo = HI_SUCCESS;
    pCmdEcho->voChn   = voChn;
    
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT(HI_FAILURE != s32Ret); 
    
    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StopVo(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret, s32CurPos;
    VO_CHN voChn = -1;
    PCIV_VOCMD_DESTROY_S *pCmd     = (PCIV_VOCMD_DESTROY_S *)pMsg->cMsgBody;
    PCIV_VOCMD_ECHO_S    *pCmdEcho = (PCIV_VOCMD_ECHO_S   *)pMsg->cMsgBody;

    s32CurPos = 0;
    while(s32CurPos < pMsg->u32MsgLen)
    {
        voChn  = pCmd->voChn;
        s32Ret = HI_MPI_VO_DisableChn(voChn);
        HI_ASSERT(HI_SUCCESS == s32Ret); 

        pCmd++;
        s32CurPos += sizeof(*pCmd);
    }

    /* Echo the cmd */
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_VOCHN;
    pMsg->u32Target   = 0;
    pMsg->u32MsgLen   = sizeof(PCIV_VICMD_ECHO_S);
    pCmdEcho->voChn   = voChn;
    pCmdEcho->s32Echo = HI_SUCCESS;
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_SetPcivAttr(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret;
    PCIV_PCIVCMD_ATTR_S *pCmd     = (PCIV_PCIVCMD_ATTR_S *)pMsg->cMsgBody;
    PCIV_PCIVCMD_ECHO_S *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)pMsg->cMsgBody;

    s32Ret = HI_MPI_PCIV_SetAttr(&pCmd->stDevAttr);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    s32Ret = PCIV_InitReceiver(&pCmd->stDevAttr, PCIV_DefaultRecFun);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd. Attation: the memory may be overlay. */
    memmove(&pCmdEcho->stDevAttr, &pCmd->stDevAttr, sizeof(PCIV_DEVICE_ATTR_S));
    pCmdEcho->s32Echo = HI_SUCCESS;
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_PCIV;
    pMsg->u32Target   = 0; /* To host */
    pMsg->u32MsgLen   = sizeof(PCIV_PCIVCMD_ECHO_S);
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_GetPcivAttr(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret;
    PCIV_PCIVCMD_ATTR_S *pCmd     = (PCIV_PCIVCMD_ATTR_S *)pMsg->cMsgBody;
    PCIV_PCIVCMD_ECHO_S *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)pMsg->cMsgBody;

    s32Ret = HI_MPI_PCIV_GetAttr(&pCmd->stDevAttr);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd. Attation: the memory may be overlay. */
    memmove(&pCmdEcho->stDevAttr, &pCmd->stDevAttr, sizeof(PCIV_DEVICE_ATTR_S));
    pCmdEcho->s32Echo = HI_SUCCESS;
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_PCIV;
    pMsg->u32Target   = 0; /* To host */
    pMsg->u32MsgLen   = sizeof(PCIV_PCIVCMD_ECHO_S);
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_CreatePciv(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret, i;
    PCIV_PCIVCMD_CREATE_S *pCmd     = (PCIV_PCIVCMD_CREATE_S *)pMsg->cMsgBody;
    PCIV_PCIVCMD_ECHO_S   *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)pMsg->cMsgBody;
    PCIV_DEVICE_S         *pPcivDev = NULL;

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

        /* Attation: return the offset from PCI shm_phys_addr */
        for(i=0; i<pCmd->stDevAttr.u32Count; i++)
        {
            pCmd->stDevAttr.u32AddrArray[i] -= g_u32PfAhbBase;
        }
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

    /* Echo the cmd. Attation: the memory may be overlay. */
    memmove(&pCmdEcho->stDevAttr, &pCmd->stDevAttr, sizeof(PCIV_DEVICE_ATTR_S));
    pCmdEcho->s32Echo = HI_SUCCESS;
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_PCIV;
    pMsg->u32Target   = 0; /* To host */
    pMsg->u32MsgLen   = sizeof(PCIV_PCIVCMD_ECHO_S);
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

HI_S32 PCIV_Slave_StartPciv(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    PCIV_PCIVCMD_START_S *pCmd     = (PCIV_PCIVCMD_START_S *)pMsg->cMsgBody;
    PCIV_PCIVCMD_ECHO_S  *pCmdEcho = (PCIV_PCIVCMD_ECHO_S  *)pMsg->cMsgBody;
    PCIV_DEVICE_S        *pPcivDev = NULL;

    pPcivDev = &(pCmd->stPciDev);
    s32Ret  = HI_MPI_PCIV_Start(pPcivDev);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd. Attation: the memory may be overlay. */
    pCmdEcho->s32Echo = HI_SUCCESS;
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_PCIV;
    pMsg->u32Target   = 0; /* To host */
    pMsg->u32MsgLen   = sizeof(PCIV_PCIVCMD_ECHO_S);
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}


HI_S32 PCIV_Slave_StopPciv(PCIV_MSGHEAD_S *pMsg)
{
    HI_S32 s32Ret = HI_SUCCESS;
    PCIV_PCIVCMD_DESTROY_S *pCmd     = (PCIV_PCIVCMD_DESTROY_S *)pMsg->cMsgBody;
    PCIV_PCIVCMD_ECHO_S    *pCmdEcho = (PCIV_PCIVCMD_ECHO_S   *)pMsg->cMsgBody;
    PCIV_DEVICE_S         *pPcivDev = NULL;

    pPcivDev = &(pCmd->stPciDev);
    s32Ret  = HI_MPI_PCIV_Stop(pPcivDev);
    s32Ret |= HI_MPI_PCIV_Free(pPcivDev);
    s32Ret |= HI_MPI_PCIV_Destroy(pPcivDev);
    s32Ret |= PCIV_DeInitReceiver(pPcivDev);
    s32Ret |= PCIV_DeInitCreater(pPcivDev);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Echo the cmd. Attation: the memory may be overlay. */
    pCmdEcho->s32Echo = HI_SUCCESS;
    pMsg->enMsgType   = PCIV_MSGTYPE_CMDECHO;
    pMsg->enDevType   = PCIV_DEVTYPE_PCIV;
    pMsg->u32Target   = 0; /* To host */
    pMsg->u32MsgLen   = sizeof(PCIV_PCIVCMD_ECHO_S);
    s32Ret = PCIV_SendMsg(0, PCIV_MSGPORT_USERCMD, pMsg);
    HI_ASSERT((HI_FAILURE != s32Ret));

    return HI_SUCCESS;
}

int main()
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S stMsg;
    pthread_t           stThread[2];
    PCIV_THREAD_PARAM_S stThreadParam[2];

    /* set AD tw2815 */
    s32Ret = SAMPLE_2815_open();
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
	s32Ret = SAMPLE_2815_set_2d1(); 
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    /* mpp sys init */
    s32Ret = PCIV_Slave_SysInit();
    HI_ASSERT((HI_SUCCESS == s32Ret));

    /* Open the message port. On slave board only 0 can be used. */
    s32Ret = PCIV_OpenMsgPort(0);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    {
        PCIV_BASEWINDOW_S stbase;
        stbase.s32SlotIndex = 0;
        s32Ret = HI_MPI_PCIV_GetBaseWindow(&stbase);
        HI_ASSERT((HI_SUCCESS == s32Ret));
        g_u32PfAhbBase = stbase.u32PfAHBAddr;
        printf("AHB PF Address is 0x%08x\n", g_u32PfAhbBase);
    }   

    PCIV_InitBuffer();

    stThreadParam[0].s32DstTarget = 0;
    stThreadParam[0].s32LocalId   = HI_MPI_PCIV_GetLocalId();
    s32Ret = pthread_create(&stThread[0], NULL, PCIV_ThreadRev, &stThreadParam[0]);
    HI_ASSERT((HI_SUCCESS == s32Ret));

    stThreadParam[1].s32DstTarget = 0;
    stThreadParam[1].s32LocalId   = HI_MPI_PCIV_GetLocalId();
    s32Ret = pthread_create(&stThread[1], NULL, PCIV_ThreadSend, &stThreadParam[1]);
    HI_ASSERT((HI_SUCCESS == s32Ret));
    
    while(1)
    {
        s32Ret = PCIV_ReadMsg(0, PCIV_MSGPORT_USERCMD, &stMsg);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        switch(stMsg.enMsgType)
        {
        case PCIV_MSGTYPE_CREATE:
        {
            switch(stMsg.enDevType)
            {
                case PCIV_DEVTYPE_VICHN:
                {
                    s32Ret = PCIV_Slave_StartVi(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_VOCHN:
                {
                    s32Ret = PCIV_Slave_StartVo(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_PCIV:
                {
                    s32Ret = PCIV_Slave_CreatePciv(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_GROUP:
                {
                    s32Ret = PCIV_Slave_StartGrp(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_VENCCHN:
                {
                    s32Ret = PCIV_Slave_StartVenc(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PCIV_MSGTYPE_START:
        {
            switch(stMsg.enDevType)
            {
                case PCIV_DEVTYPE_PCIV:
                {
                    s32Ret = PCIV_Slave_StartPciv(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PCIV_MSGTYPE_DESTROY:
        {
            switch(stMsg.enDevType)
            {
                case PCIV_DEVTYPE_VICHN:
                {
                    s32Ret = PCIV_Slave_StopVi(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_VOCHN:
                {
                    s32Ret = PCIV_Slave_StopVo(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_PCIV:
                {
                    s32Ret = PCIV_Slave_StopPciv(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_GROUP:
                {
                    s32Ret = PCIV_Slave_StopGrp(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                case PCIV_DEVTYPE_VENCCHN:
                {
                    s32Ret = PCIV_Slave_StopVenc(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PCIV_MSGTYPE_SETATTR:
        {
            switch(stMsg.enDevType)
            {
                case PCIV_DEVTYPE_PCIV:
                {
                    s32Ret = PCIV_Slave_SetPcivAttr(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PCIV_MSGTYPE_GETATTR:
        {
            switch(stMsg.enDevType)
            {
                case PCIV_DEVTYPE_PCIV:
                {
                    s32Ret = PCIV_Slave_GetPcivAttr(&stMsg);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
        }
        
    }
    return 0; 
}

