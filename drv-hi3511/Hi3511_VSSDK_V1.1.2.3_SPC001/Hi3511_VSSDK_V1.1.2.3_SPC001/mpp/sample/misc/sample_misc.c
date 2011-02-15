
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


HI_S32 s_s32MemDev = 0;

HI_S32 memopen( void )
{
    if (s_s32MemDev <= 0)
    {
        s_s32MemDev = open ("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
        if (s_s32MemDev <= 0)
        {
            return -1;
        }
    }   
    return 0;
}

HI_VOID memclose()
{
	close(s_s32MemDev);
}

void * memmap( HI_U32 u32PhyAddr, HI_U32 u32Size )
{
    HI_U32 u32Diff;
    HI_U32 u32PagePhy;
    HI_U32 u32PageSize;
    HI_U8 * pPageAddr;

    u32PagePhy = u32PhyAddr & 0xfffff000;   
    u32Diff = u32PhyAddr - u32PagePhy;
    
    /* size in page_size */ 
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;  
    pPageAddr = mmap ((void *)0, u32PageSize, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, s_s32MemDev, u32PagePhy);   
    if (MAP_FAILED == pPageAddr )   
    {       
            return NULL;    
    }
    return (void *) (pPageAddr + u32Diff);
}
HI_S32 memunmap(HI_VOID* pVirAddr, HI_U32 u32Size)
{
    HI_U32 u32PageAddr;
    HI_U32 u32PageSize;
    HI_U32 u32Diff;

    u32PageAddr = (((HI_U32)pVirAddr) & 0xfffff000);
    /* size in page_size */
    u32Diff     = (HI_U32)pVirAddr - u32PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;

    return munmap((HI_VOID*)u32PageAddr, u32PageSize);
}


HI_S32 SAMPLE_MISC_InitMpp()
{
    HI_S32 s32ret;
    MPP_SYS_CONF_S struSysConf;
	VB_CONF_S struVbConf;
    MPP_VERSION_S stMppVersion;

	memset(&struVbConf, 0, sizeof(struVbConf));
	memset(&struSysConf, 0, sizeof(struSysConf));

	struVbConf.u32MaxPoolCnt = 128;
    struVbConf.astCommPool[0].u32BlkSize = 884736;/* 768*576*1.5*/
    struVbConf.astCommPool[0].u32BlkCnt = 10;
    struVbConf.astCommPool[1].u32BlkSize = 663552;
    struVbConf.astCommPool[1].u32BlkCnt = 30;
	struVbConf.astCommPool[2].u32BlkSize = 165888;/* 384*288*1.5*/
    struVbConf.astCommPool[2].u32BlkCnt = 50;
	
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	s32ret = HI_MPI_VB_SetConf(&struVbConf);
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
    	
	HI_MPI_SYS_GetVersion(&stMppVersion);
    printf("%s\n", stMppVersion.aVersion);
    
	return HI_SUCCESS;
}

HI_S32 SAMPLE_MISC_ExitMpp()
{
    HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
    return 0;
}

HI_S32 SAMPLE_MISC_VB()
{
    VB_BLK VbBlk;
    HI_S32 s32ret;
    HI_U32 u32PhyAddr;
    HI_U8 *pVirAddr;
    HI_U32 u32PoolId;
    HI_U32 u32Size = 768*576*1.5;

    /* Mpp sys init*/
    s32ret = SAMPLE_MISC_InitMpp();
    if (HI_SUCCESS != s32ret)
    {
        return s32ret;
    }
    
    memopen();        

    /* get a video block*/
    VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size);
    if (VB_INVALID_HANDLE == VbBlk)
    {
		memclose();
        return -1;
    }
    /* get physaddr of this block*/
    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u32PhyAddr)
    {
        return -1;
    }
    /* mmap*/
    pVirAddr = (HI_U8 *) memmap(u32PhyAddr, u32Size);
    if (NULL == pVirAddr)
    {
		memclose();
        return -1;
    }

    u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
    if (VB_INVALID_POOLID == u32PoolId)
    {
		memclose();
		memunmap(pVirAddr, u32Size);
        return -1;
    }
    printf("pool id :%d, phyAddr:0x%x,virAddr:0x%x\n" ,u32PoolId,u32PhyAddr,(int)pVirAddr);

    strcpy(pVirAddr, "Hi3511MPP");

    printf("content of my buffer: \"%s\" \n", pVirAddr);

	memclose();
    memunmap(pVirAddr, u32Size);

    SAMPLE_MISC_ExitMpp();

    return 0;
}

void SAMPLE_MISC_printhelp(void)
{
    printf("usage :./sample_misc 1\n");
    printf("1:  get a buffer from commom VB, get its virtual address \n");
}

HI_S32 main(int argc, char *argv[])
{	 
    int index;

    if (2 != argc)
    {
        SAMPLE_MISC_printhelp();
        return HI_FAILURE;
    }
    
    index = atoi(argv[1]);
    
    if (1 != index)
    {
        SAMPLE_MISC_printhelp();
        return HI_FAILURE;
    }
    
	if (1 == index)
	{
		SAMPLE_MISC_VB();
	}	

	return HI_SUCCESS;
}


