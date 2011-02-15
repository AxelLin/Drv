/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv.h
  Version       : Initial Draft
  Author        : Hisilicon Hi3511 MPP Team
  Created       : 2008/06/18
  Last Modified :
  Description   : mpi functions declaration
  Function List :
  History       :
  1.Date        : 2008/06/18
    Author      : z50825
    Modification: Create
******************************************************************************/
#ifndef __PCIV_H__
#define __PCIV_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <linux/list.h>

#include "hi_type.h"
#include "hi_common.h"
#include "hi_comm_vi.h"
#include "hi_comm_vdec.h"
#include "hi_comm_pciv.h"
#include "vpp_ext.h"


#define MAKE_DWORD(high,low) (((low)&0x0000ffff)|((high)<<16))
#define HIGH_WORD(x) (((x)&0xffff0000)>>16)
#define LOW_WORD(x) ((x)&0x0000ffff)

#define PCIV_TRACE(level, fmt...)\
do{ \
        HI_TRACE(level, HI_ID_PCIV,"[Func]:%s [Line]:%d [Info]:", __FUNCTION__, __LINE__);\
        HI_TRACE(level,HI_ID_PCIV,##fmt);\
}while(0)


#define FUNC_ENTRANCE(type,id) ((type*)(g_astModules[id].pstExportFuncs))

#define CHECK_FUNC_ENTRANCE(id)\
do{\
    if(NULL == (g_astModules[id].pstExportFuncs) )\
        return HI_ERR_PCIV_SYS_NOTREADY;\
}while(0)


#define CHECK_NULL_PTR(ptr)\
do{\
    if(NULL == ptr)\
     {\
        return HI_ERR_PCIV_NULL_PTR;\
     }\
  }while(0)

#define PCIV_CHECK_PCIDEV(PciDev)\
do{\
    if(((PciDev) < 0) || ((PciDev) >= PCIV_MAX_DEV_NUM))\
        return HI_ERR_PCIV_ILLEGAL_PARAM;\
}while(0)

#define PCIV_CHECK_PORT(PciPort)\
do{\
    if(((PciPort) < 0) || ((PciPort) >= PCIV_MAX_PORT_NUM))\
        return HI_ERR_PCIV_INVALID_CHNID;\
}while(0)

#define PCIV_CHECK_FIELD(u32Field)\
do{\
    if((VIDEO_FIELD_INTERLACED != u32Field) && (VIDEO_FIELD_FRAME != u32Field))\
    {\
        PCIV_TRACE(HI_DBG_ERR,\
            "arg u32Field invalid,must be VIDEO_FIELD_INTERLACED or VIDEO_FIELD_FRAME\n");\
        return HI_ERR_PCIV_ILLEGAL_PARAM;\
    }\
}while(0)

#define SEQ_AFTER(unknown,known) ( (HI_S32)((HI_S32)(known) - (HI_S32)(unknown)) < (HI_S32)0)
#define SEQ_NEWER(unknown,known) (SEQ_AFTER(unknown,known) ? (unknown) : (known))



typedef struct hiPCIV_CHANNEL_S
{
    PCIV_BIND_OBJ_S stBindObj;
    PCIV_PIC_ATTR_S stPicAttr;
    
    /* caculate the following three fileds when alloc memory of pic. */
    HI_U32         u32BufSize;
    HI_U32         u32OffsetU;
    HI_U32         u32OffsetV;    
    HI_U32         au32PhyAddr[PCIV_MAX_BUF_NUM]; /* AHB addr*/
    HI_U32         u32PoolId[PCIV_MAX_BUF_NUM]; /* needed by the memory owner */
    HI_VOID       *pVirtAddr[PCIV_MAX_BUF_NUM]; /* needed by the memory owner */
    VB_BLKHANDLE   vbBlkHdl[PCIV_MAX_BUF_NUM];  /* needed by the memory owner */
    HI_BOOL        bBufFree[PCIV_MAX_BUF_NUM];  /* Used by sender. */
    HI_U32         u32BufUseCnt[PCIV_MAX_BUF_NUM];  /* Used by sender. */
    
    HI_U32         u32Count;     /* The total buffer count */

    /* Set OSD when translate by PCI */
    VPP_SOVERLAY_S stSftOsdReg[MAX_OVERLAY_NUM];  /* OSD region attribute */
    HI_U32         u32OsdRegNum;                  /* OSD region numbers */

    HI_BOOL  bMalloc;
    HI_BOOL  bCreate;
    HI_BOOL  bConfigured;
    HI_BOOL  bStart;

    HI_U32 u32RecvCnt; /* count of pic from bind obj(PCI,VIU or VDEC)  */
    HI_U32 u32SendCnt; /* count of pic send to bind obj(PCI or VOU)    */
    HI_U32 u32RespCnt; /* count of pic being transfer to host successfully .*/

    PCIV_REMOTE_OBJ_S stRemoteObj;  /* The remote target board and PCI device */

    struct timer_list stBufTimer;
}PCIV_CHANNEL_S;

#define PCIV_VBBLK_NUM 4
typedef struct hiPCIV_RXBUF_S
{
    VB_POOL aVbPool[PCIV_VBBLK_NUM];
    HI_U32  au32BufSize[PCIV_VBBLK_NUM];
    HI_U32  u32PoolCnt;
    HI_BOOL bCreate;
} PCIV_RXBUF_S;


typedef struct hiPCI_TASK_S
{
    struct list_head    list;    /* internal data, don't touch */

    HI_BOOL bRead;
	HI_U32  u32SrcPhyAddr;
	HI_U32  u32DstPhyAddr;
	HI_U32  u32Len;
    HI_U32  privateData;
    HI_U32  resv;
    HI_U32  resv1;
    HI_U32  resv2;
    void   (*pCallBack)(struct hiPCI_TASK_S *pstTask);
} SEND_TASK_S;

typedef HI_S32 PCIV_PCITRANS_FUNC(SEND_TASK_S *pTask);
typedef HI_S32 PCIV_DMAEND_FUNC(PCIV_REMOTE_OBJ_S *pRemoteDev, HI_S32 s32Index, HI_U32 u32Count);
typedef HI_S32 PCIV_BUFFREE_FUNC(PCIV_REMOTE_OBJ_S *pRemoteDev, HI_S32 s32Index, HI_U32 u32Count);

HI_S32  PCIV_Init(void);
HI_VOID PCIV_Exit(void);

HI_S32  PCIV_Create(PCIV_DEVICE_S *pDevice);
HI_S32  PCIV_Destroy(PCIV_DEVICE_S *pDevice);

HI_S32  PCIV_SetAttr(PCIV_DEVICE_ATTR_S *pAttr);
HI_S32  PCIV_GetAttr(PCIV_DEVICE_ATTR_S *pAttr);

HI_S32  PCIV_Start(PCIV_DEVICE_S *pDevice);
HI_S32  PCIV_Stop(PCIV_DEVICE_S *pDevice);

HI_S32  PCIV_EnableDiflicker(HI_BOOL bDiflicker);

HI_S32  PCIV_Malloc(PCIV_DEVICE_ATTR_S *pAttr);
HI_S32  PCIV_Free(PCIV_DEVICE_S *pDevice);

HI_S32  PCIV_Bind(PCIV_BIND_S *pBindObj);
HI_S32  PCIV_UnBind(PCIV_BIND_S *pBindObj);

HI_S32 PCIV_DmaTask(PCIV_DMA_TASK_S *pTask);

HI_VOID PCIV_RxBufFreeNotify(PCIV_DEVICE_S *pPciDev, HI_U32 u32Index, HI_U32 u32Count);
HI_VOID PcivRxSend2VO(PCIV_DEVICE_S *pPciDev, HI_U32 u32Index, HI_U32 u32Count);
HI_VOID  PCIV_RegisterFunction(HI_U32 u32LocalId,
            PCIV_PCITRANS_FUNC *pPciSlaveSend, PCIV_DMAEND_FUNC *pSendMsg,
            PCIV_BUFFREE_FUNC *pBufFree);

HI_VOID  PCIV_SetBaseWindow(PCIV_BASEWINDOW_S *pBase);
HI_VOID  PCIV_SetLocalId(HI_S32 s32LocalId);

HI_S32  PCIV_TxSendPic(PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pPicInfo);
HI_S32  PCIV_ViuSendPic(VI_DEV viDev, VI_CHN viChn, const VIDEO_FRAME_INFO_S *pPicInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */



#endif
