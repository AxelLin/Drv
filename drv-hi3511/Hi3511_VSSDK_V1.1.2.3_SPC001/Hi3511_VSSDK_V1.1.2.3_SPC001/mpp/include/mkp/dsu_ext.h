/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dsu_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/02/13
  Description   :
  History       :
  1.Date        : 2006/02/13
    Author      :
    Modification: Created file

  2.Date         : 2006/2/13
    Author       : QuYaxin 46153
    Modification : Add setfilter and setSize interface,modified the parameter
                   as HI_BOOL type in setFilterEnable function.

  3.Date         : 2008/9/8
    Author       : x00100808
    Modification : According Problem No.AE6D03668,Add only move data operation
                   in DSU work mode which is used to support PCI's preview
                   service.
******************************************************************************/

#ifndef __DSU_EXT_H__
#define __DSU_EXT_H__

#include "hi_comm_video.h"
#include "vb_ext.h"

#define DSU_MAX_TASK_NUM 40

/*choose scale filter, 
frequency more less, picture may be more obscurer,
Notes:225M means 2.25M*/
typedef enum hiDSU_SCALE_FILTER_E
{
    DSU_SCALE_FILTER_DEFAULT,
    DSU_SCALE_FILTER_2M,
    DSU_SCALE_FILTER_225M,
    DSU_SCALE_FILTER_25M,
    DSU_SCALE_FILTER_275M,
    DSU_SCALE_FILTER_3M,
    DSU_SCALE_FILTER_325M,
    DSU_SCALE_FILTER_35M,
    DSU_SCALE_FILTER_375M,
    DSU_SCALE_FILTER_4M,
    DSU_SCALE_FILTER_5M,    /*10*/
    DSU_SCALE_FILTER_6M,
    DSU_SCALE_FILTER_65M,
    
    DSU_SCALE_FILTER_DEFAULT_EX,
    DSU_SCALE_FILTER_3M_EX,
    DSU_SCALE_FILTER_325M_EX,
    DSU_SCALE_FILTER_5M_EX,
    DSU_SCALE_FILTER_BUTT,
}DSU_SCALE_FILTER_E;

/*choose denoise arg*/
typedef enum hiDSU_DENOISE_E
{
    DSU_DENOISE_ONLYEDAGE,      /* only edge improve, no denoise*/
    DSU_DENOISE_LOWNOISE,       /* low denoise*/
    DSU_DENOISE_MIDNOISE,       /* mid denoise*/
    DSU_DENOISE_HIGHNOISE,      /* high denoise*/
    DSU_DENOISE_VERYHIGHNOISE,  /* very high denoise*/
    DSU_DENOISE_BUTT,
}DSU_DENOISE_E;

/*choose color entrans arg*/
typedef enum hiDSU_CE_E
{
    DSU_CE_DISABLE,
    DSU_CE_ENABLE,
    DSU_CE_BUTT,
}DSU_CE_E;

/*choose luma strenth arg*/
typedef enum hiDSU_LUMA_STR_E
{
    DSU_LUMA_STR_DISABLE,
    DSU_LUMA_STR_ENABLE,
    DSU_LUMA_STR_BUTT,
}DSU_LUMA_STR_E;

/*define DSU struct here*/
/*choose using scale or denoise function*/
typedef enum hiDSU_TASK_CHOICE_E
{
    DSU_TASK_SCALE,
    DSU_TASK_DENOISE,
    DSU_TASK_MOVE,
    DSU_TASK_EXTRACT,   /* extract half of picture */
    DSU_TASK_BUTT,
}DSU_TASK_CHOICE_E;

/*define dsu task struct here*/
typedef struct hiDSU_TASK_DATA_S
{
    struct list_head        list;           /* internal data, don't touch */
    VIDEO_FRAME_INFO_S      stImgIn;        /* input picture */
    VIDEO_FRAME_INFO_S      stImgOut;       /* output picture */
    DSU_TASK_CHOICE_E       enChoice;       /* scale or denoise switch */
    DSU_SCALE_FILTER_E      enHFilter;      /* horizontal filter */
    DSU_SCALE_FILTER_E      enVFilterL;     /* vertical filter of luminance */
    DSU_SCALE_FILTER_E      enVFilterC;     /* vertical filter of chroma */

    DSU_DENOISE_E           enDnoise;       /* denoise switch */
    DSU_CE_E                enCE;           /* chroma enhancement */
    DSU_LUMA_STR_E          enLumaStr;      /* contrast stretch */
    HI_U32                  privateData;    /* private data */
    void             (*pCallBack)(struct hiDSU_TASK_DATA_S *pTask); /* callback */
	MOD_ID_E	            enCallModId;    /* module ID */
	HI_U32 		            u32InField;     /* field flag */
    HI_U32                  reserved;       /* save current picture's state while debug */
    HI_U64                  u64Pts;         /* save current picture's pts while debug */

}DSU_TASK_DATA_S;

typedef HI_S32 	FN_DSU_Init(HI_U32 tasknum);
typedef HI_VOID  FN_DSU_Exit(HI_VOID);
typedef HI_S32   FN_DSU_CreateTask(DSU_TASK_DATA_S *pTask);

typedef struct hiDSU_EXPORT_FUNC_S
{
    FN_DSU_Init *pfnInit;
    FN_DSU_Exit *pfnExit;
    FN_DSU_CreateTask *pfnDsuCreateTask;
}DSU_EXPORT_FUNC_S;

#endif /* __DSU_H__ */

