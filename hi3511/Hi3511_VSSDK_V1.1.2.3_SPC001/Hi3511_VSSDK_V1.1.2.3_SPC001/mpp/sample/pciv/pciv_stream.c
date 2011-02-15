#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include "hi_comm_venc.h"
#include "hi_comm_vb.h"
#include "hi_comm_pciv.h"
#include "pciv_stream.h"

static PCIV_STREAM_CREATOR_S  g_stStreamCreator[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];
static PCIV_STREAM_RECEIVER_S g_stStreamReceiver[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];

static int s32MemFd = -1;

#define PAGE_SIZE 0x1000
#define PAGE_SIZE_MASK 0xfffff000

HI_VOID PCIV_InitBuffer(HI_VOID)
{
    HI_S32 i, j;
    
    for(i=0; i<PCIV_MAX_DEV_NUM; i++)
    {
        for(j=0; j<PCIV_MAX_PORT_NUM; j++)
        {
            g_stStreamCreator[i][j].s32TargetId  = -1;
            g_stStreamCreator[i][j].vencChn      = -1;
            g_stStreamCreator[i][j].pFile        = NULL;
            g_stStreamReceiver[i][j].s32TargetId = -1;
        }
    }
}

PCIV_STREAM_CREATOR_S* PCIV_GetCreater(PCIV_DEVICE_S *pDev)
{
    return &g_stStreamCreator[pDev->s32PciDev][pDev->s32Port];
}

PCIV_STREAM_RECEIVER_S* PCIV_GetReceiver(PCIV_DEVICE_S *pDev)
{
    return &g_stStreamReceiver[pDev->s32PciDev][pDev->s32Port];
}

HI_S32 PCIV_InitCreater(PCIV_DEVICE_ATTR_S *pAttr, PCIV_BIND_OBJ_S *pBind)
{
    PCIV_STREAM_CREATOR_S *pCreater = NULL;
    PCIV_REMOTE_OBJ_S     *pRmtObj  = &pAttr->stRemoteObj;
    PCIV_DEVICE_S         *pPcivDev = &(pAttr->stPCIDevice);
    
    pCreater = &g_stStreamCreator[pPcivDev->s32PciDev][pPcivDev->s32Port];
    
    pCreater->s32TargetId = pRmtObj->u32MsgTargetId;
    pCreater->stTargetPciDev.s32PciDev = pRmtObj->stTargetDev.s32PciDev;
    pCreater->stTargetPciDev.s32Port   = pRmtObj->stTargetDev.s32Port;

    /* This address is needed by DMA, so not need to be maped */
    pCreater->stBuffer.s32BaseAddr     = pAttr->u32AddrArray[0];
    pCreater->stBuffer.s32Length       = pAttr->u32BlkSize[0];
    pCreater->stBuffer.s32ReadPos      = 0;
    pCreater->stBuffer.s32WritePos     = 0;
    pCreater->vencChn                  = -1;
    pCreater->pFile                    = NULL;

    if(pBind != NULL)
    {
        HI_ASSERT((PCIV_BIND_VENC == pBind->enType));
        pCreater->vencChn = pBind->unAttachObj.vencDevice.vencChn;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_DeInitCreater(PCIV_DEVICE_S *pDev)
{
    PCIV_STREAM_CREATOR_S *pCreater = NULL;
    
    pCreater = &g_stStreamCreator[pDev->s32PciDev][pDev->s32Port];
    
    pCreater->s32TargetId = -1;
    pCreater->stTargetPciDev.s32PciDev = -1;
    pCreater->stTargetPciDev.s32Port   = -1;

    /* This address is needed by DMA, so not need to be maped */
    pCreater->stBuffer.s32BaseAddr     = 0;
    pCreater->stBuffer.s32Length       = 0;
    pCreater->stBuffer.s32ReadPos      = 0;
    pCreater->stBuffer.s32WritePos     = 0;
    pCreater->vencChn                  = -1;
    if(pCreater->pFile != NULL)
    {
        //fclose(pCreater->pFile);
        pCreater->pFile = NULL;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_InitReceiver(PCIV_DEVICE_ATTR_S *pAttr, PCIV_REVSTREAM_FUNC *pRevStreamFun)
{
    PCIV_STREAM_RECEIVER_S *pRev = NULL;
    PCIV_REMOTE_OBJ_S      *pRmtObj  = &pAttr->stRemoteObj;
    PCIV_DEVICE_S          *pPcivDev = &(pAttr->stPCIDevice);
    HI_U32 u32Diff, u32PageAddr, u32PageSize;
    
    pRev = &g_stStreamReceiver[pPcivDev->s32PciDev][pPcivDev->s32Port];
    
    pRev->s32TargetId = pRmtObj->u32MsgTargetId;
    pRev->stTargetPciDev.s32PciDev = pRmtObj->stTargetDev.s32PciDev;
    pRev->stTargetPciDev.s32Port   = pRmtObj->stTargetDev.s32Port;

    /* This address is used by user, so we need to map it */
    pRev->u32BufBaseAddr           = pAttr->u32AddrArray[0];
    pRev->u32BufLen                = pAttr->u32BlkSize[0];
    if(s32MemFd <= 0)
    {
    	s32MemFd = open ("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
	    if (s32MemFd <= 0)
	    {
	        return HI_FAILURE;
	    }
    }
	u32PageAddr = pRev->u32BufBaseAddr & PAGE_SIZE_MASK;	
	u32Diff = pRev->u32BufBaseAddr - u32PageAddr;
	
	/* size in page_size */	
	u32PageSize =((pRev->u32BufLen + u32Diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;	
	pRev->u32BufBaseAddr = (HI_U32)mmap((void *)0, u32PageSize, PROT_READ,
	                            MAP_SHARED, s32MemFd, u32PageAddr);

	if (MAP_FAILED == (HI_VOID*)pRev->u32BufBaseAddr )	
	{		
        return HI_FAILURE;
	}

	pRev->u32BufBaseAddr += u32Diff;
	pRev->pRevStreamFun   = pRevStreamFun;
    return HI_SUCCESS;
}

HI_S32 PCIV_DeInitReceiver(PCIV_DEVICE_S *pDev)
{
    PCIV_STREAM_RECEIVER_S *pRev = NULL;
    HI_U32 u32Diff, u32PageAddr, u32PageSize;

    pRev = &g_stStreamReceiver[pDev->s32PciDev][pDev->s32Port];
    if((pRev->s32TargetId == -1))
    {
        /* No need to destroy, but return hi_success also */
        return HI_SUCCESS;
    }
	u32PageAddr = pRev->u32BufBaseAddr & PAGE_SIZE_MASK;	
	u32Diff     = pRev->u32BufBaseAddr - u32PageAddr;
	u32PageSize =((pRev->u32BufLen + u32Diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;	
    munmap((HI_VOID*)u32PageAddr, u32PageSize);

    pRev->s32TargetId   = -1;
	pRev->pRevStreamFun = NULL;
    return HI_SUCCESS;
}

/*
** If the buffer tail is not enough then go to the head.
** And the tail will not be used 
*/
HI_S32 PCIV_GetBuffer(PCIV_DEVICE_S *pDev, HI_S32 s32Size, HI_S32 *pOffset)
{
    PCIV_STREAM_BUF_S *pBuf = NULL;
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32ReadPos;
    
    pBuf = &g_stStreamCreator[pDev->s32PciDev][pDev->s32Port].stBuffer;
    s32ReadPos = pBuf->s32ReadPos;
    if(pBuf->s32WritePos >= s32ReadPos)
    {
        if( (pBuf->s32WritePos + s32Size) <= (pBuf->s32Length))
        {
            *pOffset = pBuf->s32WritePos;
            pBuf->s32WritePos += s32Size;

            s32Ret = HI_SUCCESS;
        }
        else if(s32Size <= s32ReadPos)
        {
            *pOffset = 0;
            pBuf->s32WritePos = s32Size;

            s32Ret = HI_SUCCESS;
        }
    }
    else
    {
        if((pBuf->s32WritePos + s32Size) <= (s32ReadPos))
        {
            *pOffset = pBuf->s32WritePos;
            pBuf->s32WritePos += s32Size;

            s32Ret = HI_SUCCESS;
        }
    }
    
    return s32Ret;
}

HI_S32 PCIV_ReleaseBuffer(PCIV_DEVICE_S *pDev, HI_S32 s32Offset)
{
    PCIV_STREAM_BUF_S *pBuf = NULL;
    HI_S32 s32Ret = HI_FAILURE;
    
    pBuf = &g_stStreamCreator[pDev->s32PciDev][pDev->s32Port].stBuffer;
    pBuf->s32ReadPos = s32Offset;

    return HI_SUCCESS;
}

HI_S32 PCIV_SaveStream(HI_S32 s32LocalId, PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo)
{
    HI_S32 s32Ret;
    HI_U32 u32Addr, u32Len;
    PCIV_STREAM_RECEIVER_S *pReceiver = NULL;
    HI_S32 s32Dev  = pStreamInfo->stDstDev.s32PciDev;
    HI_S32 s32Port = pStreamInfo->stDstDev.s32Port;
    static FILE *pFile[PCIV_MAX_PORT_NUM] = {NULL};
    
    if(pFile[s32Port] == NULL)
    {
        char cFileName[80];
        sprintf(cFileName, "file_port%d.h264", s32Port);
        pFile[s32Port] = fopen(cFileName, "wb");
        HI_ASSERT(NULL != pFile[s32Port]);
    }

    pReceiver = PCIV_GetReceiver(&pStreamInfo->stDstDev);
    u32Addr = pStreamInfo->s32Start + pReceiver->u32BufBaseAddr;
    u32Len  = pStreamInfo->s32End - pStreamInfo->s32Start;

    s32Ret = fwrite((HI_VOID *)u32Addr, u32Len, 1, pFile[s32Port]);
    HI_ASSERT(1 == s32Ret);
    
    return HI_SUCCESS;
}

HI_S32 PCIV_DefaultRecFun(HI_S32 s32LocalId, PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo)
{
    PCIV_STREAM_RECEIVER_S    *pReceiver   = NULL;
    HI_U32 u32Addr, u32SliceHead;

    pReceiver = PCIV_GetReceiver(&pStreamInfo->stDstDev);
    HI_ASSERT(NULL != pReceiver);          

    u32Addr = pStreamInfo->s32Start + pReceiver->u32BufBaseAddr;
    memcpy(&u32SliceHead, (HI_VOID*)u32Addr, sizeof(u32SliceHead));
    HI_ASSERT(u32SliceHead == 0x01000000);
    
    if(pStreamInfo->u32Seq % 1000 == 0)
    {
        printf("Dev(%d,%d,%d) Get Stream From(%d,%d,%d) Seq=%d\n", 
            s32LocalId,
            pStreamInfo->stDstDev.s32PciDev, 
            pStreamInfo->stDstDev.s32Port, 
            pReceiver->s32TargetId,
            pReceiver->stTargetPciDev.s32PciDev,
            pReceiver->stTargetPciDev.s32Port,
            pStreamInfo->u32Seq);
    }

    return HI_SUCCESS;
}

/*
** Function:
** 1. If the message target is not the host then transmit it.
** 2. If 
*/
HI_VOID * PCIV_ThreadRev(HI_VOID *p)
{
    HI_S32 s32Ret, s32DstTarget, s32LocalId, s32RevPort, s32SndPort;
    PCIV_MSGHEAD_S stMsgRev, stMsgEcho;
    PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo = NULL;
    PCIV_NOTIFY_STREAMREAD_S  *pReadInfo = NULL;

    PCIV_STREAM_RECEIVER_S    *pReceiver   = NULL;

    s32DstTarget = ((PCIV_THREAD_PARAM_S *)p)->s32DstTarget;
    s32LocalId   = ((PCIV_THREAD_PARAM_S *)p)->s32LocalId;

    /* If the local device is the host */
    if(s32LocalId == 0)
    {
        s32RevPort = PCIV_MSGPORT_USERNOTIFY2HOST;
        s32SndPort = PCIV_MSGPORT_USERNOTIFY2SLAVE;
    }
    else
    {
        s32RevPort = PCIV_MSGPORT_USERNOTIFY2SLAVE;
        s32SndPort = PCIV_MSGPORT_USERNOTIFY2HOST;
    }
    
    
    while(1)
    {
        s32Ret = PCIV_ReadMsg(s32DstTarget, s32RevPort, &stMsgRev);
        HI_ASSERT((HI_SUCCESS == s32Ret));

        /* If not send to the host then transmit it */
        if((s32LocalId == 0) && (stMsgRev.u32Target != 0))
        {
            s32Ret = PCIV_SendMsg(stMsgRev.u32Target, s32SndPort,&stMsgRev);
            HI_ASSERT(HI_FAILURE != s32Ret);

            continue;
        }

        if(stMsgRev.enDevType != PCIV_DEVTYPE_PCIV)

        {
            printf("Device Type %d is wrong\n", stMsgRev.enDevType);
            continue;
        }

        /* If read end message is send to host then free the buffer */
        pReadInfo =  (PCIV_NOTIFY_STREAMREAD_S *)&stMsgRev.cMsgBody;
        if(stMsgRev.enMsgType == PCIV_MSGTYPE_READDONE)
        {
            s32Ret = PCIV_ReleaseBuffer(&pReadInfo->stDstDev, pReadInfo->s32End);
            HI_ASSERT(HI_SUCCESS == s32Ret);  
            
            continue;
        }
        
        /* Process the stream */
        pStreamInfo = (PCIV_NOTIFY_STREAMWRITE_S *)&stMsgRev.cMsgBody;
        pReceiver = PCIV_GetReceiver(&pStreamInfo->stDstDev);
        HI_ASSERT(NULL != pReceiver);          
        if(pReceiver->pRevStreamFun != NULL)
        {
            pReceiver->pRevStreamFun(s32LocalId, pStreamInfo);
        }

        /* The data is used, then we free it */
        stMsgEcho.enDevType = PCIV_DEVTYPE_PCIV;
        stMsgEcho.enMsgType = PCIV_MSGTYPE_READDONE;
        stMsgEcho.u32MsgLen = sizeof(PCIV_NOTIFY_STREAMREAD_S);
        stMsgEcho.u32Target = pReceiver->s32TargetId;

        pReadInfo = (PCIV_NOTIFY_STREAMREAD_S *)&stMsgEcho.cMsgBody;
        pReadInfo->stDstDev.s32PciDev = pReceiver->stTargetPciDev.s32PciDev;
        pReadInfo->stDstDev.s32Port   = pReceiver->stTargetPciDev.s32Port;
        pReadInfo->s32Start           = pStreamInfo->s32Start;
        pReadInfo->s32End             = pStreamInfo->s32End;
        if(s32LocalId == 0)
        {
            HI_ASSERT(stMsgEcho.u32Target == s32DstTarget);  
            s32Ret = PCIV_SendMsg(stMsgEcho.u32Target, s32SndPort,&stMsgEcho);
        }
        else
        {
            s32Ret = PCIV_SendMsg(0, s32SndPort,&stMsgEcho);
        }
        HI_ASSERT(HI_FAILURE != s32Ret);  
        
    }
}

HI_S32 PCIV_GetStreamFromFile(FILE *pFile, VENC_STREAM_S *pStream)
{
    HI_S32          s32Ret, s32Size, k;
	VENC_CHN_STAT_S stStat;
    

    return HI_SUCCESS;
}

HI_S32 PCIV_GetStreamFromVenc(VENC_CHN vencChn, VENC_STREAM_S *pStream)
{
    HI_S32          s32Ret, s32Size, k;
	VENC_CHN_STAT_S stStat;
    
    s32Ret  = HI_MPI_VENC_Query(vencChn, &stStat);
    if( (s32Ret != HI_SUCCESS) || (stStat.u32CurPacks == 0))
    {
        return HI_FAILURE;
    }
    
    pStream->u32PackCount = stStat.u32CurPacks;
    s32Ret = HI_MPI_VENC_GetStream(vencChn, pStream, HI_IO_NOBLOCK);
    if(s32Ret != HI_SUCCESS)
    {
        pStream->u32PackCount = 0;
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_VOID * PCIV_ThreadSend(HI_VOID *p)
{
    HI_S32          s32Ret, i, j, k, s32DstTarget, s32LocalId, s32SndPort;
    HI_S32          s32Size, s32Offset[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];
    HI_BOOL         bHit;
    VENC_CHN        vencChn;
    VB_BLK          vbBlkHdl;
    const HI_U32    u32BlkSize = 0x20000;
    PCIV_MSGHEAD_S  stMsgSend;
    PCIV_DEVICE_S   stDev;
    VENC_STREAM_S   stVStream[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM];
	VENC_PACK_S     stPack[PCIV_MAX_DEV_NUM][PCIV_MAX_PORT_NUM][128];
    PCIV_STREAM_CREATOR_S     *pCreator    = NULL;
    PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo = NULL;


    s32DstTarget = ((PCIV_THREAD_PARAM_S *)p)->s32DstTarget;
    s32LocalId   = ((PCIV_THREAD_PARAM_S *)p)->s32LocalId;
    /* If the local device is the host */
    if(s32LocalId == 0)
    {
        s32SndPort = PCIV_MSGPORT_USERNOTIFY2SLAVE;
    }
    else
    {
        s32SndPort = PCIV_MSGPORT_USERNOTIFY2HOST;
    }

    memset(stPack, 0, sizeof(stPack));
    memset(stVStream, 0, sizeof(stVStream));
    for(i=0; i<PCIV_MAX_DEV_NUM; i++)
    {
        for(j=0; j<PCIV_MAX_PORT_NUM; j++)
        {
            stVStream[i][j].pstPack = stPack[i][j];
            s32Offset[i][j] = -1;
        }
    }
    
    while(1)
    {
        bHit = HI_FALSE;
        for(i=0; i<PCIV_MAX_DEV_NUM; i++)
        {
            for(j=0; j<PCIV_MAX_PORT_NUM; j++)
            {
                stDev.s32PciDev = i;
                stDev.s32Port   = j;
                pCreator = PCIV_GetCreater(&stDev);
                if(pCreator->s32TargetId == -1) continue;
                if((pCreator->vencChn == -1) && (pCreator->pFile == NULL)) continue;

                /* Get new stream from venc */
                /* If last frame is not send out then try again */
                vencChn = pCreator->vencChn;
                if(stVStream[i][j].u32PackCount == 0)
                {
                    if(vencChn != -1)
                    {
                        s32Ret = PCIV_GetStreamFromVenc(vencChn, &stVStream[i][j]);
                        if(s32Ret != HI_SUCCESS)
                        {
                            continue;
                        }
                        
        			    /* Only when get a new frame we set te hit flag */
                        bHit = HI_TRUE;
                    }
                    else
                    {
                        vbBlkHdl = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32BlkSize);

                        if(vbBlkHdl == VB_INVALID_HANDLE)
                        {
                            printf("No free block\n");
                            continue;
                        }
                        
                        s32Ret = PCIV_GetStreamFromFile(pCreator->pFile, &stVStream[i][j]);
                        if(s32Ret != HI_SUCCESS)
                        {
                            continue;
                        }
                    }
                }

 
                /* If the last buffer is not used then do not malloc new */
                if( s32Offset[i][j] == -1)
                {
                    s32Size = 0;
                    for(k=0; k<stVStream[i][j].u32PackCount; k++)
                    {
                        s32Size += stVStream[i][j].pstPack[k].u32Len[0];
                        s32Size += stVStream[i][j].pstPack[k].u32Len[1];
                    }

                    s32Ret = PCIV_GetBuffer(&stDev, s32Size, &s32Offset[i][j]);
                    if(HI_SUCCESS != s32Ret)
                    {
                        continue;
                    }

                    /* Only when get a new frame we set te hit flag */
                    bHit = HI_TRUE;
                }

                /* Call the driver to send on frame. */
                {
                    PCIV_DMA_TASK_S  stTask;
                    PCIV_DMA_BLOCK_S stDmaBlk[PCIV_MAX_DMABLK];
                    HI_U32           u32Count, u32DstAddr;

                    u32Count   = 0;
                    s32Size    = 0;
                    u32DstAddr = pCreator->stBuffer.s32BaseAddr + s32Offset[i][j];
                    for(k=0; k<stVStream[i][j].u32PackCount; k++)
                    {
                        stDmaBlk[u32Count].u32BlkSize = stVStream[i][j].pstPack[k].u32Len[0];
                        stDmaBlk[u32Count].u32DstAddr = u32DstAddr + (HI_U32)s32Size;
                        s32Size += stDmaBlk[u32Count].u32BlkSize;
                        stDmaBlk[u32Count].u32SrcAddr = stVStream[i][j].pstPack[k].u32PhyAddr[0];
                        u32Count++;

                        if(stVStream[i][j].pstPack[k].u32Len[1] == 0)
                        {
                            continue;
                        }
                        
                        stDmaBlk[u32Count].u32BlkSize = stVStream[i][j].pstPack[k].u32Len[1];
                        stDmaBlk[u32Count].u32DstAddr = u32DstAddr + (HI_U32)s32Size;
                        s32Size += stDmaBlk[u32Count].u32BlkSize;
                        stDmaBlk[u32Count].u32SrcAddr = stVStream[i][j].pstPack[k].u32PhyAddr[1];
                        u32Count++;
                    }

                    stTask.pBlock   = &stDmaBlk[0];
                    stTask.u32Count = u32Count;
                    stTask.bRead    = HI_FALSE;

                    s32Ret = HI_MPI_PCIV_DmaTask(&stTask);
                    while(HI_ERR_PCIV_BUSY == s32Ret)
                    {
                        usleep(0);
                        s32Ret = HI_MPI_PCIV_DmaTask(&stTask);
                    }
                }
                
                if(HI_SUCCESS != s32Ret)
                {
                    continue;
                }
                
                /* Notify the receiver */
                stMsgSend.enDevType = PCIV_DEVTYPE_PCIV;
                stMsgSend.enMsgType = PCIV_MSGTYPE_WRITEDONE;
                stMsgSend.u32MsgLen = sizeof(PCIV_NOTIFY_STREAMWRITE_S);
                stMsgSend.u32Target = pCreator->s32TargetId;
                pStreamInfo = (PCIV_NOTIFY_STREAMWRITE_S *)&stMsgSend.cMsgBody;
                pStreamInfo->stDstDev.s32PciDev = pCreator->stTargetPciDev.s32PciDev;
                pStreamInfo->stDstDev.s32Port   = pCreator->stTargetPciDev.s32Port;
                pStreamInfo->s32Start           = s32Offset[i][j];
                pStreamInfo->s32End             = s32Offset[i][j] + s32Size;
                pStreamInfo->u32Seq             = stVStream[i][j].u32Seq;
                pStreamInfo->u64PTS             = stVStream[i][j].pstPack[0].u64PTS;
                pStreamInfo->bFieldEnd          = stVStream[i][j].pstPack[0].bFieldEnd;
                pStreamInfo->bFrameEnd          = stVStream[i][j].pstPack[0].bFrameEnd;
                pStreamInfo->enDataType         = stVStream[i][j].pstPack[0].DataType;
                if(s32LocalId == 0)
                {
                    s32Ret = PCIV_SendMsg(stMsgSend.u32Target, s32SndPort, &stMsgSend);
                }
                else
                {
                    s32Ret = PCIV_SendMsg(0, s32SndPort, &stMsgSend);
                }
                HI_ASSERT((HI_FAILURE != s32Ret));

                if(vencChn != -1)
                {
            		s32Ret = HI_MPI_VENC_ReleaseStream(vencChn,&stVStream[i][j]);
                    HI_ASSERT((HI_SUCCESS == s32Ret));
                }
                else
                {
                    
                }
                stVStream[i][j].u32PackCount = 0;
                stVStream[i][j].u32Seq       = 0;
                s32Offset[i][j]              = -1;
            }       
        }

        if(!bHit) 
        {
            usleep(0);
        }
    }
}


