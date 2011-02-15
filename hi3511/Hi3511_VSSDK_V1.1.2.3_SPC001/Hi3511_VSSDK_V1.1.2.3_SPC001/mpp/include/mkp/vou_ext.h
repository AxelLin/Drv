/******************************************************************************
 
Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
******************************************************************************
File Name     : vou_ext.h
Version       : Initial Draft
Author        : Hisilicon multimedia software group
Created       : 2006/12/13
Description   : 
History       :
  1.Date         : 2006/02/13
    Author       : wangjian
    Modification : Created file

  2.Date         : 2008/7/16
    Author       : x00100808
    Modification : According to CR20080716007, add clear vo channel buffer flag,
                   which to indicate whether clear the last image buffer.

  3.Date         : 2008/8/15
    Author       : x00100808
    Modification : According to CR20080805049, we display frame which from VDEC
                   by pts, but we let it bypass while under preview mode. Concequ-
                   ently, we add another interface for VDEC especially.

  4.Date         : 2008/9/8
    Author       : x00100808
    Modification : According to AE6D03668, add an extent interface for other modu-
                   les sending pictures to VO, this interface add a paramether
                   which is used to distinguish picture source of VO.
******************************************************************************/

#ifndef __HI_VOU_EXT_H__
#define __HI_VOU_EXT_H__

#include <hi_common.h>
#include "vb_ext.h"

/*  initializing and exit service of VO module,
    called by SYS module*/
typedef HI_S32  FN_VOU_Init(HI_VOID);
typedef HI_VOID FN_VOU_Exit(HI_VOID);

/* send pic to VO ,called by VI */
typedef HI_S32 FN_VOU_ChnSendPic(VO_CHN VoChn, VIDEO_FRAME_INFO_S *pVFrame);

/* send pic to VO ,called by PCIV  */
typedef HI_S32 FN_VOU_ChnSendPicEx(VO_CHN VoChn, VIDEO_FRAME_INFO_S *pVFrame, VOU_WHO_SENDPIC_E eWhoSend);

/* send pic to VO ,called by VDEC */
typedef HI_S32 FN_VOU_ChnSendPicFromVdec(VO_CHN VoChn, VIDEO_FRAME_INFO_S *pVFrame);

/* clear buffer of VO channle */
typedef HI_S32 FN_VOU_ClearChnBuf(VO_CHN VoChn, HI_BOOL bClearLastBuf);

typedef struct hiVOU_EXPORT_FUNC_S
{
    FN_VOU_Init *pfnInit;
    FN_VOU_Exit *pfnExit;
    FN_VOU_ChnSendPic *pfnVouChnSendPic;
    FN_VOU_ChnSendPic *pfnVouChnSendPicFromVdec;
    FN_VOU_ChnSendPicEx *pfnVouChnSendPicEx;
    FN_VOU_ClearChnBuf *pfnVouClearChnBuf;
}VOU_EXPORT_FUNC_S;

#endif

