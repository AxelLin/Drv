
/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_comm_pciv.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software pciv
  Created       : 2008/06/04
  Last Modified :
  Description   : common struct definition for PCIV
  Function List :
  History       :
  1.Date        : 2008/06/04
    Author      : z50825
    Modification: Created file

******************************************************************************/

#ifndef __HI_COMM_PCIV_H__
#define __HI_COMM_PCIV_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "hi_type.h"
#include "hi_common.h"
#include "hi_errno.h"
#include "hi_comm_video.h"
#include "hi_comm_vdec.h"

/* invlalid device ID */
#define HI_ERR_PCIV_INVALID_DEVID     HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* invlalid channel ID */
#define HI_ERR_PCIV_INVALID_CHNID     HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* at lease one parameter is illagal ,eg, an illegal enumeration value  */
#define HI_ERR_PCIV_ILLEGAL_PARAM     HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* channel exists */
#define HI_ERR_PCIV_EXIST             HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* channel exists */
#define HI_ERR_PCIV_UNEXIST             HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* using a NULL point */
#define HI_ERR_PCIV_NULL_PTR          HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* try to enable or initialize system,device or channel, before configing attribute */
#define HI_ERR_PCIV_NOT_CONFIG        HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* operation is not supported by NOW */
#define HI_ERR_PCIV_NOT_SURPPORT      HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SURPPORT)
/* operation is not permitted ,eg, try to change stati attribute */
#define HI_ERR_PCIV_NOT_PERM          HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* failure caused by malloc memory */
#define HI_ERR_PCIV_NOMEM             HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* failure caused by malloc buffer */
#define HI_ERR_PCIV_NOBUF             HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* no data in buffer */
#define HI_ERR_PCIV_BUF_EMPTY         HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* no buffer for new data */
#define HI_ERR_PCIV_BUF_FULL          HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* system is not ready,had not initialed or loaded*/
#define HI_ERR_PCIV_SYS_NOTREADY      HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)

/* One DMA task is working, wait a minute */
#define HI_ERR_PCIV_BUSY    HI_DEF_ERR(HI_ID_PCIV, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

#define EDGE_ALIGN(x, align)  ((((x)-1)/(align)+1)*(align))


#define PCIV_MAX_BUF_NUM  4
#define PCIV_MAX_DEV_NUM  1
#define PCIV_MAX_PORT_NUM 64

typedef struct
{
	PIXEL_FORMAT_E  enPixelFormat;
	HI_U32   u32Stride;
	HI_U32   u32Width;
	HI_U32   u32Height;
	HI_U32   u32Field;
}PCIV_PIC_ATTR_S;

#define PCIV_DEFAULT_DEVID 0
/* PCI device description */
typedef struct hiPCIV_DEVICE_S
{
	HI_S32     s32PciDev; /* PCI device ID */
	HI_S32     s32Port;   /* Port of a PCI device */
} PCIV_DEVICE_S;

typedef struct hiPCIV_VIDEVICE_S
{
    VI_DEV viDevId;
    VI_CHN viChn;
} PCIV_VIDEVICE_S;

typedef struct hiPCIV_VODEVICE_S
{
    VO_CHN voChn;
} PCIV_VODEVICE_S;

typedef struct hiPCIV_VDECDEVICE_S
{
    VDEC_CHN vdecChn;
} PCIV_VDECDEVICE_S;

typedef struct hiPCIV_VENCDEVICE_S
{
    VENC_CHN vencChn;
} PCIV_VENCDEVICE_S;

typedef enum hiPCIV_BIND_TYPE_E
{
    PCIV_BIND_VI         = 0,
    PCIV_BIND_VO,
    PCIV_BIND_VDEC,
    PCIV_BIND_VENC,
    PCIV_BIND_VSTREAMREV,
    PCIV_BIND_VSTREAMSND, 
    PCIV_BIND_NULL, /* Bind nothing */
    PCIV_BIND_BUTT
} PCIV_BIND_TYPE_E;
#define PCIV_BIND_DEVNUM PCIV_BIND_BUTT

typedef struct hiPCI_BIND_OBJ_S
{
    PCIV_BIND_TYPE_E enType;
    union
    {
        PCIV_VIDEVICE_S   viDevice;
        PCIV_VODEVICE_S   voDevice;
        PCIV_VDECDEVICE_S vdecDevice;
        PCIV_VENCDEVICE_S vencDevice;
    } unAttachObj;    
} PCIV_BIND_OBJ_S;

typedef struct hiPCIV_BIND_S
{
	PCIV_DEVICE_S   stPCIDevice;
	PCIV_BIND_OBJ_S stBindObj;
} PCIV_BIND_S;

typedef struct hiPCIV_REMOTE_OBJ_S
{
    HI_U32          u32MsgTargetId;
	PCIV_DEVICE_S   stTargetDev;
} PCIV_REMOTE_OBJ_S;

typedef struct hiPCIV_DEVICE_ATTR_S
{
	PCIV_DEVICE_S     stPCIDevice;   /* PCI device */
	PCIV_PIC_ATTR_S   stPicAttr;     /* picture attibute */
	HI_U32            u32AddrArray[PCIV_MAX_BUF_NUM]; /* address list for picture move */
	HI_U32            u32BlkSize[PCIV_MAX_BUF_NUM];
	HI_U32            u32Count;      /* lenght of address list */
	PCIV_REMOTE_OBJ_S stRemoteObj;   
} PCIV_DEVICE_ATTR_S;

typedef struct hiPCIV_BASEWINDOW_S
{
    HI_S32 s32SlotIndex;
    HI_U32 u32NpWinBase;
    HI_U32 u32PfWinBase;
    HI_U32 u32CfgWinBase;
    HI_U32 u32PfAHBAddr;
} PCIV_BASEWINDOW_S;

#define PCIV_MAX_DMABLK 128
typedef struct hiPCIV_DMA_BLOCK_S
{
    HI_U32  u32SrcAddr;
    HI_U32  u32DstAddr;
    HI_U32  u32BlkSize;
} PCIV_DMA_BLOCK_S;

typedef struct hiPCIV_DMA_TASK_S
{
    HI_U32  u32Count;
    HI_BOOL bRead;
    PCIV_DMA_BLOCK_S *pBlock;
} PCIV_DMA_TASK_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __HI_COMM_GROUP_H__ */

