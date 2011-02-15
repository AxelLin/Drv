#ifndef _HI_DAES_API_H_
#define _HI_DAES_API_H_
#include "hi_daes_driver.h" 
#include <pthread.h>
#include <time.h>
//software reverse string MACRO
//#define _REVERSE_BYTE_SEQ_

//DMA mode test MACRO,just test case use this macro!!
#define _DMA_MODE_

//#define DEBUG


//#define _PROMANCE_TEST_


#ifdef 	_PROMANCE_TEST_
	#define _ENTER() 	\
	do {					\
		getbegintime();				\
		printf("Enter: %s, begin time is:%u\n",__FUNCTION__, u_begin_seconds);	\
	}while(0)						
	
	#define _LEAVE() 					\
	do {								\
		getendtime();				\
		printf("Leave: %s, end time is:%u\n",__FUNCTION__, u_end_seconds);	\
	}while(0)	
#else
	#define _ENTER()
	#define _LEAVE()
#endif

extern unsigned int  u_begin_seconds;
extern unsigned int  u_end_seconds;
extern struct timeval timeval;

#define  ECS_BASE_VIRTUAL_ADDR ((unsigned long)p_map)
#define  CIPHER_DIN1_OFFSET  (0x00+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DIN2_OFFSET  (0x04+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DIN3_OFFSET  (0x08+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DIN4_OFFSET  (0x0c+ECS_BASE_VIRTUAL_ADDR)

#define  CIPHER_IVIN1_OFFSET   (0x10+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVIN2_OFFSET   (0x14+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVIN3_OFFSET   (0x18+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVIN4_OFFSET   (0x1c+ECS_BASE_VIRTUAL_ADDR)

#define  CIPHER_KEY1_OFFSET    (0x20+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY2_OFFSET    (0x24+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY3_OFFSET    (0x28+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY4_OFFSET    (0x2c+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY5_OFFSET    (0x30+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY6_OFFSET    (0x34+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY7_OFFSET    (0x38+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_KEY8_OFFSET    (0x3c+ECS_BASE_VIRTUAL_ADDR)

#define  CIPHER_DOUT1_OFFSET   (0x40+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DOUT2_OFFSET   (0x44+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DOUT3_OFFSET   (0x48+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_DOUT4_OFFSET   (0x4c+ECS_BASE_VIRTUAL_ADDR)

#define  CIPHER_IVOUT1_OFFSET   (0x50+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVOUT2_OFFSET   (0x54+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVOUT3_OFFSET   (0x58+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_IVOUT4_OFFSET   (0x5c+ECS_BASE_VIRTUAL_ADDR)

#define  CIPHER_CTRL_OFFSET      (0x60+ECS_BASE_VIRTUAL_ADDR)
#define  INT_CIPHER__OFFSET      (0x64+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_BUSY_OFFSET      (0x68+ECS_BASE_VIRTUAL_ADDR)
#define  CIPHER_START_OFFSET     (0x6c+ECS_BASE_VIRTUAL_ADDR)
#define  SRC_START_ADDR_OFFSET   (0x70+ECS_BASE_VIRTUAL_ADDR)
#define  MEM_LENGTH_OFFSET       (0x74+ECS_BASE_VIRTUAL_ADDR)
#define  DEST_MEM_START_OFFSET   (0x78+ECS_BASE_VIRTUAL_ADDR)
#define  MMAP_LENGTH             0x1000

#define  DAES_ALG_MODE               0x300

#define  DAES_START               0x01

#ifndef WRITE_REGISTER_ULONG
#define WRITE_REGISTER_ULONG(reg,value)  (*((volatile unsigned int *)(reg))=value)
#endif

#ifndef WRITE_REGISTER_CHAR
#define WRITE_REGISTER_CHAR(reg, value) (*((volatile unsigned char *)(reg))=(value))
#endif

#ifndef READ_REGISTER_ULONG
#define READ_REGISTER_ULONG(reg,result) ((result)=*(volatile unsigned int *)(reg))
#endif

unsigned int * p_map;
unsigned int ctrl_word;

#pragma pack(1) 
/*
#ifdef 	_PROMANCE_TEST_
	unsigned int  u_begin_seconds;
	unsigned int  u_end_seconds;
	struct timeval timeval;

	void getbegintime(void)
	{
	   gettimeofday(&timeval,0);
	   u_begin_seconds=timeval.tv_sec*1000000+timeval.tv_usec;
	   printf("begin time is:%u\n",u_begin_seconds);

	}
	void getendtime(void)
	{
		gettimeofday(&timeval,0);
		u_end_seconds=timeval.tv_sec*1000000+timeval.tv_usec;
		printf("end time is:%u\n",u_end_seconds);
	}
#endif
*/
struct daes_ctrl{
			/*
				work mode,
					when using AES:
						000£­ECB mode;
						001£­CBC mode;
						010£­CFB mode;
						011£­OFB mode;
						100£­CTR mode;
						else£­ECB mode;
    				when using DES:
						x00£­ECB mode;
						x01£­CBC mode;
						x10£­CFB mode;
						x11£­OFB mode;
			*/
               unsigned int daes_work_mod;
			
			/*
				bit width:
					00£­64 bits when using DES/3DES;
			    		128 bits when using AES;
					01£­8 bits;
					10£­1 bit;
					11£­128 bits when using AES;
						 64 bits when using DES/3DES;
			*/
               unsigned int daes_bit_width;
			
			/*
			key length:
				00£­means 128 bits key length when using AES;
				    means 3 keys when using 3DES;
				01£­means 192 bits key length when using AES;
				    means 3 keys when using 3DES;
				10£­means 256 bits key length when using AES;
				    means 3 keys when using 3DES;
				11£­means 128 bits key length when using AES;
				    means 2 keys when using 3DES;
			*/
               unsigned int daes_key_length;
			
			/*
				algorithm:
					00£­DES
					01£­3DES
					10£­AES
					11£­DES
			*/
               unsigned int daes_alg; 

			/*
				CIPHER working mode:
					0£­single group
					1£­multi gropup
			*/
               unsigned int daes_group;
                 
};

struct key_ivin_des{
           unsigned char cipher_key1[8];
           unsigned char cipher_key2[8];
           unsigned char cipher_key3[8];
           unsigned char ivin[8];
}key_ivin_tmp_des;

unsigned char feedback_iv[16];		//20071011 add

struct key_ivin_aes{
           unsigned char cipher_key[32];
           unsigned char ivin[16];
}key_ivin_tmp_aes;

struct cipher_info  cipher_info_tmp;

// this struct contains the  result, the feedback ivin
struct outtrans{
unsigned char outtrans[16];
unsigned char ivouttrans[16];
};

#pragma pack() 
//DECLARE_MUTEX(g_app_mutex);
//struct semaphore *g_app_mutex;
pthread_mutex_t g_app_mutex;

unsigned int* malloc_dma_space(unsigned int byte_length);
void free_dma_space();

/*every crypt/decrypt compute include three steps:
	1, initialize the algorithm;
	2, config the algorithm base control parameter;
	3, call the crypt/decrypt funtion;
*/
int hi_daes_init();
int hi_daes_exit();

/*config DES/3DES or AES crypt/decrypt config info*/
int hi_daes_config(struct daes_ctrl * daes_ctrl );

/*AES crypt/decrypt algorithm(ECB,CBC,CFB and OFB but not include CTR mode) function*/
int hi_aes_crypt(unsigned char *src,unsigned char* dest,
							unsigned int byte_length, struct key_ivin_aes* key_ivin);
int hi_aes_decrypt(unsigned char *src,unsigned char* dest,
							unsigned int byte_length, struct key_ivin_aes* key_ivin);

/*DES/3DES (ECB,CBC,CFB and OFB) crypt/decrypt function*/
int hi_des_crypt(unsigned char *src,unsigned char* dest,
						unsigned int byte_length, struct key_ivin_des *key_ivin);
int hi_des_decrypt(unsigned char *src,unsigned char* dest,
						unsigned int byte_length, struct key_ivin_des *key_ivin);


#endif

