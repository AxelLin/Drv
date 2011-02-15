/******************************************************************************
 
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : pci.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   : 
  History       :
  1.Date        : 2008/06/16
    Author      :  
    Modification: Created file
******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>

#include "mod_ext.h"
#include "hi3511_board.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_pciv.h"
#include "pci_trans.h"
#include "pciv.h"
#include "pciv_msg.h"
#include "hi_mcc_usrdev.h"

typedef enum hiPCIV_HANDLE_E
{
    PCIV_HANDLE_DATA = 0,
    PCIV_HANDLE_TIME = 1,
    PCIV_HANDLE_BUTT = 2
} PCIV_HANDLE_E;
static hios_mcc_handle_t  *g_pstMsgHandle[HI_MCC_TARGET_ID_NR][PCIV_HANDLE_BUTT];
static struct hi_mcc_handle_attr  g_stMsgHandleAttr[PCIV_HANDLE_BUTT] =
                       {{0, PCIV_MSGPORT_KERNEL, 0}, {0, PCIV_MSGPORT_TIME, 1}};

static int g_nLocalId = -1;


static struct list_head  g_listDmaTask;
static spinlock_t        g_lockDmaTask;
static spinlock_t        g_lockMccMsg;

static struct timer_list g_stSyncTimer;
static HI_BOOL           g_bFirstSync = HI_TRUE;

void finish(struct pcit_dma_task *task);

/* The interupter must be lock when call this function */
HI_VOID PcivStartDmaTask(HI_VOID)
{
    SEND_TASK_S         *pTask;
    struct pcit_dma_task pciTask;

    if(list_empty(&(g_listDmaTask)))
    {
        return ;
    }
    
    pTask = list_entry(g_listDmaTask.next, SEND_TASK_S, list);
    pciTask.dir = pTask->bRead ? HI_PCIT_DMA_READ : HI_PCIT_DMA_WRITE;
    pciTask.src = pTask->u32SrcPhyAddr;
    pciTask.dest = pTask->u32DstPhyAddr;
    pciTask.len = pTask->u32Len;
    pciTask.finish = finish;
    pciTask.private_data = pTask;
    
    if(HI_SUCCESS == pcit_create_task(&pciTask))
    {
        list_del(g_listDmaTask.next);
    }
}

void finish(struct pcit_dma_task *task)
{
    SEND_TASK_S *stask = (SEND_TASK_S *)task->private_data;
    if(stask != NULL)
    {
        if(stask->pCallBack != NULL)
        {
            stask->pCallBack(stask);
        }
        
        kfree(stask);
    }

    PcivStartDmaTask();
}


HI_S32 PcivAddDmaTask(SEND_TASK_S *pTask)
{
    SEND_TASK_S  *pTaskTmp;
    unsigned long lockFlag;
    
    pTaskTmp = kmalloc(sizeof(SEND_TASK_S), GFP_ATOMIC);
    if (! pTaskTmp) {
        printk("alloc memory SEND_TASK_S failed!\n");
        return HI_FAILURE;
    }
    memcpy(pTaskTmp, pTask, sizeof(SEND_TASK_S));

    spin_lock_irqsave(&(g_lockDmaTask), lockFlag);

    list_add_tail(&(pTaskTmp->list), &(g_listDmaTask));

    PcivStartDmaTask();

    spin_unlock_irqrestore(&(g_lockDmaTask), lockFlag);

    return HI_SUCCESS;
}

/* It's a half work */
HI_S32 PcivSendMsg(PCIV_HANDLE_E enHandle, PCIV_MSGHEAD_S *pstMsg)
{
    unsigned long lockFlag;
    HI_S32 s32Ret = HI_FAILURE;
    static hios_mcc_handle_t  *pHandle;

    HI_ASSERT(enHandle < PCIV_HANDLE_BUTT);
    
    if(g_nLocalId == 0)
    {
        pHandle = g_pstMsgHandle[pstMsg->u32Target][enHandle];
    }
    else
    {
        pHandle = g_pstMsgHandle[0][enHandle];
    }

    if(NULL == pHandle) return HI_FAILURE;
    
    spin_lock_irqsave(&(g_lockMccMsg), lockFlag);

    s32Ret = hios_mcc_sendto(pHandle, pstMsg, pstMsg->u32MsgLen);
    if(pstMsg->u32MsgLen == s32Ret)
    {   
        s32Ret = HI_SUCCESS;
    }
    else
    {
        printk("Send Msg Error%d[%p], %d, %d\n", pstMsg->u32Target, 
                 pHandle, pstMsg->enMsgType, s32Ret);
    }
    
    spin_unlock_irqrestore(&(g_lockMccMsg), lockFlag);
    
    return s32Ret;
}

HI_S32 PcivDmaEndNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, HI_S32 s32Index, HI_U32 u32Count)
{
    PCIV_MSGHEAD_S stMsg;
    PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)stMsg.cMsgBody;
    
    stMsg.u32Target = pRemoteObj->u32MsgTargetId;
    stMsg.enMsgType = PCIV_MSGTYPE_WRITEDONE;
    stMsg.enDevType = PCIV_DEVTYPE_VOCHN;
    stMsg.u32MsgLen = sizeof(PCIV_NOTIFY_PICEND_S) + PCIV_MSG_HEADLEN;

    pNotify->s32Index = s32Index;
    pNotify->u32Count = u32Count;
    pNotify->stPciDev.s32PciDev = pRemoteObj->stTargetDev.s32PciDev;
    pNotify->stPciDev.s32Port   = pRemoteObj->stTargetDev.s32Port;

    return PcivSendMsg(PCIV_HANDLE_DATA, &stMsg);
}

HI_S32 PcivBufFreeNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, HI_S32 s32Index, HI_U32 u32Count)
{
    PCIV_MSGHEAD_S stMsg;
    PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)stMsg.cMsgBody;
    
    stMsg.u32Target = pRemoteObj->u32MsgTargetId;
    stMsg.enMsgType = PCIV_MSGTYPE_READDONE;
    stMsg.enDevType = PCIV_DEVTYPE_VOCHN;
    stMsg.u32MsgLen = sizeof(PCIV_NOTIFY_PICEND_S) + PCIV_MSG_HEADLEN;

    pNotify->s32Index = s32Index;
    pNotify->u32Count = u32Count;
    pNotify->stPciDev.s32PciDev = pRemoteObj->stTargetDev.s32PciDev;
    pNotify->stPciDev.s32Port   = pRemoteObj->stTargetDev.s32Port;

    return PcivSendMsg(PCIV_HANDLE_DATA, &stMsg);
}

HI_S32 PciDataRecv_MsgNotify(void *handle ,void *buf, unsigned int data_len)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S *pMsg = (PCIV_MSGHEAD_S *)buf;

    if(pMsg->u32Target > HI_MCC_TARGET_ID_NR)
    {
        printk("No this target %d\n", pMsg->u32Target);
        return -1;
    }

    /* If the target is not host then tranlate it */
    if((pMsg->u32Target != g_nLocalId))
    {
        return PcivSendMsg(PCIV_HANDLE_DATA, pMsg);;
    }

    s32Ret = HI_FAILURE;
    switch(pMsg->enMsgType)
    {
        case PCIV_MSGTYPE_WRITEDONE:
        {
            switch(pMsg->enDevType)
            {
                case PCIV_DEVTYPE_VOCHN:
                {
                    PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)pMsg->cMsgBody;
                    PcivRxSend2VO(&pNotify->stPciDev, pNotify->s32Index, pNotify->u32Count);
                    s32Ret = HI_SUCCESS;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PCIV_MSGTYPE_READDONE:
        {
            switch(pMsg->enDevType)
            {
                case PCIV_DEVTYPE_VOCHN:
                {
                    PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)pMsg->cMsgBody;
                    PCIV_RxBufFreeNotify(&pNotify->stPciDev, pNotify->s32Index, pNotify->u32Count);
                    s32Ret = HI_SUCCESS;
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

    if(HI_SUCCESS != s32Ret)
    {
        printk("Unkown how to process\n");
    }
    return s32Ret;
}

HI_S32 PcivTimeSync_MsgNotify(void *handle ,void *buf, unsigned int data_len)
{
    HI_S32 s32Ret;
    PCIV_MSGHEAD_S *pMsg = (PCIV_MSGHEAD_S *)buf;

    if((pMsg->u32Target != g_nLocalId) || (pMsg->enDevType != PCIV_DEVTYPE_TIMER))
    {
        printk("Who are you? Target=%d, DevType=%d\n", pMsg->u32Target, pMsg->enDevType);
        return 0;
    }

    s32Ret = HI_FAILURE;
    switch(pMsg->enMsgType)
    {
        case PCIV_MSGTYPE_GETATTR:
        {
            PCIV_MSGHEAD_S stEchoMsg;
            PCIV_TIME_SYNC_S *pEchoSync = (PCIV_TIME_SYNC_S *)stEchoMsg.cMsgBody;
            PCIV_TIME_SYNC_S *pReqSync = (PCIV_TIME_SYNC_S *)pMsg->cMsgBody;
            SYS_EXPORT_FUNC_S *pstSysExtFun = NULL;

            HI_ASSERT(0 == g_nLocalId);
            
            /* Get the local time and PTS */
            do_gettimeofday(&(pEchoSync->stSysTime));

            pstSysExtFun = FUNC_ENTRANCE(SYS_EXPORT_FUNC_S, HI_ID_SYS);
            HI_ASSERT(NULL != pstSysExtFun);
           
            pEchoSync->u64PtsBase = pstSysExtFun->pfnSysGetTspUs();

            stEchoMsg.enDevType = PCIV_DEVTYPE_TIMER;
            stEchoMsg.enMsgType = PCIV_MSGTYPE_SETATTR;
            stEchoMsg.u32Target = pReqSync->u32ReqTagId;
            stEchoMsg.u32MsgLen = sizeof(PCIV_TIME_SYNC_S) + PCIV_MSG_HEADLEN;

            s32Ret = PcivSendMsg(PCIV_HANDLE_TIME, &stEchoMsg);
            break;
        }
        case PCIV_MSGTYPE_SETATTR:
        {
            PCIV_TIME_SYNC_S *pSync = (PCIV_TIME_SYNC_S *)pMsg->cMsgBody;
            SYS_EXPORT_FUNC_S *pstSysExtFun = NULL;
            struct timespec stTimeSpc;
            
            pstSysExtFun = FUNC_ENTRANCE(SYS_EXPORT_FUNC_S, HI_ID_SYS);
            HI_ASSERT(NULL != pstSysExtFun);
            HI_ASSERT(0 != g_nLocalId);

            /* Sync the system local time */
            stTimeSpc.tv_sec  = pSync->stSysTime.tv_sec;
            stTimeSpc.tv_nsec = pSync->stSysTime.tv_usec * NSEC_PER_USEC;
            do_settimeofday(&stTimeSpc);

            /* Sync the PTS */
            s32Ret = pstSysExtFun->pfnSysSyncPtsBase(pSync->u64PtsBase, g_bFirstSync);
            g_bFirstSync = HI_FALSE;
            
            break;
        }
        default:
            break;
    }

    if(HI_SUCCESS != s32Ret)
    {
        printk("Unkown how to process\n");
    }
    return s32Ret;
}


HI_VOID PcivSyncTimerFunc(unsigned long data)
{
    PCIV_MSGHEAD_S stMsg;
    PCIV_TIME_SYNC_S *pSync =(PCIV_TIME_SYNC_S *)stMsg.cMsgBody;
    
    /* timer will be restarted after 40ms */
    g_stSyncTimer.expires  = jiffies + 100;
    g_stSyncTimer.function = PcivSyncTimerFunc;
    g_stSyncTimer.data     = 0;
    add_timer(&g_stSyncTimer);

    stMsg.enMsgType = PCIV_MSGTYPE_GETATTR;
    stMsg.enDevType = PCIV_DEVTYPE_TIMER;
    stMsg.u32Target = 0;
    stMsg.u32MsgLen = sizeof(PCIV_TIME_SYNC_S) + PCIV_MSG_HEADLEN;
    pSync->u32ReqTagId = g_nLocalId;
    
    (HI_VOID)PcivSendMsg(PCIV_HANDLE_TIME, &stMsg);
}

static int __init PCIV2T_ModInit(void)
{
    hios_mcc_handle_opt_t stopt;
    int i, nRemotId[HI_MCC_TARGET_ID_NR];
    PCIV_BASEWINDOW_S stBaseWin;

    INIT_LIST_HEAD(&(g_listDmaTask));
    spin_lock_init(&(g_lockDmaTask)); 
    spin_lock_init(&g_lockMccMsg);
    
    g_nLocalId = hios_mcc_getlocalid();
    PCIV_SetLocalId(g_nLocalId);
    
    PCIV_RegisterFunction(g_nLocalId, PcivAddDmaTask, 
                           PcivDmaEndNotify, PcivBufFreeNotify);

    if(g_nLocalId == 0)
    {
        for(i=0; i<HI_MCC_TARGET_ID_NR; i++)
        {
            nRemotId[i] = -1;
        }
        
        hios_mcc_getremoteids(nRemotId);
        for(i=0; i<HI_MCC_TARGET_ID_NR; i++)
        {
            if(nRemotId[i] != -1)
            {
                /* Open the data port */
                g_stMsgHandleAttr[PCIV_HANDLE_DATA].target_id = nRemotId[i];
                g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_DATA] = hios_mcc_open(&g_stMsgHandleAttr[PCIV_HANDLE_DATA]);
                if(g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_DATA] == NULL)
                {
                    continue;
                }
                
                stopt.recvfrom_notify = PciDataRecv_MsgNotify;
                hios_mcc_setopt(g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_DATA], &stopt);

                /* Open the time sync port */
                g_stMsgHandleAttr[PCIV_HANDLE_TIME].target_id = nRemotId[i];
                g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_TIME] = hios_mcc_open(&g_stMsgHandleAttr[PCIV_HANDLE_TIME]);
                if(g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_TIME] == NULL)
                {
                    continue;
                }
                
                stopt.recvfrom_notify = PcivTimeSync_MsgNotify;
                hios_mcc_setopt(g_pstMsgHandle[nRemotId[i]][PCIV_HANDLE_TIME], &stopt);

                /* Init the window information */
                stBaseWin.s32SlotIndex  = nRemotId[i];
                stBaseWin.u32NpWinBase  = pcit_get_window_base(nRemotId[i]-1, HI_PCIT_PCI_NP);
                stBaseWin.u32PfWinBase  = pcit_get_window_base(nRemotId[i]-1, HI_PCIT_PCI_PF);
                stBaseWin.u32CfgWinBase = pcit_get_window_base(nRemotId[i]-1, HI_PCIT_PCI_CFG);
                stBaseWin.u32PfAHBAddr  = 0;
                PCIV_SetBaseWindow(&stBaseWin);
            }
        }
    }
    else
    {
        /* Open the data port */
        g_pstMsgHandle[0][PCIV_HANDLE_DATA] = hios_mcc_open(&g_stMsgHandleAttr[PCIV_HANDLE_DATA]);
        if(g_pstMsgHandle[0][PCIV_HANDLE_DATA] == NULL)
        {
            printk("Can't open mcc device 0!\n");
            return -1;
        }
        
        stopt.recvfrom_notify = PciDataRecv_MsgNotify;
        stopt.data = g_nLocalId;
        hios_mcc_setopt(g_pstMsgHandle[0][PCIV_HANDLE_DATA], &stopt);

        /* Open the time sync port */
        g_pstMsgHandle[0][PCIV_HANDLE_TIME] = hios_mcc_open(&g_stMsgHandleAttr[PCIV_HANDLE_TIME]);
        if(g_pstMsgHandle[0][PCIV_HANDLE_TIME] == NULL)
        {
            printk("Can't open mcc device 0!\n");
            return -1;
        }
        
        stopt.recvfrom_notify = PcivTimeSync_MsgNotify;
        stopt.data = g_nLocalId;
        hios_mcc_setopt(g_pstMsgHandle[0][PCIV_HANDLE_TIME], &stopt);

        init_timer(&g_stSyncTimer);
        g_stSyncTimer.expires  = jiffies + 100;
        g_stSyncTimer.function = PcivSyncTimerFunc;
        g_stSyncTimer.data     = 0;
        add_timer(&g_stSyncTimer);

        stBaseWin.s32SlotIndex  = 0;
        stBaseWin.u32NpWinBase  = 0;
        stBaseWin.u32PfWinBase  = 0;
        stBaseWin.u32CfgWinBase = 0;
        stBaseWin.u32PfAHBAddr  = pcit_get_window_base(0, HI_PCIT_AHB_PF);
        PCIV_SetBaseWindow(&stBaseWin);

    }


    
    return 0;
}

static void __exit PCIV2T_ModExit(void)
{    
    int i, j;
    for(i=0; i<HI_MCC_TARGET_ID_NR; i++)
    {
        for(j=0; j<PCIV_HANDLE_BUTT; j++)
        {
            if(g_pstMsgHandle[i][j] != NULL)
            {
                hios_mcc_close(g_pstMsgHandle[i][j]);
            }
        }
    }
    
    return ;
}


module_init(PCIV2T_ModInit);
module_exit(PCIV2T_ModExit);

MODULE_AUTHOR("Hi3511 MPP GRP");
MODULE_LICENSE("GPL");

