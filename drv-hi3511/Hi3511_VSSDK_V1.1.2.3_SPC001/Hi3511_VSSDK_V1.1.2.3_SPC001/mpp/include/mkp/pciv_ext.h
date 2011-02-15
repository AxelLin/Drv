/******************************************************************************
 
  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : pciv_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   : 
  History       :
  1.Date        : 2008/06/16
    Author      :  
    Modification: Created file
******************************************************************************/
#ifndef PCIV_EXT_H__
#define PCIV_EXT_H__

#include "hi_comm_vi.h"

typedef HI_S32  FN_PCIV_SendPic(VI_DEV ViDevId, VI_CHN ViChn, const VIDEO_FRAME_INFO_S *pstPic);

typedef struct hiPCIV_EXPORT_FUNC_S
{
    FN_PCIV_SendPic *pfnPcivSendPic;
}PCIV_EXPORT_FUNC_S;



#endif

