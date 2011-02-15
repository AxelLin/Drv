/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : vdec_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/05/18
  Description   : 
  History       :
  1.Date        : 2006/05/18
    Author      : z50825
    Modification: Created file
  2.Date        : 2008/06/05
    Author      : z50825
    Modification: AE6D03456£¬Add FN_VDEC_FrameOverNotify
  3.Date        : 2008/11/19
    Author      : c55300
    Modification: Add kernel interface: VDEC_KernelGetData/VDEC_KernelReleaseData.
******************************************************************************/

#ifndef  __VDEC_EXT_H__
#define  __VDEC_EXT_H__

#include "hi_type.h"
#include "hi_comm_video.h"
#include "hi_comm_vdec.h"
#include "vb_ext.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */
typedef struct hiVDEC_STREAM_PACK_S
{
    HI_U32   u32PhyAddr;
    HI_VOID* pVirtAddr;
    HI_U32   u32Len;
    HI_U64   u64PTS;
}VDEC_STREAM_PACK_S;

/*
typedef HI_VOID (*PTR_VDEC_CALLBACK_FN)(HI_S32 s32ChnID);
*/

typedef struct hiALG_STAT_S
{
	HI_U32 u32DecodedFrmNum;
}ALG_STAT_S;

typedef HI_S32  FN_VDEC_SendPic(HI_S32 s32ChnID, VIDEO_FRAME_INFO_S	*pstFrameInfo);
typedef HI_S32  FN_VDEC_SendWMData(HI_S32 s32ChnID, HI_VOID *pVirtAddr ,HI_U32 u32Len);
typedef HI_S32  FN_VDEC_SendUserData(HI_S32 s32ChnID, HI_VOID *pVirtAddr ,HI_U32 u32Len);
typedef HI_S32  FN_VDEC_RleaseStreamBuf(HI_S32 s32ChnID, HI_VOID *pVirtAddr, HI_U32 u32NalLen);
typedef HI_VOID FN_VDEC_FrameOverNotify(HI_S32 s32ChnID);
typedef HI_S32 FN_VDEC_KernelGetData(HI_S32 s32ChnID, VDEC_DATA_S *pData);
typedef HI_S32 FN_VDEC_KernelReleaseData(HI_S32 s32ChnID, VDEC_DATA_S *pData);
/*
typedef HI_VOID FN_VDEC_DestoryChnCallBack(HI_S32 s32ChnID);
*/

typedef HI_S32 FN_VDEC_Init(HI_VOID);
typedef HI_VOID FN_VDEC_Exit(HI_VOID);


typedef struct hiVDEC_EXPORT_FUNC_S
{
    FN_VDEC_SendPic *pfnVdecSendPic;
    FN_VDEC_SendWMData *pfnVdecSendWMData;
    FN_VDEC_SendUserData *pfnVdecSendUserData;
    FN_VDEC_RleaseStreamBuf *pfnVdecRleaseStreamBuf;
	FN_VDEC_FrameOverNotify *pfnVdecFrameOverNotify;
	FN_VDEC_KernelGetData *pfnVdecKernelGetData;
	FN_VDEC_KernelReleaseData *pfnVdecKernelReleaseData;
/*
    FN_VDEC_DestoryChnCallBack *pfnVdecDestoryChnCallBack;
*/
    FN_VDEC_Init *pfnInit;
    FN_VDEC_Exit *pfnExit;
}VDEC_EXPORT_FUNC_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __VDEC_EXT_H__ */

