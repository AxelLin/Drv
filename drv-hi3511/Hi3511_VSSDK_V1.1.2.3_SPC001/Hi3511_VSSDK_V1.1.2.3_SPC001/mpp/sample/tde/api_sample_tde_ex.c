#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h> 
#include <sys/mman.h>
#include <fcntl.h>

#include "hi_comm_sys.h"
#include "mpi_sys.h"

#include "tde_interface.h"
#include "hifb.h"    

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 576
#define SCREEN_OFFSET (SCREEN_WIDTH * SCREEN_HEIGHT * 4)
typedef struct hiTDE_RECT_S
{
    HI_S32 x;
    HI_S32 y;
    HI_U32 w;
    HI_U32 h;
}TDE_RECT_S;

/*****************************************************************************
 Prototype    : tde_getbpp
 Description  : 根据像素格式获取每个像素所占有的字节数
 Input        : TDE_COLOR_FMT_E enColorFmt  
 Output       : None
 Return Value : static
 Calls        : 
 Called By    : 
 
*****************************************************************************/
static inline HI_S32 tde_getbpp(TDE_COLOR_FMT_E enColorFmt)
{
    switch(enColorFmt)
    {
        case TDE_COLOR_FMT_RGB444:
        case TDE_COLOR_FMT_RGB4444:
        case TDE_COLOR_FMT_RGB555:
        case TDE_COLOR_FMT_RGB565:
        case TDE_COLOR_FMT_RGB1555:
            return 2;
            
        case TDE_COLOR_FMT_RGB888:
        case TDE_COLOR_FMT_RGB8888:
            return 4;
            
        default:
            return -1;
    }
}

/*****************************************************************************
 Prototype    : tde_subsurface
 Description  : 根据源surface中的一块矩形区域生成一个子surface
 Input        : TDE_SURFACE_S *pstSurface     
                TDE_RECT_S *pstRect           
 Output       : TDE_SURFACE_S *pstSubSurface  
 
 Return Value : 
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  tde_subsurface(TDE_SURFACE_S *pstSurface, TDE_RECT_S *pstRect, TDE_SURFACE_S *pstSubSurface)
{
    HI_S32 s32Bpp;

    if(NULL == pstRect)
    {
        memcpy(pstSubSurface, pstSurface, sizeof(TDE_SURFACE_S));
        return HI_SUCCESS;
    }
    
    if((pstRect->x < 0) || (pstRect->y < 0))
    {
        printf("invalide parameter!\n");
        return HI_FAILURE;
    }

    s32Bpp = tde_getbpp(pstSurface->enColorFmt);
    if(s32Bpp < 0)
    {
        printf("invalide colorformat!\n");
        return HI_FAILURE;
    }
        
    /* clip area */
    if((pstRect->x + pstRect->w) > pstSurface->u16Width)
    {
        pstRect->w = pstSurface->u16Width - pstRect->x;
    }

    if((pstRect->y + pstRect->h) > pstSurface->u16Height)
    {
        pstRect->h = pstSurface->u16Height - pstRect->y;
    }

    pstSubSurface->pu8PhyAddr = pstSurface->pu8PhyAddr + pstRect->y * pstSurface->u16Stride + pstRect->x * s32Bpp;
    pstSubSurface->u16Width = pstRect->w;
    pstSubSurface->u16Height = pstRect->h;
    pstSubSurface->enColorFmt = pstSurface->enColorFmt;
    pstSubSurface->u16Stride = pstSurface->u16Stride;

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype    : tde_memset
 Description  : 将surface中一块矩形区域填成指定的值
 Input        : TDE_SURFACE_S *pstSurface  
                TDE_RECT_S *pstRect        
                HI_U32 value               
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
*****************************************************************************/
HI_S32  tde_memset(TDE_SURFACE_S *pstSurface, TDE_RECT_S *pstRect, HI_U32 value)
{
    TDE_HANDLE s32Handle;
    TDE_OPT_S stOpt = {0};
    TDE_RGB_S stFillColor;
    TDE_SURFACE_S stSubSurface;
    HI_S32 s32Bpp;

    if(tde_subsurface(pstSurface, pstRect, &stSubSurface) < 0)
    {
        return HI_FAILURE;
    }

    if(TDE_COLOR_FMT_RGB8888 != stSubSurface.enColorFmt)
    {
        s32Bpp = tde_getbpp(stSubSurface.enColorFmt);
        stSubSurface.enColorFmt = TDE_COLOR_FMT_RGB8888;
        stSubSurface.u16Width = (stSubSurface.u16Width * s32Bpp)/4;
    }

    s32Handle = HI_API_TDE_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        printf("start job failed!\n");
        return HI_FAILURE;
    }

    stFillColor.u8B = value&0xff;
    stFillColor.u8G = (value>>8)&0xff;
    stFillColor.u8R = (value>>16)&0xff;
    stOpt.stAlpha.enOutAlphaFrom = TDE_OUTALPHA_FROM_REG;
    stOpt.stAlpha.u8AlphaGlobal = (value>>24)&0xff;

    HI_API_TDE_SolidDraw(s32Handle, &stSubSurface, stFillColor, &stOpt);

    HI_API_TDE_EndJob(s32Handle, HI_TRUE, 20);

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype    : tde_deflicker_partly
 Description  : 将源surface中的一块矩形区域2阶抗闪烁后输出到目标surface上一块矩形区域
 Input        : TDE_SURFACE_S *pstSrcSurface  
                TDE_RECT_S *pstSrcRect        
                TDE_SURFACE_S *pstDstSurface  
                TDE_RECT_S *pstDstRect        
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32  tde_deflicker_partly(TDE_SURFACE_S *pstSrcSurface, TDE_RECT_S *pstSrcRect, TDE_SURFACE_S *pstDstSurface, TDE_RECT_S *pstDstRect)
{
    TDE_HANDLE s32Handle;
    TDE_DEFLICKER_COEF_S stDeflicker;
    TDE_SURFACE_S stSrcSub;
    TDE_SURFACE_S stDstSub;
    HI_U8 u8Coef = 0x80;
    
    if(tde_subsurface(pstSrcSurface, pstSrcRect, &stSrcSub) < 0)
    {
        return HI_FAILURE;
    }
    
    if(tde_subsurface(pstDstSurface, pstDstRect, &stDstSub) < 0)
    {
        return HI_FAILURE;
    }

    s32Handle = HI_API_TDE_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        printf("start job failed!\n");
        return HI_FAILURE;
    }

    stDeflicker.u32HDfLevel = 2;
    stDeflicker.u32VDfLevel = 2;
    stDeflicker.pu8HDfCoef = &u8Coef;
    stDeflicker.pu8VDfCoef = &u8Coef;
    HI_API_TDE_Deflicker(s32Handle, &stSrcSub, &stDstSub, stDeflicker);

    HI_API_TDE_EndJob(s32Handle, HI_TRUE, 20);
    
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype    : tde_createsurfacebyfile
 Description  : 根据输入的bits文件创建surface，并将bits位图的数据写到surface
                上
 Input        : TDE_SURFACE_S *pstSurface  
                HI_CHAR *pscFileName       
                HI_U32 u32PhyAddr          
                HI_U8 *pu8VirtAddr         
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
*****************************************************************************/
HI_S32 tde_createsurfacebyfile(const HI_CHAR *pscFileName, TDE_SURFACE_S *pstSurface, HI_U32 u32PhyAddr, HI_U8 *pu8VirtAddr)
{
    FILE *fp;
    HI_U32 colorfmt, w, h, stride;
    
    fp = fopen(pscFileName, "rb");
    if(NULL == fp)
    {
        printf("error when open file %s, line:%d\n", pscFileName, __LINE__); 
        return HI_FAILURE;
    }

    fread(&colorfmt, 1, 4, fp);
    fread(&w, 1, 4, fp);
    fread(&h, 1, 4, fp);
    fread(&stride, 1, 4, fp);

    pstSurface->pu8PhyAddr = (HI_U8*)u32PhyAddr;
    pstSurface->enColorFmt = colorfmt;
    pstSurface->u16Width = w;
    pstSurface->u16Height = h;
    pstSurface->u16Stride = stride;

    fread(pu8VirtAddr, 1, stride*h, fp);

    fclose(fp);

    return HI_SUCCESS;
}

static int tde_select(TDE_SURFACE_S *pstSurface, TDE_RECT_S *pstRect)
{
    TDE_HANDLE s32Handle;
    TDE_OPT_S stOpt = {0};
    TDE_SURFACE_S stSub;
    
    if(tde_subsurface(pstSurface, pstRect, &stSub) < 0)
    {
        return HI_FAILURE;
    }

    s32Handle = HI_API_TDE_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        printf("start job failed!\n");
        return HI_FAILURE;
    }

    stOpt.enDataConv = TDE_DATA_CONV_ROP;
    stOpt.enRopCode = TDE_ROP_PSn;
    stOpt.stAlpha.enOutAlphaFrom = TDE_OUTALPHA_FROM_SRC;

    HI_API_TDE_BitBlt(s32Handle, &stSub, &stSub, &stOpt);
    
    HI_API_TDE_EndJob(s32Handle, HI_TRUE, 20);
    
    return HI_SUCCESS;
}


static int fblayer_init(char *pszLayer)
{
    int fd;
    struct fb_bitfield stA32 = {24, 8, 0};
    struct fb_bitfield stR32 = {16, 8, 0};
    struct fb_bitfield stG32 = {8, 8, 0};
    struct fb_bitfield stB32 = {0, 8, 0};    
    struct fb_var_screeninfo var;

    fd = open(pszLayer, O_RDWR);
    if(fd < 0)
    {
        printf("open %s failed!\n", pszLayer);
        return -1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
   	    printf("Get variable screen info failed!\n");
        close(fd);
        return -1;
    }

    var.xres = var.xres_virtual = SCREEN_WIDTH;
    var.yres = var.yres_virtual = SCREEN_HEIGHT;
    
    var.transp= stA32;
    var.red = stR32;
    var.green = stG32;
    var.blue = stB32;
    var.bits_per_pixel = 32;

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
   	    printf("Put variable screen info failed!\n");
        close(fd);
        return -1;
    }

    return fd;
}

static int fblayer_term(int fd)
{
    return close(fd);
}

static int fblayer_getfix(int fd, unsigned int *phyaddr, unsigned int *length)
{
    struct fb_fix_screeninfo fix;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
   	    printf("Get fix screen info failed!\n");
        return -1;
    }
    
    *phyaddr = fix.smem_start;
    *length = fix.smem_len;

    return 0;
}

static void * fblayer_mmap(int fd, unsigned int length)
{
    void *ptr;
    ptr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == ptr)
    {
        return NULL;
    }

    memset(ptr, 0, length);
    
    return ptr;
}

static int fblayer_munmap(void *start, unsigned int length)
{
    return munmap(start, length);
}

#define N_IMAGES 6

#define START_OFFSET_X 50
#define START_OFFSET_Y 60

const HI_CHAR *pszImageNames[N_IMAGES] =
{
    "src/menu_games.bits",
    "src/menu_citymap.bits",
    "src/menu_app.bits",
    "src/item_num.bits",
    "src/item_menu.bits",
    "src/item_prog.bits",
};

static TDE_SURFACE_S g_stBmp[N_IMAGES];
static TDE_RECT_S g_stPosition[N_IMAGES];
static TDE_SURFACE_S g_stScreen;

static int tde_control(void)
{
    char ch;
    int current_select = -1;

    while((ch = getchar()) != 'q')
    {
        switch(ch)
        {
            case 'g': //show or hide games menu
            {
                if(0 == current_select)
                {
                    break;
                }
                
                if((current_select != -1))
                {
                    tde_select(&g_stScreen, &g_stPosition[current_select]);
                    tde_memset(&g_stScreen, &g_stPosition[3 + current_select], 0);
                }

                tde_select(&g_stScreen, &g_stPosition[0]);
                tde_deflicker_partly(&g_stBmp[3], NULL, &g_stScreen, &g_stPosition[3]);
                
                current_select = 0;
                break;
            }
            case 'c': //show or hide citymaps menu
            {
                if(1 == current_select)
                {
                    break;
                }
                if(current_select != -1)
                {
                    tde_select(&g_stScreen, &g_stPosition[current_select]);
                    tde_memset(&g_stScreen, &g_stPosition[3 + current_select], 0);
                }
                
                tde_select(&g_stScreen, &g_stPosition[1]);
                tde_deflicker_partly(&g_stBmp[4], NULL, &g_stScreen, &g_stPosition[4]);
                
                current_select = 1;
                break;
            }
            case 'a': //show or hide applications menu
            {
                if(2 == current_select)
                {
                    break;
                }
                
                if(current_select != -1)
                {
                    tde_select(&g_stScreen, &g_stPosition[current_select]);
                    tde_memset(&g_stScreen, &g_stPosition[3 + current_select], 0);
                }
                
                tde_select(&g_stScreen, &g_stPosition[2]);
                tde_deflicker_partly(&g_stBmp[5], NULL, &g_stScreen, &g_stPosition[5]);

                current_select = 2;
                break;
            }
            default:
                continue;
        }

    }

    return 0;
}

static void tde_usage(void)
{
    printf("sample usage:\n");
    printf("\t\tg--------select games\n");
    printf("\t\tc--------select citymap\n");
    printf("\t\ta--------select applications\n");
    printf("\t\tq--------quit\n");
    printf("\n");
}

HI_S32 main(HI_VOID)
{
    HI_S32 s32Fd;
    HI_U32 u32PhyAddr;
    HI_U32 u32Size;
    HI_U8* pu8Screen;
    HI_U32 u32PhyTmp;
    HI_U8 *pu8VrtTmp;
    TDE_RECT_S stRect;
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

    /* 1. open tde device */
    HI_API_TDE_Open();

    s32Fd = fblayer_init("/dev/fb/0");
    if(s32Fd < 0)
    {
        goto  FB_OPEN_ERROR;
    }

    fblayer_getfix(s32Fd, &u32PhyAddr, &u32Size);
    pu8Screen = fblayer_mmap(s32Fd, u32Size);

    g_stScreen.pu8PhyAddr = (HI_U8*)u32PhyAddr;
    g_stScreen.enColorFmt = TDE_COLOR_FMT_RGB8888;
    g_stScreen.u16Width = SCREEN_WIDTH;
    g_stScreen.u16Height = SCREEN_HEIGHT;
    g_stScreen.u16Stride = SCREEN_WIDTH * 4;
    
    stRect.x = 0;
    stRect.y = 0;
    stRect.w = g_stScreen.u16Width;
    stRect.h = g_stScreen.u16Height;

    tde_memset(&g_stScreen, &stRect, 0);

    u32PhyTmp = (HI_U32)g_stScreen.pu8PhyAddr + g_stScreen.u16Stride * g_stScreen.u16Height;
    pu8VrtTmp = pu8Screen + g_stScreen.u16Stride * g_stScreen.u16Height;
    for(i = 0; i < N_IMAGES; i++)
    {
        tde_createsurfacebyfile(pszImageNames[i], &g_stBmp[i], u32PhyTmp, pu8VrtTmp);
        u32PhyTmp = (HI_U32)g_stBmp[i].pu8PhyAddr + g_stBmp[i].u16Stride * g_stBmp[i].u16Height;
        pu8VrtTmp = pu8VrtTmp + g_stBmp[i].u16Stride * g_stBmp[i].u16Height;
    }

    /* menu games */
    g_stPosition[0].x = START_OFFSET_X;
    g_stPosition[0].y = START_OFFSET_Y;
    g_stPosition[0].w = g_stBmp[0].u16Width;
    g_stPosition[0].h = g_stBmp[0].u16Height;

    /* menu city_map */
    g_stPosition[1].x = START_OFFSET_X;
    g_stPosition[1].y = g_stPosition[0].y + g_stPosition[0].h;
    g_stPosition[1].w = g_stBmp[1].u16Width;
    g_stPosition[1].h = g_stBmp[1].u16Height;

    /* menu applications */
    g_stPosition[2].x = START_OFFSET_X;
    g_stPosition[2].y = g_stPosition[1].y + g_stPosition[0].h;
    g_stPosition[2].w = g_stBmp[2].u16Width;
    g_stPosition[2].h = g_stBmp[2].u16Height;

    /*item for games */
    g_stPosition[3].x = START_OFFSET_X;
    g_stPosition[3].y = g_stPosition[2].y + g_stPosition[2].h;
    g_stPosition[3].w = g_stBmp[3].u16Width;
    g_stPosition[3].h = g_stBmp[3].u16Height;

    /* item for city_map */
    g_stPosition[4].x = g_stPosition[3].x + g_stPosition[3].w;
    g_stPosition[4].y = g_stPosition[3].y;
    g_stPosition[4].w = g_stBmp[4].u16Width;
    g_stPosition[4].h = g_stBmp[4].u16Height;

    /*item for applications */
    g_stPosition[5].x = g_stPosition[4].x + g_stPosition[4].w;
    g_stPosition[5].y = g_stPosition[4].y;
    g_stPosition[5].w = g_stBmp[5].u16Width;
    g_stPosition[5].h = g_stBmp[5].u16Height;

    /* show menus */
    for(i = 0; i < N_IMAGES/2; i++)
    {
        tde_deflicker_partly(&g_stBmp[i], NULL, &g_stScreen, &g_stPosition[i]);
    }

    tde_usage();
    
    tde_control();

    fblayer_munmap(pu8Screen, u32Size);
    fblayer_term(s32Fd);
    
FB_OPEN_ERROR:    
    HI_API_TDE_Close(); 
    

    return HI_SUCCESS;
}


