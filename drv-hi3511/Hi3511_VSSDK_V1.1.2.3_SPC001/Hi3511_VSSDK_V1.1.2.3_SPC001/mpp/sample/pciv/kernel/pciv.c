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
  2.Date        : 2008/11/20
    Author      : z44949
    Modification: Modify to support VDEC 
    
******************************************************************************/
#include <asm/uaccess.h>                /*copy_from_user, copy_to_user*/
#include <linux/module.h>
#include <linux/kernel.h>

#include "mod_ext.h"
#include "hi3511_board.h"
#include "vb_ext.h"
#include "dsu_ext.h"
#include "vou_ext.h"
#include "vdec_ext.h"
#include "pciv.h"

#define PCIV_RESERVE_POOL_SIZE 1024*1024
#define PCIV_VB_POOL1_SIZE  704*576*2
#define PCIV_VB_POOL2_SIZE  384*288*2
#define PCIV_VB_POOL3_SIZE  ((384*288*3)>>1)
#define PCIV_VB_POOL4_SIZE  ((192*144*3)>>1)
#define PCIV_VB_POOL1_COUNT 5
#define PCIV_VB_POOL2_COUNT 32
#define PCIV_VB_POOL3_COUNT 16
#define PCIV_VB_POOL4_COUNT 16

static spinlock_t g_PcivLock;

#define PCIV_SPIN_LOCK     spin_lock_irqsave(&g_PcivLock,flags)
#define PCIV_SPIN_UNLOCK   spin_unlock_irqrestore(&g_PcivLock,flags)

PCIV_CHANNEL_S g_stPcivChn[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];
static PCIV_DEVICE_S  g_stPortMap[PCIV_BIND_DEVNUM][PCIV_MAX_PORT_NUM];

static PCIV_PCITRANS_FUNC *g_pPciDmaRW         = NULL;
static PCIV_DMAEND_FUNC   *g_pPciDmaEndNotify  = NULL;
static PCIV_BUFFREE_FUNC  *g_pPciBufFreeNotify = NULL;
static HI_U32              g_u32LocalId  = -1;
static wait_queue_head_t   g_stwqDmaDone;
static HI_BOOL g_bDmaDone = HI_TRUE;

static HI_BOOL s_bDiflicker = HI_TRUE;
static struct timer_list s_stPcivTimer;

HI_VOID  PCIV_RxSend2VO(HI_S32 s32PciDev);
HI_VOID  PciTxISR(SEND_TASK_S *pstTask);
HI_VOID  PCIV_VdecTimerFunc(unsigned long data);
HI_VOID  PCIV_BufTimerFunc(unsigned long data);

HI_S32 PcivFmt2Addr(const PCIV_PIC_ATTR_S *pstPicAttr, HI_U32 *pu32OffsetU, 
    HI_U32 *pu32OffsetV, HI_U32 *pu32BufSize)
{
    HI_U32 u32Height, u32Stride;

    u32Height = pstPicAttr->u32Height;
    u32Stride = pstPicAttr->u32Stride;
    switch(pstPicAttr->enPixelFormat)
    {
        case PIXEL_FORMAT_YUV_PLANAR_420:
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
            *pu32OffsetU = u32Height * u32Stride;
            *pu32OffsetV = *pu32OffsetU + (u32Height * u32Stride >> 2);
            *pu32BufSize = *pu32OffsetV + (u32Height * u32Stride >> 2);
            break;
            
        case PIXEL_FORMAT_YUV_A422:
        case PIXEL_FORMAT_YUV_PLANAR_422:
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:  
            *pu32OffsetU = u32Height * u32Stride;
            *pu32OffsetV = *pu32OffsetU + (u32Height * u32Stride >> 1);
            *pu32BufSize = *pu32OffsetV + (u32Height * u32Stride >> 1);
            break;
        case PIXEL_FORMAT_UYVY_PACKAGE_422:
        case PIXEL_FORMAT_YUYV_PACKAGE_422:    
            *pu32OffsetU = u32Height * u32Stride;
            *pu32BufSize = *pu32OffsetU;
            break;    
        default:
            PCIV_TRACE(HI_DBG_ERR, "pixel format <%x> is illegal !\n", pstPicAttr->enPixelFormat);
            return -1;
    }
    return HI_SUCCESS;

}

static volatile unsigned int g_Addr = 0;

HI_S32   PCIV_Init(void)
{
    HI_S32 i, j;
    
    memset(g_stPcivChn,0,sizeof(g_stPcivChn));
    spin_lock_init(&g_PcivLock);
    init_waitqueue_head(&g_stwqDmaDone);

    for(i = 0; i < PCIV_MAX_DEV_NUM; i++)
    {
        for (j = 0; j < PCIV_MAX_PORT_NUM; j++)
        {
            g_stPcivChn[i][j].stBindObj.enType = PCIV_BIND_BUTT;
            g_stPcivChn[i][j].stRemoteObj.stTargetDev.s32PciDev = -1;
            g_stPcivChn[i][j].stRemoteObj.stTargetDev.s32Port   = -1;
            g_stPcivChn[i][j].stRemoteObj.u32MsgTargetId        = -1;
            init_timer(&g_stPcivChn[i][j].stBufTimer);
        }
    }

    for(i = 0; i < PCIV_BIND_DEVNUM; i++)
    {
        for (j = 0; j < PCIV_MAX_PORT_NUM; j++)
        {
            g_stPortMap[i][j].s32PciDev = -1;
            g_stPortMap[i][j].s32Port   = -1;
        }
    }

    if(NULL == (g_astModules[HI_ID_SYS].pstExportFuncs) ||
       NULL == (g_astModules[HI_ID_DSU].pstExportFuncs) )
    {
        PCIV_TRACE(HI_DBG_ERR, "PCIV needs SYS and DSU!\n");
        return HI_ERR_PCIV_SYS_NOTREADY;
    }    

	init_timer(&s_stPcivTimer);
    s_stPcivTimer.expires  = jiffies + 4;
    s_stPcivTimer.function = PCIV_VdecTimerFunc;
    s_stPcivTimer.data     = 0;
    add_timer(&s_stPcivTimer);
   
    PCIV_TRACE(HI_DBG_INFO,"PCIV Init Successfully!\n");
    return HI_SUCCESS;
}

HI_VOID  PCIV_Exit(void)
{
    HI_S32 i, j;

	del_timer(&s_stPcivTimer);
    for(i = 0; i < PCIV_MAX_DEV_NUM; i++)
    {
        for (j = 0; j < PCIV_MAX_PORT_NUM; j++)
        {
            del_timer(&g_stPcivChn[i][j].stBufTimer);
        }
    }
    PCIV_TRACE(HI_DBG_INFO, "PCIV Exit Successfully!\n");
}

HI_VOID  PCIV_RegisterFunction(HI_U32 u32LocalId, PCIV_PCITRANS_FUNC *pPciDmaSend,
                       PCIV_DMAEND_FUNC *pSendMsg, PCIV_BUFFREE_FUNC *pBufFree)
{
    g_pPciDmaRW        = pPciDmaSend;
    g_pPciDmaEndNotify = pSendMsg;
    g_pPciBufFreeNotify= pBufFree;
    g_u32LocalId       = u32LocalId;
}
EXPORT_SYMBOL(PCIV_RegisterFunction);

HI_S32  PCIV_Create(PCIV_DEVICE_S *pDevice)
{
    PCIV_CHANNEL_S *pstPcivChn;
    HI_S32 s32Index;
    CHECK_NULL_PTR(pDevice);
    PCIV_CHECK_PCIDEV(pDevice->s32PciDev);
    
    if( (pDevice->s32Port != -1) && (pDevice->s32Port < PCIV_MAX_PORT_NUM))
    {
        pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][pDevice->s32Port];
        if(HI_TRUE == pstPcivChn->bCreate)
        {
            PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_EXIST!\n");
            return HI_ERR_PCIV_EXIST;
        }
    }
    else
    {
        s32Index = PCIV_MAX_PORT_NUM;
        while(s32Index--)
        {
            pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][s32Index];
            if(HI_TRUE != pstPcivChn->bCreate)
            {
                pDevice->s32Port = s32Index;
                break;
            }
        }
    }
    
    if( (pDevice->s32Port != -1) && (pDevice->s32Port < PCIV_MAX_PORT_NUM))
    {
        pstPcivChn->bCreate     = HI_TRUE;
        pstPcivChn->bStart      = HI_FALSE;
        pstPcivChn->bConfigured = HI_FALSE;
        pstPcivChn->bMalloc     = HI_FALSE;
        pstPcivChn->u32SendCnt  = 0;
        pstPcivChn->u32RecvCnt  = 0;
        pstPcivChn->u32RespCnt  = 0;
        pstPcivChn->stBindObj.enType = PCIV_BIND_BUTT;
        return HI_SUCCESS;
    }
    
    PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_SYS_NOFREEPORT!\n");
    return HI_ERR_PCIV_BUSY;
}

HI_S32  PCIV_Destroy(PCIV_DEVICE_S *pDevice)
{
    PCIV_CHANNEL_S *pstPcivChn;
    PCIV_CHECK_PCIDEV(pDevice->s32PciDev);
    PCIV_CHECK_PORT(pDevice->s32Port);

    pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][pDevice->s32Port];
    pstPcivChn->bCreate     = HI_FALSE;
    pstPcivChn->bStart      = HI_FALSE;
    pstPcivChn->bConfigured = HI_FALSE;
    pstPcivChn->bMalloc     = HI_FALSE;
    pstPcivChn->stBindObj.enType = PCIV_BIND_BUTT;

    return HI_SUCCESS;
}

/* mpi slave used function */
HI_S32  PCIV_SetAttr(PCIV_DEVICE_ATTR_S *pAttr)
{
    HI_S32          i;
    PCIV_CHANNEL_S *pstPcivChn;
    
    CHECK_NULL_PTR(pAttr);
    PCIV_CHECK_PCIDEV(pAttr->stPCIDevice.s32PciDev);
    PCIV_CHECK_PORT(pAttr->stPCIDevice.s32Port);

    if(pAttr->u32Count > PCIV_MAX_BUF_NUM)
    {
        PCIV_TRACE(HI_DBG_ERR,"Count too large %d > PCIV_MAX_BUF_NUM!\n", pAttr->u32Count);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    
    pstPcivChn = &g_stPcivChn[pAttr->stPCIDevice.s32PciDev][pAttr->stPCIDevice.s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR, "HI_ERR_PCIV_UNEXIST!\n");
        return HI_ERR_PCIV_UNEXIST;
    }

    memcpy(&pstPcivChn->stRemoteObj, &(pAttr->stRemoteObj), sizeof(PCIV_REMOTE_OBJ_S));
    
    if(pstPcivChn->bStart != HI_FALSE)
    {
        PCIV_TRACE(HI_DBG_ERR, "Only Remote object can be setted!\n");
        return HI_SUCCESS;
    }
    
    if (PcivFmt2Addr(&(pAttr->stPicAttr), &pstPcivChn->u32OffsetU, 
                  &pstPcivChn->u32OffsetV, &pstPcivChn->u32BufSize))
    {
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    
    memcpy(&pstPcivChn->stPicAttr,&(pAttr->stPicAttr),sizeof(PCIV_PIC_ATTR_S));
    
    if(pAttr->u32Count != 0)
    {
        memcpy(pstPcivChn->au32PhyAddr, pAttr->u32AddrArray, 
                          pAttr->u32Count*sizeof(HI_U32));
                          
        pstPcivChn->u32Count = pAttr->u32Count;
        for(i=0; i<pAttr->u32Count; i++)
        {
            pstPcivChn->bBufFree[i]     = HI_TRUE;
            pstPcivChn->u32BufUseCnt[i] = 0;
        }
    }

    pstPcivChn->bConfigured = HI_TRUE;
    return HI_SUCCESS;
}

HI_S32  PCIV_GetAttr(PCIV_DEVICE_ATTR_S *pAttr)
{
    PCIV_CHANNEL_S *pstPcivChn;
    HI_S32 i;
    
    CHECK_NULL_PTR(pAttr);
    PCIV_CHECK_PCIDEV(pAttr->stPCIDevice.s32PciDev);
    PCIV_CHECK_PORT(pAttr->stPCIDevice.s32Port);
    
    pstPcivChn = &g_stPcivChn[pAttr->stPCIDevice.s32PciDev][pAttr->stPCIDevice.s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_UNEXIST!\n");
        return HI_ERR_PCIV_UNEXIST;
    }

    if(HI_TRUE != pstPcivChn->bConfigured)
    {
        PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_NOT_PERM!\n");
        return HI_ERR_PCIV_NOT_PERM;
    }
    
    memcpy(&(pAttr->stRemoteObj), &pstPcivChn->stRemoteObj, sizeof(PCIV_REMOTE_OBJ_S));
    memcpy(&(pAttr->stPicAttr), &pstPcivChn->stPicAttr,sizeof(PCIV_PIC_ATTR_S));
    pAttr->u32Count = pstPcivChn->u32Count;
    memcpy(pAttr->u32AddrArray, pstPcivChn->au32PhyAddr, pAttr->u32Count * sizeof(HI_U32));
    for(i=0; i<pAttr->u32Count; i++)
    {
        pAttr->u32BlkSize[i] = pstPcivChn->u32BufSize;
    }
        
    return HI_SUCCESS;
}

HI_S32  PCIV_Start(PCIV_DEVICE_S *pDevice)
{
    PCIV_CHANNEL_S *pstPcivChn;
    
    CHECK_NULL_PTR(pDevice);
    PCIV_CHECK_PCIDEV(pDevice->s32PciDev);
    PCIV_CHECK_PORT(pDevice->s32Port);

    pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][pDevice->s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_UNEXIST!\n");
        return HI_ERR_PCIV_UNEXIST;
    }

    if(pstPcivChn->bConfigured != HI_TRUE )
    {
        PCIV_TRACE(HI_DBG_ERR, "Attr or Addr not configurated!\n");
        return HI_ERR_PCIV_NOT_CONFIG;
    }
    
    pstPcivChn->bStart = HI_TRUE;
    
    return HI_SUCCESS;
}

HI_S32  PCIV_Stop(PCIV_DEVICE_S *pDevice)
{
    PCIV_CHANNEL_S *pstPcivChn;
    
    CHECK_NULL_PTR(pDevice);
    PCIV_CHECK_PCIDEV(pDevice->s32PciDev);
    PCIV_CHECK_PORT(pDevice->s32Port);

    pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][pDevice->s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_UNEXIST!\n");
        return HI_ERR_PCIV_UNEXIST;
    }

    pstPcivChn->bStart = HI_FALSE;
    
    return HI_SUCCESS;
}

HI_S32  PCIV_EnableDiflicker(HI_BOOL bDiflicker)
{
    s_bDiflicker = bDiflicker;
    return HI_SUCCESS;
}


static PCIV_RXBUF_S s_stRxBuf;

HI_S32 PcivCreateRxPool(HI_VOID )
{
    HI_S32 s32Ret;    
    HI_U32 u32PoolId, u32BlkNum = 0;

    if (HI_TRUE == s_stRxBuf.bCreate)
    {
        return HI_SUCCESS;
    }

    u32BlkNum = 0;

    /* D1 Preview pool */
    s32Ret = VB_CreatePoolExt(&u32PoolId, PCIV_VB_POOL1_COUNT, PCIV_VB_POOL1_SIZE, RESERVE_MMZ_NAME);
    if (s32Ret)
    {
        PCIV_TRACE(HI_DBG_ALERT, "create pool_1 fail\n");
        return s32Ret;
    }
    s_stRxBuf.aVbPool[u32BlkNum]     = u32PoolId;
    s_stRxBuf.au32BufSize[u32BlkNum] = PCIV_VB_POOL1_SIZE;
    u32BlkNum++;
    
    /* CIF Preview pool */
    s32Ret = VB_CreatePoolExt(&u32PoolId, PCIV_VB_POOL2_COUNT, PCIV_VB_POOL2_SIZE, RESERVE_MMZ_NAME);
    if (s32Ret)
    {
        PCIV_TRACE(HI_DBG_ALERT, "create pool_2 fail\n");
        return s32Ret;
    }
    s_stRxBuf.aVbPool[u32BlkNum]     = u32PoolId;
    s_stRxBuf.au32BufSize[u32BlkNum] = PCIV_VB_POOL2_SIZE;
    u32BlkNum++;

    /* CIF Stream pool */
    s32Ret = VB_CreatePoolExt(&u32PoolId, PCIV_VB_POOL3_COUNT, PCIV_VB_POOL3_SIZE, RESERVE_MMZ_NAME);
    if (s32Ret)
    {
        PCIV_TRACE(HI_DBG_ALERT, "create pool_2 fail\n");
        return s32Ret;
    }
    s_stRxBuf.aVbPool[u32BlkNum]     = u32PoolId;
    s_stRxBuf.au32BufSize[u32BlkNum] = PCIV_VB_POOL3_SIZE;
    u32BlkNum++;

    /* QCIF Stream pool */
    s32Ret = VB_CreatePoolExt(&u32PoolId, PCIV_VB_POOL4_COUNT, PCIV_VB_POOL4_SIZE, RESERVE_MMZ_NAME);
    if (s32Ret)
    {
        PCIV_TRACE(HI_DBG_ALERT, "create pool_2 fail\n");
        return s32Ret;
    }
    s_stRxBuf.aVbPool[u32BlkNum]     = u32PoolId;
    s_stRxBuf.au32BufSize[u32BlkNum] = PCIV_VB_POOL4_SIZE;
    u32BlkNum++;

    s_stRxBuf.u32PoolCnt = u32BlkNum;
    s_stRxBuf.bCreate = HI_TRUE;
    PCIV_TRACE(HI_DBG_INFO, "slave Rx creat pool ok\n");
    return HI_SUCCESS;
}

HI_S32 PcivAllocRxBuf(PCIV_CHANNEL_S *pstPcivChn, PCIV_DEVICE_ATTR_S *pAttr)
{
    HI_S32 s32Ret, i, j;
    VB_BLKHANDLE handle;   
    
    if ((s32Ret = PcivCreateRxPool()))/* only create onetime */
    {
        return s32Ret;
    }
    
    /* first get vb from CIF pool, then from D1 pool */        
    i = 0;
    for (j=s_stRxBuf.u32PoolCnt-1; j >= 0; j--)
    {
        if (s_stRxBuf.au32BufSize[j] >= pstPcivChn->u32BufSize)
        {
            for(;i < pAttr->u32Count; i++)
            {
                handle = VB_GetBlkByPoolId(s_stRxBuf.aVbPool[j], VB_UID_PCIV);
                if(VB_INVALID_HANDLE == handle)
                {
                    PCIV_TRACE(HI_DBG_ERR,"VB_GetBlkBySize Index %d fail,size:%d!\n", i, pstPcivChn->u32BufSize);
                    break;
                }
                pstPcivChn->au32PhyAddr[i] = VB_Handle2Phys(handle);
                pstPcivChn->pVirtAddr[i]   = (HI_VOID*)VB_Handle2Kern(handle);
                pstPcivChn->u32PoolId[i]   = VB_Handle2PoolId(handle);
                pstPcivChn->vbBlkHdl[i]    = handle;
                memset(pstPcivChn->pVirtAddr[i],0,pstPcivChn->u32BufSize);
            }
            if(i == pAttr->u32Count) break;
        }
    }  

    if(i < pAttr->u32Count)
    {
        for(j = i; j >= 0;j --)
        {
            VB_UserSub(pstPcivChn->u32PoolId[j], pstPcivChn->au32PhyAddr[j], VB_UID_PCIV);
        }
        
        return HI_ERR_PCIV_NOBUF;
    }
    
    pstPcivChn->u32Count = pAttr->u32Count;
    
    return HI_SUCCESS;
}

HI_S32 MasterAllocRxBuf(PCIV_CHANNEL_S *pstPcivChn, PCIV_DEVICE_ATTR_S *pAttr)
{
    HI_S32 s32Ret, i, j;
    
    for(i = 0;i < pAttr->u32Count; i++)
    {
        VB_BLKHANDLE handle;        
        handle = VB_GetBlkBySize(pstPcivChn->u32BufSize, VB_UID_PCIV);
        if(VB_INVALID_HANDLE == handle)
        {
            PCIV_TRACE(HI_DBG_ERR,"VB_GetBlkBySize fail,size:%d!\n",pstPcivChn->u32BufSize);
            s32Ret = HI_ERR_PCIV_NOBUF;
            goto FAILED;
        }        
        pstPcivChn->au32PhyAddr[i] = VB_Handle2Phys(handle);
        pstPcivChn->pVirtAddr[i]   = (HI_VOID*)VB_Handle2Kern(handle);
        pstPcivChn->u32PoolId[i]   = VB_Handle2PoolId(handle);
        pstPcivChn->vbBlkHdl[i]    = (handle);
        
        memset(pstPcivChn->pVirtAddr[i],0,pstPcivChn->u32BufSize);
    }
    pstPcivChn->u32Count = pAttr->u32Count;
    
    return HI_SUCCESS;
FAILED:
    for(j = i; j != 0;j --)
    {
        VB_UserSub(pstPcivChn->u32PoolId[j], pstPcivChn->au32PhyAddr[j], VB_UID_PCIV);
    }
    
    return s32Ret;
}


/*mpi host used function*/
HI_S32  PCIV_Malloc(PCIV_DEVICE_ATTR_S *pAttr)
{
    PCIV_CHANNEL_S *pstPcivChn;
    HI_S32 s32Ret, i;    
    
    CHECK_NULL_PTR(pAttr);
    PCIV_CHECK_PCIDEV(pAttr->stPCIDevice.s32PciDev);
    PCIV_CHECK_PORT(pAttr->stPCIDevice.s32Port);

    if(pAttr->u32Count > PCIV_MAX_BUF_NUM)
    {
        PCIV_TRACE(HI_DBG_ERR, "count is invalid,(0,%d)\n",PCIV_MAX_BUF_NUM);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
        
    pstPcivChn = &g_stPcivChn[pAttr->stPCIDevice.s32PciDev][pAttr->stPCIDevice.s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        PCIV_TRACE(HI_DBG_ERR,"HI_ERR_PCIV_UNEXIST!\n");
        return HI_ERR_PCIV_UNEXIST;
    }
    
    if(pstPcivChn->bStart)
    {
        PCIV_TRACE(HI_DBG_ERR,  "pciv have to stop!\n");
        return HI_ERR_PCIV_NOT_PERM;
    }

    if (PcivFmt2Addr(&(pAttr->stPicAttr), &pstPcivChn->u32OffsetU, 
            &pstPcivChn->u32OffsetV, &pstPcivChn->u32BufSize))
    {
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    memcpy(&pstPcivChn->stPicAttr, &(pAttr->stPicAttr),sizeof(PCIV_PIC_ATTR_S));

    if(g_u32LocalId == 0)
    {
        s32Ret = MasterAllocRxBuf(pstPcivChn, pAttr);
    }
    else
    {
        s32Ret = PcivAllocRxBuf(pstPcivChn, pAttr);
    }
    
    if (s32Ret)
    {
        PCIV_TRACE(HI_DBG_ERR,  "pciv malloc buffer failure!\n");
        return s32Ret;
    }

    memcpy(pAttr->u32AddrArray, pstPcivChn->au32PhyAddr, pAttr->u32Count * sizeof(HI_U32));
    for(i=0; i<pAttr->u32Count; i++)
    {
        pAttr->u32BlkSize[i]        = pstPcivChn->u32BufSize;
        pstPcivChn->bBufFree[i]     = HI_TRUE;
        pstPcivChn->u32BufUseCnt[i] = 0;
    }
                
    pstPcivChn->bConfigured = HI_TRUE;
    pstPcivChn->bMalloc     = HI_TRUE;
    return HI_SUCCESS;   
}

HI_S32  PCIV_Free(PCIV_DEVICE_S *pDevice)
{
    HI_U32 i;
    PCIV_CHANNEL_S *pstPcivChn;
    
    CHECK_NULL_PTR(pDevice);
    PCIV_CHECK_PCIDEV(pDevice->s32PciDev);
    PCIV_CHECK_PORT(pDevice->s32Port);
    
    pstPcivChn = &g_stPcivChn[pDevice->s32PciDev][pDevice->s32Port];
    if((HI_TRUE != pstPcivChn->bCreate) || (HI_TRUE != pstPcivChn->bConfigured))
    {
        return HI_ERR_PCIV_UNEXIST;
    }
    
    if(pstPcivChn->bStart)
    {
        return HI_ERR_PCIV_NOT_PERM;
    }

    if(pstPcivChn->bMalloc)
    {
        for(i = 0; i < pstPcivChn->u32Count; i++)
        {
            VB_UserSub(pstPcivChn->u32PoolId[i], pstPcivChn->au32PhyAddr[i], VB_UID_PCIV);
        }
    }

    pstPcivChn->bConfigured = HI_FALSE;
    pstPcivChn->bMalloc     = HI_FALSE;
    PCIV_TRACE(HI_DBG_INFO,"free dest address ok\n");
    
    return HI_SUCCESS;
}

HI_S32  PCIV_Bind(PCIV_BIND_S *pBindObj)
{
    PCIV_CHANNEL_S *pstPcivChn = NULL;
    PCIV_DEVICE_S  *pDevice    = NULL;
    HI_S32   s32Index;

    CHECK_NULL_PTR(pBindObj);
    PCIV_CHECK_PCIDEV(pBindObj->stPCIDevice.s32PciDev);
    PCIV_CHECK_PORT(pBindObj->stPCIDevice.s32Port);

    
    pstPcivChn = &g_stPcivChn[pBindObj->stPCIDevice.s32PciDev][pBindObj->stPCIDevice.s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        return HI_ERR_PCIV_UNEXIST;
    }
    
    if(HI_TRUE != pstPcivChn->bConfigured)
    {
        return HI_ERR_PCIV_NOT_CONFIG;
    }

    memcpy(&pstPcivChn->stBindObj, &(pBindObj->stBindObj), sizeof(PCIV_BIND_OBJ_S));

    switch ( pBindObj->stBindObj.enType )/* resource of pictrue */
    {
        case PCIV_BIND_VI:
        {
            VI_DEV viDev = pBindObj->stBindObj.unAttachObj.viDevice.viDevId;
            VI_CHN viChn = pBindObj->stBindObj.unAttachObj.viDevice.viChn;
            s32Index     = viDev*VIU_MAX_CHN_NUM_PER_DEV + viChn;

            pDevice = &g_stPortMap[PCIV_BIND_VI][s32Index];
            memcpy(pDevice, &pBindObj->stPCIDevice, sizeof(PCIV_DEVICE_S));
            break;
        }
        case PCIV_BIND_VDEC:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.vdecDevice.vdecChn;

            pDevice = &g_stPortMap[PCIV_BIND_VDEC][s32Index];
            memcpy(pDevice, &pBindObj->stPCIDevice, sizeof(PCIV_DEVICE_S));
            break;
        }
        case PCIV_BIND_VO:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.voDevice.voChn;

            pDevice = &g_stPortMap[PCIV_BIND_VO][s32Index];
            memcpy(pDevice, &pBindObj->stPCIDevice, sizeof(PCIV_DEVICE_S));
            break;
        }
        case PCIV_BIND_VSTREAMREV:
        {
            /* Nothing need to do */
            break;
        }
        case PCIV_BIND_VSTREAMSND:
        {
            /* Nothing need to do */
            break;
        }
        case PCIV_BIND_VENC:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.vencDevice.vencChn;

            pDevice = &g_stPortMap[PCIV_BIND_VENC][s32Index];
            memcpy(pDevice, &pBindObj->stPCIDevice, sizeof(PCIV_DEVICE_S));
            break;
        }
        default:
        {
            PCIV_TRACE(HI_DBG_WARN, "Unsupported picture soucre\n");
            return HI_ERR_PCIV_NOT_SURPPORT;
        }
    }

    if(pstPcivChn->stBindObj.enType == PCIV_BIND_VO)
    {
        VO_CHN voChn = pstPcivChn->stBindObj.unAttachObj.voDevice.voChn;
        /*clear frame buf in vo*/
        if(NULL != FUNC_ENTRANCE(VOU_EXPORT_FUNC_S, HI_ID_VOU)->pfnVouClearChnBuf)
        {
            if(voChn != HI_INVALID_CHN)
            {
                (void)FUNC_ENTRANCE(VOU_EXPORT_FUNC_S, HI_ID_VOU)->pfnVouClearChnBuf(voChn,HI_TRUE);
            }
        }
    }

    return HI_SUCCESS;
}

HI_S32  PCIV_UnBind(PCIV_BIND_S *pBindObj)
{
    PCIV_CHANNEL_S *pstPcivChn;
    HI_S32   s32Index;

    CHECK_NULL_PTR(pBindObj);
    PCIV_CHECK_PCIDEV(pBindObj->stPCIDevice.s32PciDev);
    PCIV_CHECK_PORT(pBindObj->stPCIDevice.s32Port);
    
    pstPcivChn = &g_stPcivChn[pBindObj->stPCIDevice.s32PciDev][pBindObj->stPCIDevice.s32Port];
    if(HI_TRUE != pstPcivChn->bCreate)
    {
        return HI_ERR_PCIV_UNEXIST;
    }

    switch ( pstPcivChn->stBindObj.enType )/* resource of pictrue */
    {
        case PCIV_BIND_VI:
        {
            VI_DEV viDev = pBindObj->stBindObj.unAttachObj.viDevice.viDevId;
            VI_CHN viChn = pBindObj->stBindObj.unAttachObj.viDevice.viChn;
            s32Index     = viDev*VIU_MAX_CHN_NUM_PER_DEV + viChn;

            g_stPortMap[PCIV_BIND_VI][s32Index].s32PciDev = -1;
            g_stPortMap[PCIV_BIND_VI][s32Index].s32Port   = -1;
            break;
        }
        case PCIV_BIND_VDEC:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.vdecDevice.vdecChn;

            g_stPortMap[PCIV_BIND_VDEC][s32Index].s32PciDev = -1;
            g_stPortMap[PCIV_BIND_VDEC][s32Index].s32Port   = -1;
            break;
        }
        case PCIV_BIND_VO:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.voDevice.voChn;

            g_stPortMap[PCIV_BIND_VO][s32Index].s32PciDev = -1;
            g_stPortMap[PCIV_BIND_VO][s32Index].s32Port   = -1;
            break;
        }
        case PCIV_BIND_VSTREAMREV:
        {
            /* Nothing need to do */
            break;
        }
        case PCIV_BIND_VSTREAMSND:
        {
            /* Nothing need to do */
            break;
        }
        case PCIV_BIND_VENC:
        {
            s32Index = pBindObj->stBindObj.unAttachObj.vencDevice.vencChn;

            g_stPortMap[PCIV_BIND_VENC][s32Index].s32PciDev = -1;
            g_stPortMap[PCIV_BIND_VENC][s32Index].s32Port   = -1;
            break;
        }
        default:
        {
            PCIV_TRACE(HI_DBG_WARN, "Unsupported picture soucre\n");
            return HI_ERR_PCIV_NOT_SURPPORT;
        }
    }

    pstPcivChn->stBindObj.enType = PCIV_BIND_BUTT;
    PCIV_TRACE(HI_DBG_INFO,"unbind vo ok\n");
    
    return HI_SUCCESS;
}

HI_VOID  PcivDmaDone(SEND_TASK_S *pstTask)
{
    HI_ASSERT((pstTask->privateData + 1) == pstTask->resv);
    
    g_bDmaDone = HI_TRUE;
    wake_up(&g_stwqDmaDone);
}

HI_S32 PCIV_DmaTask(PCIV_DMA_TASK_S *pTask)
{
    HI_S32 i, s32Ret = HI_SUCCESS;
    SEND_TASK_S stPciTask;

    if(HI_TRUE != g_bDmaDone)
    {
        return HI_ERR_PCIV_BUSY;
    }

    g_bDmaDone = HI_FALSE;
    
    for(i=0; i<pTask->u32Count; i++)
    {
        stPciTask.u32SrcPhyAddr = pTask->pBlock[i].u32SrcAddr;
        stPciTask.u32DstPhyAddr = pTask->pBlock[i].u32DstAddr;
        stPciTask.u32Len        = pTask->pBlock[i].u32BlkSize;
        stPciTask.bRead         = pTask->bRead;
        stPciTask.privateData   = i;
        stPciTask.resv          = pTask->u32Count;
        stPciTask.pCallBack     = NULL;

        /* If this is the last task node, we set the callback */
        if( (i+1) == pTask->u32Count)
            stPciTask.pCallBack = PcivDmaDone;
        if(NULL != g_pPciDmaRW)
        {
            if(HI_SUCCESS != g_pPciDmaRW(&stPciTask))
            {
                g_bDmaDone = HI_TRUE;
                s32Ret     = HI_ERR_PCIV_NOMEM;
                break;
            }
        }
    }
    
    wait_event(g_stwqDmaDone, (g_bDmaDone == HI_TRUE));

    return s32Ret;
}

static HI_S32 PcivNotifyFreeBuf(PCIV_CHANNEL_S *pstPcivChn)
{
    HI_U32 i, u32Count;
    HI_BOOL bHit = HI_FALSE;
    HI_S32  s32Ret;
    
    if(pstPcivChn->bStart != HI_TRUE)
    {
        return HI_FAILURE;
    }
    
    if(pstPcivChn->u32Count <= 2)
    {
        return HI_FAILURE;
    }

    u32Count = pstPcivChn->u32BufUseCnt[0];
    /* If buffer is free then notify the sender */
    for(i=0; i<pstPcivChn->u32Count; i++)
    {
        if(VB_InquireUserCnt(pstPcivChn->vbBlkHdl[i]) > 1)
        {
            continue;
        }
        
        if(g_pPciBufFreeNotify != NULL)
        {
            s32Ret = g_pPciBufFreeNotify(&pstPcivChn->stRemoteObj, i, u32Count);
            if(HI_SUCCESS == s32Ret)
            {
                bHit = HI_TRUE;
            }
        }
    }
    
    /* If  no buffer is free then start the timer to check later */
    if(bHit != HI_TRUE)
    {
        pstPcivChn->stBufTimer.expires  = jiffies + 1;
        pstPcivChn->stBufTimer.function = PCIV_BufTimerFunc;
        pstPcivChn->stBufTimer.data     = (unsigned long)pstPcivChn;
        add_timer(&pstPcivChn->stBufTimer);
    }
    
    return HI_SUCCESS;
}

static HI_S32 PcivGetFreeBuf(PCIV_CHANNEL_S *pstPcivChn, HI_U32 *pCur)
{
    HI_U32 i;
    for(i=0; i<pstPcivChn->u32Count; i++)
    {
        if(pstPcivChn->bBufFree[i])
        {
            *pCur = i;
            return HI_SUCCESS;
        }
    }
    return HI_FAILURE;
}

static HI_VOID PcivDsuCallback(DSU_TASK_DATA_S *pstDsuTask)
{
    PCIV_CHANNEL_S *pstPcivChn;
    SEND_TASK_S     stPciTask;
    HI_S32 s32Ret, s32PciDev, s32Port;
    HI_U32 u32Cur;

    HI_ASSERT(NULL != pstDsuTask);
    
    s32Ret = VB_UserSub(pstDsuTask->stImgIn.u32PoolId, 
        pstDsuTask->stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    s32PciDev = (HI_S32)(HIGH_WORD(pstDsuTask->privateData));
    s32Port   = (HI_S32)(LOW_WORD(pstDsuTask->privateData));

    if(   (s32PciDev < 0) || (s32PciDev >= PCIV_MAX_DEV_NUM)
       || (s32Port < 0) || (s32Port >= PCIV_MAX_PORT_NUM))
        return ;
    
    pstPcivChn = &g_stPcivChn[s32PciDev][s32Port];    

    /* If data is from vdec, then the release process is needed */
    if(PCIV_BIND_VDEC == pstPcivChn->stBindObj.enType)
    {
        VDEC_CHN                   vdecChn;
        VDEC_DATA_S                stVdecData;
        FN_VDEC_KernelReleaseData *pfnVdecReleaseData = NULL;

        vdecChn = pstPcivChn->stBindObj.unAttachObj.vdecDevice.vdecChn;
        
        if(NULL != g_astModules[HI_ID_VDEC].pstExportFuncs)
        {
            pfnVdecReleaseData = FUNC_ENTRANCE(VDEC_EXPORT_FUNC_S, HI_ID_VDEC)->pfnVdecKernelReleaseData;
            if( (NULL != pfnVdecReleaseData) )
            {
                stVdecData.stUserData.bValid  = HI_FALSE;
                stVdecData.stWaterMark.bValid = HI_FALSE;
                /* Only picture frame need to be released */
                stVdecData.stFrameInfo.bValid = HI_TRUE; 
                memcpy(&stVdecData.stFrameInfo.stVideoFrameInfo, 
                       &pstDsuTask->stImgIn, sizeof(VIDEO_FRAME_INFO_S));
                pfnVdecReleaseData(vdecChn, &stVdecData);
            }
        }
    }

    s32Ret = HI_FAILURE;
    do
    {    
        if(NULL == g_pPciDmaRW) break;

        /* Only when the total buffer number is larger than two, we do sync */
        if(pstPcivChn->u32Count <= 2)
        {
            u32Cur = (pstPcivChn->u32SendCnt) % pstPcivChn->u32Count;
        }
        else
        {
            s32Ret = PcivGetFreeBuf(pstPcivChn, &u32Cur);
            if(HI_SUCCESS != s32Ret)
            {
                printk("[%d,%d]No free buffer\n", s32PciDev, s32Port);
                break;
            }
        }

        stPciTask.u32SrcPhyAddr = pstDsuTask->stImgOut.stVFrame.u32PhyAddr[0];
        stPciTask.u32DstPhyAddr = pstPcivChn->au32PhyAddr[u32Cur];
        stPciTask.u32Len        = pstPcivChn->u32BufSize;
        stPciTask.bRead         = HI_FALSE;
        stPciTask.privateData   = MAKE_DWORD(s32PciDev, s32Port);
        stPciTask.resv          = pstDsuTask->stImgOut.u32PoolId;
        stPciTask.resv1         = u32Cur;
        stPciTask.resv2         = pstPcivChn->u32SendCnt + 1;
        stPciTask.pCallBack     = PciTxISR;
        s32Ret = g_pPciDmaRW(&stPciTask);
        if(HI_SUCCESS == s32Ret)
        {
            pstPcivChn->bBufFree[u32Cur] = HI_FALSE;
            pstPcivChn->u32SendCnt++;
            pstPcivChn->u32BufUseCnt[u32Cur] = pstPcivChn->u32SendCnt;
        }
    }while(0); 
    
    if(HI_SUCCESS != s32Ret)
    {
        s32Ret = VB_UserSub(pstDsuTask->stImgOut.u32PoolId,
            pstDsuTask->stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

}

static DSU_SCALE_FILTER_E PcivGetDsuFilterHConef(HI_U32 u32InArgs,HI_U32 u32OutArgs)
{
	HI_U32 div,scale;

	HI_ASSERT(u32OutArgs!=0);

	div = 704/u32OutArgs;	
	scale = u32InArgs/u32OutArgs;
	
	if(scale<1)
	{
		return DSU_SCALE_FILTER_DEFAULT;
	}
	else if(div<=2)
	{
		return DSU_SCALE_FILTER_5M;
	}
	else if(div<=3)
	{
		return DSU_SCALE_FILTER_325M;
	}
	else if(div<=4)
	{
		return DSU_SCALE_FILTER_3M;
	}
	else
	{
		return DSU_SCALE_FILTER_3M;
	}
	return 0;
}

static DSU_SCALE_FILTER_E PcivGetDsuFilterVlConef(HI_U32 u32InArgs,HI_U32 u32OutArgs)
{
	HI_U32 div,scale;

	HI_ASSERT(u32OutArgs!=0);

	div = 576/u32OutArgs;	
	scale = u32InArgs/u32OutArgs;

	if(scale<1)
	{
		return DSU_SCALE_FILTER_DEFAULT;
	}
	if(div==1)
	{
		return DSU_SCALE_FILTER_DEFAULT;
	}	
	else if(div<=2)
	{
		if(scale<2)
		{
			return DSU_SCALE_FILTER_4M;
		}
		else
		{
			return DSU_SCALE_FILTER_375M;
		}
	}
	else if(div<=3)
	{
		if(scale<2)
		{
			return DSU_SCALE_FILTER_325M;
		}
		else
		{
			return DSU_SCALE_FILTER_3M;
		}
	}
	else if(div<=4)
	{
		return DSU_SCALE_FILTER_3M;
	}
	else
	{
		return DSU_SCALE_FILTER_3M;
	}
	
	return 0;
}

static DSU_SCALE_FILTER_E PcivGetDsuFilterVcConef(HI_U32 u32InArgs,HI_U32 u32OutArgs)
{
	HI_U32 div,scale;

	HI_ASSERT(u32OutArgs!=0);

	div = 576/u32OutArgs;	
	scale = u32InArgs/u32OutArgs;
	
	if(scale<1)
	{
		return DSU_SCALE_FILTER_DEFAULT;
	}	

	if(div==1)
	{
		return DSU_SCALE_FILTER_DEFAULT;
	}		
	else if(div<=2)
	{
		return DSU_SCALE_FILTER_4M;
	}
	else if(div<=3)
	{
		if(scale<2)
		{
			return DSU_SCALE_FILTER_325M;
		}
		else
		{
			return DSU_SCALE_FILTER_3M;
		}
	}
	else if(div<=4)
	{
		return DSU_SCALE_FILTER_3M;
	}
	else
	{
		return DSU_SCALE_FILTER_3M;
	}
	
	return 0;
}


/* be called in VIU interrupt handler */
HI_S32 PCIV_ViuSendPic(VI_DEV viDev, VI_CHN viChn, const VIDEO_FRAME_INFO_S *pPicInfo)
{
    PCIV_BIND_OBJ_S stObj;
    stObj.enType = PCIV_BIND_VI;
    stObj.unAttachObj.viDevice.viDevId = viDev;
    stObj.unAttachObj.viDevice.viChn = viChn;

    return PCIV_TxSendPic(&stObj, pPicInfo);
}


/*
** timer function of Sender 
** 1. Get data from vdec and send to another chip through pci
*/
HI_VOID PCIV_VdecTimerFunc(unsigned long data)
{
    HI_S32 i,j, s32Ret;
    PCIV_CHANNEL_S    *pstPcivChn = NULL;
    PCIV_BIND_OBJ_S    stObj;
    VDEC_DATA_S        stVdecData;
	FN_VDEC_KernelGetData     *pfnVdecGetData;
	FN_VDEC_KernelReleaseData *pfnVdecReleaseData;

    /* timer will be restarted after 40ms */
    s_stPcivTimer.expires  = jiffies + 4;
    s_stPcivTimer.function = PCIV_VdecTimerFunc;
    s_stPcivTimer.data     = 0;
    add_timer(&s_stPcivTimer);
    
    if(NULL == g_astModules[HI_ID_VDEC].pstExportFuncs)
    {
        PCIV_TRACE(HI_DBG_ERR, "No VDEC function avalid\n");
        return;
    }

    pfnVdecGetData     = FUNC_ENTRANCE(VDEC_EXPORT_FUNC_S, HI_ID_VDEC)->pfnVdecKernelGetData;
    pfnVdecReleaseData = FUNC_ENTRANCE(VDEC_EXPORT_FUNC_S, HI_ID_VDEC)->pfnVdecKernelReleaseData;
    if( (NULL == pfnVdecGetData) || (NULL == pfnVdecReleaseData))
    {
        PCIV_TRACE(HI_DBG_ERR, "No VDEC function avalid\n");
        return;
    }

    i = PCIV_BIND_VDEC;
    for(j=0 ; j<PCIV_MAX_PORT_NUM; j++)
	{
        VDEC_CHN      vdecChn;
	    HI_BOOL       bReleaseFrame = HI_FALSE;
	    PCIV_DEVICE_S stPciDev;

	    stPciDev = g_stPortMap[i][j];
	    if(  (stPciDev.s32PciDev < 0)||(stPciDev.s32PciDev > PCIV_MAX_DEV_NUM) 
	       ||(stPciDev.s32Port < 0) || ( stPciDev.s32Port > PCIV_MAX_PORT_NUM))
	    {
	        continue;
	    }
	    
	    pstPcivChn = &(g_stPcivChn[stPciDev.s32PciDev][stPciDev.s32Port]);
        if(HI_TRUE != pstPcivChn->bStart) continue;

        /* Get the vdec channel number */
        if(pstPcivChn->stBindObj.enType != PCIV_BIND_VDEC) continue;
        vdecChn = pstPcivChn->stBindObj.unAttachObj.vdecDevice.vdecChn;

        /* Get the Vdec data. This call must return immediately. */
        s32Ret = pfnVdecGetData(vdecChn, &stVdecData);
        if(HI_SUCCESS != s32Ret) continue;

        /* We only need the picture frame in this version */
        if(stVdecData.stFrameInfo.bValid == HI_TRUE)
        {
            stObj.enType = PCIV_BIND_VDEC;
            stObj.unAttachObj.vdecDevice.vdecChn = vdecChn;
            s32Ret = PCIV_TxSendPic(&stObj, &(stVdecData.stFrameInfo.stVideoFrameInfo));
            if(HI_SUCCESS != s32Ret)
            {
                /* The frame should be released */
                bReleaseFrame = HI_TRUE;
            }
        }

        /* other data will be released now */
        stVdecData.stFrameInfo.bValid = bReleaseFrame;
        pfnVdecReleaseData(vdecChn, &stVdecData);
	}

	return;
}

/*
** timer function of Sender 
** 
*/
HI_VOID PCIV_BufTimerFunc(unsigned long data)
{
    (HI_VOID)PcivNotifyFreeBuf((PCIV_CHANNEL_S *)data);
   
	return;
}


/* 获取两个32位整数之间的距离，处理溢出回饶状态 */
inline HI_U32 do_GetU32Span(HI_U32 u32Front, HI_U32 u32Back)
{
    return (u32Front >= u32Back) ? (u32Front-u32Back) : (u32Front + ((~0UL)-u32Back));
}

/* be called in VIU interrupt handler or VDEC interrupt handler. */
HI_S32  PCIV_TxSendPic(PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pPicInfo)
{
    PCIV_CHANNEL_S *pstPcivChn;
    VIDEO_FRAME_S  *pstOutFrame, *pstInFrame;
    VB_BLKHANDLE    handle;
    DSU_TASK_DATA_S stDsuTask;
    HI_S32  s32Ret, s32PciDev, s32Port;

    switch ( pObj->enType )/* resource of pictrue */
    {
        case PCIV_BIND_VI:
        {
            PCIV_DEVICE_S *pDevice;
            VI_DEV viDev = pObj->unAttachObj.viDevice.viDevId;
            VI_CHN viChn = pObj->unAttachObj.viDevice.viChn;
            pDevice = &g_stPortMap[PCIV_BIND_VI][viDev*VIU_MAX_CHN_NUM_PER_DEV + viChn];
            s32PciDev = pDevice->s32PciDev;
            s32Port   = pDevice->s32Port;
            break;
        }
        case PCIV_BIND_VDEC:
        {
            PCIV_DEVICE_S *pDevice;
            VDEC_CHN vdecChn = pObj->unAttachObj.vdecDevice.vdecChn;
            pDevice = &g_stPortMap[PCIV_BIND_VDEC][vdecChn];
            s32PciDev = pDevice->s32PciDev;
            s32Port   = pDevice->s32Port;
            break;
        }
        default:
        {
            PCIV_TRACE(HI_DBG_WARN, "Unsupported picture soucre\n");
            return HI_FAILURE;
        }
    }

    PCIV_CHECK_PCIDEV(s32PciDev);
    PCIV_CHECK_PORT(s32Port);

    pstPcivChn = &g_stPcivChn[s32PciDev][s32Port];
    if (    (pstPcivChn->bStart == HI_FALSE)
         || (pstPcivChn->stBindObj.enType != pObj->enType)
         || (pstPcivChn->stRemoteObj.u32MsgTargetId == g_u32LocalId))
    {
        return HI_FAILURE;
    }

    pstPcivChn->u32RecvCnt++;
    
    /* If too much task is not finished then do not add new */
    {
        HI_U32 u32Over;
        u32Over = do_GetU32Span(pstPcivChn->u32SendCnt, pstPcivChn->u32RespCnt);
        if(u32Over > pstPcivChn->u32Count)
        {
            PCIV_TRACE(HI_DBG_ERR, "PCI(%d,%d), has too much task %d finished %d\n",
            s32PciDev, s32Port, pstPcivChn->u32SendCnt, pstPcivChn->u32RespCnt);
            return HI_ERR_PCIV_BUSY;
        }
    }
    
    handle = VB_GetBlkBySize(pstPcivChn->u32BufSize, VB_UID_PCIV);    
    if (VB_INVALID_HANDLE == handle )
    {
        PCIV_TRACE(HI_DBG_ERR, "get VB(%dByte) fail,vi(%d,%d)\n",pstPcivChn->u32BufSize, s32PciDev, s32Port);
        return HI_ERR_PCIV_NOBUF;
    }
    memcpy(&stDsuTask.stImgIn, pPicInfo, sizeof(*pPicInfo));
    pstInFrame = &stDsuTask.stImgIn.stVFrame;

    /*if VI pic is interlace,Dst pic is frame, only choose top frame send to DSU*/
    if (VIDEO_FIELD_INTERLACED == pstInFrame->u32Field 
        &&   VIDEO_FIELD_FRAME == pstPcivChn->stPicAttr.u32Field)
    {
        pstInFrame->u32Stride[0] <<= 1;
        pstInFrame->u32Stride[1] <<= 1;
        pstInFrame->u32Height >>= 1;
        pstInFrame->u32Field = VIDEO_FIELD_FRAME;
    }

    stDsuTask.stImgOut.u32PoolId = VB_Handle2PoolId(handle);
    
    pstOutFrame = &stDsuTask.stImgOut.stVFrame;    
    pstOutFrame->u32TimeRef = pPicInfo->stVFrame.u64pts;
    
    pstOutFrame->enPixelFormat = pstPcivChn->stPicAttr.enPixelFormat;
    
    /*if Field of VI pic is FRAME, out pic should be FRAME also */
    pstOutFrame->u32Field = (VIDEO_FIELD_FRAME == pstInFrame->u32Field)\
            ?VIDEO_FIELD_FRAME:pstPcivChn->stPicAttr.u32Field;
    
    pstOutFrame->u32Width = pstPcivChn->stPicAttr.u32Width;
    pstOutFrame->u32Height = pstPcivChn->stPicAttr.u32Height;
    pstOutFrame->u32Stride[0] = pstPcivChn->stPicAttr.u32Stride;
    if (PIXEL_FORMAT_YUV_PLANAR_420 == pstPcivChn->stPicAttr.enPixelFormat
        || PIXEL_FORMAT_YUV_PLANAR_422 == pstPcivChn->stPicAttr.enPixelFormat)
    {
        pstOutFrame->u32Stride[1] = pstPcivChn->stPicAttr.u32Stride >> 1;
        pstOutFrame->u32Stride[2] = pstPcivChn->stPicAttr.u32Stride >> 1;
    }
    else if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pstPcivChn->stPicAttr.enPixelFormat
        || PIXEL_FORMAT_YUV_SEMIPLANAR_422 == pstPcivChn->stPicAttr.enPixelFormat)
    {
        pstOutFrame->u32Stride[1] = pstPcivChn->stPicAttr.u32Stride;
        pstOutFrame->u32Stride[2] = 0;
    }
    else if(PIXEL_FORMAT_UYVY_PACKAGE_422 == pstPcivChn->stPicAttr.enPixelFormat
        || PIXEL_FORMAT_YUYV_PACKAGE_422 == pstPcivChn->stPicAttr.enPixelFormat)
    {
        pstOutFrame->u32Stride[1] = 0;
        pstOutFrame->u32Stride[2] = 0;
    }
    else
    {
        return HI_FAILURE;
    }
    
    /*packe format have not u32PhyAddr[1] and u32PhyAddr[2]*/
    pstOutFrame->u32PhyAddr[0] = VB_Handle2Phys(handle);    
    pstOutFrame->u32PhyAddr[1] = pstOutFrame->u32PhyAddr[0] + pstPcivChn->u32OffsetU;
    pstOutFrame->u32PhyAddr[2] = pstOutFrame->u32PhyAddr[0] + pstPcivChn->u32OffsetV;

    /* only used for VI channl ID */
    pstOutFrame->pVirAddr[0] = (HI_VOID*)VB_Handle2Kern(handle);
    pstOutFrame->pVirAddr[1] = pstOutFrame->pVirAddr[0] + pstPcivChn->u32OffsetU;
    pstOutFrame->pVirAddr[2] = pstOutFrame->pVirAddr[0] + pstPcivChn->u32OffsetV;
        
    /*Set DsuTask*/
    if (HI_TRUE == s_bDiflicker)
    {
        stDsuTask.enHFilter 
            = PcivGetDsuFilterHConef(pstInFrame->u32Width,pstOutFrame->u32Width);
        stDsuTask.enVFilterL 
            = PcivGetDsuFilterVlConef(pstInFrame->u32Height,pstOutFrame->u32Height);
        stDsuTask.enVFilterC 
            = PcivGetDsuFilterVcConef(pstInFrame->u32Height,pstOutFrame->u32Height);         
    }
    else
    {
        stDsuTask.enHFilter  = DSU_SCALE_FILTER_DEFAULT;
        stDsuTask.enVFilterL = DSU_SCALE_FILTER_DEFAULT;
        stDsuTask.enVFilterC = DSU_SCALE_FILTER_DEFAULT;
    }

    stDsuTask.enChoice   = DSU_TASK_SCALE;    
    stDsuTask.enDnoise   = DSU_DENOISE_ONLYEDAGE;
    stDsuTask.enLumaStr  = DSU_LUMA_STR_DISABLE;
    stDsuTask.enCE       = DSU_CE_DISABLE;
    
    /*save ViDev and ViChn in privatedata,the high word is ViDev,low word is ViChn*/
    stDsuTask.privateData = MAKE_DWORD(s32PciDev, s32Port);
    stDsuTask.pCallBack   = PcivDsuCallback;
    stDsuTask.enCallModId = HI_ID_PCIV;
    stDsuTask.reserved    = 0;

    /*if create dsu task failure,must release the tmp out videobuf*/
    s32Ret = FUNC_ENTRANCE(DSU_EXPORT_FUNC_S, HI_ID_DSU)->pfnDsuCreateTask(&stDsuTask);
    if(HI_SUCCESS != s32Ret)
    {
        VB_UserSub(VB_Handle2PoolId(handle), pstOutFrame->u32PhyAddr[0], VB_UID_PCIV);
        PCIV_TRACE(2, "create dsu task failed,errno:%x,will lost this frame\n",s32Ret); 
        return HI_FAILURE;
    }    
    VB_UserAdd(pPicInfo->u32PoolId, pPicInfo->stVFrame.u32PhyAddr[0],VB_UID_PCIV);

    return HI_SUCCESS;
}

HI_VOID  PciTxISR(SEND_TASK_S *pstTask)
{
    unsigned long  flags;
    PCIV_CHANNEL_S *pstChn;
    HI_S32 s32Ret, s32PciDev, s32Port, s32BufIndex;
    HI_U32 u32PoolId, u32Count;
    
    s32PciDev = (HI_S32)(HIGH_WORD(pstTask->privateData));
    s32Port   = (HI_S32)(LOW_WORD(pstTask->privateData));
    if(   (s32PciDev < 0) || (s32PciDev >= PCIV_MAX_DEV_NUM)
       || (s32Port < 0) || (s32Port >= PCIV_MAX_PORT_NUM))
        return ;

    u32PoolId = pstTask->resv;

    pstChn = &g_stPcivChn[s32PciDev][s32Port];

    if(pstChn->u32Count < 2)
    {
        s32BufIndex = (pstChn->u32RespCnt) % (pstChn->u32Count);
    }
    else
    {
        s32BufIndex = pstTask->resv1;
    }
    u32Count = pstTask->resv2;
    pstChn->u32RespCnt++;
    if(g_pPciDmaEndNotify != NULL)
    {
        (void)g_pPciDmaEndNotify(&pstChn->stRemoteObj, s32BufIndex, u32Count);
    }
    PCIV_SPIN_LOCK;
    s32Ret = VB_UserSub(u32PoolId,pstTask->u32SrcPhyAddr, VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    PCIV_SPIN_UNLOCK;
}


EXPORT_SYMBOL(PcivRxSend2VO);
HI_VOID PcivRxSend2VO(PCIV_DEVICE_S *pPciDev, HI_U32 u32Index, HI_U32 u32Count)
{
    unsigned long      flags;
    VIDEO_FRAME_INFO_S stVideoFrmInfo;
    PCIV_CHANNEL_S    *pstPcivChn;
    VO_CHN             voChn;

    if(   (pPciDev->s32PciDev < 0) || (pPciDev->s32PciDev >= PCIV_MAX_DEV_NUM)
       || (pPciDev->s32Port < 0) || (pPciDev->s32Port >= PCIV_MAX_PORT_NUM))
        return ;

    pstPcivChn = &g_stPcivChn[pPciDev->s32PciDev][pPciDev->s32Port];        
    if(!pstPcivChn->bStart)
    {
        return;
    }
    
    if(pstPcivChn->stBindObj.enType != PCIV_BIND_VO)
    {
        return;
    }

    voChn = pstPcivChn->stBindObj.unAttachObj.voDevice.voChn;
    if(voChn < 0 || voChn >= VO_MAX_CHN_NUM)
    {
        return;
    }

    pstPcivChn->u32RecvCnt++;

    memset(&stVideoFrmInfo,0,sizeof(stVideoFrmInfo));

    stVideoFrmInfo.u32PoolId = pstPcivChn->u32PoolId[u32Index];
    stVideoFrmInfo.stVFrame.enPixelFormat = pstPcivChn->stPicAttr.enPixelFormat;
    stVideoFrmInfo.stVFrame.u32Field = pstPcivChn->stPicAttr.u32Field;
    stVideoFrmInfo.stVFrame.u32Width = pstPcivChn->stPicAttr.u32Width;
    stVideoFrmInfo.stVFrame.u32Height = pstPcivChn->stPicAttr.u32Height;
    stVideoFrmInfo.stVFrame.u32Stride[0] = pstPcivChn->stPicAttr.u32Stride;
    stVideoFrmInfo.stVFrame.u32Stride[1] = pstPcivChn->stPicAttr.u32Stride;
    
    stVideoFrmInfo.stVFrame.u32PhyAddr[0] = pstPcivChn->au32PhyAddr[u32Index];
    stVideoFrmInfo.stVFrame.u32PhyAddr[1] = pstPcivChn->au32PhyAddr[u32Index] + \
        pstPcivChn->stPicAttr.u32Stride * stVideoFrmInfo.stVFrame.u32Height;

    PCIV_SPIN_LOCK;
    if(FUNC_ENTRANCE(VOU_EXPORT_FUNC_S, HI_ID_VOU) != NULL)
    {
        FUNC_ENTRANCE(VOU_EXPORT_FUNC_S, HI_ID_VOU)->pfnVouChnSendPicEx
                    (voChn ,&stVideoFrmInfo,VOU_WHO_SENDPIC_PCIV);
        pstPcivChn->u32SendCnt ++;
    }
    PCIV_SPIN_UNLOCK;

    pstPcivChn->u32BufUseCnt[0] = u32Count;
    (HI_VOID)PcivNotifyFreeBuf(pstPcivChn);
    
}

EXPORT_SYMBOL(PCIV_RxBufFreeNotify);
HI_VOID PCIV_RxBufFreeNotify(PCIV_DEVICE_S *pPciDev, HI_U32 u32Index, HI_U32 u32Count)
{
    PCIV_CHANNEL_S    *pstPcivChn;

    if(   (pPciDev->s32PciDev < 0) || (pPciDev->s32PciDev >= PCIV_MAX_DEV_NUM)
       || (pPciDev->s32Port < 0) || (pPciDev->s32Port >= PCIV_MAX_PORT_NUM))
        return ;

    pstPcivChn = &g_stPcivChn[pPciDev->s32PciDev][pPciDev->s32Port];        
    if(!pstPcivChn->bStart)
    {
        return;
    }
    
    if(u32Count >= pstPcivChn->u32BufUseCnt[u32Index])
    {
        pstPcivChn->bBufFree[u32Index] = HI_TRUE;
    }
    else
    {
        printk("[%d, %d, %d, %d]", u32Index, pPciDev->s32Port, u32Count, pstPcivChn->u32BufUseCnt[u32Index]);
    }
}


