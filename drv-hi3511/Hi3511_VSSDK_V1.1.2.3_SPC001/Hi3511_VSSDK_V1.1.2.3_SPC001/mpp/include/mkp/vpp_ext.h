/******************************************************************************

                  ��Ȩ���� (C), 2001-2011, ��Ϊ�������޹�˾

 ******************************************************************************
  �� �� ��   : vpp_ext.h
  �� �� ��   : ����
  ��    ��   : ����� 64467
  ��������   : 2007��10��26��
  ����޸�   :
  ��������   : 
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2007��10��26��
    ��    ��   : ����� 64467
    �޸�����   : �����ļ�
  2.��    ��   : 2008��08��20��
    ��    ��   : ����� 64467
    �޸�����   : ���ӵ�����DE_INTERLACE�ӿ�
  3.��    ��   : 2009/01/18
    ��    ��   : z44949
    �޸�����   : ���Ӷ�VPP�������Գ�ʼ���Ľӿ�

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
    HI_BOOL       bPixCutEn; /*���Ŷ���ʹ��*/
    HI_BOOL       bHzMen;    /*ˮƽ����ʹ��*/
    HI_BOOL       bVzMen;    /*��ֱ����ʹ��*/
    HI_U32        u32VhRint; /*ˮƽ���ű���*/
    HI_U32        u32WhRint; /*��ֱ���ű���*/
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
	HI_BOOL            bDiTp;          /*�������ݸ�ʽ��1��422��0��420*/
	
	VIDEO_FRAME_INFO_S stPreImg;       /*ǰһ֡ͼ����Ϣ*/
    VIDEO_FRAME_INFO_S stCurImg;       /*��ǰ֡ͼ����Ϣ*/
    VIDEO_FRAME_INFO_S stNxtImg;	   /*��һ֡ͼ����Ϣ*/
	
	HI_U32             u32MsStPhyAddr; /*��ֹ�������ݴ洢�׵�ַ   */
    HI_U32             u32MsStStride;  /*�洢��ֹ��Ϣ���ݵ�stride */

	HI_VOID            (*pCallBack)(struct hiVPP_DEINTERLACE_S *pstTask);
    VIDEO_FRAME_INFO_S stOutPic;      /*VPP��ͼ�����*/
	
	HI_U32             privateData;
    HI_U32             reserved;  

}VPP_DEINTERLACE_S;




typedef struct hiVPP_TASK_DATA_S
{
    struct list_head    list;

	/************************************************************************
	 *ע��:
	 *�ظ�����ģʽ�£������о�ms_en�����
	 *de-interlaceģʽ�£���ֵ������ʹ��
	 ***********************************************************************/
	 
    /***********************************************************************
     *оƬ�ṩ���Ƽ�����ģʽ:
     *DIE����ģʽ          die_en  tf_en  ms_en  edge_en  med_en  rept_enc_ind
     *bypassģʽ             0       0      0       0       0        0
     *dintlģʽ              1       0      1       1       1        0
     *bypass+�����о�ģʽ    0       0      1       0       0        0
     *dintl+ʱ���˲�ģʽ     1       1      1       1       1        0
     *bypass+ʱ���˲�ģʽ    0       1      1       0       0        0
     *�ظ�����+bypassģʽ    0       x      1       0       0        1
     *�ظ�����+dintlģʽ     1       x      1       1       1        1
     ***********************************************************************/
	 
    HI_BOOL	           bFieldMode;       /*֡����ʶ,֡:HI_FALSE;��:HI_TRUE  */
    HI_BOOL	           bTopField;        /*���׳�,����:HI_TRUE�׳�:HI_FALSE */	
	HI_BOOL            bReptEncInd;      /*�ظ�����,1:�ظ�����,0:���ظ����� */
	HI_BOOL            bDiTp;            /*�������ݸ�ʽ��1��422��0��420     */
    HI_BOOL            bDie;             /*de-interalceʹ��ָʾ             */
    HI_BOOL            bMs;              /*��鶯���о�ʹ��ָʾ             */
    HI_BOOL            bTf;              /*ʱ���˲�ʹ��ָʾ                 */
    HI_BOOL            bMed;             /*��ֵ���ʹ��ָʾ                 */
    HI_BOOL            bEdge;            /*��Ե���ʹ��ָʾ                 */
	HI_BOOL            bTxtureRemove;    /*��ֱ������                     */
	HI_U32             u32TxThrd;		 /*��ֱ��������ֵ                 */
    HI_U32             u32SadThrd;       /*��ֹ�о���SAD��ֵ����ΧΪ(0~31)  */
    HI_U32             u32TimeThrd;      /*���Ծ�ֹ�жϴ�����ֵ,��ΧΪ(1~15)*/
	
	VIDEO_FRAME_INFO_S stPreImg;         /*ǰһ֡ͼ����Ϣ                   */
    VIDEO_FRAME_INFO_S stCurImg;         /*��ǰ֡ͼ����Ϣ                   */
    VIDEO_FRAME_INFO_S stNxtImg;	     /*��һ֡ͼ����Ϣ                   */
	
    void               (*pCallBack)(struct hiVPP_TASK_DATA_S *pstTask);

	

	/*����������GROUP���������,�㷨���ù�ע*/
	
	/************************************************************************/
	HI_U32             u32MsStPhyAddr;   /*��ֹ�������ݴ洢�׵�ַ   */
    HI_U32             u32MsStStride;    /*�洢��ֹ��Ϣ���ݵ�stride */
	/*OSD��Ϣ*/
    HI_U32             u32OsdRegNum;               /*OSD�������Ŀ*/
    VPP_OVERLAY_S      stOsdReg[MAX_OVERLAY_NUM];  /*OSD���������   */
    VPP_ZMECFG_S       stZmeCfg;                   /*����ϵ��     */

	/************************************************************************/
	HI_U32             privateData;
    HI_U32             reserved;  

	/*******�㷨�õ��Ķ����о���Ϣ����������Ϣ****************************/
	HI_U32             u32MsResult[VPP_MAX_MS_NUM];  /*�����о����*/
	HI_U32             u32GlbMotNum;    /*��ǰ֡���˶���8x8�����Ŀ*/


	/*���Խӿ�*/
	HI_BOOL            bImgOutaBig;      /*VPP�������ͼ���������*/
    VIDEO_FRAME_INFO_S stOutImgBig;      /*VPP�������ͼ�����    */
    HI_BOOL            bImgOutLittle;    /*VPP��С����ͼ���������*/
    VIDEO_FRAME_INFO_S stOutImgLittle;   /*VPP��С����ͼ�����    */
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

