/******************************************************************************

Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
File Name     : hi_vi.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2006/11/27
Last Modified :
Description   : 
Function List :
History       :
1.Date        : 2006/11/27
Author        : wangjian
Modification  : Created file

******************************************************************************/

#ifndef __HI_COMM_VI_H__
#define __HI_COMM_VI_H__

#include "hi_common.h"
#include "hi_errno.h"
#include "hi_comm_video.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


typedef enum hiEN_VIU_ERR_CODE_E
{
    ERR_VI_FAILED_NOTENABLE = 64,   /* device or channel not enable*/
    ERR_VI_FAILED_NOTDISABLE,       /* device not disable*/
    ERR_VI_FAILED_CHNOTDISABLE,     /* channel not disable*/
	ERR_VI_CFG_TIMEOUT,             /* config timeout*/
	ERR_VI_NORM_UNMATCH,            /* video norm of ADC and VIU is unmatch*/     
} EN_VIU_ERR_CODE_E;

#define HI_ERR_VI_INVALID_PARA          HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM) 
#define HI_ERR_VI_INVALID_DEVID         HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define HI_ERR_VI_INVALID_CHNID         HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define HI_ERR_VI_INVALID_NULL_PTR      HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define HI_ERR_VI_FAILED_NOTCONFIG      HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
#define HI_ERR_VI_SYS_NOTREADY          HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define HI_ERR_VI_BUF_EMPTY          	HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define HI_ERR_VI_BUF_FULL          	HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define HI_ERR_VI_NOMEM         		HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define HI_ERR_VI_NOT_SURPPORT         	HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SURPPORT)
#define HI_ERR_VI_BUSY              	HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define HI_ERR_VI_NOT_PERM              HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)

#define HI_ERR_VI_FAILED_NOTENABLE      HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_NOTENABLE)/* 0xA0108040*/
#define HI_ERR_VI_FAILED_NOTDISABLE     HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_NOTDISABLE)/* 0xA0108041*/
#define HI_ERR_VI_FAILED_CHNOTDISABLE   HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, ERR_VI_FAILED_CHNOTDISABLE)/* 0xA0108042*/
#define HI_ERR_VI_CFG_TIMEOUT           HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, ERR_VI_CFG_TIMEOUT)/* 0xA0108043*/
#define HI_ERR_VI_NORM_UNMATCH          HI_DEF_ERR(HI_ID_VIU, EN_ERR_LEVEL_ERROR, ERR_VI_NORM_UNMATCH)/* 0xA0108044*/




typedef enum hiVI_ADTYPE_E
{
    AD_2815 = 0,
    AD_OTHERS  
} VI_ADTYPE_E;

typedef enum hiVI_CAPSEL_E
{
    VI_CAPSEL_TOP = 0,		/* top field*/
    VI_CAPSEL_BOTTOM,		/* bottom field*/
    VI_CAPSEL_BOTH,			/* top and bottom field*/
    VI_CAPSEL_BUTT
} VI_CAPSEL_E;

typedef enum hiVI_INPUT_MODE_E
{
    VI_MODE_BT656 = 0,		/* normative ITU-R BT.656 YUV4:2:2 */
    VI_MODE_BT601,			/* normative ITU-R BT.601 YUV4:2:2 */			
    VI_MODE_DIGITAL_CAMERA,	/* digatal camera*/
    VI_MODE_720P,
    VI_MODE_BUTT
} VI_INPUT_MODE_E;

typedef enum hiVI_WORK_MODE_E
{
    VI_WORK_MODE_1D1 = 0,
    VI_WORK_MODE_2D1,
    VI_WORK_MODE_4HALFD1,
    VI_WORK_MODE_BUTT
} VI_WORK_MODE_E;

typedef enum hiVI_PIXEL_FORMAT_E
{
    VI_PIXFORMAT_422,
    VI_PIXFORMAT_420,    
    VI_PIXFORMAT_BUTT    
} VI_PIXEL_FORMAT_E;

/* attribute of VI channel*/
typedef struct hiVI_CHN_ATTR_S
{
    RECT_S  		stCapRect;      	/* captrue region(heed meaning of width and height) */
    VI_CAPSEL_E 	enCapSel;   		/* select of filed or frame */ 
    HI_BOOL 		bDownScale;     	/* downscaling switch(1/2) in horizon */
    HI_BOOL 		bChromaResample;	/* chroma resample */
    HI_BOOL 		bHighPri;           /* support one chn high priority */		
    PIXEL_FORMAT_E 	enViPixFormat;      /* support sp420 and sp422 */
} VI_CHN_ATTR_S;

/* public attribute of VI device*/
typedef struct hiVI_PUB_ATTR_S
{
    VI_INPUT_MODE_E enInputMode;		/* video input mode */
    VI_WORK_MODE_E  enWorkMode; 		/* work mode(only for ITU-R.BT656) */ 
    VI_ADTYPE_E 	u32AdType; 			/* AD type(not used now) */
    VIDEO_NORM_E 	enViNorm; 			/* video input normal(only for BT.601) */
    RECT_S  		stRect;            	/* whole captrue region(not used now) */
} VI_PUB_ATTR_S;

typedef struct hiVI_CH_LUM_S
{
    HI_U32 u32FramId;
    HI_U32 u32Lum;    
} VI_CH_LUM_S;


typedef struct hiVI_POLL_ATTR_S
{
    HI_U32          u32MainFramerate;
    VI_CHN_ATTR_S   stMinorChnAttr;    
} VI_POLL_ATTR_S;



typedef struct hiVI_CHN_LUMSTRH_ATTR_S
{
	HI_U32 u32TargetK;
} VI_CHN_LUMSTRH_ATTR_S;

typedef struct hiVI_DBG_INFO_S
{
    HI_BOOL bEnable;   /* Whether this channel is enabled */
    HI_U32 u32IntCnt;  /* The video frame interrupt count */
    HI_U32 u32LostInt; /* The interrupt is received but nobody care */
    HI_U32 u32VbFail;  /* Video buffer malloc failure */
} VI_DBG_INFO_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef__HI_COMM_VIDEO_IN_H__ */


