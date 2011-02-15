#include "hi_common.h"
#include "hi_type.h"
#include "hi_comm_pciv.h"
#include "pciv_msg.h"
#ifndef __PCI_STREAM_H__
#define __PCI_STREAM_H__

typedef struct hiPCIV_STREAM_BUF_S
{
    HI_S32  s32BaseAddr;
    HI_S32  s32Length;
    HI_S32  s32ReadPos;
    HI_S32  s32WritePos;
} PCIV_STREAM_BUF_S;

typedef HI_S32 PCIV_REVSTREAM_FUNC(HI_S32 s32LocalId, PCIV_NOTIFY_STREAMWRITE_S *pStream);
typedef struct hiPCIV_STREAM_RECEIVER_S
{
    HI_S32         s32TargetId;
    PCIV_DEVICE_S  stTargetPciDev;
    HI_U32         u32BufBaseAddr;
    HI_U32         u32BufLen;
    PCIV_REVSTREAM_FUNC *pRevStreamFun;
} PCIV_STREAM_RECEIVER_S;

typedef struct hiPCIV_STREAM_CREATOR_S
{
    HI_S32            s32TargetId;
    PCIV_DEVICE_S     stTargetPciDev;
    PCIV_STREAM_BUF_S stBuffer;
    VENC_CHN          vencChn;
    FILE             *pFile;
} PCIV_STREAM_CREATOR_S;

typedef struct hiPCIV_THREAD_PARAM_S
{
    HI_S32 s32DstTarget;
    HI_S32 s32LocalId;
} PCIV_THREAD_PARAM_S;

HI_VOID  PCIV_InitBuffer(HI_VOID);
HI_S32   PCIV_InitCreater(PCIV_DEVICE_ATTR_S *pAttr, PCIV_BIND_OBJ_S *pBind);
HI_S32   PCIV_InitReceiver(PCIV_DEVICE_ATTR_S *pAttr, PCIV_REVSTREAM_FUNC *pRevStreamFun);
HI_VOID *PCIV_ThreadRev(HI_VOID *p);
HI_VOID *PCIV_ThreadSend(HI_VOID *p);


HI_S32 PCIV_SaveStream(HI_S32 s32LocalId, PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo);
HI_S32 PCIV_DefaultRecFun(HI_S32 s32LocalId, PCIV_NOTIFY_STREAMWRITE_S *pStreamInfo);

#endif
