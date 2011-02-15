
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "hi_daes_api.h"
#include "hi_daes_driver.h"

#define HI_ASSERT(expr)                                     \
    do{                                                     \
        if (!(expr)) { \
            printf("\nASSERT failed at:\n  >File name: %s\n  >Function : %s\n  >Line No. : %d\n  >Condition: %s\n", \
                __FILE__,__FUNCTION__, __LINE__, #expr);}\
    }while(0)

static int aes_config_key( unsigned char * cipher_key);
static int aes_config_iv(unsigned char  * ivin);
static int aes_config_plaintext(unsigned char  * pplaintext);
static int aes_get_data(unsigned char * cipher_out_tmp,unsigned char * feedback_ivin_tmp);
static int aes_set_key_ivin(struct key_ivin_aes* key_ivin_aes);
static int aes_console(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int ctr_aes_module(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int ecb_module(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int cbc_module(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int cfb_1_aes_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int cfb_8_aes_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int cfb_aes_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int ofb_aes_module(unsigned char* src,unsigned char* dest,unsigned int length);
int aes_run();

extern  int require_loop();
extern  int dma_access(unsigned char  * argp);
extern  unsigned int fd ,fd_dev;
extern unsigned int g_open_cnt;

#ifdef _REVERSE_BYTE_SEQ_
static int reverse_array( unsigned char *src, unsigned char *des)
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
            des[i] = tmp[i];
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

static int aes_config_key( unsigned char * cipher_key)
{
    unsigned int * ptmpkey;
#ifdef _REVERSE_BYTE_SEQ_
    unsigned char key_char[32];
#endif
#ifdef DEBUG    
    unsigned int stavalue;
#endif

#ifdef _REVERSE_BYTE_SEQ_
	if (reverse_array( cipher_key, &key_char[0]) != 0 )
	{	
		printf("aes_config_key reverse_array 1 error!\n");
		return -1;
	}
	if ( reverse_array( cipher_key+8, &key_char[8]) != 0 )
	{
		printf("aes_config_key reverse_array 2 error!\n");
		return -1;
	}
	ptmpkey=(unsigned int *) key_char;
#else
	ptmpkey=(unsigned int *) cipher_key;
#endif

    if ( -1==require_loop())
    {
        return -1;
    }

#ifndef _REVERSE_BYTE_SEQ_		//not reverse
       WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*ptmpkey++);
       WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*ptmpkey++);
#endif

       if (KEY_LENGTH_128_1==( ctrl_word & KEY_LENGTH_MASK) 
                       || KEY_LENGTH_128_2 ==( ctrl_word & KEY_LENGTH_MASK))
       {
       	WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*ptmpkey++);
           	WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*ptmpkey++);    
       }
       else if (KEY_LENGTH_192==( ctrl_word &KEY_LENGTH_MASK))
       { 
       #ifdef _REVERSE_BYTE_SEQ_
		reverse_array( cipher_key+16, &key_char[16]);
		WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*ptmpkey++); 
	#else
		WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET, *ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*ptmpkey++); 
	#endif
       }
       else		//key length 256
       {
       #ifdef _REVERSE_BYTE_SEQ_
		reverse_array( cipher_key+16, &key_char[16]);
       	reverse_array( cipher_key+24, &key_char[24]);
		WRITE_REGISTER_ULONG(CIPHER_KEY7_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY8_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*ptmpkey++);
	#else
		WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY7_OFFSET,*ptmpkey++);
		WRITE_REGISTER_ULONG(CIPHER_KEY8_OFFSET,*ptmpkey);
	#endif
       }

	#ifdef _REVERSE_BYTE_SEQ_
		WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*ptmpkey++);
	       WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*ptmpkey);
	#endif
           
    #ifdef DEBUG
       READ_REGISTER_ULONG(CIPHER_KEY1_OFFSET,stavalue);
       printf("key1:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY2_OFFSET,stavalue);
       printf("key2:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY3_OFFSET,stavalue);
       printf("key3:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY4_OFFSET,stavalue);
       printf("key4:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY5_OFFSET,stavalue);
       printf("key5:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY6_OFFSET,stavalue);
       printf("key6:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY7_OFFSET,stavalue);
       printf("key7:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY8_OFFSET,stavalue);
       printf("key8:%x\n",stavalue);  
    #endif

    return 0;
}

/*  
 *to set the ivin value to the register, no matter it is in des,3des,aes.
 * ecb and ctr mode don't need to set this value 
 */
static int aes_config_iv(unsigned char  * ivin)
{
	
  	unsigned  int * ivin_tmp;
#ifdef _REVERSE_BYTE_SEQ_  	
  	unsigned char ivin_char[16];
#endif
#ifdef DEBUG  	
  	unsigned  int stavalue;
#endif

#ifdef _REVERSE_BYTE_SEQ_
	reverse_array( ivin, &ivin_char[0]); 		//reverse_8char test Ok
	reverse_array( ivin+8, &ivin_char[8]);
   	ivin_tmp= (unsigned int *) ivin_char;
#else
	ivin_tmp= (unsigned int *) ivin;
#endif

   if(NULL==ivin)
   {
        printf("the ivin pointer is null!\n");
        return -1;
   }   

   if(-1==require_loop())
   {
        printf("the busy signal is always 1!\n");
        return -1;
   }
  
   if( ECB_MODE==(ctrl_word & AES_MODE_MASK ))		//not complete
   {
        printf(" ecb mode can't be set  ivin!\n");
        return -1;
   }

   // if the alg is AES, not DES or 3DES 
      
   #ifdef _REVERSE_BYTE_SEQ_
         WRITE_REGISTER_ULONG(CIPHER_IVIN3_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN4_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN1_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN2_OFFSET,*ivin_tmp);
   #else
   	  WRITE_REGISTER_ULONG(CIPHER_IVIN1_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN2_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN3_OFFSET,*ivin_tmp++);
         WRITE_REGISTER_ULONG(CIPHER_IVIN4_OFFSET,*ivin_tmp);
   #endif

   #ifdef DEBUG
       READ_REGISTER_ULONG(CIPHER_IVIN1_OFFSET,stavalue);
       printf("ivin1:%x\n",stavalue);  

       READ_REGISTER_ULONG(CIPHER_IVIN2_OFFSET,stavalue);
       printf("ivin2:%x\n",stavalue); 

       READ_REGISTER_ULONG(CIPHER_IVIN3_OFFSET,stavalue);
       printf("ivin3:%x\n",stavalue); 

       READ_REGISTER_ULONG(CIPHER_IVIN4_OFFSET,stavalue);
       printf("ivin4:%x\n",stavalue); 
   #endif 
   
   return 0;
}

/*
 * we here set the pplaintext to the register
 */
static int aes_config_plaintext(unsigned char  * pplaintext)
{
	unsigned int * pplaintext_tmp;
#ifdef _REVERSE_BYTE_SEQ_    
	unsigned char plain_char[16];
#endif
#ifdef DEBUG
     	unsigned int stavalue;
#endif 
    
   if( NULL == pplaintext )
   {
        printf("the pplaintext pointer is null!\n");
        return -1;
   }   

	#ifdef _REVERSE_BYTE_SEQ_
		if ( ((ctrl_word & 0x30) == 0) || ((ctrl_word & 0x30) == 0x30) )
		{
			reverse_array( pplaintext, &plain_char[0]);
			reverse_array( pplaintext+8, &plain_char[8]);
			pplaintext_tmp = (unsigned int*) (plain_char);
		}
		else
		{
			pplaintext_tmp = (unsigned int *)pplaintext;
		}
	#else
		pplaintext_tmp = (unsigned int *)pplaintext;
	#endif

   if( -1 == require_loop() )
   {
       printf("the busy signal is always true, aes_config_plaintext fail!\n");
       return -1;
   }

	#ifdef _REVERSE_BYTE_SEQ_
		if ( ((ctrl_word & 0x30) == 0) || ((ctrl_word & 0x30) == 0x30) )
		{
			WRITE_REGISTER_ULONG(CIPHER_DIN3_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN4_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN1_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN2_OFFSET, *pplaintext_tmp );
		}
		else
		{
			WRITE_REGISTER_ULONG(CIPHER_DIN1_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN2_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN3_OFFSET, *pplaintext_tmp++ );
			WRITE_REGISTER_ULONG(CIPHER_DIN4_OFFSET, *pplaintext_tmp );
		}
	#else
		WRITE_REGISTER_ULONG(CIPHER_DIN1_OFFSET, *pplaintext_tmp++ );
		WRITE_REGISTER_ULONG(CIPHER_DIN2_OFFSET, *pplaintext_tmp++ );
		WRITE_REGISTER_ULONG(CIPHER_DIN3_OFFSET, *pplaintext_tmp++ );
		WRITE_REGISTER_ULONG(CIPHER_DIN4_OFFSET, *pplaintext_tmp );
	#endif
    
    #ifdef DEBUG
     if(-1==require_loop())
   {
       printf("the busy signal is always 1!\n");
       return -1;
   }
     
       READ_REGISTER_ULONG(CIPHER_DIN1_OFFSET,stavalue);
     	printf("pplaintext1:0x%x\n",stavalue);  

       READ_REGISTER_ULONG(CIPHER_DIN2_OFFSET,stavalue);
       printf("pplaintext2:0x%x\n",stavalue); 

       READ_REGISTER_ULONG(CIPHER_DIN3_OFFSET,stavalue);
       printf("pplaintext3:0x%x\n",stavalue); 

       READ_REGISTER_ULONG(CIPHER_DIN4_OFFSET,stavalue);
       printf("pplaintext4:0x%x\n",stavalue); 
       
       READ_REGISTER_ULONG(CIPHER_CTRL_OFFSET,stavalue);
       printf("ctrl_word is :0x%x\n",	stavalue); 
    #endif 
   
    return 0;

}


/*
 * get data processed by the module ,and clear the interrupt register
 */
static int aes_get_data(unsigned char * cipher_out_tmp, unsigned char * feedback_ivin_tmp)
{
	unsigned int * cipher_out  = (unsigned int *) cipher_out_tmp;
	unsigned int * feedback_ivin = (unsigned int*) feedback_ivin_tmp;

	if(NULL == cipher_out)
	{
		printf("the outtrans pointer is null !\n\n");
		return -1;
	}    

	if(-1== ioctl(fd_dev, SLAVE_GET_DATA, 0))
	{
		printf("in slave mod ,the signal for getting data does not come!\n");
		return -1;
	}

    	//deal with the cipher_out. 
       READ_REGISTER_ULONG(CIPHER_DOUT1_OFFSET, *cipher_out++);    
    
       READ_REGISTER_ULONG(CIPHER_DOUT2_OFFSET, *cipher_out++);
       
       READ_REGISTER_ULONG(CIPHER_DOUT3_OFFSET, *cipher_out++);
 
       READ_REGISTER_ULONG(CIPHER_DOUT4_OFFSET, *cipher_out);
	if( ((ctrl_word & AES_MODE_MASK) == CBC_MODE) || ((ctrl_word & AES_MODE_MASK) == CFB_MODE)
					|| ((ctrl_word & AES_MODE_MASK) == OFB_MODE))
	{
		READ_REGISTER_ULONG(CIPHER_IVOUT1_OFFSET, *feedback_ivin++);
		READ_REGISTER_ULONG(CIPHER_IVOUT2_OFFSET, *feedback_ivin++);
		READ_REGISTER_ULONG(CIPHER_IVOUT3_OFFSET, *feedback_ivin++);
		READ_REGISTER_ULONG(CIPHER_IVOUT4_OFFSET, *feedback_ivin);
	}

	#ifdef _REVERSE_BYTE_SEQ_
		reverse_16char(cipher_out_tmp);
		if ( feedback_ivin_tmp != NULL )	//ctr mode 
			 reverse_16char(feedback_ivin_tmp);
	#endif

	return 0;
}


/*
 * non-ctr mode use this function to set key and ivin
 * value to register for the first time
 */
static int aes_set_key_ivin(struct key_ivin_aes* key_ivin_aes)
{

    if( NULL==key_ivin_aes)
    {
        printf("the  pointer in set_keyivin is null !\n");
        return -1;
    }

    if(KEY_LENGTH_256==(ctrl_word & KEY_LENGTH_MASK))		//key length 256
    {
         memcpy(key_ivin_tmp_aes.cipher_key,key_ivin_aes->cipher_key,32);
    }
    else if(KEY_LENGTH_192==(ctrl_word & KEY_LENGTH_MASK))
    {
         memcpy(key_ivin_tmp_aes.cipher_key,key_ivin_aes->cipher_key,24);
    }
    else 		//key length 128
    {
         memcpy(key_ivin_tmp_aes.cipher_key,key_ivin_aes->cipher_key,16);
    }

     memcpy(key_ivin_tmp_aes.ivin,key_ivin_aes->ivin,16);

     if( (CBC_MODE==(ctrl_word & AES_MODE_MASK ) )||		\
		( CFB_MODE==(ctrl_word & AES_MODE_MASK ) )||		\
		 ( OFB_MODE==(ctrl_word & AES_MODE_MASK ) )||	\
		  ( CTR_MODE==(ctrl_word & AES_MODE_MASK ) ))	
    	{
	     if(-1== aes_config_iv(&key_ivin_tmp_aes.ivin[0]))
	     {
	          printf("set ivin error in set_keyivin!\n");
	          return -1;
	     }
    	}

	if(-1== aes_config_key(&key_ivin_tmp_aes.cipher_key[0]))
	{
	     printf("set key  error in set_keyivin!\n");
	     return -1;
	}

   return 0;
}



/*
 * the crypt entrance functions for aes
 */
int hi_aes_crypt(unsigned char *src,unsigned char* dest,unsigned int  byte_length, struct key_ivin_aes* key_ivin_aes)
{

    unsigned int srcvalue;
    unsigned int destvalue;
   
    srcvalue = (unsigned int)src;
    destvalue = (unsigned int) dest;

    if( (NULL==src) || (NULL==dest) || (NULL==key_ivin_aes))
    {
        printf("the  pointer in hi_aes_crypt is null !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }
    
	if((byte_length % 16) != 0)
    	{
		printf("aes encrypt data byte_length is error.\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
    	}
      
    if (ALG_TYPE_AES != (ctrl_word & ALG_TYPE_MASK) )
    {
        printf("the name of the function is not the same with your input !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }

/*
    if( CTR_MODE== (ctrl_word & AES_MODE_MASK))
    {
        printf("this function can't be used in CTR mode !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }
*/

    // to deal with the length in des,aes,3des
    if((byte_length % 16) != 0)
    {
         printf("aes crypt data byte_length is error.\n");
         pthread_mutex_unlock(&g_app_mutex);
         return -1;
    }

    /*info used in dma mode */
    cipher_info_tmp.src=src;
    cipher_info_tmp.byte_length=byte_length;

    //to determine wether the source address is the same with destination address
    if(srcvalue==destvalue)
    {
        cipher_info_tmp.flag = 0;
        ctrl_word &= 0xefff;
    }
    else
    { 
        cipher_info_tmp.flag = 1;
        ctrl_word |= 0x1000;
    }

    //accept the slave interface config
    ctrl_word |=0x0400;	
    // set the register's  corresponding bit to crypt but not decrypt    
    ctrl_word &= 0xfffe;

	#ifndef _REVERSE_BYTE_SEQ_	//软件不反字节序,硬件反转
		ctrl_word |=0x6000;
	#endif

    WRITE_REGISTER_ULONG(CIPHER_CTRL_OFFSET, ctrl_word);

      #ifdef DEBUG
	    printf("in hi_aes_crypt function ctrl_word value is :0x%x\n", ctrl_word);
	#endif

	if(aes_set_key_ivin(key_ivin_aes) != 0)
    	{
		printf("aes crypt aes_set_key_ivin error.\n");
		pthread_mutex_unlock(&g_app_mutex);
        	return -1;
    	}
	    
    if(-1 == aes_console(src, dest, byte_length))
    {
        printf("sorry,your crypt in aes failed!\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }

    pthread_mutex_unlock(&g_app_mutex);
    return 0;
    
}

/*
 * the decrypt functions  fro aes
 */
int hi_aes_decrypt(unsigned char *src,unsigned char* dest,unsigned int  byte_length, struct key_ivin_aes* key_ivin_aes)
{
    unsigned int srcvalue;
    unsigned int destvalue;

    srcvalue=(unsigned int)src;
    destvalue=(unsigned int) dest;

    if( (NULL==src)|| (NULL==dest) || (NULL==key_ivin_aes))
    {
        printf("the  pointer in hi_aes_crypt is null !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }

    // to deal with the length in des,aes,3des
    if((byte_length % 16) != 0)
    {
         printf("aes decrypt data byte_length is error.\n");
         pthread_mutex_unlock(&g_app_mutex);
         return -1;
    }
  
    cipher_info_tmp.src=src;
    cipher_info_tmp.byte_length=byte_length;

/*
    printf("in decrypt cipher_info_tmp.src is:\n");
    int i;
    for(i=0;i<16;i++)
    	{
    		
printf("0x%2x\n", *(cipher_info_tmp.src+i));
    	}
*/
    
    //to determine wether the source address is the same with destination address
    if(srcvalue==destvalue)
    {
       cipher_info_tmp.flag=0;
       ctrl_word &=0xefff;
    }
    else
    {
        cipher_info_tmp.flag=1;
        ctrl_word |=0x1000;
    }

    //accept the slave interface config
    ctrl_word |=0x0400;
    // set the register's  corresponding bit to decrypt but not crypt
    ctrl_word |=0x0001;

	#ifndef _REVERSE_BYTE_SEQ_	//软件不反字节序
		ctrl_word |=0x6000;
	#endif
	
    if (ALG_TYPE_AES!=(ctrl_word & ALG_TYPE_MASK) )
    {
        printf("the name of the function is not the same with your input !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }

/*
    if( CTR_MODE== (ctrl_word & AES_MODE_MASK))
    {
        printf("this function can't be used in CTR mode !\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }
*/

    WRITE_REGISTER_ULONG(CIPHER_CTRL_OFFSET,ctrl_word);

        if(aes_set_key_ivin(key_ivin_aes) != 0)
        {
		printf("aes crypt aes_set_key_ivin error.\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
    	}
        
    if(-1 == aes_console(src,dest,byte_length))
    {
        printf("sorry,your decrypt in aes failed!\n");
        pthread_mutex_unlock(&g_app_mutex);
        return -1;
    }

	pthread_mutex_unlock(&g_app_mutex);
	return 0;
}


/*
 * the controlling  functions to determine  which functions we call
 */
static int aes_console(unsigned char* src,unsigned char* dest,unsigned int byte_length)
{
  
   unsigned int mode;

   /*single-group mode*/
   mode = ctrl_word & AES_MODE_MASK;

   #ifdef DEBUG
	    printf("come in aes_console function ctrl_word value is :0x%x\n", ctrl_word);
	#endif
   /*multi-group mode, use dma method*/
   if (MULTI_MODE == (ctrl_word & SIGLE_MASK))
   {
   	if(CTR_MODE == mode)
	{
		printf("CTR  module can't support DMA mode!\n");
		return -1;
	}
	if(-1 == dma_access(dest))
	{
		return -1;
	}
	
   	return 0;
   }

   
   
    switch(mode)
    {
        case ECB_MODE:
        {      
        	  #ifdef DEBUG
        		printf("come int ase ecb decrypt module !\n");
        	  #endif
        	  
                if(-1 == ecb_module(src, dest, byte_length))
                {
                    printf("ecb decrypt module error!\n");
                    return -1;
                }
                break;
                return 0;
        }
       case CBC_MODE:
        {      
                if(-1 == cbc_module(src, dest, byte_length))
                {
                    printf("cbc decrypt module error!\n");
                    return -1;
                }
                break;
                return 0;
        }
       case OFB_MODE:
        {      
		  #ifdef DEBUG
                   printf("in  ofb_aes_module !\n");
		  #endif
		  
                    if(-1 == ofb_aes_module(src, dest, byte_length))
                   {
                       printf("ofb_aes_module  decrypt module error!");
                       return -1;
                   }
                   break;
                   return 0;
        }
       case CTR_MODE:
        {      
		  #ifdef DEBUG
                   printf("in  ctr_aes_module !\n");
		  #endif
                    if(-1 == ctr_aes_module(src, dest, byte_length))
                   {
                       printf("ctr_aes_module  decrypt module error!");
                       return -1;
                   }
                   break;
                   return 0;
        }
         case CFB_MODE:
        {
              if (SHIFT_1 == (ctrl_word & SHIFT_MASK))
               {
			  #ifdef DEBUG
    		                 printf("in CFB_MODE shift_1\n!");
			  #endif
			  
                       if(-1==cfb_1_aes_module(src, dest, byte_length))
                       {
                           printf("cfb_1_aes_module  decrypt module error!");
                           return -1;
                       }
                       return 0;
               }
               else if(SHIFT_8 == (ctrl_word & SHIFT_MASK))
               {  
			  #ifdef DEBUG
                       printf("in CFB_MODE shift_8!");
			  #endif
                       if(-1 == cfb_8_aes_module(src, dest, byte_length))
                       {
                           printf("cfb_8_aes_module  decrypt module error!");
                           return -1;
                       }
                       return 0;
               }
                else 
               {
			  #ifdef DEBUG
	                      printf("in CFB_MODE shift_128!");
			  #endif
                    if(-1 == cfb_aes_module(src, dest, byte_length))
                   {
                       printf("ofb_aes_module  decrypt module error!");
                       return -1;
                   }
                   return 0;
               }
               break;
               
        }
        default:		/*other case be regard as ecb work model*/
        {
               if(-1 == ecb_module(src, dest, byte_length))
                {
                    printf("ecb decrypt module error!\n");
                    return -1;
                }
                break;
                return 0;
        }
    }
 
   return 0;
}


/*
 * the functions below is ctr single-group 
 		mode AES crypt/decrypt algorithm 
 */
static int ctr_aes_module(unsigned char* src,unsigned char* dest,unsigned int byte_length)
{
    unsigned char tmp_out_aes[16];

    if((NULL==src) || (NULL==dest))
    {
        printf("the pointer  operate is null!\n");
        return -1;
    }    

	if(16 != byte_length)
	{
		printf("the ctr mode data length is not 16 bytes!\n");
        	return -1;
	}
	
    if(-1 == aes_config_plaintext(&src[0]) )
        return -1;
    if(-1 == aes_run())
        return -1;
    
    if(-1 == aes_get_data(tmp_out_aes, 0))
        return -1;

    memcpy(&dest[0],tmp_out_aes,16);

    return 0;   
}

/*
 * the functions below is  ecb single-group 
 		mode AES crypt/decrypt algorithm 
 */
static int ecb_module(unsigned char* src,unsigned char* dest,unsigned int byte_length)
{
    unsigned int i;
    unsigned char tmp_out_aes[16];
    if((NULL==src) || (NULL==dest))
    {
        printf("the pointer  operate is null!\n");
        return -1;
    }    
	for(i=0; i<byte_length; )
	{
	    if(-1 == aes_config_plaintext(&src[i]) )
	        return -1;
	    if(-1 == aes_run())
	        return -1;
	    
	    if(-1 == aes_get_data(tmp_out_aes, 0))
	        return -1;

	    memcpy(&dest[i],tmp_out_aes,16);
	    i+=16; 
	} 
   
    return 0;   
}

static int cbc_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i;
    unsigned char tmp_out_aes[16];
    unsigned char  tmp_ivin_out_aes[16];

    #ifdef DEBUG
	    printf("in cbc_module function ctrl_word value is :0x%x\n", ctrl_word);
	#endif

    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

	for(i=0;i<length;)
	{
	        if(i  != 0)
	        {
	        	if(-1==aes_config_iv(tmp_ivin_out_aes))
	               {
	                   return -1;
	               }
	        }                

	//tmp test
	        if(-1==aes_config_plaintext(&src[i]) )
	        {
	            return -1;
	        }
	        
	        if(-1 == aes_run())
	        {
	            return -1;
	        }

	        if(-1==aes_get_data(tmp_out_aes,tmp_ivin_out_aes))
	            return -1;
 
	         memcpy(&dest[i],tmp_out_aes,16);
	         i += 16;
	}

	/*get the final feedback iv value*/
	memcpy(&feedback_iv[0], tmp_ivin_out_aes, 16);
	
	return 0;
}

static int cfb_1_aes_module(unsigned char* src,unsigned char* dest,unsigned int length)
{ 
	unsigned int i,k;

	unsigned char out[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char rout[16];

	unsigned char tmp_out[16];
	unsigned char tmp_ivin_out[16];
	unsigned char temp_plain[4];

	if((NULL==src)||(NULL==dest))
	{
		printf("the pointer  operate is null!");
		return -1;
	}
   
	for(i=0; i<length; )
	{
	        for(k=0;k<128;k++)
	        {
	                if(!((i == 0) &&  (k == 0)))
	                {
	                    if(-1==aes_config_iv(tmp_ivin_out))
	                        return -1;
	                }

	                temp_plain[0] = src[k/8 + i] >> k%8; 
//               tmp = temp_plain[0] & 0x1;		//tmp test 0928
//               if(aes_config_plaintext(&tmp) != 0)	//tmp test 0928
	                if(aes_config_plaintext(temp_plain) != 0)
	                    return -1;
	                if( -1==aes_run())
	                    return -1;
	                if(aes_get_data(tmp_out,tmp_ivin_out) != 0)
	                    return -1;
	                rout[k/8] =  (tmp_out[0] & 0x01) << k%8;
	                out[k/8] |= rout[k/8];
	        }
	        memcpy(&dest[i],out,16);
	        i+= 16;
	 }

	/*get the final feedback iv value*/
	memcpy(&feedback_iv[0], tmp_ivin_out, 16);
	
	return 0;
}

/*
 * the processing module  for cfb&aes&8
 *
 */
static int cfb_8_aes_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i,j;

    unsigned char out[16];

    unsigned char tmp_out[16];
    unsigned char tmp_ivin_out[16];

    unsigned char temp_plain[4];

    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

	for(i=0;i<length;i++)
	{
		for(j=0;j<16;j++)
		{
		    if(!((i == 0) && (j == 0) ))
		    {
		       if(-1==aes_config_iv(tmp_ivin_out))
		         return -1;      
		    }

		    temp_plain[0] = src[j + i];                                                      
		    if(aes_config_plaintext(temp_plain) != 0)
		        return -1;
		    if(aes_run()!= 0)
		        return -1;
		    if(aes_get_data(tmp_out,tmp_ivin_out) != 0)
		        return -1;
		    out[j] = tmp_out[0];

		    
		}
		memcpy(&dest[i],out,16);
		//printf("this is myout from cfb_8_module:%x\n",dest[i]);
		i+= 16;
	}

	/*get the final feedback iv value*/
	memcpy(&feedback_iv[0], tmp_ivin_out, 16);

	return 0;
}

/*
 * the processing module for cfb&aes
 */
static int cfb_aes_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i;


    unsigned char tmp_out[16];
    unsigned char tmp_ivin_out[16];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

	for(i=0; i<length;)
	{	
	    if(i != 0)
	    {
	        if(-1==aes_config_iv(tmp_ivin_out))
	          return -1;
	    }

	    if(aes_config_plaintext(&src[i]) != 0)
	        return -1;

	    if(aes_run()!= 0)
	        return -1;

	    if(aes_get_data(tmp_out,tmp_ivin_out) != 0)
	        return -1;

 	    memcpy(&dest[i],tmp_out,16);

	    i +=16;
	}

   	/*get the final feedback iv value*/
	memcpy(&feedback_iv[0], tmp_ivin_out, 16);
   	
	return 0;
}

/*
 * the processiing module for ofb&aes
 */
static int ofb_aes_module(unsigned char* src,unsigned char* dest,unsigned int length)
{

    unsigned int i;


    unsigned char tmp_out[16];
    unsigned char tmp_ivin_out[16];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

	for(i=0;i<length;)
	{ 
	       if(i != 0)
	       {
	          if(-1==aes_config_iv(tmp_ivin_out))
	          return -1;                   
	       }
	       if(aes_config_plaintext(&src[i]) != 0)
	           return -1;
	       if(aes_run() != 0)
	           return -1;
	       if(aes_get_data(tmp_out,tmp_ivin_out) != 0)
	           return -1;

	       memcpy(&dest[i],tmp_out,16);

	       i += 16;
	}

	/*get the final feedback iv value*/
	memcpy(&feedback_iv[0], tmp_ivin_out, 16);

	return 0;
}

int aes_run()
{
	int i;
 
	i = ioctl(fd_dev, SET_RUN, 0);
	if(-1 == i)
    {
        printf("set the run register failed!\n");
        return -1;
    }

    return 0;
}

