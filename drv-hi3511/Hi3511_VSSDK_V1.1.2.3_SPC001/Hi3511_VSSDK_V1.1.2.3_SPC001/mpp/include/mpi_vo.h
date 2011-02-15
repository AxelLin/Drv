/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_vo.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/11/27
  Description   :
  History       :
  1.Date        : 2006/11/27
    Author      : w47960
    Modification: Created file
******************************************************************************/

#ifndef __MPI_VO_H__
#define __MPI_VO_H__

/******************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */
/******************************************/

HI_S32 HI_MPI_VO_Open();
HI_S32 HI_MPI_VO_Close();

HI_S32 HI_MPI_VO_SetChnAttr(VO_CHN VoChn,const VO_CHN_ATTR_S *pstAttr);
HI_S32 HI_MPI_VO_GetChnAttr(VO_CHN VoChn,VO_CHN_ATTR_S *pstAttr);

HI_S32 HI_MPI_VO_GetPubAttr(VO_PUB_ATTR_S *pPubAttr);
HI_S32 HI_MPI_VO_SetPubAttr(const VO_PUB_ATTR_S *pPubAttr);

HI_S32 HI_MPI_VO_Enable();
HI_S32 HI_MPI_VO_Disable();
HI_S32 HI_MPI_VO_EnableChn(VO_CHN VoChn);
HI_S32 HI_MPI_VO_DisableChn(VO_CHN VoChn);

HI_S32 HI_MPI_VO_SendFrame(VO_CHN VoChn,VIDEO_FRAME_INFO_S *pstVFrame);

HI_S32 HI_MPI_VO_SetChnField(VO_CHN VoChn,const VO_DISPLAY_FIELD_E enField);
HI_S32 HI_MPI_VO_GetChnField(VO_CHN VoChn,VO_DISPLAY_FIELD_E *pField);

HI_S32 HI_MPI_VO_SetFrameRate(VO_CHN VoChn,HI_S32 s32VoFramerate);
HI_S32 HI_MPI_VO_GetFrameRate(VO_CHN VoChn,HI_S32 *ps32VoFramerate);

HI_S32 HI_MPI_VO_ChnPause(VO_CHN VoChn);
HI_S32 HI_MPI_VO_ChnResume(VO_CHN VoChn);
HI_S32 HI_MPI_VO_ChnStep(VO_CHN VoChn);

HI_S32 HI_MPI_VO_EnableDeflicker();
HI_S32 HI_MPI_VO_DisableDeflicker();

HI_S32 HI_MPI_VO_SetZoomInWindow(VO_CHN VoChn, const RECT_S stZoomRect);
HI_S32 HI_MPI_VO_GetZoomInWindow(VO_CHN VoChn, RECT_S *pstZoomRect);

HI_S32 HI_MPI_VO_GetChnPts(VO_CHN VoChn, HI_U64 *pu64VoChnPts);

HI_S32 HI_MPI_VO_SetAttrBegin(HI_VOID);
HI_S32 HI_MPI_VO_SetAttrEnd(HI_VOID);

HI_S32 HI_MPI_VO_ChnShow(VO_CHN VoChn);
HI_S32 HI_MPI_VO_ChnHide(VO_CHN VoChn);

HI_S32 HI_MPI_VO_Query(VO_CHN VoChn, VO_QUERY_STATUS_S *pstQueryStatus);

HI_S32 HI_MPI_VO_SetDisplayFrameRate(HI_S32 s32VoFramerate);
HI_S32 HI_MPI_VO_GetDisplayFrameRate(HI_S32 *ps32VoFramerate);

HI_S32 HI_MPI_VO_GetChnFrame(VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstFrame);
HI_S32 HI_MPI_VO_ReleaseChnFrame(VO_CHN VoChn, VIDEO_FRAME_INFO_S *pstFrame);

HI_S32 HI_MPI_VO_SetVbiInfo(VO_VBI_INFO_S *pstVbiInfo);
HI_S32 HI_MPI_VO_ClrVbiInfo(HI_VOID);
HI_S32 HI_MPI_VO_GetVbiInfo(VO_VBI_INFO_S *pstVbiInfo);

HI_S32 HI_MPI_VO_ClearChnBuffer(VO_CHN VoChn);

HI_S32 HI_MPI_VO_SetDisplayRect(RECT_S *pstRect);
HI_S32 HI_MPI_VO_GetDisplayRect(RECT_S *pstRect);


/*----------------------------------------------------------------------
	the following functions are used for multi-channel synchronization
-----------------------------------------------------------------------*/
HI_S32 HI_MPI_VO_CreateSyncGroup(VO_GRP VoGroup, VO_SYNC_GROUP_ATTR_S *pstSycGrpAttr);
HI_S32 HI_MPI_VO_DestroySyncGroup(VO_GRP VoGroup);

HI_S32 HI_MPI_VO_RegisterSyncGroup(VO_CHN VoChn, VO_GRP VoGroup);
HI_S32 HI_MPI_VO_UnRegisterSyncGroup(VO_CHN VoChn, VO_GRP VoGroup);

HI_S32 HI_MPI_VO_SyncGroupStart(VO_GRP VoGroup);
HI_S32 HI_MPI_VO_SyncGroupStop(VO_GRP VoGroup);

HI_S32 HI_MPI_VO_SetSyncGroupBase(VO_GRP VoGroup, const VO_SYNC_BASE_S *pstSyncBase);
HI_S32 HI_MPI_VO_GetSyncGroupBase(VO_GRP VoGroup, VO_SYNC_BASE_S *pstSyncBase);

HI_S32 HI_MPI_VO_SetSyncGroupFrameRate(VO_GRP VoGroup, HI_S32 s32VoGrpFramerate);
HI_S32 HI_MPI_VO_GetSyncGroupFrameRate(VO_GRP VoGroup, HI_S32 *ps32VoGrpFramerate);

HI_S32 HI_MPI_VO_PauseSyncGroup(VO_GRP VoGroup);
HI_S32 HI_MPI_VO_ResumeSyncGroup(VO_GRP VoGroup);
HI_S32 HI_MPI_VO_StepSyncGroup(VO_GRP VoGroup);

HI_S32 HI_MPI_VO_CloseFd(HI_VOID);


/******************************************/
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
/******************************************/
#endif /*__MPI_VI_H__ */


