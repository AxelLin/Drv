#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <linux/fb.h> 
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "hi_comm_sys.h"
#include "mpi_sys.h"

#include "tde_interface.h"
#include "hifb.h"    

typedef struct tag_OSD_BITMAPINFOHEADER{
        HI_U16      biSize;
        HI_U32       biWidth;
        HI_S32       biHeight;
        HI_U16       biPlanes;
        HI_U16       biBitCount;
        HI_U32      biCompression;
        HI_U32      biSizeImage;
        HI_U32       biXPelsPerMeter;
        HI_U32       biYPelsPerMeter;
        HI_U32      biClrUsed;
        HI_U32      biClrImportant;
} OSD_BITMAPINFOHEADER;

typedef struct tag_OSD_BITMAPFILEHEADER {
        HI_U32   bfSize;
        HI_U16    bfReserved1;
        HI_U16    bfReserved2;
        HI_U32   bfOffBits;
} OSD_BITMAPFILEHEADER; 

typedef struct tag_OSD_RGBQUAD {
        HI_U8    rgbBlue;
        HI_U8    rgbGreen;
        HI_U8    rgbRed;
        HI_U8    rgbReserved;
} OSD_RGBQUAD;

typedef struct tag_OSD_BITMAPINFO {
    OSD_BITMAPINFOHEADER    bmiHeader;
    OSD_RGBQUAD                 bmiColors[1];
} OSD_BITMAPINFO;

typedef struct tag_OSD_Logo
{
    HI_U32    width;        /* out */
    HI_U32    height;       /* out */
    HI_U32    stride;       /* in */
    HI_U8 *   pRGBBuffer;   /* in */
}OSD_LOGO_T;

int LoadBMP(const char *filename, OSD_LOGO_T *pVideoLogo)
{
    FILE *pFile;
    HI_U16  i,j;

    HI_U16  bfType ;
    HI_U32  w,h;
    HI_U16 Bpp;
    HI_U16 dstBpp;

    OSD_BITMAPFILEHEADER  bmpFileHeader;
    OSD_BITMAPINFO            bmpInfo;

    HI_U8 *pOrigBMPBuf;
    HI_U8 *pRGBBuf;
    HI_U32 stride;

   if( (pFile = fopen((char *)filename, "rb")) == NULL)
    {
        printf("Open file faild:%s!\n", filename);
        return -1;
    }

    (void)fread(&bfType, 1, sizeof(bfType), pFile);
    (void)fread(&bmpFileHeader, 1, sizeof(OSD_BITMAPFILEHEADER), pFile);
    (void)fread(&bmpInfo, 1, sizeof(OSD_BITMAPINFO), pFile);

    if(bfType != 0x4d42)
    {
        printf("not bitmap file\n");
        fclose(pFile);
        return -1;
    }
    
    if(bmpInfo.bmiHeader.biCompression != 0)
    {
        printf("Unsupport compressed bitmap file!\n");
        fclose(pFile);
        return -1;
    }

    Bpp = bmpInfo.bmiHeader.biBitCount/8;
    if((Bpp < 2))
    {
        /* only support 1555¡¢8888  888 bitmap */
        printf("bitmap format not supported!\n");
        fclose(pFile);
        return -1;
    }
    
    pVideoLogo->width = (HI_U16)bmpInfo.bmiHeader.biWidth;
    pVideoLogo->height = (HI_U16)((bmpInfo.bmiHeader.biHeight>0)?bmpInfo.bmiHeader.biHeight:(-bmpInfo.bmiHeader.biHeight));
    w = pVideoLogo->width;
    h = pVideoLogo->height;

    if(bmpInfo.bmiHeader.biHeight < 0)
    {
        printf("bmpInfo.bmiHeader.biHeight < 0\n");
        fclose(pFile);
        return -1;
    }

    stride = w*Bpp;
    if(stride%4)
    {
        stride = (stride/4 + 1)*4;
    }
    
    /* RGB8888 or RGB1555 */
    pOrigBMPBuf = (HI_U8 *)malloc(h*stride);
    if(NULL == pOrigBMPBuf)
    {
        printf("not enough memory to malloc!\n");
        fclose(pFile);
        return -1;
    }
    
    pRGBBuf = pVideoLogo->pRGBBuffer;

    fseek(pFile, bmpFileHeader.bfOffBits, 0);
    if(fread(pOrigBMPBuf, 1, h*stride, pFile) != (h*stride) )
    {
        printf("fread error!\n");
    }

    if(Bpp > 2)
    {
        dstBpp = 4;
    }
    else
    {
        dstBpp = 2;
    }

    if(0 == pVideoLogo->stride)
    {
        pVideoLogo->stride = pVideoLogo->width * dstBpp;
    }
    
    for(i=0; i<h; i++)
    {
        for(j=0; j<w; j++)
        {
            memcpy(pRGBBuf + i*pVideoLogo->stride + j*dstBpp, pOrigBMPBuf + ((h-1)-i)*stride+j*Bpp, Bpp);
           
            if(dstBpp == 4)
            {
                *(pRGBBuf + i*pVideoLogo->stride + j*dstBpp + 3) = 0xff; /*alpha*/
            }
        }
        
    }

    free(pOrigBMPBuf);
    pOrigBMPBuf = NULL;

    fclose(pFile);
    return 0;
}

int LoadImage(const char *filename, OSD_LOGO_T *pVideoLogo)
{
    if(filename && strchr(filename, '.' ) 
        && (strcasecmp(strchr(filename, '.'), ".bmp") == 0))
    {
        if(pVideoLogo == NULL)
        {
            printf("null pointer!\n");
            return -1;
        }
        if(0 != LoadBMP(filename, pVideoLogo))
        {
            printf("OSD_LoadBMP error!\n");
            return -1;
        }
    }
    else
    {
        printf("Unsupported image file!\n");
        return -1;
    }

    return 0;
}

HI_S32 CreateSurfaceByBitMap(const HI_CHAR *pszFileName, TDE_SURFACE_S *pstSurface, HI_U8 *pu8Virt)
{
    OSD_LOGO_T stLogo;
    stLogo.pRGBBuffer = pu8Virt;
    stLogo.stride = 0;
    if(LoadImage(pszFileName, &stLogo) < 0)
    {
        printf("load bmp error!\n");
        return -1;
    }

    pstSurface->u16Height = stLogo.height;
    pstSurface->u16Width = stLogo.width;
    pstSurface->u16Stride = stLogo.stride;
    switch(stLogo.stride/stLogo.width)
    {
        case 2:
            pstSurface->enColorFmt = TDE_COLOR_FMT_RGB1555;
            break;
        case 4:
            pstSurface->enColorFmt = TDE_COLOR_FMT_RGB8888;
            break;
    }
    
    return 0;
}

#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static const HI_CHAR *pszImageNames[] =
{
    "src/apple.bmp",
    "src/applets.bmp",
    "src/calendar.bmp",
    "src/foot.bmp",
    "src/gmush.bmp",
    "src/gimp.bmp",
    "src/gsame.bmp",
    "src/keys.bmp"
};

#define N_IMAGES (HI_S32)((sizeof (pszImageNames) / sizeof (pszImageNames[0])))

#define BACKGROUND_NAME  "src/background.bmp"

#define PIXFMT  TDE_COLOR_FMT_RGB8888
#define BPP     4
#define SCREEN_WIDTH    720
#define SCREEN_HEIGHT   576
#define CYCLE_LEN       60
#define MIN_VIDEO_MEMORY  (720*576*4*3 + 48*48*4*8)

static HI_S32   g_s32FrameNum;
static TDE_SURFACE_S g_stScreen[2];
static TDE_SURFACE_S g_stBackGround;
static TDE_SURFACE_S g_stImgSur[N_IMAGES];


static HI_VOID circumrotate (HI_U32 u32CurOnShow)
{
    TDE_HANDLE s32Handle;
    TDE_OPT_S stOpt = {0};
    HI_FLOAT eXMid, eYMid;
    HI_FLOAT eRadius;
    HI_U32 i;
    HI_FLOAT f;
    HI_U32 u32NextOnShow;
    
    u32NextOnShow = !u32CurOnShow;
    stOpt.enDataConv = TDE_DATA_CONV_NONE;
    stOpt.stAlpha.enOutAlphaFrom = TDE_OUTALPHA_FROM_SRC;
    stOpt.stColorSpace.bColorSpace = HI_TRUE;
    stOpt.stColorSpace.enColorSpaceTarget = TDE_COLORSPACE_SRC;
    memset(&stOpt.stColorSpace.stColorMin, 0, sizeof(TDE_RGB_S));
    memset(&stOpt.stColorSpace.stColorMax, 0, sizeof(TDE_RGB_S));

    f = (float) (g_s32FrameNum % CYCLE_LEN) / CYCLE_LEN;

    eXMid = g_stBackGround.u16Width/2.16f;
    eYMid = g_stBackGround.u16Height/2.304f;

    eRadius = MIN (eXMid, eYMid) / 2.0f;

    /* 1. start job */
    s32Handle = HI_API_TDE_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        printf("start job failed!\n");
        return ;
    }

    /* 2. bitblt background to screen */
    HI_API_TDE_BitBlt(s32Handle, &g_stBackGround, &g_stScreen[u32NextOnShow], &stOpt);

    for(i = 0; i < N_IMAGES; i++)
    {
        HI_FLOAT ang;
        HI_FLOAT r;
        HI_S32 xpos, ypos;
        TDE_SURFACE_S stSurface;

        ang = 2.0f * (HI_FLOAT) M_PI * (HI_FLOAT) i / N_IMAGES - f * 2.0f * (HI_FLOAT) M_PI;
        r = eRadius + (eRadius / 3.0f) * sinf (f * 2.0 * M_PI);

        /* 3. calculate new pisition */
        xpos = eXMid + r * cosf (ang) - g_stImgSur[i].u16Width / 2.0f;
        ypos = eYMid + r * sinf (ang) - g_stImgSur[i].u16Height / 2.0f;
        stSurface = g_stScreen[u32NextOnShow];
        stSurface.pu8PhyAddr = g_stScreen[u32NextOnShow].pu8PhyAddr + ypos * g_stScreen[u32NextOnShow].u16Stride
                                + xpos * BPP;

        /* 4. bitblt image to screen */
        HI_API_TDE_BitBlt(s32Handle, &g_stImgSur[i], &stSurface, &stOpt);
    }

    /* 5. submit job */
    HI_API_TDE_EndJob(s32Handle, HI_TRUE, 10);
    
    g_s32FrameNum++;

    return;
}

HI_S32 main(HI_VOID)
{
    HI_U32 u32Size;
    HI_S32 s32Fd;
    HI_U32 u32Times;
    HI_U8* pu8Screen;
        
    HI_U32 u32PhyAddr;
    HI_S32 s32Ret = -1;
    HI_U32 i = 0;

    MPP_SYS_CONF_S struSysConf;
    HI_S32 s32ret;

    /* mpi sys init */
    HI_MPI_SYS_Exit();
    struSysConf.u32AlignWidth = 64;
	s32ret = HI_MPI_SYS_SetConf(&struSysConf);
	if (HI_SUCCESS != s32ret)
	{
        printf("HI_MPI_SYS_SetConf err\n");
		return s32ret;
	}
	s32ret = HI_MPI_SYS_Init();
	if (HI_SUCCESS != s32ret)
	{
        printf("HI_MPI_SYS_Init err\n");
		return s32ret;
	}
     
    struct fb_fix_screeninfo stFixInfo;
    struct fb_var_screeninfo stVarInfo;
    struct fb_bitfield stR32 = {16, 8, 0};
    struct fb_bitfield stG32 = {8, 8, 0};
    struct fb_bitfield stB32 = {0, 8, 0}; 	
    struct fb_bitfield stA32 = {24, 8, 0};

    /* 1. open tde device */
    HI_API_TDE_Open();

    /* 2. framebuffer operation */
    s32Fd = open("/dev/fb/0", O_RDWR);
    if (s32Fd == -1)
    {
        printf("open frame buffer device error\n");
        goto FB_OPEN_ERROR; 
    }

    stVarInfo.xres_virtual	 	= SCREEN_WIDTH;    
    stVarInfo.yres_virtual		= SCREEN_HEIGHT*2;    
    stVarInfo.xres      		= SCREEN_WIDTH;  
    stVarInfo.yres      		= SCREEN_HEIGHT;  
    stVarInfo.activate  		= FB_ACTIVATE_FORCE;  
    stVarInfo.bits_per_pixel	= 32;     
    stVarInfo.xoffset = 0;
    stVarInfo.yoffset = 0;
    stVarInfo.red   = stR32;
    stVarInfo.green = stG32; 
    stVarInfo.blue  = stB32;
    stVarInfo.transp = stA32;
    
    if (ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVarInfo) < 0)
    {
        printf("process frame buffer device error\n");
        goto FB_PROCESS_ERROR0;
    }

    if (ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFixInfo) < 0)
    {
        printf("process frame buffer device error\n");
        goto FB_PROCESS_ERROR0;
    }

    u32Size 	= stFixInfo.smem_len;
    if(u32Size < MIN_VIDEO_MEMORY)
    {
        printf("need more video memory to run the sample, the minimum size is %d\n", MIN_VIDEO_MEMORY);
        goto FB_PROCESS_ERROR0;
    }
    u32PhyAddr  = stFixInfo.smem_start;  
    pu8Screen   = mmap(NULL, u32Size, PROT_READ|PROT_WRITE, MAP_SHARED, s32Fd, 0);
      
    memset(pu8Screen, 0xff, SCREEN_WIDTH*SCREEN_HEIGHT*4);

    /* 3. create surface */
    g_stScreen[0].enColorFmt = PIXFMT;
    g_stScreen[0].pu8PhyAddr = (HI_U8*)u32PhyAddr;
    g_stScreen[0].u16Width = SCREEN_WIDTH;
    g_stScreen[0].u16Height = SCREEN_HEIGHT;
    g_stScreen[0].u16Stride = g_stScreen[0].u16Width * BPP;

    g_stScreen[1] = g_stScreen[0];
    g_stScreen[1].pu8PhyAddr = g_stScreen[0].pu8PhyAddr + g_stScreen[0].u16Stride * g_stScreen[0].u16Height;

    g_stBackGround.pu8PhyAddr = g_stScreen[1].pu8PhyAddr + g_stScreen[1].u16Stride * g_stScreen[1].u16Height;
    CreateSurfaceByBitMap(BACKGROUND_NAME, &g_stBackGround, pu8Screen + ((HI_U32)g_stBackGround.pu8PhyAddr - u32PhyAddr));
    g_stImgSur[0].pu8PhyAddr = g_stBackGround.pu8PhyAddr + g_stBackGround.u16Stride * g_stBackGround.u16Height;
    for(i = 0; i < N_IMAGES - 1; i++)
    {
        CreateSurfaceByBitMap(pszImageNames[i], &g_stImgSur[i], pu8Screen + ((HI_U32)g_stImgSur[i].pu8PhyAddr - u32PhyAddr));
        g_stImgSur[i+1].pu8PhyAddr = g_stImgSur[i].pu8PhyAddr + g_stImgSur[i].u16Stride * g_stImgSur[i].u16Height;
    }
    CreateSurfaceByBitMap(pszImageNames[i], &g_stImgSur[i], pu8Screen + ((HI_U32)g_stImgSur[i].pu8PhyAddr - u32PhyAddr));
    
    g_s32FrameNum = 0;

    /* 3. use tde and framebuffer to realize rotational effect */
    for (u32Times = 0; u32Times < 1000; u32Times++)
    {        
            circumrotate(u32Times%2);
            stVarInfo.yoffset = (u32Times%2)?0:576;

            /*set frame buffer start position*/ 
            if (ioctl(s32Fd, FBIOPAN_DISPLAY, &stVarInfo) < 0)
            {
                printf("process frame buffer device error\n");
                goto FB_PROCESS_ERROR1; 
            }
    }

    s32Ret = 0;

FB_PROCESS_ERROR1:    
    munmap(pu8Screen, u32Size);
FB_PROCESS_ERROR0:    
    close(s32Fd);
FB_OPEN_ERROR:    
    HI_API_TDE_Close();

    return s32Ret;
}


