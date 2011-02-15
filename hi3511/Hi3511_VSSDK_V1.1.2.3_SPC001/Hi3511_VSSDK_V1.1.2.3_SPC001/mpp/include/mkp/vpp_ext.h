/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : vpp_ext.h
  版 本 号   : 初稿
  作    者   : 刘书广 64467
  生成日期   : 2007年10月26日
  最近修改   :
  功能描述   : 
  函数列表   :
  修改历史   :
  1.日    期   : 2007年10月26日
    作    者   : 刘书广 64467
    修改内容   : 创建文件
  2.日    期   : 2008年08月20日
    作    者   : 刘书广 64467
    修改内容   : 增加单独做DE_INTERLACE接口
  3.日    期   : 2009/01/18
    作    者   : z44949
    修改内容   : 增加对VPP配置属性初始化的接口

******************************************************************************/
#ifndef __VPP_EXT_H__
#define __VPP_EXT_H__

#include "hi_comm_vpp.h"
#include "dsu_ext.h"

#ifdef __cplusplus
    #if __cplusplus
    extern "C"{
    #endif
#endif /* End of #ifdef __cplusplus */


#define VPP_MAX_MS_NUM 16



typedef struct hiVPP_ZMECFG_S
{
    HI_BOOL       bPixCutEn; /*缩放丢点使能*/
    HI_BOOL       bHzMen;    /*水平缩放使能*/
    HI_BOOL       bVzMen;    /*垂直缩放使能*/
    HI_U32        u32VhRint; /*水平缩放倍数*/
    HI_U32        u32WhRint; /*垂直缩放倍数*/
}VPP_ZMECFG_S;

typedef struct hiVPP_OVERLAY_S
{
    PIXEL_FORMAT_E enPixelFmt;
    
    HI_U32 u32FgAlpha;
    HI_U32 u32BgAlpha;
    
    RECT_S stRect;
    HI_U32 u32PhyAddr;
	HI_S32 u32Stride;
}VPP_OVERLAY_S;


typedef struct hiVPP_SOVERLAY_S
{
    RECT_S stRect;
	PIXEL_FORMAT_E enPixelFormat;
	
    HI_U8  u8FgAlpha; 
	HI_U8  u8BgAlpha;
    HI_U8  u8Tcy;
    HI_U8  u8Tccb;
    HI_U8  u8Tccr;
	
    void * pdata; 
	HI_U32 u32PhyAddr;
}VPP_SOVERLAY_S;

typedef struct hiVPP_MOSAIC_S
{
    RECT_S stRect;
	PIXEL_FORMAT_E enPixelFormat;
	
    void * pdata;
	HI_U32 u32PhyAddr;
	HI_U32 u32Stride;
}VPP_MOSAIC_REGION_S;


typedef struct hiVPP_DEINTERLACE_S
{ 
	HI_BOOL            bDiTp;          /*输入数据格式，1：422，0：420*/
	
	VIDEO_FRAME_INFO_S stPreImg;       /*前一帧图像信息*/
    VIDEO_FRAME_INFO_S stCurImg;       /*当前帧图像信息*/
    VIDEO_FRAME_INFO_S stNxtImg;	   /*后一帧图像信息*/
	
	HI_U32             u32MsStPhyAddr; /*静止次数数据存储首地址   */
    HI_U32             u32MsStStride;  /*存储静止信息数据的stride */

	HI_VOID            (*pCallBack)(struct hiVPP_DEINTERLACE_S *pstTask);
    VIDEO_FRAME_INFO_S stOutPic;      /*VPP后图像输出*/
	
	HI_U32             privateData;
    HI_U32             reserved;  

}VPP_DEINTERLACE_S;




typedef struct hiVPP_TASK_DATA_S
{
    struct list_head    list;

	/************************************************************************
	 *注意:
	 *重复编码模式下，动静判决ms_en必须打开
	 *de-interlace模式下，中值检测必须使能
	 ***********************************************************************/
	 
    /***********************************************************************
     *芯片提供的推荐工作模式:
     *DIE工作模式          die_en  tf_en  ms_en  edge_en  med_en  rept_enc_ind
     *bypass模式             0       0      0       0       0        0
     *dintl模式              1       0      1       1       1        0
     *bypass+动静判决模式    0       0      1       0       0        0
     *dintl+时域滤波模式     1       1      1       1       1        0
     *bypass+时域滤波模式    0       1      1       0       0        0
     *重复编码+bypass模式    0       x      1       0       0        1
     *重复编码+dintl模式     1       x      1       1       1        1
     ***********************************************************************/
	 
    HI_BOOL	           bFieldMode;       /*帧场标识,帧:HI_FALSE;场:HI_TRUE  */
    HI_BOOL	           bTopField;        /*顶底场,顶场:HI_TRUE底场:HI_FALSE */	
	HI_BOOL            bReptEncInd;      /*重复编码,1:重复编码,0:非重复编码 */
	HI_BOOL            bDiTp;            /*输入数据格式，1：422，0：420     */
    HI_BOOL            bDie;             /*de-interalce使能指示             */
    HI_BOOL            bMs;              /*宏块动静判决使能指示             */
    HI_BOOL            bTf;              /*时域滤波使能指示                 */
    HI_BOOL            bMed;             /*中值检测使能指示                 */
    HI_BOOL            bEdge;            /*边缘检测使能指示                 */
	HI_BOOL            bTxtureRemove;    /*垂直纹理检测                     */
	HI_U32             u32TxThrd;		 /*垂直纹理检测阈值                 */
    HI_U32             u32SadThrd;       /*静止判决的SAD阈值，范围为(0~31)  */
    HI_U32             u32TimeThrd;      /*绝对静止判断次数阈值,范围为(1~15)*/
	
	VIDEO_FRAME_INFO_S stPreImg;         /*前一帧图像信息                   */
    VIDEO_FRAME_INFO_S stCurImg;         /*当前帧图像信息                   */
    VIDEO_FRAME_INFO_S stNxtImg;	     /*后一帧图像信息                   */
	
    void               (*pCallBack)(struct hiVPP_TASK_DATA_S *pstTask);

	

	/*下面数据有GROUP来单独填充,算法不用关注*/
	
	/************************************************************************/
	HI_U32             u32MsStPhyAddr;   /*静止次数数据存储首地址   */
    HI_U32             u32MsStStride;    /*存储静止信息数据的stride */
	/*OSD信息*/
    HI_U32             u32OsdRegNum;               /*OSD区域的数目*/
    VPP_OVERLAY_S      stOsdReg[MAX_OVERLAY_NUM];  /*OSD区域的属性   */
    VPP_ZMECFG_S       stZmeCfg;                   /*缩放系数     */

	/************************************************************************/
	HI_U32             privateData;
    HI_U32             reserved;  

	/*******算法用到的动静判决信息和纹理检测信息****************************/
	HI_U32             u32MsResult[VPP_MAX_MS_NUM];  /*动静判决结果*/
	HI_U32             u32GlbMotNum;    /*当前帧中运动的8x8块的数目*/


	/*调试接口*/
	HI_BOOL            bImgOutaBig;      /*VPP后大码流图像输出开关*/
    VIDEO_FRAME_INFO_S stOutImgBig;      /*VPP后大码流图像输出    */
    HI_BOOL            bImgOutLittle;    /*VPP后小码流图像输出开关*/
    VIDEO_FRAME_INFO_S stOutImgLittle;   /*VPP后小码流图像输出    */
	void               (*pDeIntCallBack)(struct hiVPP_DEINTERLACE_S *pstTask);
	
}VPP_TASK_DATA_S;





/* following four functions is for VENC */
typedef HI_S32 FN_VPP_GetOverlay(VENC_GRP VeGroup,HI_U32 *pu32RegionNum, 
									VPP_OVERLAY_S astRegions[MAX_OVERLAY_NUM]);
typedef HI_S32 FN_VPP_PutOverlay(VENC_GRP VeGroup);
typedef HI_S32 FN_VPP_InitVppConf(VENC_GRP VeGroup);
typedef HI_S32 FN_VPP_QueryVppConf(VENC_GRP VeGroup, VIDEO_PREPROC_CONF_S *pstConf);

/* following three functions is for PCIV */
typedef HI_S32 FN_VPP_GetSftOverlay(VI_DEV ViDev,VI_CHN ViChn,HI_U32 *pu32RegionNum, 
									VPP_SOVERLAY_S astRegions[MAX_SOVERLAY_NUM]);
typedef HI_S32 FN_VPP_PutSftOverlay(VI_DEV ViDev,VI_CHN ViChn);


/* following three functions is for VI */
typedef HI_S32 FN_VPP_GetMosaicPic(VI_DEV ViDev,VI_CHN ViChn,HI_U32 *pu32RegionNum,
					VPP_MOSAIC_REGION_S astRegions[MAX_MOSAIC_REGION_NUM]);
typedef HI_S32 FN_VPP_PutMosaicPic(VI_DEV ViDev,VI_CHN ViChn);


/*for MPGE4 or ...*/
typedef HI_S32 FN_VPP_CreateTask(const VPP_TASK_DATA_S *pstTask);

/*for VENC_GROUP or VPP_TASK*/
typedef HI_VOID FN_VPP_ConfigRegs(VPP_TASK_DATA_S *pstTask);
typedef HI_S32 FN_VPP_IntHandle(VPP_TASK_DATA_S *pstTask);

/*for  ...*/
typedef HI_S32 FN_VPP_CreateDeInterlaceTask(const VPP_DEINTERLACE_S *pstTask);

typedef HI_S32 FN_VPP_SendPic(const VIDEO_FRAME_INFO_S *pstPic);

typedef struct hiVPP_EXPORT_FUNC_S
{
    FN_VPP_GetOverlay   *pfnVppGetOverlay;    
    FN_VPP_PutOverlay   *pfnVppPutOverlay;
    FN_VPP_InitVppConf  *pfnVppInitVppConf;
    FN_VPP_QueryVppConf *pfnVppQueryVppConf;

	FN_VPP_GetSftOverlay *pfnVppGetSftOverlay;
	FN_VPP_PutSftOverlay *pfnVppPutSftOverlay;

	FN_VPP_GetMosaicPic *pfnVppGetMosaicPic;
	FN_VPP_PutMosaicPic *pfnVppPutMosaicpic;
	
	FN_VPP_CreateTask   *pfnVppCreateTask;
	FN_VPP_ConfigRegs   *pfnVppConfigRegs;
	FN_VPP_IntHandle    *pfnVppIntHandle;
	FN_VPP_CreateDeInterlaceTask *pfnVppCreateDeInterlaceTask;
	FN_VPP_SendPic      *pfnVppSendPic;
	
}VPP_EXPORT_FUNC_S;


#ifdef __cplusplus
    #if __cplusplus
    }
    #endif
#endif /* End of #ifdef __cplusplus */


#endif /* __VPP_EXT_H__ */

