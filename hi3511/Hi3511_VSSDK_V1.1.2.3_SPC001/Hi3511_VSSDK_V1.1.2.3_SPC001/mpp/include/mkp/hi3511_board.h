/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi3511_board.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/11/09
  Description   : 
  History       :
  1.Date        : 2006/11/09
    Author      : c42025
    Modification: Created file
    
  2.Date        : 2007/2/1
    Author      : c42025
    Modification: update all definition according Hi3511V100 ECS Manual

  3.Date        : 2007/12/26
    Author      : c42025
    Modification: update some definition according Hi3511V100 ECS Manual 1.61

  4.Date        : 2008/03/26
    Author      : c42025
    Modification: add function SYS_GetChipVersion

  5.Date        : 2008/06/02
    Author      : c42025
    Modification: add some macros for test performance
    
  6.Date        : 2008/06/14
    Author      : z50825
    Modification: fix test performance macro(sometimes may cause compile warning)

  7.Date        : 2009/01/14
    Author      : z44949
    Modification: Add the register address of DDRC
    
  8.Date        : 2009/01/14
    Author      : z44949
    Modification: Add the FN_SYS_Sync_PtsBase for PTS setting
******************************************************************************/
#ifndef __HI3511_BOARD_H__
#define __HI3511_BOARD_H__

#include <linux/list.h>

/***************************************/
#define DDRC_REG_ADDR	    0x10150000
#define DDRC_REGS_SIZE      0x1000

/***************************************/
#define SYS_REGS_ADDR   0x101E0000
#define SYS_REGS_SIZE   0x1000


/***************************************/
#define SIO0_REGS_ADDR   0x80080000
#define SIO1_REGS_ADDR   0x900a0000
#define SIO_REGS_SIZE   0x10000

/***************************************/
#define VOU_REGS_ADDR   0x90050000
#define VOU_REGS_SIZE   0x10000

/***************************************/
#define VIU_REGS_ADDR   0x90060000
#define VIU_REGS_SIZE   0x10000

/***************************************/
#define DSU_REGS_ADDR   0x90070000
#define DSU_REGS_SIZE   0x10000

/***************************************/
#define VEDU_REGS_ADDR   0x90080000
#define VEDU_REGS_SIZE   0x10000

/***************************************/
#define MDU_REGS_ADDR   0x90090000
#define MDU_REGS_SIZE   0x10000


/* Interrupt Request Number  */

#define DMA_IRQ_NR     17

#define VOU_IRQ_NR     16
#define VIU_IRQ_NR     25
#define DSU_IRQ_NR     26

#define SIO_1_IRQ_NR   18
#define SIO_0_IRQ_NR   27

#define VEDU_IRQ_NR    28
#define MDU_IRQ_NR     29

#define SYS_PERCTL0_ADDR (0x1C + SYS_REGS_ADDR)
#define SYS_PERCTL1_ADDR (0x20 + SYS_REGS_ADDR)
#define SYS_PERCTL2_ADDR (0x34 + SYS_REGS_ADDR)
#define SYS_PERCTL3_ADDR (0x38 + SYS_REGS_ADDR)
#define SYS_PERCTL4_ADDR (0x3C + SYS_REGS_ADDR)

/* DDRC QOS ADDR AND DEFAULT VALUE*/
#define DDRC_CH0_QOS_ADDR	(0x098 + DDRC_REG_ADDR) /* VO   */
#define DDRC_CH1_QOS_ADDR	(0x09C + DDRC_REG_ADDR) /* VI   */
#define DDRC_CH2_QOS_ADDR	(0x0A0 + DDRC_REG_ADDR) /* DMA  */
#define DDRC_CH3_QOS_ADDR	(0x0A4 + DDRC_REG_ADDR) /* EXP  */
#define DDRC_CH4_QOS_ADDR	(0x0A8 + DDRC_REG_ADDR) /* ARMD */
#define DDRC_CH5_QOS_ADDR	(0x0AC + DDRC_REG_ADDR) /* ARMI */
#define DDRC_CH6_QOS_ADDR	(0x0B0 + DDRC_REG_ADDR) /* DSU  */
#define DDRC_CH7_QOS_ADDR	(0x0B4 + DDRC_REG_ADDR) /* ?    */

#define DDRC_CH6_QOS_VALUE  (0x00010405) /* DSU TimeOut is 40 */
#define DDRC_CH2_QOS_VALUE  (0x00010702) /* DMA TimeOut is 70 */
#define DDRC_CH3_QOS_VALUE  (0x00010703) /* EXP TimeOut is 70 */



/* Get the version of local chip */
typedef HI_U32 FN_SYS_GetChipVersion(HI_VOID);

/* only VI_0 & VI_2 need call this function */
typedef HI_S32 FN_SYS_PinMuxViSync(HI_S32 s32ViDev);

/* you should call this function if you want VI work in 720P. */
typedef HI_S32 FN_SYS_PinMuxVi720p(HI_S32 s32ViDev);

/* All VI interfaces you want use need call this function. */
typedef HI_S32 FN_SYS_PinMuxViData(HI_S32 s32ViDev);

#define SYS_VI_DIV_SEL2 0x0000 /* 2 division  */
#define SYS_VI_DIV_SEL4 0x1000 /* 4 division  */
#define SYS_VI_DIV_SEL1 0x2000 /* no division */
typedef HI_S32 FN_SYS_ViDivSel(HI_S32 s32ViDev, HI_U32 u32DivSel);
typedef HI_S32 FN_SYS_ViClkSel(HI_S32 s32ViDev, HI_BOOL bSelf);


typedef HI_S32 FN_SYS_VouOutPhaseSel(HI_BOOL bPositive);

/* only SIO 0 need call the followin functions */
typedef HI_S32 FN_SYS_PinMuxSioSend(HI_S32 s32Sio, HI_BOOL bSend);
typedef HI_S32 FN_SYS_PinMuxSioXFS(HI_S32 s32Sio);

#define SYS_SIO_SAMPLE_CLK2   0x00 /* 2 division */
#define SYS_SIO_SAMPLE_CLK4   0x01 /* 4 division */
#define SYS_SIO_SAMPLE_CLK8   0x02 /* 8 division */
#define SYS_SIO_SAMPLE_CLK16  0x03 /* 16 division */
#define SYS_SIO_SAMPLE_CLK32  0x04 /* 32 division */
#define SYS_SIO_SAMPLE_CLK48  0x05 /* 48 division */
#define SYS_SIO_SAMPLE_CLK64  0x06 /* 64 division */
#define SYS_SIO_SAMPLE_CLKNO   0x07 /* no sample clock */
typedef HI_S32 FN_SYS_SioSampleClkDivSel(HI_U32 u32DivSel);

#define SYS_SIO_BS_CLK1   0x00 /* 1 division */
#define SYS_SIO_BS_CLK2   0x01 /* 2 division */
#define SYS_SIO_BS_CLK4   0x02 /* 4 division */
#define SYS_SIO_BS_CLK8   0x03 /* 8 division */
#define SYS_SIO_BS_CLK16  0x04 /* 16 division */
#define SYS_SIO_BS_CLK32  0x05 /* 32 division */
#define SYS_SIO_BS_CLKNO  0x07 /* no sample clock */
typedef HI_S32 FN_SYS_SioBitStreamClkDivSel(HI_U32 u32DivSel);

typedef HI_S32 FN_SYS_SioSysClkSel(HI_U32 u32Clk);

typedef HI_S32 FN_SYS_SioPCMModeSel(HI_S32 s32Sio,HI_BOOL bPCM);
typedef HI_S32 FN_SYS_SioClkSyncSel(HI_S32 s32Sio,HI_BOOL bCRG);

typedef HI_S32  FN_SYS_GetStride(HI_U32 u32Width, HI_U32 *pu32Stride);
/*
** u64Base is the global PTS of the system.
** ADVICE:
** 1. Bester to call pfnSysGetTspUs on the host board to get the u64Base.
** 2. When the linux start up, set bInit=HI_TRUE to set the init pts.
** 3. When media bussines is running, synchronize the PTS one time per minute.
**    And the bInit should be set as HI_FALSE.
*/
typedef HI_U32  FN_SYS_Sync_PtsBase(HI_U64 u64Base, HI_BOOL bInit);
typedef HI_U64  FN_SYS_GetTspUs(HI_VOID);
typedef HI_BOOL FN_SYS_IsInitialized(HI_VOID);

typedef HI_S32 FN_SYS_VeduSoftRst(HI_BOOL);

typedef struct hiSYS_EXPORT_FUNC_S
{
    FN_SYS_PinMuxViSync *pfnSysPinMuxViSync;
    FN_SYS_PinMuxVi720p *pfnSysPinMuxVi720p;
    FN_SYS_PinMuxViData *pfnSysPinMuxViData;    
    FN_SYS_ViDivSel     *pfnSysViDivSel;
    FN_SYS_ViClkSel     *pfnSysViClkSel;
    
    FN_SYS_VouOutPhaseSel *pfnSysVouOutPhaseSel;

    FN_SYS_PinMuxSioSend *pfnPinMuxSioSend;
    FN_SYS_PinMuxSioXFS  *pfnPinMuxSioXFS;
    
    FN_SYS_SioSampleClkDivSel    *pfnSysSioSampleClkDivSel;
    FN_SYS_SioBitStreamClkDivSel *pfnSysSioBitStreamClkDivSel;

    FN_SYS_SioSysClkSel  *pfnSysSioSysClkSel;
    FN_SYS_SioPCMModeSel *pfnSysSioPCMModeSel;
    FN_SYS_SioClkSyncSel *pfnSysSioClkSyncSel;

    FN_SYS_GetStride     *pfnSysGetStride;
    FN_SYS_Sync_PtsBase  *pfnSysSyncPtsBase;
    FN_SYS_GetTspUs      *pfnSysGetTspUs;
    FN_SYS_IsInitialized *pfnSysIsInitialized;

    FN_SYS_VeduSoftRst *pfnSysVeduSoftRst;

    FN_SYS_GetChipVersion *pfnSysGetChipVersion;
}SYS_EXPORT_FUNC_S;


#ifdef HI_BENCHMARK
#define TIMEVAL 64
extern struct list_head g_benchmark_list;

typedef struct hiBENCHMARK_ITEM_S
{
	HI_U8  u8Name[64];
	HI_U32 u32CallTimes;
	HI_U32 u32Intval;
	struct list_head list;
}BENCHMARK_ITEM_S;

#define DECLARE_BENCHMARK_ITEM(name)\
	BENCHMARK_ITEM_S g_##name;\
	static struct timeval t##name,prev##name,cur##name;\
	static HI_U32 time##name,count##name;\
	EXPORT_SYMBOL(g_##name);
	
#define INIT_BENCHMARK_ITEM(name)\
do{\
    struct list_head *pos;\
    HI_BOOL bFind = HI_FALSE;\
    BENCHMARK_ITEM_S* pNode;\
    list_for_each(pos,&g_benchmark_list)\
    {\
    	pNode = list_entry(pos,BENCHMARK_ITEM_S,list);\
		if(0 == strcmp(pNode->u8Name,(g_##name).u8Name) )\
		{\
			bFind = HI_TRUE;\
		}\
    }\
    if(!bFind)\
    {\
		strncpy((g_##name).u8Name,#name,sizeof((g_##name).u8Name));\
		list_add_tail((&(g_##name).list),&g_benchmark_list);\
    }\
}while(0)

#define BEGIN_BENCHMARK(name)\
	do_gettimeofday(&(t##name));

#define END_BENCHMARK(name)\
do{\
	do_gettimeofday(&(cur##name));\
	(count##name) ++;\
	(time##name) += ((cur##name).tv_sec - (t##name).tv_sec)*1000000ul + (cur##name).tv_usec - (t##name).tv_usec;\
	if( ((cur##name).tv_sec - (prev##name).tv_sec)*1000000ul + (cur##name).tv_usec - (prev##name).tv_usec >= TIMEVAL*1000000ul )\
	{\
	    (g_##name).u32CallTimes = (count##name);\
	    (g_##name).u32Intval   =  (time##name);\
	    (count##name) = 0;(time##name) = 0;\
        (prev##name) = (cur##name);\
	}\
}while(0)

#else

#define DECLARE_BENCHMARK_ITEM(name)
#define INIT_BENCHMARK_ITEM(name)
#define BEGIN_BENCHMARK(name)
#define END_BENCHMARK(name)

#endif 




#endif /* __HI3511_BOARD_H__ */

