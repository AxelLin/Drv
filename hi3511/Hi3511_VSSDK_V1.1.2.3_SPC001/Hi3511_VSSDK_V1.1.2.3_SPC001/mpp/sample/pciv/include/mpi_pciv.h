/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_pciv.h
  Version       : Initial Draft
  Author        : Hisilicon Hi3511 MPP Team
  Created       : 2008/06/18
  Last Modified :
  Description   : mpi functions declaration
  Function List :
  History       :
  1.Date        : 2008/06/18
    Author      : z50929
    Modification: Create
******************************************************************************/
#ifndef __MPI_PCIV_H__
#define __MPI_PCIV_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_pciv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

HI_S32 HI_MPI_PCIV_Create(PCIV_DEVICE_S *pDevice);
HI_S32 HI_MPI_PCIV_Destroy(PCIV_DEVICE_S *pDevice);
HI_S32 HI_MPI_PCIV_SetAttr(PCIV_DEVICE_ATTR_S *pPcivAttr);
HI_S32 HI_MPI_PCIV_GetAttr(PCIV_DEVICE_ATTR_S *pPcivAttr);
HI_S32 HI_MPI_PCIV_Start(PCIV_DEVICE_S *pDevice);
HI_S32 HI_MPI_PCIV_Stop(PCIV_DEVICE_S *pDevice);
HI_S32 HI_MPI_PCIV_Bind(PCIV_BIND_S *pBind);
HI_S32 HI_MPI_PCIV_UnBind(PCIV_BIND_S *pBind);
HI_S32 HI_MPI_PCIV_EnableDeflicker(HI_VOID);
HI_S32 HI_MPI_PCIV_DisableDeflicker(HI_VOID);

/* Create a series of dma task */
HI_S32  HI_MPI_PCIV_DmaTask(PCIV_DMA_TASK_S *pTask);


/*
** in pDevAttr: 
** input:  stPCIDevice,stPciAttr, u32Count 
** output: pPhyAddrArray 
*/
HI_S32  HI_MPI_PCIV_Malloc(PCIV_DEVICE_ATTR_S *pDevAttr);
HI_S32  HI_MPI_PCIV_Free(PCIV_DEVICE_S *pDevice);

/* 
** On the host, you can get all the slave borad's NP,PF and CFG window 
** On the slave, you can get the PF AHB Addres only.
** Input : pBase->s32SlotIndex
** Output: On host  pBase->u32NpWinBase,pBase->u32PfWinBase,pBase->u32CfgWinBase
**         On Slave pBase->u32PfAHBAddr
*/
HI_S32  HI_MPI_PCIV_GetBaseWindow(PCIV_BASEWINDOW_S *pBase);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __MPI_VENC_H__ */

