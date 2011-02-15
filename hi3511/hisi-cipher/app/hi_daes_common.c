#include "hi_daes_api.h"
#include "hi_daes_driver.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define HI_ASSERT(expr)                                     \
    do{                                                     \
        if (!(expr)) { \
            printf("\nASSERT failed at:\n  >File name: %s\n  >Function : %s\n  >Line No. : %d\n  >Condition: %s\n", \
                __FILE__,__FUNCTION__, __LINE__, #expr);}\
    }while(0)

unsigned int  fd,fd_dev;
unsigned int g_open_cnt = 0;
int require_loop();
int daes_run();
int dma_access(unsigned char  * argp);

#ifdef 	_PROMANCE_TEST_
	unsigned int  u_begin_seconds;
	unsigned int  u_end_seconds;
	struct timeval timeval;

	void getbegintime(void)
	{
	   gettimeofday(&timeval,0);
	   u_begin_seconds=timeval.tv_sec*1000000+timeval.tv_usec;
	//   printf("Enter: %s, begin time is:%u\n",__FUNCTION__, u_begin_seconds);
	}
	void getendtime(void)
	{
		gettimeofday(&timeval,0);
		u_end_seconds=timeval.tv_sec*1000000+timeval.tv_usec;
	//	printf("Leave: %s, end time is:%u\n",__FUNCTION__, u_end_seconds);
	}
#endif	
	
#ifdef _REVERSE_BYTE_SEQ_
static int reverse( unsigned char *src)
{
        int i;
        unsigned char tmp[8];
        if(NULL==src)
        {
            printf("the pointer in  reverse is null!\n");
            return -1;
        }

        for( i = 0; i < 8; i++ )
        {
            tmp[i] = src[(8-1) - i];
        }

        for( i = 0; i < 8; i++ )
        {
            src[i] = tmp[i];
        }
        return 0;
}

static int reverse_16char( unsigned char *src)
{
        int i;
        unsigned char tmp[16];
        if(NULL==src)
        {
            printf("the pointer in  reverse is null!\n");
            return -1;
        }

        for( i = 0; i < 16; i++ )
        {
            tmp[i] = src[(16-1) - i];
        }

        for( i = 0; i < 16; i++ )
        {
            src[i] = tmp[i];
        }
        return 0;
}
#endif

/*
 * 
 * open the /dev/mem file,map the physical address of registers to virtual address 
 */
int hi_daes_init()
{

	if(g_open_cnt == 0)
	{
		fd=open("/dev/mem", O_RDWR|O_SYNC, 00777);
		if(fd<=0)
		{
		    return -1;
		}
		p_map = (unsigned int *) mmap( NULL,MMAP_LENGTH,PROT_READ|PROT_WRITE,MAP_SHARED,fd,ECS_BASE_ADDR);

		if(p_map==0)
		{
		    return -1;
		}

		fd_dev=open("/dev/misc/des",O_RDWR | O_SYNC,00777);
		 if(fd_dev<=0)
		 {
		      printf("Can't open des device.\n");
		      return -1;
		 }

		 pthread_mutex_init(&g_app_mutex, NULL);
	}

	g_open_cnt++;	
	return 0;
}

/*
 * exit function to release the resource
 */
int hi_daes_exit()
{
	g_open_cnt--;
	
       if(g_open_cnt == 0)
       {
		if(-1==close( fd ))
		{
		    printf("close the file operator failed\n");
		    return -1;
		}

		if(-1== munmap( p_map, MMAP_LENGTH))
		{
		    printf("mumap failed\n");
		    return -1;
		}


		if(-1==close(fd_dev))
		{
		    printf("i have come to here in fd_dev\n");
		    printf("close the file operator failed:%u\n",fd_dev);
		    return -1;
		}
       }

    	return 0;
}

/*
 * to force  the module not to run 
 */
int hi_daes_stop()
{
    unsigned int stavalue,k;

    WRITE_REGISTER_ULONG(CIPHER_START_OFFSET,0x0);

    //see whether  the  CIPHER_BUSY_OFFSET register value is 0;
    READ_REGISTER_ULONG(CIPHER_BUSY_OFFSET,stavalue); 
    for( k=0;(k<2000)&&(stavalue!=0);k++)
    {
        READ_REGISTER_ULONG(CIPHER_BUSY_OFFSET,stavalue);
    }

    if(k>=2000)
    {
         printf("we can't force the module to stop.\n\n");
         return -1;
    }

    return 0;
}


/*
 * user use the space provided by the daes crypt module
 */
unsigned int* malloc_dma_space(unsigned int byte_length)
{
    return NULL;
}


/*
 * user use this function to free the space provided by 
 * daes crypt module 
 */
void  free_dma_space()
{
    return ;
}

/*
 *  when we get the data have been dealed with ,we first use 
 * this function to see whether the process ends
 */
 int require_loop()
{
    unsigned int stavalue,k;

   READ_REGISTER_ULONG(CIPHER_BUSY_OFFSET,stavalue); 
    for( k=0;(k<200000)&&(stavalue!=0);k++)
   {
       READ_REGISTER_ULONG(CIPHER_BUSY_OFFSET,stavalue);
   }

   if(k>=200000)
   {
        printf("there is something wrong with the config ivin!\n");
        return -1;
   }
   return 0;
}


/*
 * to judge wether the input is incompatible,and make a control word contains 
 *all  the things
 */
int hi_daes_config(struct daes_ctrl * daes_ctrl )
{
   // WRITE_REGISTER_ULONG(CIPHER_START_OFFSET,0x0);
    if(NULL==daes_ctrl)
    {
        printf(" the contrl information pointer in hi_daes_config is null!\n");
        return -1;
    }
  
    if((daes_ctrl->daes_work_mod > 7) || (daes_ctrl->daes_bit_width > 3) || (daes_ctrl->daes_key_length > 3) 
                  || (daes_ctrl->daes_alg > 3) || (daes_ctrl->daes_group > 1))
    {
        printf(" your input is out of range!\n");
        return -1;
    }
   
    if(daes_ctrl->daes_work_mod >4)
    {
        daes_ctrl->daes_work_mod =0;
    }
    // in the aes alg,the ctr mod do not support dma; 
    if(( 2==daes_ctrl->daes_alg)&&(4==daes_ctrl->daes_work_mod))
    {
        // if the cipher_mod is dma
        if(1==daes_ctrl->daes_group)
        {
            printf(" the ctr mod could not run in dma ,please input again!\n");
            return -1;
        }

    }

    //ECB,CBC CTR's bit width  have  full size ,not 1,8
    if (((0==daes_ctrl->daes_work_mod)||(1==daes_ctrl->daes_work_mod)||(( 2==daes_ctrl->daes_alg)&&(4==daes_ctrl->daes_work_mod)))
                         && ((1==daes_ctrl->daes_bit_width) ||(2==daes_ctrl->daes_bit_width)))
    {
        printf("ecb,cbc ctr's bit width is full size, not 1 or 8 !\n");
        return -1;
    }
    //OFB mod in AES, the bit width is full size ,not 1 or 8
    if((2==daes_ctrl->daes_alg)&&(3==daes_ctrl->daes_work_mod)
                           &&((1==daes_ctrl->daes_bit_width) ||(2==daes_ctrl->daes_bit_width)))
    {
        printf("ofb mod in aes, the bit width is full size ,not 1 or 8 !\n");
        return -1;
    }

 	//synch data
	pthread_mutex_lock(&g_app_mutex);
 	
    ctrl_word = 0;
    ctrl_word  |= (daes_ctrl->daes_work_mod <<1) | ( daes_ctrl->daes_bit_width << 4 ) | (daes_ctrl->daes_key_length <<6) 
                | ( daes_ctrl->daes_alg << 8) |( daes_ctrl->daes_group << 11) ;
    //printf("end hi_daes_config ctrl:0x%x", ctrl_word);
          
    return 0;
}

int dma_access(unsigned char  * argp)
{
	//_ENTER();
	if(NULL==argp)
	{
		printf("the pointer passed to dma_access is null!\n");
		return -1;
	}

	//reverse dma src data
	#ifdef _REVERSE_BYTE_SEQ_
		int i;
		if ( ((ctrl_word & 0x30) == 0x30) || ((ctrl_word & 0x30) == 0x0) )	//64bits or 128 bits
		{
			if ( (ctrl_word&DAES_ALG_MODE) == 0x200 )	//AES mode ,the reverse string length is 16
			{
				for(i=0; i< cipher_info_tmp.byte_length; i+=16)
				{
					reverse_16char(cipher_info_tmp.src+i);
				}
			}
			else
			{	
				for(i=0; i< cipher_info_tmp.byte_length; i+=8)
				{
					reverse(cipher_info_tmp.src+i);
				}

			}	
		}

	#endif

	#ifdef DEBUG
	    printf("before set data in dma_access !\n");
	#endif

	//printf("\n*********************** begin ioctl DAES_SET_M ********\n");    
	//_ENTER();
    if(-1==ioctl(fd_dev, DAES_SET_M, &cipher_info_tmp))
    {
        printf("set plaintext or cipher error in dma mod!\n");
        return -1;
    }
    //_LEAVE();
    //printf("*********************** end ioctl DAES_SET_M ********\n");
    
    #ifdef DEBUG
    	printf("before run in dma_access !\n");
    #endif
    
    //printf("\n*********************** begin ioctl SET_RUN ********\n");    
	//_ENTER();
    if(-1 == ioctl(fd_dev, SET_RUN, 0))
    {
        printf("set run signal error in dma mod!\n");
        return -1;
    }
    //_LEAVE();
    //printf("*********************** end ioctl SET_RUN ********\n");
    
    #ifdef DEBUG
    	printf("before get data in dma !\n");
    #endif
    
	//printf("\n*********************** begin ioctl MASTER_GET_DATA ********\n");    
	//_ENTER();
	if(-1== ioctl(fd_dev,MASTER_GET_DATA,argp))
	{
		printf("MASTER_GET_DATA error in dma mod!\n");
		return -1;
	}
	//_LEAVE();
	//printf("*********************** end ioctl MASTER_GET_DATA ********\n\n");
	
	//reverse dma dst data
	#ifdef _REVERSE_BYTE_SEQ_
		if ( ((ctrl_word & 0x30) == 0x30) || ((ctrl_word & 0x30) == 0x0) )	//64bits or 128 bits
		{
			if ( (ctrl_word&DAES_ALG_MODE) == 0x200 )	//AES mode ,the reverse string length is 16
			{
				for(i=0; i< cipher_info_tmp.byte_length; i+=16)
				{
					reverse_16char(argp+i);
				}
			}
			else 
			{
				for(i=0; i< cipher_info_tmp.byte_length; i+=8)
				{
					reverse(argp+i);
				}
			}	
		}
		
	#endif
	
	  #ifdef DEBUG
	    printf("end get data in dma !\n");
    	#endif
    	//_LEAVE();
    return 0;
}

