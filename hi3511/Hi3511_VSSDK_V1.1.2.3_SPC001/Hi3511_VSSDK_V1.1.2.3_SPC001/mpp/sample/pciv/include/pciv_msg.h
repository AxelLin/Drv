
#ifndef __KERNEL__
#include <sys/time.h>
#endif

#include "hi_common.h"
#include "hi_type.h"
#include "hi_comm_video.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vo.h"
#include "hi_comm_sys.h"
#include "hi_comm_pciv.h"
#include "hi_mcc_usrdev.h"


#ifndef __PCIV_MSG_H__
#define __PCIV_MSG_H__

#define PCIV_MSGPORT_MAXPORT    100
#define PCIV_MSGPORT_TIME       79  /* The most high priority*/
#define PCIV_MSGPORT_KERNEL     80
#define PCIV_MSGPORT_USERCMD    81
#define PCIV_MSGPORT_USERNOTIFY2HOST  82
#define PCIV_MSGPORT_USERNOTIFY2SLAVE 83

typedef enum hiPCIV_DEVTYPE_E
{
    PCIV_DEVTYPE_VICHN,
    PCIV_DEVTYPE_VOCHN,
    PCIV_DEVTYPE_VENCCHN,
    PCIV_DEVTYPE_VDECCHN,
    PCIV_DEVTYPE_AICHN,
    PCIV_DEVTYPE_AOCHN,
    PCIV_DEVTYPE_AENCCHN,
    PCIV_DEVTYPE_ADECCHN,
    PCIV_DEVTYPE_GROUP,
    PCIV_DEVTYPE_PCIV,
    PCIV_DEVTYPE_TIMER,    
    PCIV_DEVTYPE_BUTT
} PCIV_DEVTYPE_E;

typedef enum hiPCIV_MSGTYPE_E
{
    PCIV_MSGTYPE_CREATE,
    PCIV_MSGTYPE_START,
    PCIV_MSGTYPE_DESTROY,
    PCIV_MSGTYPE_SETATTR,
    PCIV_MSGTYPE_GETATTR,
    PCIV_MSGTYPE_CMDECHO,
    PCIV_MSGTYPE_WRITEDONE,
    PCIV_MSGTYPE_READDONE,
    PCIV_MSGTYPE_NOTIFY,
    PCIV_MSGTYPE_BUTT
} PCIV_MSGTYPE_E;

/*  (sizeof(PCIV_MSGTYPE_E) + sizeof(PCIV_DEVTYPE_E) + sizeof(HI_U32))
    + sizeof(u32Target) = 16 */
#define PCIV_MSG_HEADLEN (16)
#define PCIV_MSG_MAXLEN  (512 - PCIV_MSG_HEADLEN)

typedef struct hiPCIV_MSGHEAD_S
{
    HI_U32         u32Target; /* The final user of this message */
    PCIV_MSGTYPE_E enMsgType;
    PCIV_DEVTYPE_E enDevType;
    HI_U32         u32MsgLen;
    HI_U8          cMsgBody[PCIV_MSG_MAXLEN];
} PCIV_MSGHEAD_S;

typedef struct hiPCIV_VICMD_CREATE_S
{
    VI_DEV         viDev;
    VI_CHN         viChn;
    VI_CHN_ATTR_S  stAttr;
} PCIV_VICMD_CREATE_S;

typedef struct hiPCIV_VICMD_DESTROY_S
{
    VI_DEV  viDev;
    VI_CHN  viChn;
} PCIV_VICMD_DESTROY_S;

typedef struct hiPCIV_VICMD_ECHO_S
{
    VI_DEV  viDev;
    VI_CHN  viChn;
    HI_S32  s32Echo;
} PCIV_VICMD_ECHO_S;

typedef struct hiPCIV_VOCMD_CREATE_S
{
    VO_CHN         voChn;
    VO_CHN_ATTR_S  stAttr;    
} PCIV_VOCMD_CREATE_S;

typedef struct hiPCIV_VOCMD_DESTROY_S
{
    VO_CHN voChn;
} PCIV_VOCMD_DESTROY_S;

typedef struct hiPCIV_VOCMD_ECHO_S
{
    VO_CHN voChn;
    HI_S32 s32Echo;
} PCIV_VOCMD_ECHO_S;

typedef struct hiPCIV_PCIVCMD_ATTR_S
{
    PCIV_DEVICE_ATTR_S stDevAttr; /* The attribute should set */
} PCIV_PCIVCMD_ATTR_S;

typedef struct hiPCIV_PCIVCMD_CREATE_S
{
    HI_BOOL            bMalloc;   /* if need malloc memory by remote PCI */
    PCIV_DEVICE_ATTR_S stDevAttr; /* The attribute should set */
    PCIV_BIND_S        stBindObj; /* The bind object */
} PCIV_PCIVCMD_CREATE_S;

typedef struct hiPCIV_PCIVCMD_START_S
{
    PCIV_DEVICE_S stPciDev;
} PCIV_PCIVCMD_START_S;

typedef struct hiPCIV_PCIVCMD_DESTROY_S
{
    PCIV_DEVICE_S stPciDev;
} PCIV_PCIVCMD_DESTROY_S;

typedef struct hiPCIV_PCIVCMD_ECHO_S
{
    PCIV_DEVICE_ATTR_S stDevAttr;
    HI_S32 s32Echo;
} PCIV_PCIVCMD_ECHO_S;

typedef struct hiPCIV_GRPCMD_CREATE_S
{
    VENC_GRP vencGrp;
    VI_DEV   viDev;   /* The VI device and channel which group bind to */
    VI_CHN   viChn;
} PCIV_GRPCMD_CREATE_S;

typedef struct hiPCIV_GRPCMD_DESTROY_S
{
    VENC_GRP vencGrp;
} PCIV_GRPCMD_DESTROY_S;

typedef struct hiPCIV_GRPCMD_ECHO_S
{
    VENC_GRP vencGrp;
    HI_S32   s32Echo;
} PCIV_GRPCMD_ECHO_S;

typedef struct hiPCIV_VENCCMD_CREATE_S
{
    VENC_GRP vencGrp;
    VENC_CHN vencChn;
    VENC_CHN_ATTR_S  stAttr;
} PCIV_VENCCMD_CREATE_S;

typedef struct hiPCIV_VENCCMD_DESTROY_S
{
    VENC_CHN vencChn;
} PCIV_VENCCMD_DESTROY_S;

typedef struct hiPCIV_VENCCMD_ECHO_S
{
    VENC_CHN vencChn;
    HI_S32   s32Echo;
} PCIV_VENCCMD_ECHO_S;

/* The notify message when a new frame is writed */
typedef struct hiPCIV_NOTIFY_STREAMWRITE_S
{
    PCIV_DEVICE_S stDstDev;  /* The PCI device to be notified */
    HI_S32        s32Start;  /* Write or read start position of this frame */
    HI_S32        s32End;    /* Write or read end position of this frame */

    /* The stream's attribute */
    HI_U32        u32Seq;
    HI_U64        u64PTS;     /*PTS*/
    HI_BOOL       bFieldEnd;  /*field end */
    HI_BOOL       bFrameEnd;  /*frame end */
	VENC_DATA_TYPE_U enDataType;   /*the type of stream*/
} PCIV_NOTIFY_STREAMWRITE_S;

/* The notify message when a frame has been readed */
typedef struct hiPCIV_NOTIFY_STREAMREAD_S
{
    PCIV_DEVICE_S stDstDev;  /* The PCI device to be notified */
    HI_S32        s32Start;  /* Write or read start position of this frame */
    HI_S32        s32End;    /* Write or read end position of this frame */
} PCIV_NOTIFY_STREAMREAD_S;

typedef struct hiPCIV_NOTIFY_PICEND_S
{
    PCIV_DEVICE_S stPciDev;  /* The PCI device to be notified */
    HI_S32        s32Index;  /* The buffer index that DMA is end */
    HI_U32        u32Count;
} PCIV_NOTIFY_PICEND_S;

/* Synchronize the time and PTS. The host is base. */
typedef struct hiPCIV_TIME_SYNC_S
{
    struct timeval stSysTime;  /* The current system time */
    HI_U64         u64PtsBase; /* The media PTS */
    HI_U32         u32ReqTagId;
} PCIV_TIME_SYNC_S;

HI_S32 PCIV_OpenMsgPort(HI_S32 s32TgtId);
HI_S32 PCIV_SendMsg(HI_S32 s32TgtId, HI_S32 s32Port, PCIV_MSGHEAD_S *pMsg);
HI_S32 PCIV_ReadMsg(HI_S32 s32TgtId, HI_S32 s32Port, PCIV_MSGHEAD_S *pMsg);


#endif
