
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpp.h"
#include "hi_comm_venc.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vpp.h"
#include "mpi_venc.h"
#include "tw2815.h"
#include "adv7179.h"

typedef enum hiSAMPLE_VO_DIV_MODE
{
    DIV_MODE_1 = 0,
    DIV_MODE_4,
    DIV_MODE_9,
    DIV_MODE_BUTT
} SAMPLE_VO_DIV_MODE;

typedef enum hiSAMPLE_VI_SIZE
{
    VI_SIZE_D1 = 0,
    VI_SIZE_CIF,
    VI_SIZE_HD1,
    VI_SIZE_2CIF,
    VI_SIZE_BUTT
} SAMPLE_VI_SIZE;


#define TW2815_A_FILE   "/dev/misc/tw2815adev"
#define TW2815_B_FILE   "/dev/misc/tw2815bdev"

/*VIDEO_ENCODING_MODE_NTSC or VIDEO_ENCODING_MODE_PAL*/
VIDEO_NORM_E s_enVideoNorm = VIDEO_ENCODING_MODE_PAL;


static HI_S32 SAMPLE_TW2815_CfgV(VI_WORK_MODE_E enWorkMode)
{
    HI_S32 i, j, s32Ret;    
    HI_S32 fd2815a, fd2815b;
    HI_S32 fd2815[2];
    HI_S32 s32VideoMode;
    tw2815_set_videomode stNorm;
        
    fd2815a = open(TW2815_A_FILE,O_RDWR);
    fd2815b = open(TW2815_B_FILE,O_RDWR);
    if (fd2815b < 0 || fd2815a < 0)
    {
        printf("can't open tw2815,fd(%d,%d)\n", fd2815a, fd2815b);
        return HI_FAILURE;
    }
    
    fd2815[0] = fd2815a;
    fd2815[1] = fd2815b;

    s32VideoMode = (VIDEO_ENCODING_MODE_PAL == s_enVideoNorm)?2:1;
    
    for (i=0; i<2; i++)
    {
        for (j=0; j<4; j++)
        {
            stNorm.ch = j;
            stNorm.mode = s32VideoMode; 
            s32Ret = ioctl(fd2815[i],TW2815_SET_VIDEO_MODE,&stNorm);
            if (0 != s32Ret)
            {
                printf("set 2815(%d) video mode fail;err:%x!\n", i, s32Ret);
                return s32Ret;
            }
        }  
        
        if (VI_WORK_MODE_2D1 == enWorkMode)
        {
            tw2815_set_2d1 tw2815set2d1;
            tw2815set2d1.ch1 = 0;
            tw2815set2d1.ch2 = 2;
            s32Ret = ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1);
            if (0 != s32Ret)
            {
                printf("set 2815(%d) 2D1 fail,chn:0,2;err:%x!\n", i, s32Ret);
                return s32Ret;
            }
            
            tw2815set2d1.ch1 = 1;
            tw2815set2d1.ch2 = 3;
            s32Ret = ioctl(fd2815[i],TW2815_SET_2_D1,&tw2815set2d1);
            if (0 != s32Ret)
            {
                printf("set 2815(%d) 2D1 fail,chn:1,3;err:%x!\n", i, s32Ret);
                return s32Ret;
            }            
        }    
        else if (VI_WORK_MODE_4HALFD1 == enWorkMode)
        {
            int tmp = 0;
            s32Ret = ioctl(fd2815[i],TW2815_SET_4HALF_D1,&tmp);
            if (0 != s32Ret)
            {
                printf("set 2815(%d) 4 halfD1 fail;err:%x!\n", i, s32Ret);
                return s32Ret;
            }
        }
    }

    close(fd2815a);
    close(fd2815b);
        
    return HI_SUCCESS;
}


static HI_S32 SAMPLE_7179_CfgV()
{
    int fd;
    
    fd = open("/dev/misc/adv7179",O_RDWR);
    if (fd < 0)
    {
        printf("can't open 7179\n");
        return HI_FAILURE;
    }    

    if(s_enVideoNorm == VIDEO_ENCODING_MODE_PAL)
    {
        if(0!=ioctl(fd,ENCODER_SET_NORM,VIDEO_MODE_656_PAL))
        {
            printf("7179 errno %d\r\n",errno);
            return HI_FAILURE;
        }
    }
    else if(s_enVideoNorm == VIDEO_ENCODING_MODE_NTSC)
    {
        if(0!=ioctl(fd,ENCODER_SET_NORM,VIDEO_MODE_656_NTSC))
        {
            printf("7179 errno %d\r\n",errno);
            return HI_FAILURE;
        }
    }
    
    close(fd);
    return HI_SUCCESS;
}

static HI_S32 SAMPLE_InitMPP(VB_CONF_S *pstVbConf)
{
    HI_S32 s32ret;    
    MPP_SYS_CONF_S struSysConf = {0};
    
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();           

    s32ret = HI_MPI_VB_SetConf(pstVbConf);
    if (s32ret)
    {
        printf("HI_MPI_VB_SetConf ERR 0x%x\n",s32ret);
        return s32ret;
    }
    s32ret = HI_MPI_VB_Init();
    if (s32ret)
    {
        printf("HI_MPI_VB_Init ERR 0x%x\n", s32ret);
        return s32ret;
    }

    struSysConf.u32AlignWidth = 64;
    s32ret = HI_MPI_SYS_SetConf(&struSysConf);
    if (HI_SUCCESS != s32ret)
    {
        printf("Set mpi sys config failed!\n");
        return s32ret;
    }
    s32ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != s32ret)
    {
        printf("Mpi init failed!\n");
        return s32ret;
    }
    
    return HI_SUCCESS;
}

static HI_S32 SAMPLE_ExitMPP()
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    return 0;
}

/* this function only for VI BT.656 2D1 mode */
int SAMPLE_ViInit(HI_U32 u32ChnCnt, SAMPLE_VI_SIZE ViSize)
{
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_S32 s32Ret, s32DevCnt, s32ChnPDev, i, j, k;
    VI_PUB_ATTR_S stDevAttr;
    VI_CHN_ATTR_S stChnAttr;
    
    s32ChnPDev = 2;
    s32DevCnt  = (u32ChnCnt-1)/s32ChnPDev + 1;

    stDevAttr.enInputMode = VI_MODE_BT656;
    stDevAttr.enWorkMode = VI_WORK_MODE_2D1;
    stDevAttr.enViNorm = s_enVideoNorm;  

    stChnAttr.stCapRect.u32Width = 704;
    stChnAttr.stCapRect.u32Height = ((0 == s_enVideoNorm)?288:240);    
    stChnAttr.stCapRect.s32X = 8;
    stChnAttr.stCapRect.s32Y = 0;
    stChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    stChnAttr.bHighPri = HI_FALSE;
    stChnAttr.bChromaResample = HI_FALSE;    
    stChnAttr.enCapSel = (VI_SIZE_D1==ViSize||VI_SIZE_2CIF==ViSize)?\
            VI_CAPSEL_BOTH:VI_CAPSEL_BOTTOM;
    stChnAttr.bDownScale = (VI_SIZE_D1==ViSize||VI_SIZE_HD1==ViSize)?\
            HI_FALSE:HI_TRUE;
    k = 0;
    
    for (i=0; i<s32DevCnt; i++)
    {
        ViDev = i;
        if (0 != HI_MPI_VI_SetPubAttr(ViDev, &stDevAttr))
        {
            printf("set vi dev %d attr fail,%x\n", ViDev, s32Ret);
            return -1;       
        }
        if (0 != HI_MPI_VI_Enable(ViDev))
        {
            printf("VI dev %d enable fail \n",ViDev);
            return -1;
        }  
        printf("enable vi dev %d ok\n", ViDev);
        
        for (j=0; (j<s32ChnPDev)&&(k<u32ChnCnt); j++,k++)
        {
            ViChn = j;
            s32Ret = HI_MPI_VI_SetChnAttr(ViDev, ViChn, &stChnAttr);
            if (s32Ret)
            {
                printf("set vi(%d,%d) attr fail,%x\n", ViDev, ViChn, s32Ret);
                return -1;       
            }
            s32Ret = HI_MPI_VI_EnableChn(ViDev, ViChn);
            if (s32Ret)
            {
                printf("set vi(%d,%d) attr fail,%x\n", ViDev, ViChn, s32Ret);
                return -1;       
            }
            if (HI_MPI_VI_SetSrcFrameRate(ViDev, ViChn, (0 == s_enVideoNorm)?25:30))
            {
                return HI_FAILURE;
            }
            printf("enable vi(%d,%d) ok\n", ViDev, ViChn);
        }
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_BindVi2Vo(HI_U32 u32ChnCnt)
{
    HI_S32 s32Ret, i;
    VI_DEV ViDevId;
    VI_CHN ViChn;
    VO_CHN VoChn;
    
    for (i=0; i<u32ChnCnt; i++)
    {
        ViDevId = i/2;
        ViChn = i%2;
        VoChn = i;
        s32Ret = HI_MPI_VI_BindOutput(ViDevId, ViChn, VoChn);
        if (0 != s32Ret)
        {
            printf("bind vi2vo failed, vi(%d,%d),vo:%d!\n", ViDevId, ViChn, VoChn);
            return s32Ret;     
        }
    }
    
    return 0;
}

HI_S32 SAMPLE_VoSwitch(HI_S32 s32VoDivMode, VO_CHN aVoChn[], HI_S32 s32VoCnt)
{
    HI_S32 i,div,u32ScreemDiv,u32PicWidth,u32PicHeight;
    VO_CHN VoChn;
    VO_CHN_ATTR_S VoChnAttr;
    
    u32ScreemDiv = s32VoDivMode;
    
    if (0 != HI_MPI_VO_SetAttrBegin())
    {
        return -1;
    }

    for (i=0; i<VO_MAX_CHN_NUM; i++)
    {
        HI_MPI_VO_ChnHide(i);
    }
    
    div = sqrt(u32ScreemDiv);
    u32PicWidth = 720/div;
    u32PicHeight = ((VIDEO_ENCODING_MODE_PAL == s_enVideoNorm)?576:480)/div;

    for (i=0; i<s32VoCnt; i++)
    {
        VoChn = aVoChn[i];
        VoChnAttr.stRect.s32X = (i%div)*u32PicWidth;
        VoChnAttr.stRect.s32Y = (i/div)*u32PicHeight;
        VoChnAttr.stRect.u32Width = u32PicWidth;
        VoChnAttr.stRect.u32Height = u32PicHeight;
        VoChnAttr.u32Priority = 1;
        VoChnAttr.bZoomEnable = HI_TRUE;
        if (0 != HI_MPI_VO_SetChnAttr(VoChn,&VoChnAttr))
        {
            printf("set VO Chn %d attribute(%d,%d,%d,%d) failed !\n",
                VoChn,VoChnAttr.stRect.s32X,VoChnAttr.stRect.s32Y,
                VoChnAttr.stRect.u32Width,VoChnAttr.stRect.u32Height);
            return -1;
        }
        if (0 != HI_MPI_VO_ChnShow(VoChn))
        {
            return -1;
        }
    }

    if (0 != HI_MPI_VO_SetAttrEnd())
    {
        return -1;
    }
    
    return 0;
}

HI_S32 SAMPLE_VoSwitchEx(SAMPLE_VO_DIV_MODE enVoDivMode)
{
    HI_S32 i, s32VoCnt, s32VoDivMode;
    VO_CHN aVoChn[VO_MAX_CHN_NUM];
    
    if (DIV_MODE_1 == enVoDivMode)
    {        
        s32VoCnt = 1;
        s32VoDivMode = 1;
        aVoChn[0] = 0;
    }
    else if (DIV_MODE_4 == enVoDivMode)
    {
        s32VoCnt = 4;
        s32VoDivMode = 4;
        for (i=0; i<s32VoCnt; i++)
        {
            aVoChn[i] = i;
        }
    }
    else if (DIV_MODE_9 == enVoDivMode)
    {
        s32VoCnt = 9;
        s32VoDivMode= 9;
        for (i=0; i<s32VoCnt; i++)
        {
            aVoChn[i] = i;
        }
    }
    else
    {
        printf("not support vo div mode %d\n", enVoDivMode);
        return -1;
    }
    
    if (SAMPLE_VoSwitch(s32VoDivMode, aVoChn, s32VoCnt))
    {
        return -1;
    }
    return 0;
}

HI_S32 SAMPLE_VoInit(HI_S32 s32VoCnt)
{
    HI_S32 i, div, u32PicWidth, u32PicHeight;
    VO_CHN VoChn;
    VO_PUB_ATTR_S VoAttr;
    VO_CHN_ATTR_S VoChnAttr;
    VO_CHN aVoChn[] = {0,1,2,3,4,5,6,7,8}; 
    
    /* enable VO device */
    VoAttr.u32BgColor = 0xff0000;   
    VoAttr.stTvConfig.stComposeMode = s_enVideoNorm;  
    if (0 != HI_MPI_VO_SetPubAttr(&VoAttr))
    {
        printf("set VO dev attribute failed !\n");
        return -1;     
    }
    if (0 != HI_MPI_VO_Enable())
    {
        printf("enable VO device failed !\n");
        return -1;
    }
    
    div = sqrt(s32VoCnt);
    u32PicWidth = 720/div;
    u32PicHeight = ((VIDEO_ENCODING_MODE_PAL == s_enVideoNorm)?576:480)/div;
    
    for (i=0; i<s32VoCnt; i++)
    {
        VoChn = aVoChn[i];
        VoChnAttr.stRect.s32X = (i%div)*u32PicWidth;
        VoChnAttr.stRect.s32Y = (i/div)*u32PicHeight;
        VoChnAttr.stRect.u32Width = u32PicWidth;
        VoChnAttr.stRect.u32Height = u32PicHeight;
        VoChnAttr.u32Priority = 1;
        VoChnAttr.bZoomEnable = HI_TRUE;
        if (0 != HI_MPI_VO_SetChnAttr(VoChn,&VoChnAttr))
        {
            printf("set VO Chn %d attribute(%d,%d,%d,%d) failed !\n",
                VoChn,VoChnAttr.stRect.s32X,VoChnAttr.stRect.s32Y,
                VoChnAttr.stRect.u32Width,VoChnAttr.stRect.u32Height);
            return -1;
        }
        if (0 != HI_MPI_VO_EnableChn(VoChn))
        {
            return -1;
        }
    }
    
    return 0;
}

static HI_S32 SAMPLE_Vi720P( )
{
    HI_S32 s32Ret;
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;
	VI_PUB_ATTR_S stDevAttr;
	VI_CHN_ATTR_S stChnAttr;
        
	stDevAttr.enInputMode = VI_MODE_720P;/*720P*/
    
	stChnAttr.stCapRect.s32X = 0;
	stChnAttr.stCapRect.s32Y = 0;
    stChnAttr.stCapRect.u32Width = 1280;
	stChnAttr.stCapRect.u32Height = 720;	
	    
    stChnAttr.enCapSel = VI_CAPSEL_BOTH;
	stChnAttr.bDownScale = HI_FALSE;
    stChnAttr.bHighPri = HI_FALSE;
	stChnAttr.bChromaResample = HI_FALSE;
	stChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    /* enable VI device*/
    s32Ret = HI_MPI_VI_SetPubAttr(ViDev, &stDevAttr);
    if (s32Ret)
    {
        printf("VI dev %d set attr fail \n", ViDev);
        return s32Ret;
    }
    s32Ret = HI_MPI_VI_Enable(ViDev);
    if (s32Ret)
    {
        printf("VI device %d enable fail \n",ViDev);
        return s32Ret;
    }        
    /* enable VI channle*/
    s32Ret = HI_MPI_VI_SetChnAttr(ViDev, ViChn, &stChnAttr);
    if (s32Ret)
    {
        printf("VI chn (%d,0) set attr fail \n", ViDev);
        return s32Ret;
    }                                  
    s32Ret = HI_MPI_VI_EnableChn(ViDev, ViChn);
    if (s32Ret)
    {
        printf("VI chn (%d,0) enable fail \n", ViDev);
        return s32Ret;
    }
    
	return 0;
}

HI_S32 SAMPLE_VIO_4HD1Mode()
{
    HI_S32 s32Ret, i, j;
    VB_CONF_S stVbConf = {0};
    VI_PUB_ATTR_S ViAttr;
    VI_CHN_ATTR_S ViChnAttr;
    
    SAMPLE_TW2815_CfgV(VI_WORK_MODE_4HALFD1); 
    SAMPLE_7179_CfgV();

    stVbConf.astCommPool[0].u32BlkSize = 704*576*2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt = 8;
    stVbConf.astCommPool[1].u32BlkSize = 384*576*2;/*2CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 32;                
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    
    /* start VO 9 screem */
    s32Ret = SAMPLE_VoInit(9);
    if (s32Ret)
    {
        return s32Ret;
    }
    
    for (i=0; i<4; i+=2)
    {
        /* enable VI device 0 and 2 */
        ViAttr.enInputMode = VI_MODE_BT656;    
        ViAttr.enWorkMode = VI_WORK_MODE_4HALFD1;
	    ViAttr.enViNorm = s_enVideoNorm;
        s32Ret = HI_MPI_VI_SetPubAttr(i, &ViAttr);
        if (s32Ret){
            printf("set vi dev attr fail,%x\n", s32Ret);
            return s32Ret;
        }
        s32Ret = HI_MPI_VI_Enable(i);
        if (s32Ret){
            printf("enable vi fail,%x\n", s32Ret);
            return s32Ret; 
        }
        /* enable VI channle */
        for (j=0; j<4; j++)
        {
            ViChnAttr.stCapRect.u32Width = 352;/* width of original pic is 352, not 720*/
            ViChnAttr.stCapRect.u32Height = (VIDEO_ENCODING_MODE_PAL == s_enVideoNorm)?288:240;	 
            ViChnAttr.stCapRect.s32X = 8;
            ViChnAttr.stCapRect.s32Y = 0;
            ViChnAttr.enCapSel = VI_CAPSEL_BOTTOM;/*you can select one or both field*/
            ViChnAttr.bDownScale = HI_FALSE;/* should not down scale */
            ViChnAttr.bChromaResample = HI_FALSE;
            ViChnAttr.bHighPri = HI_FALSE;
            ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
            s32Ret = HI_MPI_VI_SetChnAttr(i,j,&ViChnAttr);
            if (s32Ret){
                printf("set vi chn (%d,%d)attr fail,%x\n",i,j, s32Ret);
                return s32Ret; 
            }            
            s32Ret = HI_MPI_VI_EnableChn(i,j);
            if (s32Ret){
                printf("enable vi chn fail,%x\n", s32Ret);
                return s32Ret;  
            }
            s32Ret = HI_MPI_VI_BindOutput(i,j,2*i+j);
            if (s32Ret){
                printf("bind vo chn fail,%x\n", s32Ret);
                return s32Ret;  
            }
        }
    }    
    
    return 0;
}

HI_S32 SAMPLE_VIO_720P()
{
    HI_S32 s32Ret;
    VB_CONF_S stVbConf = {0};

    SAMPLE_TW2815_CfgV(VI_WORK_MODE_2D1);
    SAMPLE_7179_CfgV();
    
    stVbConf.astCommPool[0].u32BlkSize = 1280*720*2;/*720P*/
    stVbConf.astCommPool[0].u32BlkCnt = 6; 
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
            
    s32Ret = SAMPLE_Vi720P();
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    s32Ret = SAMPLE_VoInit(1);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    s32Ret = SAMPLE_BindVi2Vo(1);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    
    return 0;
}

HI_S32 SAMPLE_VIO()
{
    HI_CHAR ch;
    HI_S32 i = 0;
    VB_CONF_S stVbConf = {0};
    SAMPLE_VO_DIV_MODE enVoDivMode = 0;    
    HI_S32 s32ViCnt = 8;
    HI_S32 s32VoCnt = 9;
    
    SAMPLE_TW2815_CfgV(VI_WORK_MODE_2D1);
    SAMPLE_7179_CfgV();
    
    stVbConf.astCommPool[0].u32BlkSize = 768*288*2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt = 40;
    stVbConf.astCommPool[1].u32BlkSize = 384*288*2;/*CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 48;   
    if (0 != SAMPLE_InitMPP(&stVbConf))
    {
        return -1;
    }
    
    if (SAMPLE_ViInit(s32ViCnt, VI_SIZE_CIF))
    {
        return -1;
    }
    
    if (0 != SAMPLE_BindVi2Vo(s32ViCnt))
    {
        return -1;
    }
    
    if (0 != SAMPLE_VoInit(s32VoCnt))
    {
        return -1;
    }
    
    printf("press 'q' to exit sample !\npress ENTER to switch vo\n");
    while ((ch = getchar()) != 'q')
    {        
        
        enVoDivMode = (i++)%DIV_MODE_BUTT;
        printf("enVoDivMode: %d \n", enVoDivMode);
        if (SAMPLE_VoSwitchEx(enVoDivMode))
        {
            break;
        }
    }
    return 0;
}

/*  fps of VI main chn is s32MainFrmRate, 
    fps of VI minor chn is (s32SrcFrmRate - s32MainFrmRate);
    if you need not minor chn, set enMinorSize same as enMainSize;
    should not set all 8 chn main=D1,minor=D1, otherwise performance lowly*/
HI_S32 SampleViMixMode(VI_DEV ViDev, VI_CHN ViChn, 
    SAMPLE_VI_SIZE enMainSize,SAMPLE_VI_SIZE enMinorSize,HI_S32 s32MainFrmRate)
{
    VI_CHN_ATTR_S stChnAttr;
    
    stChnAttr.stCapRect.u32Width = 704;
    stChnAttr.stCapRect.u32Height = ((0 == s_enVideoNorm)?288:240);    
    stChnAttr.stCapRect.s32X = 0;
    stChnAttr.stCapRect.s32Y = 0;
    stChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    stChnAttr.bHighPri = HI_FALSE;
    stChnAttr.bChromaResample = HI_FALSE;    
    stChnAttr.enCapSel = (VI_SIZE_D1==enMainSize||VI_SIZE_2CIF==enMainSize)?\
            VI_CAPSEL_BOTH:VI_CAPSEL_BOTTOM;
    stChnAttr.bDownScale = (VI_SIZE_D1==enMainSize||VI_SIZE_HD1==enMainSize)?\
            HI_FALSE:HI_TRUE;
    /* set main chn attr*/
    if (HI_MPI_VI_SetChnAttr(ViDev, ViChn, &stChnAttr))
    {
        return HI_FAILURE;
    }
    stChnAttr.enCapSel = (VI_SIZE_D1==enMinorSize||VI_SIZE_2CIF==enMinorSize)?\
            VI_CAPSEL_BOTH:VI_CAPSEL_BOTTOM;
    stChnAttr.bDownScale = (VI_SIZE_D1==enMinorSize||VI_SIZE_HD1==enMinorSize)?\
            HI_FALSE:HI_TRUE;
    /* set minor chn attr*/
    if (HI_MPI_VI_SetChnMinorAttr(ViDev, ViChn, &stChnAttr))
    {
        return HI_FAILURE;
    }
    
    /* set framerate of main chn */
    if (HI_MPI_VI_SetFrameRate(ViDev, ViChn, s32MainFrmRate))
    {
        return HI_FAILURE;
    }

    /* if main is both filed, minor single filed, 
       must display only bottom field, otherwise pic may dithering*/
    if ((VI_SIZE_CIF==enMinorSize||VI_SIZE_HD1==enMinorSize)
        &&(VI_SIZE_D1==enMainSize||VI_SIZE_2CIF==enMainSize))
    {
        VO_CHN BindVo = ViDev*2+ViChn;        
        if (HI_MPI_VO_SetChnField(BindVo, VO_FIELD_BOTTOM))
        {
            return HI_FAILURE;
        }
    }
    
    return HI_SUCCESS;
}

/* ---------------------------------------------------------------------------*/
static HI_BOOL bStart;
static HI_S32 s_s32ViSrcFrmRate;    /* SRC frame rate of VI,should equal to AD*/
static HI_S32 s_s32ViMainFrmRate;   /* VI main chn frame rate */
static pthread_t Pid;

HI_S32 SAMPLE_VencSaveStream(FILE* fpH264File, VENC_STREAM_S *pstStream)
{
	HI_S32 i;
	
	for (i=0; i< pstStream->u32PackCount; i++)
	{
		fwrite(pstStream->pstPack[i].pu8Addr[0], 
				pstStream->pstPack[i].u32Len[0], 1, fpH264File);		
		fflush(fpH264File);
		if (pstStream->pstPack[i].u32Len[1] > 0)
		{

			fwrite(pstStream->pstPack[i].pu8Addr[1], 
					pstStream->pstPack[i].u32Len[1], 1, fpH264File);			
			fflush(fpH264File);
		}	
	}
	return HI_SUCCESS;
}

/* get stream of one Venc chn */
HI_VOID* SAMPLE_VencGetStream(HI_VOID *p)
{
	HI_S32 s32Ret, i, s32ChnCnt, max;
	HI_S32 VencFd[32];
    FILE *pFile[32]  = {0};
    struct timeval TimeoutVal;
	VENC_CHN VeChn;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;	
    HI_CHAR aszFileName[32] = {0};
    
	s32ChnCnt = (int)p;
    printf("venc chn cnt is %d\n", s32ChnCnt);
    
    FD_ZERO(&read_fds);
	for (i=0; i<s32ChnCnt; i++)
	{
        sprintf(aszFileName,"chn%d.h264",i);
        pFile[i] = fopen(aszFileName, "wb");
        if (!pFile[i]) 
        {
            perror("fopen fail");
    		return NULL;
        }
		VencFd[i] = HI_MPI_VENC_GetFd(i);
		FD_SET(VencFd[i],&read_fds);
		if(max <= VencFd[i]) max = VencFd[i] ;
	}

    while(bStart)
	{
        TimeoutVal.tv_sec = 5;
		TimeoutVal.tv_usec = 0;
        
		FD_ZERO(&read_fds);
		for (i=0; i<s32ChnCnt; i++)
		{
			FD_SET(VencFd[i], &read_fds);
		}
        
		s32Ret = select(max+1, &read_fds, NULL, NULL, &TimeoutVal);		
		if (s32Ret < 0) 
		{
			return NULL;
		}
		else if (0 == s32Ret) 
		{
			printf("time out\n");
			return NULL;
		}
        for (i=0; i<s32ChnCnt;i++)
        {
            VeChn = i;
            if (FD_ISSET(VencFd[VeChn], &read_fds))
    		{
    			s32Ret = HI_MPI_VENC_Query(VeChn, &stStat);				
    			if (s32Ret != HI_SUCCESS) 
    			{
    				printf("HI_MPI_VENC_Query:0x%x err\n",s32Ret);
    				fflush(stdout);
    				return NULL;
    			}
    			
    			stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);
    			if (NULL == stStream.pstPack)  
    			{
    				printf("malloc memory err!\n");
    				return NULL;
    			}
    			
    			stStream.u32PackCount = stStat.u32CurPacks;    			
    			s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_TRUE);
    			if (HI_SUCCESS != s32Ret) 
    			{
    				printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
    				free(stStream.pstPack);
    				stStream.pstPack = NULL;
    				return NULL;
    			}
    			
    			SAMPLE_VencSaveStream(pFile[i], &stStream);
    			
    			s32Ret = HI_MPI_VENC_ReleaseStream(VeChn,&stStream);
    			
    			free(stStream.pstPack);
    			stStream.pstPack = NULL;
    		}		
        }	
    }

	return NULL;
}

/* this only for VI mix output, D1 low frmrate, QCIF full frmrate */
HI_S32 SAMPLE_VencStart(HI_S32 s32ChnCnt)
{
    HI_S32 i, j, s32MainFrmRate, s32MinorFrmRate;
	VENC_CHN_ATTR_S stVencAttr;
	VENC_ATTR_H264_S stH264Attr[2];    
    
	s32MainFrmRate = s_s32ViMainFrmRate;
    s32MinorFrmRate = 15;
    
	stH264Attr[0].u32PicWidth = 704;
	stH264Attr[0].u32PicHeight = (s_enVideoNorm==0)?576:480;
	stH264Attr[0].bMainStream = HI_TRUE;
	stH264Attr[0].bByFrame = HI_TRUE;
	stH264Attr[0].bCBR = HI_TRUE;
	stH264Attr[0].bField = HI_FALSE;
	stH264Attr[0].bVIField = HI_FALSE;
	stH264Attr[0].u32Bitrate = 2048;	
	stH264Attr[0].u32BufSize = stH264Attr[0].u32PicWidth*stH264Attr[0].u32PicHeight*2;
	stH264Attr[0].u32Gop = 100;
	stH264Attr[0].u32MaxDelay = 10;
	stH264Attr[0].u32PicLevel = 0;
    stH264Attr[0].u32Priority = 0;
    /* stH264Attr.u32ViFramerate must equal to output frmrate of VI */
    /* while set vi minor chn attr,  real output frmrate of VI is same as vi src frmrate */
    stH264Attr[0].u32ViFramerate = s_s32ViSrcFrmRate;
	stH264Attr[0].u32TargetFramerate = s32MainFrmRate;
    
    stH264Attr[1].u32PicWidth = 176;
	stH264Attr[1].u32PicHeight = (s_enVideoNorm==0)?128:112;
	stH264Attr[1].bMainStream = HI_TRUE;
	stH264Attr[1].bByFrame = HI_TRUE;
	stH264Attr[1].bCBR = HI_TRUE;
	stH264Attr[1].bField = HI_FALSE;
	stH264Attr[1].bVIField = HI_FALSE;
	stH264Attr[1].u32Bitrate = 512;	
	stH264Attr[1].u32BufSize = stH264Attr[1].u32PicWidth*stH264Attr[1].u32PicHeight*2;
	stH264Attr[1].u32Gop = 100;
	stH264Attr[1].u32MaxDelay = 10;
	stH264Attr[1].u32PicLevel = 0;
    stH264Attr[1].u32Priority = 0;

    stH264Attr[1].u32ViFramerate = s_s32ViSrcFrmRate;
	stH264Attr[1].u32TargetFramerate = s32MinorFrmRate;

    for (j=0; j<2; j++)
    {
        for (i=0; i<s32ChnCnt; i++)
        {
            VENC_GRP VeGroup = i + j*s32ChnCnt;
            VENC_CHN VeChn = i + j*s32ChnCnt;
            VI_DEV ViDev = i/2;
            VI_CHN ViChn = i%2;
            VIDEO_PREPROC_CONF_S stConf;
            if (HI_MPI_VENC_CreateGroup(VeGroup))
            {
                return -1;
            }
            
            /* if vi output mix pic(DI/CIF), and some venc is QCIF,
            we should chage scale mode to VPP_SCALE_MODE_USEBOTTOM2, 
            otherwise pic may dithering for src pic is turn ceaselessly */
            /* NOTES: if you recreate group, vpp must re-cfg also */
            HI_MPI_VPP_GetConf(VeGroup, &stConf);
            stConf.enScaleMode = VPP_SCALE_MODE_USEBOTTOM2;
            if (HI_MPI_VPP_SetConf(VeGroup, &stConf))
            {
                printf("vpp %d set config err\n", VeGroup);
                return -1;
            }
            
            if (HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn))
            {
                printf("bind vi(%d,%d) to group %d\n", ViDev, ViChn, VeGroup);
                return -1;
            }
            stVencAttr.enType = PT_H264;	
            stVencAttr.pValue = (HI_VOID *)&stH264Attr[j];
            if (HI_MPI_VENC_CreateChn(VeChn, &stVencAttr, NULL))
            {
                printf("create venc chn %d fail\n", VeChn);
                return -1;
            }
            if (HI_MPI_VENC_RegisterChn(VeGroup, VeChn))
            {
                printf("register venc%d to group %d fail\n", VeChn, VeGroup);
                return -1;
            }
            if (HI_MPI_VENC_StartRecvPic(VeChn))
            {
                printf("start venc %d fail\n", VeChn);
                return -1;
            }                
        }
    }
    bStart = HI_TRUE;
    pthread_create(&Pid, 0, SAMPLE_VencGetStream, (void*)(s32ChnCnt*j));
    
    return HI_SUCCESS;
}

HI_VOID SAMPLE_VencStop()
{
    bStart = HI_FALSE;
    if (Pid)
    {
        pthread_join(Pid, 0);
    }
    /* destroy venc chn ... */
    /* ...... ......*/
}

HI_S32 SAMPLE_VIO_8D1()
{
    HI_S32 i, s32Ret, ViDev, ViChn;
    VB_CONF_S stVbConf = {0};
    
    HI_S32 s32ViChnCnt = 8;
    HI_S32 s32ViTarFrmRate = 6;
    
    SAMPLE_TW2815_CfgV(VI_WORK_MODE_2D1);
    SAMPLE_7179_CfgV();

    stVbConf.astCommPool[0].u32BlkSize = 768*576*2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt = 40;
    stVbConf.astCommPool[1].u32BlkSize = 384*288*2;/*CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 48;     
    stVbConf.astCommPool[2].u32BlkSize = 176*128*2;/*QCIF*/
    stVbConf.astCommPool[2].u32BlkCnt = 48;    
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    
    /* set main chn to D1 */
    s32Ret = SAMPLE_ViInit(s32ViChnCnt, VI_SIZE_D1);
    if (0 != s32Ret)
    {
        return s32Ret;
    }        
    
    s32Ret = SAMPLE_VoInit(9);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
        
    for (i=0; i<s32ViChnCnt; i++)
    {
        ViDev = i/2;
        ViChn = i%2;
        s32Ret = SampleViMixMode(ViDev, ViChn, VI_SIZE_D1, VI_SIZE_CIF, s32ViTarFrmRate);
        if (0 != s32Ret)
        {
            return s32Ret;
        } 
    }        
    /* bind in sequence */
    s32Ret = SAMPLE_BindVi2Vo(s32ViChnCnt);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

#if 1
    s_s32ViMainFrmRate = s32ViTarFrmRate;
    s_s32ViSrcFrmRate = ((0 == s_enVideoNorm)?25:30);
    if (SAMPLE_VencStart(s32ViChnCnt))
    {
        return -1;
    }
#endif           
    
    return 0;    
}

#define SAMPLE_VIO_HELP(void)\
{\
    printf("\n\tusage : %s 0|7|8|9|10 \n", argv[0]);\
    printf("\t 0:  VI->VO base operation \n");\
    printf("\t 7:  VI 720P mode \n");\
    printf("\t 8:  VI 4CIF(4HalfD1) mode \n");\
    printf("\t10:  8D1 mode (preview real time; encode non real time) \n");\
}


HI_S32 main(int argc, char *argv[])
{
    HI_S32 index, s32Ret;

    if (2 != argc)
    {
        SAMPLE_VIO_HELP();
        return -1;
    }
    
    index = atoi(argv[1]);        

    switch (index)
    {        
        case 0:
        {
            
            s32Ret = SAMPLE_VIO();
            break;
        }
        case 7:
        {
            s32Ret = SAMPLE_VIO_720P();
            break;
        }
        case 8:
        {            
            s32Ret = SAMPLE_VIO_4HD1Mode();    
            break;
        }
        case 10:
        {            
            s32Ret = SAMPLE_VIO_8D1();    
            break;
        }        
        default:
        {
            s32Ret = HI_FAILURE;
            SAMPLE_VIO_HELP();  
            break;
        }             
    }

    if (s32Ret)
    {
        SAMPLE_ExitMPP();
        return s32Ret;
    }
    
    printf("start VI to VO preview , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();

    SAMPLE_VencStop();
    SAMPLE_ExitMPP();
    return 0;
}

