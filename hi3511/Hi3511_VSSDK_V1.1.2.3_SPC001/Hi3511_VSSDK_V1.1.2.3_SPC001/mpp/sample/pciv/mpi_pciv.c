/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_pciv.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/18
  Last Modified :
  Description   : hisilicon MPI venc
  Function List :
  History       :
  1.Date        : 2008/06/18
    Author      : z50825 
    Modification: Created file
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <stdlib.h>    
#include "hi_type.h"
#include "hi_common.h"
#include "hi_debug.h"
#include "hi_comm_video.h"
#include "hi_comm_pciv.h"
#include "hi_comm_group.h"
#include "mkp_pciv.h"

#ifdef __cplusplus
#if __cplusplus
{
#endif
#endif 

static HI_S32 g_s32PcivFd = -1;

#define CHECK_PCIV_OPEN()\
do{\
    if(g_s32PcivFd<= 0)\
    {\
        char name[128];\
        memset(name,0,sizeof(name));\
        sprintf(name,"/dev/misc/pciv");\
        g_s32PcivFd = open(name, O_RDONLY);\
        if(g_s32PcivFd <= 0)\
        {\
            printf("open PCIV err\n");\
            return HI_ERR_PCIV_SYS_NOTREADY;\
        }\
    }\
  }while(0)
  
#define CHECK_NULL_PTR(ptr)\
do{\
    if(NULL == ptr )\
        return HI_ERR_PCIV_NULL_PTR;\
}while(0)

HI_S32 HI_MPI_PCIV_Create(PCIV_DEVICE_S *pDevice)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pDevice);
    return ioctl(g_s32PcivFd, PCIV_CREATE_DEVICE_CTRL, pDevice);
}

HI_S32 HI_MPI_PCIV_Destroy(PCIV_DEVICE_S *pDevice)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pDevice);
    return ioctl(g_s32PcivFd, PCIV_DESTROY_DEVICE_CTRL, pDevice);
}


HI_S32 HI_MPI_PCIV_SetAttr(PCIV_DEVICE_ATTR_S *pPcivAttr)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pPcivAttr);
    return ioctl(g_s32PcivFd, PCIV_SETATTR_CTRL, pPcivAttr);
}

HI_S32 HI_MPI_PCIV_GetAttr(PCIV_DEVICE_ATTR_S *pPcivAttr)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pPcivAttr);

    return ioctl(g_s32PcivFd,PCIV_GETATTR_CTRL,pPcivAttr);
}

HI_S32 HI_MPI_PCIV_Start(PCIV_DEVICE_S *pDevice)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pDevice);
    return ioctl(g_s32PcivFd, PCIV_START_CTRL, pDevice);
}

HI_S32 HI_MPI_PCIV_Stop(PCIV_DEVICE_S *pDevice)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pDevice);
    return ioctl(g_s32PcivFd, PCIV_STOP_CTRL, pDevice);
}

HI_S32 HI_MPI_PCIV_Bind(PCIV_BIND_S *pBind)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pBind);
    return ioctl(g_s32PcivFd, PCIV_BIND_CTRL, pBind);
}

HI_S32 HI_MPI_PCIV_UnBind(PCIV_BIND_S *pBind)
{
    CHECK_PCIV_OPEN();
    CHECK_NULL_PTR(pBind);
    return ioctl(g_s32PcivFd, PCIV_UNBIND_CTRL, pBind);
}

HI_S32 HI_MPI_PCIV_EnableDeflicker(HI_VOID)
{
    CHECK_PCIV_OPEN();
    
    return ioctl(g_s32PcivFd, PCIV_ENABLE_DEFLICKER_CTRL, HI_TRUE);
}

HI_S32 HI_MPI_PCIV_DisableDeflicker(HI_VOID)
{
    CHECK_PCIV_OPEN();
    
    return ioctl(g_s32PcivFd, PCIV_ENABLE_DEFLICKER_CTRL, HI_FALSE);
}

/*pci view host function*/
HI_S32  HI_MPI_PCIV_Malloc(PCIV_DEVICE_ATTR_S *pDevAttr)
{
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pDevAttr);
	
	return ioctl(g_s32PcivFd,PCIV_MALLOC_CTRL, pDevAttr);
}

HI_S32  HI_MPI_PCIV_Free(PCIV_DEVICE_S *pDevice)
{
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pDevice);
	
	return ioctl(g_s32PcivFd, PCIV_FREE_CTRL, pDevice);
}

HI_S32  HI_MPI_PCIV_GetBaseWindow(PCIV_BASEWINDOW_S *pBase)
{
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pBase);
	
	return ioctl(g_s32PcivFd, PCIV_GETBASEWINDOW_CTRL, pBase);
}

HI_S32  HI_MPI_PCIV_DmaTask(PCIV_DMA_TASK_S *pTask)
{
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pTask);
	
	return ioctl(g_s32PcivFd, PCIV_DMATASK_CTRL, pTask);
}

HI_S32  HI_MPI_PCIV_GetLocalId()
{
    HI_S32 s32LocalId = -1;
	CHECK_PCIV_OPEN();
	
	(void)ioctl(g_s32PcivFd, PCIV_GETLOCALID_CTRL, &s32LocalId);

	return s32LocalId;
}


/* ------------------------------- old interface -------------------*/  
#ifdef PCIV_USE_OLD_INTERFACE
HI_S32  HI_MPI_PCIV_SLAVE_SetDstAddr(VI_DEV ViDev,VI_CHN ViChn,HI_U32 *pPhyAddrArray,HI_U32 u32Count)
{
	PCIV_DEVICE_ATTR_S stPcivAttr;
	HI_S32 s32Ret;
	
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pPhyAddrArray);

    stPcivAttr.stPCIDevice.s32PciDev = 0;
    stPcivAttr.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
    s32Ret = ioctl(g_s32PcivFd,PCIV_GETATTR_CTRL,&stPcivAttr);
	if(HI_SUCCESS != s32Ret)
	{
	    return s32Ret;
	}
	
	stPcivAttr.cAddrArray = pPhyAddrArray;
	stPcivAttr.u32Count      = u32Count;

	return HI_MPI_PCIV_SetAttr(&stPcivAttr);
}

HI_S32  HI_MPI_PCIV_SLAVE_SetDstPicAttr(VI_DEV ViDev,VI_CHN ViChn,const PCIV_PIC_ATTR_S *pstPicAttr)
{
	PCIV_DEVICE_ATTR_S stPcivAttr;
	HI_S32 s32Ret;
	
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pstPicAttr);

    stPcivAttr.stPCIDevice.s32PciDev = 0;
    stPcivAttr.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
    s32Ret = HI_MPI_PCIV_GetAttr(&stPcivAttr);
	if(HI_SUCCESS != s32Ret)
	{
	    return s32Ret;
	}

	memcpy(&stPcivAttr.stPicAttr,pstPicAttr,sizeof(PCIV_PIC_ATTR_S));

	return HI_MPI_PCIV_SetAttr(&stPcivAttr);
}

HI_S32  HI_MPI_PCIV_SLAVE_GetDstPicAttr(VI_DEV ViDev,VI_CHN ViChn,PCIV_PIC_ATTR_S *pstPicAttr)
{
	PCIV_DEVICE_ATTR_S stPcivAttr;
	HI_S32 s32Ret;
	
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pstPicAttr);

    stPcivAttr.stPCIDevice.s32PciDev = 0;
    stPcivAttr.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
    s32Ret = HI_MPI_PCIV_GetAttr(&stPcivAttr);
	if(HI_SUCCESS == s32Ret)
	{
		memcpy(pstPicAttr,&stPcivAttr.stPicAttr,sizeof(PCIV_PIC_ATTR_S));
	}

	return s32Ret;
}

HI_S32  HI_MPI_PCIV_SLAVE_Start(VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_DEVICE_S stDevice;
	PCIV_BIND_S   stBind;
	HI_S32        s32Ret;
	
	CHECK_PCIV_OPEN();
	
	stDevice.s32PciDev = 0;
    stDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);

    memcpy(&stBind.stPCIDevice, &stDevice, sizeof(stBind));
    stBind.stBindObj.enType = PCIV_BIND_VI;
    stBind.stBindObj.unAttachObj.viDevice.viDevId = ViDev;
    stBind.stBindObj.unAttachObj.viDevice.viChn   = ViChn;
    s32Ret = HI_MPI_PCIV_Bind(&stBind);
    if(HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
	return HI_MPI_PCIV_Start(&stDevice);
}

HI_S32  HI_MPI_PCIV_SLAVE_Stop(VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_DEVICE_S stDevice;
	
	CHECK_PCIV_OPEN();
	
	stDevice.s32PciDev = 0;
    stDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
	
	return HI_MPI_PCIV_Stop(&stDevice);
}

/*pci view host function*/
HI_S32  HI_MPI_PCIV_HOST_AllocDstAddr(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn,const PCIV_PIC_ATTR_S *pstPicAttr
										,HI_U32 u32Count,HI_U32 *pPhyAddrArray)
{
	PCIV_DEVICE_ATTR_S stPcivAttr;
	
	CHECK_PCIV_OPEN();
	CHECK_NULL_PTR(pstPicAttr);

	stPcivAttr.stPCIDevice.s32PciDev = s32PciDev;
	stPcivAttr.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
	memcpy(stPcivAttr.cAddrArray, pPhyAddrArray, u32Count*sizeof(HI_U32));
	stPcivAttr.u32Count 			= u32Count;
	memcpy(&stPcivAttr.stPicAttr, pstPicAttr, sizeof(PCIV_PIC_ATTR_S));
	
	return HI_MPI_PCIV_Malloc(&stPcivAttr);
}

HI_S32  HI_MPI_PCIV_HOST_FreeDstAddr(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_DEVICE_S stDevice;
	
	CHECK_PCIV_OPEN();
	
	stDevice.s32PciDev = 0;
    stDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
	
	return HI_MPI_PCIV_Free(&stDevice);
}

HI_S32  HI_MPI_PCIV_HOST_BindVo(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn,VO_CHN VoChn)
{
	PCIV_BIND_S stHostBind;
	
	CHECK_PCIV_OPEN();
	stHostBind.stPCIDevice.s32PciDev = 0;
    stHostBind.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
    stHostBind.stBindObj.enType      = PCIV_BIND_VO;
	stHostBind.stBindObj.unAttachObj.voDevice.voChn = VoChn;
	
	return HI_MPI_PCIV_Bind(&stHostBind);
}

HI_S32  HI_MPI_PCIV_HOST_UnBindVo(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_BIND_S stHostBind;
	
	CHECK_PCIV_OPEN();
	stHostBind.stPCIDevice.s32PciDev = 0;
    stHostBind.stPCIDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
    stHostBind.stBindObj.enType      = PCIV_BIND_VO;
	stHostBind.stBindObj.unAttachObj.voDevice.voChn = -1;
	
	return HI_MPI_PCIV_UnBind(&stHostBind);
}

HI_S32  HI_MPI_PCIV_HOST_Start(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_DEVICE_S stDevice;
	
	CHECK_PCIV_OPEN();
	
	stDevice.s32PciDev = 0;
    stDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
	
	return HI_MPI_PCIV_Start(&stDevice);
}

HI_S32  HI_MPI_PCIV_HOST_Stop(HI_S32 s32PciDev,VI_DEV ViDev,VI_CHN ViChn)
{
	PCIV_DEVICE_S stDevice;
	
	CHECK_PCIV_OPEN();
	
	stDevice.s32PciDev = 0;
    stDevice.s32Port   = PCIV_VIU2PORT(ViDev, ViChn);
	
	return HI_MPI_PCIV_Stop(&stDevice);
}
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


