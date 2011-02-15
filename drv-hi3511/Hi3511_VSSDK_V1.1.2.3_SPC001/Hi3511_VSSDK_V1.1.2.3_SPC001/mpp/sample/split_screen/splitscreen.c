/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : screenmenu.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/23
  Description   : 
  History       :
  1.Date        : 2008/06/23
    Author      : x00100808
    Modification: Created file

******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <asm/page.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"

#include "hi_type.h"
#include "hifb.h"

#include "tw2815.h"
#include "loadbmp.h"
#include "tde_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


/*****************************************************************************
 Section         : MACRO Definition
 Description     : 

*****************************************************************************/
#define DEF_COLOR   0x00ff00

#define PAUSE   do{ getchar();}while(0)

#define PERR() do \
               {\
               	printf("In %s error: line: %d!\n", __FUNCTION__,__LINE__);\
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


#define FB_NAME_0   "/dev/fb/0"

#define TEST_32BPP   0
#define TEST_16BPP   1

#define GRID_ON         1
#define BUTTON_ON       1

#define RED    0xfC00
#define GREEN  0x83e0
#define BLUE   0x801f
#define WHITE  0xffff
#define BLACK  0x8000
#define GRAY   0x8001

#define SCREEN_WIDTH    720
#define SCREEN_HEIGHT   576

#define HIFB_MAX_WIDTH  720
#define HIFB_MAX_HEIGHT 576

#define GRID_PIC_OFFSET     ((SCREEN_WIDTH*SCREEN_HEIGHT)<<1)
#define BUTN_PIC_OFFSET     (GRID_PIC_OFFSET<<1)
#define DEFAULT_PIC_SIZE    ((SCREEN_WIDTH*SCREEN_HEIGHT)<<1)

#ifdef SPLIT_DEBUG
#define DBG_PRINT(fmt...) printf(##fmt)
#else
#define DBG_PRINT(fmt...) 
#endif  

#define DEBUGPOS printf("file:%s,line:%d\r\n",__FILE__,__LINE__);

/*****************************************************************************
 Section         : Global Variables Declaration
 Description     : 

*****************************************************************************/
static void *g_s32GridPhyAddress = NULL;
static void *g_s32ButnPhyAddress = NULL;
static void *g_s32GridVirAddress = NULL;
static void *g_s32ButnVirAddress = NULL;

void *g_s32MappedMemFb = NULL;
unsigned long mapped_memlen;

static struct fb_bitfield g_r16 = {10, 5, 0};
static struct fb_bitfield g_g16 = {5, 5, 0};
static struct fb_bitfield g_b16 = {0, 5, 0}; 	
static struct fb_bitfield g_a16 = {15, 1, 0};

static struct fb_bitfield g_r32 = {16, 8, 0};
static struct fb_bitfield g_g32 = {8, 8, 0};
static struct fb_bitfield g_b32 = {0, 8, 0}; 	
static struct fb_bitfield g_a32 = {24, 8, 0};

static HI_S32 g_s32FrameBufferFd = -1;
static HI_U32 g_u32SemStart = 0;
static HI_U32 g_u32SemSize = 0;
static HI_U32 g_s32FbPicStride = 0;

typedef enum hiPICTYPE_E
{
    PICTYPE_SCREEN,
    PICTYPE_GRID,
    PICTYPE_BUTN,
    PICTYPE_ALL,
} PICTYPE_E;

/* for vi config  (tw2815)                                                       */
int fd2815a = -1;
int fd2815b = -1;

static void AbnormalOut(void);

/*****************************************************************************
 Section         : Function Definition
 Description     : 

*****************************************************************************/
inline static HI_CHAR *hifb_mmap(HI_S32 fd, HI_S32 offset, size_t size)
{
    HI_CHAR * pszVirt = (HI_CHAR*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);	
    memset(pszVirt, 0, size);
    return pszVirt;
}

inline static HI_S32 hifb_munmap(HI_VOID *start, size_t size)
{
    if (NULL == start)
    {
        return -1;
    }
    else
    {
        return  munmap(start, size);
    }
}

/*****************************************************************************
 Prototype       : hifb_open
 Description     : open frame buffer for display operation
 Input           : b32Bpp     currently we only support aRGB1555
                   pstFbName  frame buffer device name
 Output          : None
 Return Value    : 
 Global Variable   
    Read Only    : 
    Read & Write : 
  History         
  1.Date         : 2008/6/25
    Author       : x00100808
    Modification : Created function

*****************************************************************************/
HI_S32 hifb_open(HI_BOOL b32Bpp, const HI_CHAR *pstFbName)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    HIFB_ALPHA_S stAlpha;
    HIFB_COLORKEY_S ck;

    /* set alpha layer 0 */
    stAlpha.bAlphaEnable = HI_TRUE;
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.u8Alpha0 = 0xff;             /* global alpha value */
    stAlpha.u8Alpha1 = 0xff;

    if(pstFbName == NULL)
    {
        printf("error null fb name!\n");
        return -1;
    }
    DBG_PRINT("%s\n", pstFbName);

    /* open frame buffer device                                             */
	g_s32FrameBufferFd = open(pstFbName, O_RDWR);
    if(g_s32FrameBufferFd < 0)
    {
        PERR();
    }

    /* get and set variable screen parameter                                 */
    if (ioctl(g_s32FrameBufferFd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
   	    PERR();
    }
      
    vinfo.xres = vinfo.xres_virtual = SCREEN_WIDTH;
    vinfo.yres = vinfo.yres_virtual = SCREEN_HEIGHT;
    vinfo.xoffset = vinfo.yoffset = 0;
    if(b32Bpp)
    {
        vinfo.red   = g_r16;
        vinfo.green = g_g16;
        vinfo.blue  = g_b16;
        vinfo.transp = g_a16;
        vinfo.bits_per_pixel = 16;
    }
    else
    {
        vinfo.red   = g_r32;
        vinfo.green = g_g32;
        vinfo.blue  = g_b32;
        vinfo.transp = g_a32;
        vinfo.bits_per_pixel = 32;
    }
    
    vinfo.activate = FB_ACTIVATE_NOW;
    if (ioctl(g_s32FrameBufferFd, FBIOPUT_VSCREENINFO, &vinfo) < 0)
    {
        printf("fd:%d, b32Bpp:%d\n", g_s32FrameBufferFd, b32Bpp);
        AbnormalOut();
   	    PERR();
    }

    /* get and set fix screen parameter                                    */
    if (ioctl(g_s32FrameBufferFd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        AbnormalOut();
   	    PERR();
    }

    if (ioctl(g_s32FrameBufferFd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
    {
        fprintf (stderr, "Couldn't set alpha\n");
        AbnormalOut();
        return HI_FAILURE;
    }

    /* get and set color key value                                          */
    if (ioctl(g_s32FrameBufferFd, FBIOGET_COLORKEY_HIFB,  &ck) < 0)
    {
        printf("Couldn't get colorkey\n");
        AbnormalOut();
        return HI_FAILURE;
    }

    DBG_PRINT("bKeyEnable=%d,bMaskEnable=%d,u32Key=%d,rm=%d,gm=%d,bm=%d\n",
            ck.bKeyEnable,
            ck.bMaskEnable,
            ck.u8RedMask,
            ck.u8GreenMask,
            ck.u8BlueMask,            
            ck.u32Key);

    /* enable color key and set default key as Black                        */
    ck.bKeyEnable = HI_TRUE;
    ck.u32Key = BLACK;  /* black */

    if (ioctl(g_s32FrameBufferFd, FBIOPUT_COLORKEY_HIFB,  &ck) < 0)
    {
        printf("Couldn't set colorkey\n");
        AbnormalOut();
        return HI_FAILURE;
    }

    /* set global screen display buffer parameter                           */
    g_u32SemStart = finfo.smem_start; 
    g_u32SemSize = finfo.smem_len;
    g_s32FbPicStride = finfo.line_length;

    /* if frame buffer is too small to run the service ,we exit!            */
    if (g_u32SemSize < 3000000)
    {
        printf("Your Frame buffer configure too small!\n");
        AbnormalOut();
        return HI_FAILURE;
    }
    
    g_s32GridPhyAddress = (void*)(g_u32SemStart + GRID_PIC_OFFSET);
    g_s32ButnPhyAddress = (void*)(g_u32SemStart + BUTN_PIC_OFFSET);

    g_s32MappedMemFb    = mmap(NULL, g_u32SemSize,
                          PROT_READ|PROT_WRITE, MAP_SHARED, g_s32FrameBufferFd, 0);

    g_s32GridVirAddress = g_s32MappedMemFb + GRID_PIC_OFFSET;
    g_s32ButnVirAddress = g_s32MappedMemFb + BUTN_PIC_OFFSET;

    if (!g_s32MappedMemFb || !g_s32GridVirAddress || !g_s32ButnVirAddress)
    {
        puts("mmap error!");
        AbnormalOut();
        return HI_FAILURE;
    }

    /* clear all fb to avoid incorrect data left */
    memset(g_s32MappedMemFb, 0x00, g_u32SemSize);
    
    DBG_PRINT("fb start phy = %#x, len = %d, stride = %d\n", 
            g_u32SemStart, g_u32SemSize,g_s32FbPicStride);
    DBG_PRINT("g_s32GridPhyAddress = %#x, g_s32ButnPhyAddress = %#x",
           g_s32GridPhyAddress,g_s32ButnPhyAddress);
    DBG_PRINT("g_s32MappedMemFb = %#x, g_s32GridVirAddress = %#x, g_s32ButnVirAddress= %#x\n",
           g_s32MappedMemFb,g_s32GridVirAddress,g_s32ButnVirAddress);

    return HI_SUCCESS;
}

inline static HI_VOID hifb_close(HI_VOID)
{
    hifb_munmap(g_s32MappedMemFb, g_u32SemSize);
    
    close(g_s32FrameBufferFd);
    g_s32FrameBufferFd = -1;

    return;
}

static void DrawBox(int x, int y, int w, int h, unsigned long stride, 
         char* pmapped_mem, unsigned short color)
{
    int i, j;
    unsigned short* pvideo = (unsigned short*)pmapped_mem;

    for (i=y; i<y+h; i++)
    {
        pvideo = (unsigned short*)(pmapped_mem + i*stride);
        for (j=x; j<x+w; j++)
        {
            *(pvideo+j) = color;
        }
    }
}

/*
 * Draw grid lines while split screen as 1, 4, 9 or more blocks
 */
static HI_S32 DrawGrid(void *GridVirAddr, unsigned short gridnum, 
                        unsigned short color,const TDE_SURFACE_S *stSrc)
{
	HI_U32 line_w = 2;
	HI_U32 line_n = sqrt(gridnum);
	HI_CHAR chname[20];
	
	HI_U32 i;
	HI_U32 x = 0,y = 0,stepw = 0,steph = 0;
	HI_U32 chnx = 0,chny = 0;
    HI_U32 stride = 0;
    char *mapmem = (char *)GridVirAddr;

    if (line_n < 2 || line_n > 10)
    {
        return HI_FAILURE;
    }

    stride = g_s32FbPicStride;

    stepw = HIFB_MAX_WIDTH/line_n;
    steph = HIFB_MAX_HEIGHT/line_n;

    for ( i = 1 ; i < line_n ; i++ )
    {
        x += stepw;
        y += steph;
        DrawBox(x,0,line_w,HIFB_MAX_HEIGHT,stride,mapmem,color);
        DrawBox(0,y,HIFB_MAX_WIDTH,line_w,stride,mapmem,color);
        
        DBG_PRINT("x=%d, y=%d, stepw=%d, steph=%d ,maxh = %d, maxw = %d, stride=%d\n",
            x,y,stepw,steph,HIFB_MAX_HEIGHT, HIFB_MAX_WIDTH, stride);
    }

    for ( i = 0 ; i < gridnum ; i++ )
    {
        chnx = (i%line_n)*stepw;
        chny = (i/line_n)*steph;

        sprintf(chname, "./res/%d.bmp", (i+1));

        DBG_PRINT("chnx=%d, y=%d, stepw=%d, steph=%d, stride=%d, gridnum = %d\n",
            chnx,chny,stepw,steph,stSrc->u16Stride,i);

        /* here we 2*chnx since that aRGB1555 BPP is 2                      */
        if(LoadBitMap2Surface(chname, (OSD_SURFACE_S*)stSrc, mapmem+(chnx+2)*2+(chny+2)*stride) < 0)
        {
            PERR();
        }    
    }

    return HI_SUCCESS;
}

/*
 * Draw buttons on screen for user to select split number he wanted
 */
static HI_S32 DrawButton(void *ButnVirAddr, unsigned short butnum, 
                            const TDE_SURFACE_S *stSrc, HI_U32 splitnum)
{
    HI_U32 i,j;
	HI_U32 y = 0,stepw = 0,margin = 0;
	HI_CHAR butname[20];
	HI_U32 selected = sqrt(splitnum);

    HI_U32 stride = 0;
    void *mapmem = ButnVirAddr;

    if (butnum < 1 || butnum > 10)
    {
        return HI_FAILURE;
    }
    
    stride = g_s32FbPicStride;        

    /* you can change this value for button position on the screen          */
    y = 400;
    margin = 154;
    stepw = 180;
    

    for ( i = 0 ; i < butnum ; i++ )
    {
        j = i+1;
        if (j == selected)
        {
            /* this button will be highlight when it is selected            */
            sprintf(butname, "./res/button%d%d.bmp", j, j);            
        }
        else
        {
            sprintf(butname, "./res/button%d.bmp", j);            
        }

        if(LoadBitMap2Surface(butname, (OSD_SURFACE_S*)stSrc, 
            (char *)(mapmem + y*stride + (margin + i*stepw)*2)) < 0)
        {
            PERR();
        }
    }

    return HI_SUCCESS;
}

/*
 * Draw grids and buttons on screen to show split screen
 */
HI_S32 DrawSplitScreenDisplay(HI_U32 splitNum, HI_U32 butnNum)
{
    HI_S32 hiRet;

    TDE_SURFACE_S stSrc,stSrc2,stDst;
    TDE_OPT_S pstOpt = {0};
    HI_U32 u32Bpp;
    TDE_COLOR_FMT_E enFmt;
    TDE_HANDLE s32Handle;

    if(TEST_32BPP)
    {
        u32Bpp = 4;
        enFmt  = TDE_COLOR_FMT_RGB8888;
    }
    else
    {
        u32Bpp = 2;
        enFmt  = TDE_COLOR_FMT_RGB1555;
    }

    /* Grid surface buffer                                                  */
    stSrc.enColorFmt = enFmt;
    stSrc.pu8PhyAddr = (HI_U8*)g_s32GridPhyAddress;
    stSrc.u16Height = SCREEN_HEIGHT;
    stSrc.u16Width  = SCREEN_WIDTH;
    stSrc.u16Stride = g_s32FbPicStride;

    /* Button surface buffer                                                */
    stSrc2.enColorFmt = enFmt;
    stSrc2.pu8PhyAddr = (HI_U8*)g_s32ButnPhyAddress;
    stSrc2.u16Height = SCREEN_HEIGHT;
    stSrc2.u16Width  = SCREEN_WIDTH;
    stSrc2.u16Stride = g_s32FbPicStride;

    /* Screen surface buffer                                                */
    stDst.enColorFmt = enFmt;
    stDst.pu8PhyAddr = (HI_U8*)g_u32SemStart;
    stDst.u16Height = SCREEN_HEIGHT;
    stDst.u16Width  = SCREEN_WIDTH;
    stDst.u16Stride = g_s32FbPicStride;

    /* no opetation will be done between srouce and destination surface */
    pstOpt.enDataConv = TDE_DATA_CONV_NONE;  

#if GRID_ON
    DrawGrid(g_s32GridVirAddress, splitNum, GRAY, &stDst);        
#endif

#if BUTTON_ON
    DrawButton(g_s32ButnVirAddress, butnNum, &stSrc2, splitNum);
#endif 

    /* Start TDE's bitblit job                                              */
    s32Handle = HI_API_TDE_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        PERR();
    }

    hiRet = HI_API_TDE_BitBlt(s32Handle, &stSrc, &stDst, &pstOpt);
    if(hiRet)
    {
        printf("s32Ret:0x%x\n", hiRet);
        HI_API_TDE_EndJob(s32Handle, HI_FALSE, 0);
        PERR();
    }

    /* we reset the TDE operation for color key transparent                 */
    pstOpt.stColorSpace.bColorSpace = HI_TRUE;
    pstOpt.stColorSpace.enColorSpaceTarget = TDE_COLORSPACE_SRC;
    pstOpt.stColorSpace.stColorMax.u8B = 0;
    pstOpt.stColorSpace.stColorMax.u8G = 0;
    pstOpt.stColorSpace.stColorMax.u8R = 0;
    pstOpt.stColorSpace.stColorMin = pstOpt.stColorSpace.stColorMax;
    
    hiRet = HI_API_TDE_BitBlt(s32Handle, &stSrc2, &stDst, &pstOpt);
    if(hiRet)
    {
        printf("s32Ret:0x%x\n", hiRet);
        HI_API_TDE_EndJob(s32Handle, HI_FALSE, 0);
        PERR();
    }

    hiRet = HI_API_TDE_EndJob(s32Handle, HI_FALSE, 0);
    if(hiRet)
    {
        if(hiRet == HI_ERR_TDE_JOB_TIMEOUT)
        {
            printf("timeout!\n");
        }
        else
        {
            PERR();
        }
    }

    return HI_SUCCESS;
}

/*
 * Clear three or one of the buffer
 */
inline static void clearPicture(PICTYPE_E pictype)
{
    void *VirAddr = NULL;
    
    switch (pictype)
    {
        case PICTYPE_SCREEN:
            VirAddr= g_s32MappedMemFb;
            break;
        case PICTYPE_GRID:
            VirAddr= g_s32GridVirAddress;
            break;
        case PICTYPE_BUTN:
            VirAddr= g_s32ButnVirAddress;
            break;
        case PICTYPE_ALL:
            break;
        default:
            puts("undefined picture type!");
            return ;
    }
    if (pictype == PICTYPE_ALL)
    {
        memset(g_s32MappedMemFb, 0x00, g_u32SemSize);            
    }
    else
    {
        memset(VirAddr, 0x00, DEFAULT_PIC_SIZE);        
    }
}

/* Do vi2vo preview                                                         */
static void do_Set2815_2d1(void)
{
	//set 2d1，时序如4.2所示
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

/*
 *  set vi2vo split screen output
 */
int do_vivo_1d1(VI_DEV ViDevid,VI_CHN ViChn,VO_CHN VoChn)
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

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;

	ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_FALSE;
	ViChnAttr.bChromaResample = HI_FALSE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = DEF_COLOR;


	VoChnAttr.bZoomEnable = HI_TRUE;
	VoChnAttr.u32Priority = 1;
	VoChnAttr.stRect.s32X = 0;
	VoChnAttr.stRect.s32Y = 0;
	VoChnAttr.stRect.u32Height = 576;
	VoChnAttr.stRect.u32Width = 720;
	
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

int do_vivo_disable_1d1(VI_DEV ViDevid,VI_CHN ViChn,VO_CHN VoChn)
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

int do_vivo_9fp(void)
{
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;
	VO_PUB_ATTR_S VoAttr;
	VO_CHN_ATTR_S VoChnAttr[9];
	HI_U32 i,j;
	
	do_Set2815_2d1();
	
    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));
    memset(&VoAttr,0,sizeof(VO_PUB_ATTR_S));
    for (i = 0; i < 8; i++)
    {
    	memset(&VoChnAttr[i],0,sizeof(VO_CHN_ATTR_S));
	}
	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;

	ViChnAttr.enCapSel = VI_CAPSEL_BOTTOM;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_TRUE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = DEF_COLOR;   /* GREEN */

	VoChnAttr[0].bZoomEnable = HI_TRUE;
	VoChnAttr[0].u32Priority = 1;
	VoChnAttr[0].stRect.s32X = 0;
	VoChnAttr[0].stRect.s32Y = 0;
	VoChnAttr[0].stRect.u32Height = 192;
	VoChnAttr[0].stRect.u32Width = 240;
	
	VoChnAttr[1].bZoomEnable = HI_TRUE;
	VoChnAttr[1].u32Priority = 1;
	VoChnAttr[1].stRect.s32X = 240;
	VoChnAttr[1].stRect.s32Y = 0;
	VoChnAttr[1].stRect.u32Height = 192;
	VoChnAttr[1].stRect.u32Width = 240;

	VoChnAttr[2].bZoomEnable = HI_TRUE;
	VoChnAttr[2].u32Priority = 1;
	VoChnAttr[2].stRect.s32X = 480;
	VoChnAttr[2].stRect.s32Y = 0;
	VoChnAttr[2].stRect.u32Height = 192;
	VoChnAttr[2].stRect.u32Width = 240;

	VoChnAttr[3].bZoomEnable = HI_TRUE;
	VoChnAttr[3].u32Priority = 1;
	VoChnAttr[3].stRect.s32X = 0;
	VoChnAttr[3].stRect.s32Y = 192;
	VoChnAttr[3].stRect.u32Height = 192;
	VoChnAttr[3].stRect.u32Width = 240;

	VoChnAttr[4].bZoomEnable = HI_TRUE;
	VoChnAttr[4].u32Priority = 1;
	VoChnAttr[4].stRect.s32X = 240;
	VoChnAttr[4].stRect.s32Y = 192;
	VoChnAttr[4].stRect.u32Height = 192;
	VoChnAttr[4].stRect.u32Width = 240;

	VoChnAttr[5].bZoomEnable = HI_TRUE;
	VoChnAttr[5].u32Priority = 1;
	VoChnAttr[5].stRect.s32X = 480;
	VoChnAttr[5].stRect.s32Y = 192;
	VoChnAttr[5].stRect.u32Height = 192;
	VoChnAttr[5].stRect.u32Width = 240;

	VoChnAttr[6].bZoomEnable = HI_TRUE;
	VoChnAttr[6].u32Priority = 1;
	VoChnAttr[6].stRect.s32X = 0;
	VoChnAttr[6].stRect.s32Y = 384;
	VoChnAttr[6].stRect.u32Height = 192;
	VoChnAttr[6].stRect.u32Width = 240;

	VoChnAttr[7].bZoomEnable = HI_TRUE;
	VoChnAttr[7].u32Priority = 1;
	VoChnAttr[7].stRect.s32X = 240;
	VoChnAttr[7].stRect.s32Y = 384;
	VoChnAttr[7].stRect.u32Height = 192;
	VoChnAttr[7].stRect.u32Width = 240;

	for (i = 0; i < 4; i++)
    {
    	if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(i,&ViAttr))
    	{
    		printf("set VI attribute failed !\n");
    		return HI_FAILURE;
    	}
        for (j = 0; j < 2; j++)
        {
        	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(i,j,&ViChnAttr))
        	{
        		printf("set VI Chn attribute failed !\n");
        		return HI_FAILURE;
        	}
        }        
    }

 	if (HI_SUCCESS!=HI_MPI_VO_SetPubAttr(&VoAttr))
	{
		printf("set VO attribute failed !\n");
		return HI_FAILURE;		
	}
	
	for (i = 0; i < 8; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(i,&VoChnAttr[i]))
		{
			printf("set VO 0 Chn attribute failed !\n");
			return HI_FAILURE;		
		}
	}
	
	if (HI_SUCCESS!=HI_MPI_VO_Enable())
	{
		printf("set VO  enable failed !\n");
		return HI_FAILURE;		
	}
	
	for (i = 0; i < 8; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VO_EnableChn(i))
		{
			printf("set VO 0 Chn attribute failed !\n");
			return HI_FAILURE;		
		}
	}

	for (i = 0; i < 4; i++)
    {
    	if (HI_SUCCESS!=HI_MPI_VI_Enable(i))
    	{
    		printf("set VI  enable failed !\n");
    		return HI_FAILURE;		
    	}
        for (j = 0; j < 2; j++)
        {
        	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(i,j))
        	{
        		printf("set VI Chn enable failed !\n");
        		return HI_FAILURE;		
        	}
        }        
    }

	for (i = 0; i < 8; i++)
    {
    	if (HI_SUCCESS!=HI_MPI_VI_BindOutput(i/2,i%2,i))
    	{
    		printf("set VI Chn enable failed !\n");
    		return HI_FAILURE;		
    	}
    }

	return HI_SUCCESS;
}

int do_vivo_disable_9fp()
{
	HI_U32 i;
	
	for (i = 0; i < 8; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VI_UnBindOutput(i/2,i%2,0))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}

		if (HI_SUCCESS != HI_MPI_VO_DisableChn(i))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_DisableChn(i/2,i%2))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}
	}

	for (i = 0; i < 4; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VI_Disable(i))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}
	}
	
	if (HI_SUCCESS != HI_MPI_VO_Disable())
	{
		DEBUGPOS;
		return HI_FAILURE;		
	}

	return HI_SUCCESS;
}

int do_vi_set_4cif(void)
{
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;

	do_Set2815_2d1();
    memset(&ViAttr,0,sizeof(VI_PUB_ATTR_S));
    memset(&ViChnAttr,0,sizeof(VI_CHN_ATTR_S));

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;
	
	ViChnAttr.enCapSel = VI_CAPSEL_TOP;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_TRUE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	
	if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(0,&ViAttr))
	{
		printf("set VI attribute failed !\n");
		return -1;
	}  
	if (HI_SUCCESS!=HI_MPI_VI_SetPubAttr(1,&ViAttr))
	{
		printf("set VI attribute failed !\n");
		return -1;
	}  	
	
	if (HI_SUCCESS!=HI_MPI_VI_Enable(0))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}
	if (HI_SUCCESS!=HI_MPI_VI_Enable(1))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}

	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(0,0,&ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(0,1,&ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}
	
	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(1,0,&ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS!=HI_MPI_VI_SetChnAttr(1,1,&ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}			

	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(0,0))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}	

	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(0,1))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}	
	
	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(1,0))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}	

	if (HI_SUCCESS!=HI_MPI_VI_EnableChn(1,1))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;		
	}	

	return HI_SUCCESS;
}


int do_vo_set_4cif(HI_BOOL bZoom)
{
	VO_PUB_ATTR_S VoAttr;
    
	VO_CHN_ATTR_S VoChnAttr0;
	VO_CHN_ATTR_S VoChnAttr1;
	VO_CHN_ATTR_S VoChnAttr2;
	VO_CHN_ATTR_S VoChnAttr3;

    #define SPLIT_4CIF_WIDTH    360

    memset(&VoAttr,0,sizeof(VO_PUB_ATTR_S));

	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = DEF_COLOR/*10*/;
    
	VoChnAttr0.bZoomEnable = bZoom;
	VoChnAttr0.u32Priority = 1;
	VoChnAttr0.stRect.s32X = 0;
	VoChnAttr0.stRect.s32Y = 0;
	VoChnAttr0.stRect.u32Height = 288;
	VoChnAttr0.stRect.u32Width = SPLIT_4CIF_WIDTH;
	
	VoChnAttr1.bZoomEnable = bZoom;
	VoChnAttr1.u32Priority = 1;
	VoChnAttr1.stRect.s32X = SPLIT_4CIF_WIDTH;
	VoChnAttr1.stRect.s32Y = 0;
	VoChnAttr1.stRect.u32Height = 288;
	VoChnAttr1.stRect.u32Width = SPLIT_4CIF_WIDTH;

	VoChnAttr2.bZoomEnable = bZoom;
	VoChnAttr2.u32Priority = 1;
	VoChnAttr2.stRect.s32X = 0;
	VoChnAttr2.stRect.s32Y = 288;
	VoChnAttr2.stRect.u32Height = 288;
	VoChnAttr2.stRect.u32Width = SPLIT_4CIF_WIDTH;

	VoChnAttr3.bZoomEnable = bZoom;
	VoChnAttr3.u32Priority = 1;
	VoChnAttr3.stRect.s32X = SPLIT_4CIF_WIDTH;
	VoChnAttr3.stRect.s32Y = 288;
	VoChnAttr3.stRect.u32Height = 288;
	VoChnAttr3.stRect.u32Width = SPLIT_4CIF_WIDTH;
    
 	if (HI_SUCCESS!=HI_MPI_VO_SetPubAttr(&VoAttr))
	{
		printf("set VO attribute failed !\n");
		return -1;		
	}
	if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(0,&VoChnAttr0))
	{
		printf("set VO 0 Chn attribute failed !\n");
		return -1;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(1,&VoChnAttr1))
	{
		printf("set VO 1 Chn attribute failed !\n");
		return -1;		
	}
	if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(2,&VoChnAttr2))
	{
		printf("set VO 0 Chn attribute failed !\n");
		return -1;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_SetChnAttr(3,&VoChnAttr3))
	{
		printf("set VO 1 Chn attribute failed !\n");
		return -1;		
	}

    return 0;
}

void do_vibindvo_4cif(HI_U32 u32Flags)
{

	if (HI_SUCCESS!=HI_MPI_VO_Enable())
	{
		printf("set VO  enable failed !\n");
		return;		
	}
	
	if (HI_SUCCESS!=HI_MPI_VO_EnableChn(0))
	{
		printf("set VO Chn 0 enable failed !\n");
		return;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_EnableChn(1))
	{
		printf("set VO Chn 1 enable failed !\n");
		return;		
	}
	
	if (HI_SUCCESS!=HI_MPI_VO_EnableChn(2))
	{
		printf("set VO Chn 2 enable failed !\n");
		return;		
	}

	if (HI_SUCCESS!=HI_MPI_VO_EnableChn(3))
	{
		printf("set VO Chn 3 enable failed !\n");
		return;		
	}
	
	if (0 == u32Flags)
	{
		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(0,0,0))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(0,1,1))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(1,0,2))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(1,1,3))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}
    }
    else
    {
		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(2,0,0))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(2,1,1))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(3,0,2))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_BindOutput(3,1,3))
		{
			printf("HI_MPI_VI_BindOutput failed !\n");
			return;		
		}		
    }
    
    return;    
}

int do_vivo_4cif(void)
{
    do_vo_set_4cif(HI_TRUE);
    do_vi_set_4cif();
    do_vibindvo_4cif(0);      

    return HI_SUCCESS;
}

int do_vivo_disable_4cif(HI_VOID)
{
	HI_U32 i;
	
	for (i = 0; i < 4; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VI_UnBindOutput(i/2,i%2,0))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}

		if (HI_SUCCESS!=HI_MPI_VO_DisableChn(i))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}

		if (HI_SUCCESS!=HI_MPI_VI_DisableChn(i/2,i%2))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (HI_SUCCESS!=HI_MPI_VI_Disable(i))
		{
			DEBUGPOS;
			return HI_FAILURE;		
		}
	}
	
	if (HI_SUCCESS!=HI_MPI_VO_Disable())
	{
		DEBUGPOS;
		return HI_FAILURE;		
	}

	return HI_SUCCESS;
}

static void AbnormalOut(void)
{
    hifb_close();
    (void)HI_MPI_SYS_Exit();
    (void)HI_MPI_VB_Exit();    
}

static void usage(void)
{
    puts("----------------------");
    puts("Usage:");
    puts("      type 1 for full screen");
    puts("      type 4 for  4 split screen");
    puts("      type 9 for  9 split screen");
    puts("      type q for exit!");
    puts("-------------------------------------");    
    printf("\033[0;32mpress any key to disable channel!\033[0;39m\n");
}

HI_VOID HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        if (-1 != g_s32FrameBufferFd)
        {
            hifb_close();
        }
        
    	(HI_VOID)HI_MPI_SYS_Exit();
    	(HI_VOID)HI_MPI_VB_Exit();
    	printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    HI_CHAR ch = 'x';

    /* for vi2vo show                                                           */
	MPP_SYS_CONF_S stSysConf = {0};
	VB_CONF_S stVbConf ={0};

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 20;	
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 60;	
	stSysConf.u32AlignWidth = 64;

    /* if program can not run anymore, you may try 'reset' adhere command  */
    if (argc > 1)
    {
        if (!strcmp(argv[1],"reset"))
        {
           	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
        	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
        	return HI_SUCCESS;
        }
    }

    /* process abnormal case                                                */
    signal(SIGINT, HandleSig);
    signal(SIGTERM, HandleSig);    

	CHECK(HI_MPI_VB_SetConf(&stVbConf),"HI_MPI_VB_SetConf");
	CHECK(HI_MPI_VB_Init(),"HI_MPI_VB_Init");
	CHECK(HI_MPI_SYS_SetConf(&stSysConf),"HI_MPI_SYS_SetConf");
	CHECK(HI_MPI_SYS_Init(),"HI_MPI_SYS_Init");	
	CHECK(hifb_open(TEST_16BPP,FB_NAME_0),"hifb_open");
    CHECK(HI_API_TDE_Open(),"HI_API_TDE_Open");

    /* main control loop                                                    */
    do
    {
        usage();
        clearPicture(PICTYPE_ALL);

        switch (ch)
        {
            case '1' :
                DrawSplitScreenDisplay(1,3);
                do_vivo_1d1(0,0,0);
                PAUSE;
                PAUSE;
                do_vivo_disable_1d1(0,0,0);
                break;
            case '4' :
                DrawSplitScreenDisplay(4,3);
                do_vivo_4cif();
                PAUSE;
                PAUSE;
                do_vivo_disable_4cif();
                break;
            case '9' :
                DrawSplitScreenDisplay(9,3);
                do_vivo_9fp();
                PAUSE;
                PAUSE;
                do_vivo_disable_9fp();
                break;
            case 'x' :
                DrawSplitScreenDisplay(1,3);
                do_vivo_1d1(0,0,0);
                PAUSE;
                do_vivo_disable_1d1(0,0,0);
                break;
            default:
                printf("\033[0;31minvalid split number!\033[0;39m\n");
                break;
        }
        printf("^_* Input split number >");
    } while ((ch = getchar()) != 'q');
    
    CHECK(HI_API_TDE_Close(),"HI_MPI_SYS_Exit");
    hifb_close();

    /* de-init sys and vb */
	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

