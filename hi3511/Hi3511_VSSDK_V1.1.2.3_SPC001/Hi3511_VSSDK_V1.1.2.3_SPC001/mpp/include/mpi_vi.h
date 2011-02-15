/******************************************************************************
 
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : mpi_vi.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/11/27
  Description   : 
  History       :
  1.Date        : 2006/11/27
    Author      : w47960
    Modification: Created file

  2.Date        : 2009/01/18
    Author      : z44949
    Modification: Add the "hi_comm_vi.h" and HI_MPI_VI_GetDbgInfo
    
******************************************************************************/
#ifndef __MPI_VI_H__
#define __MPI_VI_H__

#include "hi_comm_vi.h"
/******************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */
/******************************************/

HI_S32 HI_MPI_VI_Open();
HI_S32 HI_MPI_VI_Close();

HI_S32 HI_MPI_VI_SetPubAttr(VI_DEV ViDevId,const VI_PUB_ATTR_S *pstPubAttr);
HI_S32 HI_MPI_VI_GetPubAttr(VI_DEV ViDevId,VI_PUB_ATTR_S *pstPubAttr);

HI_S32 HI_MPI_VI_SetChnAttr(VI_DEV ViDevId,VI_CHN ViChn,const VI_CHN_ATTR_S *pstAttr);
HI_S32 HI_MPI_VI_GetChnAttr(VI_DEV ViDevId,VI_CHN ViChn,VI_CHN_ATTR_S *pstAttr);

HI_S32 HI_MPI_VI_Enable(VI_DEV ViDevId);
HI_S32 HI_MPI_VI_Disable(VI_DEV ViDevId);

HI_S32 HI_MPI_VI_EnableChn(VI_DEV ViDevId,VI_CHN ViChn);
HI_S32 HI_MPI_VI_DisableChn(VI_DEV ViDevId,VI_CHN ViChn);

HI_S32 HI_MPI_VI_BindOutput(VI_DEV ViDevId,VI_CHN ViChn,VO_CHN VoChn);
HI_S32 HI_MPI_VI_UnBindOutput(VI_DEV ViDevId,VI_CHN ViChn,VO_CHN VoChn);

HI_S32 HI_MPI_VI_GetFrame(VI_DEV ViDevId,VI_CHN ViChn,VIDEO_FRAME_INFO_S *pstFrame);
HI_S32 HI_MPI_VI_ReleaseFrame(VI_DEV ViDevId,VI_CHN ViChn,const VIDEO_FRAME_INFO_S *pstRawFrame);

HI_S32 HI_MPI_VI_GetChnLuma(VI_DEV ViDevId,VI_CHN ViChn,VI_CH_LUM_S *pstLuma);

HI_S32 HI_MPI_VI_GetFd(VI_DEV ViDevId,VI_CHN ViChn);


HI_S32 HI_MPI_VI_SetSrcFrameRate(VI_DEV ViDevId,VI_CHN ViChn,HI_U32 u32ViFramerate);
HI_S32 HI_MPI_VI_GetSrcFrameRate(VI_DEV ViDevId,VI_CHN ViChn,HI_U32 *pu32ViFramerate);
HI_S32 HI_MPI_VI_SetFrameRate(VI_DEV ViDevId,VI_CHN ViChn,HI_U32 u32ViFramerate);
HI_S32 HI_MPI_VI_GetFrameRate(VI_DEV ViDevId,VI_CHN ViChn,HI_U32 *pu32ViFramerate);

HI_S32 HI_MPI_VI_SetChnMinorAttr(VI_DEV ViDevId,VI_CHN ViChn,const VI_CHN_ATTR_S *pstAttr);
HI_S32 HI_MPI_VI_GetChnMinorAttr(VI_DEV ViDevId,VI_CHN ViChn,VI_CHN_ATTR_S *pstAttr);

HI_S32 HI_MPI_VI_SetUserPic(VI_DEV ViDevId,VI_CHN ViChn,VIDEO_FRAME_INFO_S *pstVFrame);
HI_S32 HI_MPI_VI_EnableUserPic(VI_DEV ViDevId,VI_CHN ViChn);
HI_S32 HI_MPI_VI_DisableUserPic(VI_DEV ViDevId,VI_CHN ViChn);

HI_S32 HI_MPI_VI_EnableLumstrh(VI_DEV ViDevId,VI_CHN ViChn);
HI_S32 HI_MPI_VI_DisableLumstrh(VI_DEV ViDevId,VI_CHN ViChn);


HI_S32 HI_MPI_VI_GetDbgInfo(VI_DEV ViDevId,VI_CHN ViChn,VI_DBG_INFO_S *pstDbgInfo);

HI_S32 HI_MPI_VI_CloseFd(HI_VOID);


/******************************************/
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
/******************************************/
#endif /*__MPI_VI_H__ */

