
#ifndef __HI_COMM_VO_H__
#define __HI_COMM_VO_H__

#include "hi_type.h"
#include "hi_common.h"
#include "hi_comm_video.h"
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum hiEN_VOU_ERR_CODE_E
{
    ERR_VO_FAILED_NOTENABLE     = 64,
    ERR_VO_FAILED_NOTCONFIG     = 65,
    ERR_VO_FAILED_NOTDISABLE    = 66,
    ERR_VO_FAILED_CHNOTDISABLE  = 67,
    ERR_VO_FAILED_TIMEOUT       = 68,
    ERR_VO_FAILED_CHNOTENABLE   = 69,
    ERR_VO_FAILED_CHNNOTCONFIG  = 70,

    ERR_VO_GRP_INVALID_NUMBER   = 71,
    ERR_VO_GRP_NOT_CREATE       = 72,
    ERR_VO_GRP_HAS_CREATED      = 73,
    ERR_VO_GRP_CHN_FULL         = 74,
    ERR_VO_GRP_CHN_EMPTY        = 75,
    ERR_VO_GRP_CHN_NOT_EMPTY    = 76,
    ERR_VO_GRP_INVALID_SYN_MODE = 77,
    ERR_VO_GRP_INVALID_BASE_PTS = 78,
    ERR_VO_GRP_NOT_START        = 79,
    ERR_VO_GRP_INVALID_FRMRATE  = 80,
    ERR_VO_GRP_CHN_NOT_UNREG    = 81,
    ERR_VO_GRP_BASE_NOT_CFG     = 82,

    ERR_VO_FAILED_RECTPARA      = 83,
    ERR_VO_GRP_CHN_HAS_REG      = 84,
    ERR_VO_GRP_CHN_NOT_REG      = 85,

    ERR_VO_SETBEGIN_ALREADY     = 86,
    ERR_VO_SETBEGIN_NOTYET      = 87,
    ERR_VO_SETEND_ALREADY       = 88,
    ERR_VO_SETEND_NOTYET        = 89,

    ERR_VO_REG_OUTRANGE         = 90,

}EN_VOU_ERR_CODE_E;

#define HI_ERR_VO_INVALID_CHNID         HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
#define HI_ERR_VO_ILLEGAL_PARAM         HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define HI_ERR_VO_NOMEM                 HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)

#define HI_ERR_VO_NULL_PTR              HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)

#define HI_ERR_VO_FAILED_NOTENABLE      HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_NOTENABLE)
#define HI_ERR_VO_FAILED_NOTDISABLE     HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_NOTDISABLE)
#define HI_ERR_VO_FAILED_NOTCONFIG      HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_NOTCONFIG)

#define HI_ERR_VO_FAILED_CHNOTDISABLE   HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_CHNOTDISABLE)
#define HI_ERR_VO_FAILED_CHNOTENABLE    HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_CHNOTENABLE)
#define HI_ERR_VO_FAILED_CHNNOTCONFIG   HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_CHNNOTCONFIG)

#define HI_ERR_VO_FAILED_TIMEOUT        HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_TIMEOUT)
#define HI_ERR_VO_SYS_NOTREADY          HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define HI_ERR_VO_SYS_NOTPERM           HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
#define HI_ERR_VO_BUSY              	HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)

#define HI_ERR_VO_INVALID_RECT_PARA     HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_FAILED_RECTPARA)

#define HI_ERR_VO_SETBEGIN_ALREADY      HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETBEGIN_ALREADY)
#define HI_ERR_VO_SETBEGIN_NOTYET       HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETBEGIN_NOTYET)
#define HI_ERR_VO_SETEND_ALREADY        HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETEND_ALREADY)
#define HI_ERR_VO_SETEND_NOTYET         HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_SETEND_NOTYET)
#define HI_ERR_VO_REG_OUTRANGE          HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_REG_OUTRANGE)

/* the following error code is defined for multi-channel synchronization    */
#define HI_ERR_VO_GRP_NULL_PTR        	HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define HI_ERR_VO_GRP_INVALID_NUMBER	HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_NUMBER)
#define HI_ERR_VO_GRP_NOT_CREATE		HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_CREATE)
#define HI_ERR_VO_GRP_HAS_CREATED		HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_HAS_CREATED)
#define HI_ERR_VO_GRP_CHN_FULL			HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_FULL)
#define HI_ERR_VO_GRP_CHN_EMPTY			HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_EMPTY)
#define HI_ERR_VO_GRP_CHN_NOT_EMPTY		HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_EMPTY)
#define HI_ERR_VO_GRP_INVALID_SYN_MODE	HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_SYN_MODE)
#define HI_ERR_VO_GRP_INVALID_BASE_PTS	HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_BASE_PTS)
#define HI_ERR_VO_GRP_NOT_START			HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_NOT_START)
#define HI_ERR_VO_GRP_INVALID_FRMRATE   HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_INVALID_FRMRATE)
#define HI_ERR_VO_GRP_CHN_HAS_REG       HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_HAS_REG)
#define HI_ERR_VO_GRP_CHN_NOT_REG       HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_REG)
#define HI_ERR_VO_GRP_CHN_NOT_UNREG     HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_CHN_NOT_UNREG)
#define HI_ERR_VO_GRP_BASE_NOT_CFG      HI_DEF_ERR(HI_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_GRP_BASE_NOT_CFG)



typedef struct hiTV_CFG_S
{
    VIDEO_NORM_E stComposeMode; 		/*video norm mode like Ntsc or Pal*/
} TV_CFG_S;

typedef struct hiVO_PUB_ATTR_S
{
    TV_CFG_S stTvConfig; 				/*config for tv,inlude video norm mode*/
    HI_U32 u32BgColor;  			    /*background color RGB(8:8:8)lowest 24bit is valid*/
}VO_PUB_ATTR_S;

typedef struct hiVO_CHN_ATTR_S
{
    HI_U32 u32Priority;                 /*video out overlay pri*/
    RECT_S stRect; 	                    /*rect of video out chn*/
    HI_BOOL bZoomEnable;                /*zoom or no*/
}VO_CHN_ATTR_S;

/*these struct only for debug*/
typedef struct hiVO_DEBUG_FLAG_S
{
    HI_U32 u32Flags;
}VO_DEBUG_FLAG_S;

typedef enum hiVO_SCALE_FILTER_E
{
    VO_SCALE_FILTER_DEFAULT,
    VO_SCALE_FILTER_2M,
    VO_SCALE_FILTER_225M,

    VO_SCALE_FILTER_25M,
    VO_SCALE_FILTER_275M,
    VO_SCALE_FILTER_3M,

    VO_SCALE_FILTER_325M,
    VO_SCALE_FILTER_35M,
    VO_SCALE_FILTER_375M,
    VO_SCALE_FILTER_4M,
    VO_SCALE_FILTER_5M,
    VO_SCALE_FILTER_6M,
    VO_SCALE_FILTER_BUTT,
}VO_SCALE_FILTER_E;

typedef struct hiVO_FILTER_CONEF_S
{
	VO_SCALE_FILTER_E enHConef;
	VO_SCALE_FILTER_E enVLConef;
	VO_SCALE_FILTER_E enVCConef;
	HI_BOOL bDefault;
}VO_FILTER_CONEF_S;

typedef enum hiVO_DISPLAY_FIELD_E
{
  VO_FIELD_TOP,		            /* top field*/
  VO_FIELD_BOTTOM,	            /* bottom field*/
  VO_FIELD_BOTH,	            /* top and bottom field*/
  VO_FIELD_EXTRACT,             /* extract half of picture */
  VO_FIELD_BUTT
} VO_DISPLAY_FIELD_E;

typedef struct hiVO_QUERY_STATUS_S
{
    HI_U32 u32ChnBufUsed;       /* channel buffer that been occupied */
} VO_QUERY_STATUS_S;

typedef struct hiVO_VBI_INFO_S
{
    HI_U32 u32X;
    HI_U32 u32Y;
    HI_U8 *pu8VbiInfo;
    HI_U32 u32InfoLen;

} VO_VBI_INFO_S;


/*----------------------------------------------------------------------
	the following definition is used for multi-channel synchronization
-----------------------------------------------------------------------*/
typedef enum hiVO_SYNC_MODE_E
{
	VO_SYNC_MODE_MIN_CHN,		/* use the channel which has minimal pts */
	VO_SYNC_MODE_MAX_CHN,		/* use the channel which has maximal pts */
	VO_SYNC_MODE_USR_CHN,		/* use the channel which user appointed */
	VO_SYNC_MODE_BUTT
} VO_SYNC_MODE_E;

typedef struct hiVO_SYNC_GROUP_ATTR_S
{
    HI_U32 u32SynFrameRate;     /* frame rate of this group */
    VO_SYNC_MODE_E enSyncMode;  /* synchronized mode */
    HI_S32 u32UsrBaseChn;       /* user appointed channel as reference of sync */
} VO_SYNC_GROUP_ATTR_S;

typedef struct hiVO_SYNC_BASE_S
{
    VO_CHN VoChn;               /* base channel to synchronize */
    HI_U64 u64BasePts;          /* base pts to synchronize */
} VO_SYNC_BASE_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __HI_COMM_VO_H__ */

