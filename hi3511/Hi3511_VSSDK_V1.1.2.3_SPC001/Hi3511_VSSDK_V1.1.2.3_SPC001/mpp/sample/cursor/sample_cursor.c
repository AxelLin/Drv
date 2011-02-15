/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_cursor.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/08/27
  Description   : sample for hardward cursor test.
  History       :
  1.Date        : 2008/08/27
    Author      : x00100808
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <linux/fb.h>
#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"

#include "loadbmp.h"
#include "hifb.h"

#include"tw2815.h"
#include "adv7179.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define FB_NAME             "/dev/fb/2"
#define BMP_PATH     "./res/cursor.bmp"
#define SCREEN_WIDTH                720
#define SCREEN_HEIGHT               576
#define PIX_LEN                       2
#define DEF_COLOR              0x000000     /* BLACK */

/* switch of picture format: ARGB1555                                        */
#define COLOR_FORMAT_ARGB1555

#define PERR() do\
               {\
                   	fprintf(stderr, "test error: line: %d!\n", __LINE__);\
                   	return -1;\
               }while(0)

#define CHECK(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d !\n", name, __FUNCTION__, __LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

    
/* fd for vi/vo config  (tw2815/adv7179)                                                       */
int fd2815a = -1;
int fd2815b = -1;
int fdadv7179 = -1;

static HI_U32 g_u32TvMode = VIDEO_ENCODING_MODE_PAL;

/* used to contrl cursor thread's extinguish                                */
static HI_BOOL bStopFlag = HI_FALSE;

/*****************************************************************************
 Prototype       : Sample_hifb_mmap & Sample_hifb_munmap
 Description     : the following two function's are used to map and unmap frame
                   buffer,  it's clear.

*****************************************************************************/
char* Sample_hifb_mmap(int fd, int offset, size_t size)
{
    char * pszVirt = (char*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);	
    memset(pszVirt, 0, size);
    return pszVirt;
}

static int Sample_hifb_munmap(void *start, size_t size)
{
    return  munmap(start, size);
}

/*****************************************************************************
 Prototype       : cursor_test
 Description     : this function will show you how to put a arrow-like cursor
                   on your screen.
 Input           : parg: which picture layer to use, fb2 is for hardware cursor.
 Output          : None
 Return Value    : zero for success, others' tells something wrong.

*****************************************************************************/
static int cursor_test(void *parg)
{
    HI_S8* file;
    HI_S32 size;
    HI_S32 fd;
    HI_S32 i, j, k;
    HI_U8* pscreen;
    HI_S32 iret = -2;
    HI_BOOL bShow;
    HI_U8 *pRGBBuf;
    HI_U8 tmpColor = 0x00;
    HI_U32 u32CursorBottom = 500;
    
    HI3511_CURSOR_S hcmod;
    struct fb_cmap himap;
    struct fb_var_screeninfo vinfo;
    OSD_LOGO_T *pVideoLogo = (OSD_LOGO_T*)malloc(sizeof(OSD_LOGO_T));

    /* ---------------BLACK--WHITE--XXXX--XXXX-----------*/
    HI_U16 hc_Y[4] = {   16,   236,  255,   32};
    HI_U16 hc_U[4] = {  128,   128,  186,   85};
    HI_U16 hc_V[4] = {  128,   128,   97,    0};

    HIFB_POINT_S stPoint = {8, 8};
    vinfo.xres_virtual = 32;
    vinfo.yres_virtual = 32;
    vinfo.xres = 32;
    vinfo.yres = 32;
    vinfo.activate = FB_ACTIVATE_NOW;
    vinfo.bits_per_pixel = 2;
    vinfo.xoffset = vinfo.yoffset = 0;

    /* open hi-frame-buffer                                                 */
    file = (char*)parg;
    fd = open(file, O_RDWR);
    if(fd < 0)
    {
        printf("Open file(%s) failed!\n"
                "Make sure you have allocate frame buffer for layer 2 while load!\n"
                "You can use 'cat /proc/hifb/2' to confirm,\n"
                "if the item 'Show State : OFF', you should allocate frame buffer.\n"
                "vram1_size=1024,vram2_size=1024 is enough.\n",file);
        PERR();
    } 

    /* configure virtual screen information                                 */
    iret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
    if (iret < 0)
    {
        printf("iret = 0x%x\n", iret);
        PERR();
    }

    himap.start = 0;
    himap.len = 4;
    himap.red = hc_Y;
    himap.green = hc_U;
    himap.blue = hc_V;
    himap.transp = 0;

    /* configure color map                                                  */
    if (ioctl(fd, FBIOPUTCMAP, &himap) < 0)
    {
        printf("fb ioctl put cmap err!\n");
        return -1;
    }

    iret = ioctl(fd, FBIOGET_CURSOR_HI3511, &hcmod);
    if (iret < 0)
    {
        printf("iret = 0x%x\n", iret);
        PERR();
    }

    /* we use two mouse colors, transparent and inverse color.              */
    hcmod.enCursor = HI3511_CURSOR_2COLOR;

    for (i = 0 ; i < 4; i++)
    {
        hcmod.stYCbCr[i].u8Y  = hc_Y[i];
        hcmod.stYCbCr[i].u8Cb = hc_U[i];
        hcmod.stYCbCr[i].u8Cr = hc_V[i];        
    }

    iret = ioctl(fd, FBIOPUT_CURSOR_HI3511, &hcmod);
    if (iret < 0)
    {
        printf("iret = 0x%x\n", iret);
        PERR();
    }

    bShow = HI_TRUE;
    iret = ioctl(fd, FBIOPUT_SHOW_HIFB, &bShow);
    if (iret < 0)
    {
        printf("iret = 0x%x\n", iret);
        PERR();
    }

    size = 256;
    pscreen = Sample_hifb_mmap(fd, 0, size);
    if (pscreen == (void*)-1)
    {
        PERR();
    }

    /*
     * Next, we read cursor data from outside bmp file which
     * parameters are:  32*32, ARGB1555.
     * Of course, you can change your pictures, but note that
     * you should also adjust reading strategy.
     * For example, while you use picture format as AGB8888, you
     * should note that byte per pixel is 4, not 2.
     * We show you different pixel format transform method, just
     * contrl by macro-definition switch COLOR_FORMAT_ARGB1555.
     */

#ifdef COLOR_FORMAT_ARGB1555
    pVideoLogo->stride = 32*2;
    pVideoLogo->pRGBBuffer = (HI_U8*)malloc(32*32*2);
#else
    pVideoLogo->stride = 32*4;
    pVideoLogo->pRGBBuffer = (HI_U8*)malloc(32*32*4);    
#endif

    if (NULL == pVideoLogo->pRGBBuffer)
    {
        printf("malloc buffer failed!\n");
        PERR();
    }

    iret = LoadImage(BMP_PATH, pVideoLogo);
    if (0 != iret)
    {
        printf("load bmp picture failed!\n");
        PERR();
    }

    pRGBBuf = pVideoLogo->pRGBBuffer;

#ifdef COLOR_FORMAT_ARGB1555
    for(j = 0; j < 32; j++)
    {
        // printf("\n");
        for(i = 0; i < 8; i++)
        {
            tmpColor = 0x00;
            for (k = 0; k < 4; k++)
            {
                HI_U8 tmp;
                HI_U16 trueColor = 0x00;

                trueColor = (*pRGBBuf | ((*(pRGBBuf + 1) & 0x7f) << 8)) & 0xff; //GBaR

                // printf("%4d", trueColor);
                
                if (trueColor == 0)
                {
                    tmp = 0x0;
                }
                else if (trueColor == 255)
                {
                    tmp = 0x1;
                }
                else if (trueColor == 31)
                {
                    tmp = 0x2;
                }
                else
                {
                    tmp = 0x0;
                }
                tmpColor = (tmpColor | (tmp << (2 * k))) & 0xff;

                /* for aRGB1555, 2 bytes per pixel                          */
                pRGBBuf += 2;
            }
            *(pscreen+j*8 + i) = tmpColor;
        }
    }
#else /* COLOR_FORMAT_ARGB8888 */
    for(j = 0; j < 32; j++)
    {
        // printf("\n");
        for(i = 0; i < 8; i++)
        {
            tmpColor = 0x00;
            for (k = 0; k < 4; k++)
            {
                HI_U8 tmp;
                HI_U32 trueColor = 0x00;

                trueColor = (((*pRGBBuf << 8) | *(pRGBBuf + 1))
                            | ((*(pRGBBuf + 3) & 0xff) << 16)) & 0xffffff; //GBaR

                // printf("%4d", trueColor);
                
                if (trueColor == 0)
                {
                    tmp = 0x0;
                }
                else if (trueColor == 255)
                {
                    tmp = 0x1;
                }
                else if (trueColor == 31)
                {
                    tmp = 0x2;
                }
                else
                {
                    tmp = 0x0;
                }
                tmpColor = (tmpColor | (tmp << (2 * k))) & 0xff;

                /* for aRGB8888, 4 bytes per pixel                          */
                pRGBBuf += 4;
            }
            *(pscreen+j*8 + i) = tmpColor;
        }
    }
#endif /* #ifdef COLOR_FORMAT_ARGB1555 */

    free((void*)pVideoLogo->pRGBBuffer);
    free((void*)pVideoLogo);

    if (VIDEO_ENCODING_MODE_NTSC == g_u32TvMode)
    {
        u32CursorBottom = 400;
    }
    else
    {
        u32CursorBottom = 500;
    }

    /* here, the contrl is totally free style, it's your turn, change it!   */
    while (bStopFlag != HI_TRUE)
    {
        for (i = 0; i < u32CursorBottom; i += 8)
        {
            stPoint.u32PosX += 8;
            stPoint.u32PosY += 8;
            fflush(stdout);
            iret = ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint);
            if (iret < 0)
            {
                printf("iret = 0x%x\n", iret);
                PERR();
            }
            if (bStopFlag == HI_TRUE)
            {
                goto out;
            }
            usleep(100000);
        }
        
        stPoint.u32PosX = 704;
        stPoint.u32PosY = 8;
        for (i = 0; i < u32CursorBottom; i += 8)
        {
            if (stPoint.u32PosX <= 0)
            {
                stPoint.u32PosX = 704;       
            }
            else
            {
                stPoint.u32PosX -= 8;                
            }
            stPoint.u32PosY += 8;
            fflush(stdout);
            iret = ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint);
            if (iret < 0)
            {
                printf("iret = 0x%x\n", iret);
                PERR();
            }
            if (bStopFlag == HI_TRUE)
            {
                goto out;
            }
            usleep(100000);
        }

        stPoint.u32PosX = 8;
        stPoint.u32PosY = 8;   
        i = 0;
    }    
out:
    Sample_hifb_munmap((void *)pscreen, (size_t)size);
    close(fd);
    return 0;
}

/* set A/D encoder's work mode                                                    */
static void do_Set2815_2d1(void)
{
	tw2815_set_2d1 tw2815set2d1;
    HI_U32 tmp;

	if(fd2815a <0)
	{
		fd2815a = open("/dev/misc/tw2815adev",O_RDWR);
		if(fd2815a <0)
		{
			printf("can't open tw2815\n");
		}
	}

	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);			
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(fd2815a,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);			
	}

    tmp = 0x88;
	if(0!=ioctl(fd2815a,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);			
		return;
	}
	
	if(fd2815b <0)
	{
		fd2815b = open("/dev/misc/tw2815bdev",O_RDWR);
		if(fd2815b <0)
		{
			printf("can't open tw2815\n");
		}
	}
	
	tw2815set2d1.ch1 = 0;
	tw2815set2d1.ch2 = 2;

	if(0!=ioctl(fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);			
	}

	tw2815set2d1.ch1 = 1;
	tw2815set2d1.ch2 = 3;

	if(0!=ioctl(fd2815b,TW2815_SET_2_D1,&tw2815set2d1))
	{
		printf("2815 errno %d\r\n",errno);			
	}

    tmp = 0x88;
	if(0!=ioctl(fd2815b,TW2815_SET_CLOCK_OUTPUT_DELAY,&tmp))
	{
		printf("2815 errno %d\r\n",errno);			
		return;
	}
	
	return;
}

/* set D/A decoder's work mode                                              */
static void do_Set7179(VIDEO_NORM_E stComposeMode)
{
	if(fdadv7179 <0)
	{
		fdadv7179 = open("/dev/misc/adv7179",O_RDWR);
		if(fdadv7179 <0)
		{
			printf("can't open 7179\n");
			return;
		}
	}

    if(stComposeMode==VIDEO_ENCODING_MODE_PAL)
    {
        if(0!=ioctl(fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_PAL))
    	{
    		printf("7179 errno %d\r\n",errno);			
    	}
	}
	else if(stComposeMode==VIDEO_ENCODING_MODE_NTSC)
	{
        if(0!=ioctl(fdadv7179,ENCODER_SET_NORM,VIDEO_MODE_656_NTSC))

    	{
    		printf("7179 errno %d\r\n",errno);			
    	}
	}
	else
	{
        return;
	}
	return;
}

/* open vi2vo's preview                                                     */
static HI_S32 do_vivo_1d1(VI_DEV ViDevid,VI_CHN ViChn,VO_CHN VoChn)
{
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;
	VO_PUB_ATTR_S VoAttr;
	VO_CHN_ATTR_S VoChnAttr;
    
	do_Set2815_2d1();
	
    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));
    memset(&VoAttr,0,sizeof(VO_PUB_ATTR_S));
    memset(&VoChnAttr,0,sizeof(VO_CHN_ATTR_S));

    if (g_u32TvMode == VIDEO_ENCODING_MODE_NTSC)
    {
        do_Set7179(VIDEO_ENCODING_MODE_NTSC);
        ViAttr.enViNorm = VIDEO_ENCODING_MODE_NTSC;
        ViChnAttr.stCapRect.u32Height = 240;

        VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_NTSC;
        VoChnAttr.stRect.u32Height = 480;
    }
    else
    {
        do_Set7179(VIDEO_ENCODING_MODE_PAL);
        ViAttr.enViNorm = VIDEO_ENCODING_MODE_PAL;
        ViChnAttr.stCapRect.u32Height = 288;

        VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
        VoChnAttr.stRect.u32Height = 576;
    }

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;

	ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 0;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_FALSE;
	ViChnAttr.bChromaResample = HI_FALSE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	VoAttr.u32BgColor = DEF_COLOR;

	VoChnAttr.bZoomEnable = HI_TRUE;
	VoChnAttr.u32Priority = 1;
	VoChnAttr.stRect.s32X = 0;
	VoChnAttr.stRect.s32Y = 0;
	VoChnAttr.stRect.u32Width = 704;
	
	if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(ViDevid,&ViAttr))
	{
		printf("set VI attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(ViDevid,ViChn,&ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}	
	
 	if (HI_SUCCESS!=HI_MPI_VO_SetPubAttr(&VoAttr))
	{
		printf("set VO attribute failed !\n");
		return HI_FAILURE;		
	}
	if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(VoChn,&VoChnAttr))
	{
		printf("set VO Chn attribute failed !\n");
		return HI_FAILURE;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_Enable())
	{
		printf("set VO  enable failed !\n");
		return HI_FAILURE;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_EnableChn(VoChn))
	{
		printf("set VO Chn enable failed !\n");
		return HI_FAILURE;		
	}

	if (HI_SUCCESS!=HI_MPI_VI_Enable(ViDevid))
	{
		printf("set VI  enable failed !\n");
		return HI_FAILURE;		
	}	
	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(ViDevid,ViChn))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}

    if (HI_SUCCESS!=HI_MPI_VI_BindOutput(ViDevid,ViChn,VoChn))
	{
		printf("HI_MPI_VI_BindOutput failed !\n");
		return HI_FAILURE;		
	}
	
	return HI_SUCCESS;
}

/* close vi2vo's preview                                                    */
static HI_S32 do_vivo_disable_1d1(VI_DEV ViDevid,VI_CHN ViChn,VO_CHN VoChn)
{
	if (HI_SUCCESS!=HI_MPI_VI_UnBindOutput(ViDevid,ViChn,VoChn))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS!=HI_MPI_VO_DisableChn(VoChn))
	{
		printf("HI_MPI_VO_DisableChn failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS!=HI_MPI_VO_Disable())
	{
		printf("HI_MPI_VO_Disable failed !\n");
		return HI_FAILURE;
	}
	if (HI_SUCCESS!=HI_MPI_VI_DisableChn(ViDevid,ViChn))
	{
		printf("HI_MPI_VI_DisableChn failed !\n");
		return HI_FAILURE;
	}
	
	if (HI_SUCCESS!=HI_MPI_VI_Disable(ViDevid))
	{
		printf("HI_MPI_VI_Disable failed !\n");
		return HI_FAILURE;
	}
	return HI_SUCCESS;
}

/* function to process abnormal case                                        */
HI_VOID HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
    	(HI_VOID)HI_MPI_SYS_Exit();
    	(HI_VOID)HI_MPI_VB_Exit();
    	printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}

int main(int argc, char * argv [])
{
    char ch = 'a';
    pthread_t cursorPid;

    /* configure for vi2vo preview                                         */
	MPP_SYS_CONF_S stSysConf = {0};
	VB_CONF_S stVbConf ={0};

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 10;	
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 20;	
	stSysConf.u32AlignWidth = 64;

    /* process abnormal case                                                */
    signal(SIGINT, HandleSig);
    signal(SIGTERM, HandleSig);    

    if (argc > 1)
    {
        /* if program can not run anymore, you may try 'reset' adhere command  */
        if (!strcmp(argv[1],"reset"))
        {
           	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
        	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
        	return HI_SUCCESS;
        }
        else if ((!strcmp(argv[1],"N") || (!strcmp(argv[1],"n"))))
        {
            /* if you live in Japan or North America, you may need adhere 'N/n' */
            g_u32TvMode = VIDEO_ENCODING_MODE_NTSC;
        }
    }

    /* process abnormal case                                                */
    signal(SIGINT, HandleSig);
    signal(SIGTERM, HandleSig);    

    /* configure video buffer and initial system                            */
	CHECK(HI_MPI_VB_SetConf(&stVbConf),"HI_MPI_VB_SetConf");
	CHECK(HI_MPI_VB_Init(),"HI_MPI_VB_Init");
	CHECK(HI_MPI_SYS_SetConf(&stSysConf),"HI_MPI_SYS_SetConf");
	CHECK(HI_MPI_SYS_Init(),"HI_MPI_SYS_Init");	

    /* first, we let cursor appearence on your screen, may be a blue background
      seems better together with a yellow cursor.                             */
    if (0 != pthread_create(&cursorPid, NULL, (void*)cursor_test, (void*)FB_NAME))
    {
        puts("create cursor_test thread failed!");
        return -1;
    }

    /* then we open vi2vo preview, the cursor will swimming in the picture and
      its' color are totally different.                                     */
    CHECK(do_vivo_1d1(0,0,0),"do_vivo_1d1");

    printf("\033[0;32mpress 'q' to exit!\033[0;39m\n");
    while((ch = getchar()) != 'q')
    {
        if (ch == '\n')
        {
            /* green color for hint                                         */
            printf("\033[0;32mpress 'q' to exit!\033[0;39m\n");
            continue;
        }
    }
    
    CHECK(do_vivo_disable_1d1(0,0,0),"do_vivo_disable_1d1");

    bStopFlag = HI_TRUE;
    pthread_join(cursorPid, NULL);

    /* de-init sys and vb */
	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
   
    return 0;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

