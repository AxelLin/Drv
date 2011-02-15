
#include <stdio.h>
#include <stdlib.h>
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

static int des_config_key( unsigned char * cipher_key1,unsigned char * cipher_key2,unsigned char * cipher_key3);
static int des_config_ivin(unsigned char  * ivin);
static int des_config_plaintext(unsigned char  * pplaintext);
static int des_get_data(unsigned char * cipher_out_tmp,unsigned char * feedback_ivin_tmp);
int 		des_set_key_ivin(struct key_ivin_des* key_ivin);
static int des_console(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int ecb_module(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int cbc_module(unsigned char* src,unsigned char* dest,unsigned int byte_length);
static int cfb_1_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int cfb_8_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int cfb_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int ofb_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int ofb_8_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
static int ofb_1_des_module(unsigned char* src,unsigned char* dest,unsigned int length);
int des_run();

extern  int require_loop(); 
extern  int dma_access(unsigned char  * argp);
extern unsigned  int  fd,fd_dev;

/*
 *  set the key value to  register; here i can't determine
 */
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
#endif

static int des_config_key( unsigned char * cipher_key1,unsigned char * cipher_key2,unsigned char * cipher_key3)
{
    unsigned int * ptmpkey1,*ptmpkey2,*ptmpkey3;
#ifdef _REVERSE_BYTE_SEQ_    
    unsigned char key1[8];
    unsigned char key2[8];
    unsigned char key3[8];
#endif
#ifdef DEBUG    
    unsigned int stavalue;
#endif

#ifdef _REVERSE_BYTE_SEQ_
	reverse_array(cipher_key1, &key1[0]);
	reverse_array(cipher_key2, &key2[0]);
	reverse_array(cipher_key3, &key3[0]);
	
	ptmpkey1= (unsigned int *) (&key1[0]);
	ptmpkey2= (unsigned int *) (&key2[0]);
	ptmpkey3= (unsigned int *) (&key3[0]);
#else
	ptmpkey1= (unsigned int *) (cipher_key1);
	ptmpkey2= (unsigned int *) (cipher_key2);
	ptmpkey3= (unsigned int *) (cipher_key3);
#endif	

   
    if((ALG_TYPE_3DES==(ctrl_word & ALG_TYPE_MASK))
                && ( (CFB_MODE!=(ctrl_word & ECB_MASK)) &&  (OFB_MODE!=(ctrl_word & ECB_MASK)) )
                && (DECRYPT_MASK==(ctrl_word & DENCRYPT_MASK)))		//???
    {
        if(KEY_LENGTH_128_2==(ctrl_word & KEY_LENGTH_MASK))	 // 3des mode 2key
        {   
       #ifdef _REVERSE_BYTE_SEQ_
             WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
       #else
       	WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
		
       #endif
        }
        else		// 3des mode 3key
        {
        #ifdef _REVERSE_BYTE_SEQ_
             WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*(ptmpkey3));
             WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*(ptmpkey3+1));
        #else
        	WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*(ptmpkey3));
             WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*(ptmpkey3+1));
        #endif
        }
    } // end in 3des
    else if (ALG_TYPE_DES_1==(ctrl_word & ALG_TYPE_MASK) 			
                            || ALG_TYPE_DES_2==(ctrl_word & ALG_TYPE_MASK))		//des mode
    {
        WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
        WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
    }		//end des mode 
    else 	//???
    {
        if(KEY_LENGTH_128_2==(ctrl_word & KEY_LENGTH_MASK))
        {
        #ifdef _REVERSE_BYTE_SEQ_
        	WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
       #else
		WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
       #endif
        }
        else
        {
        #ifdef _REVERSE_BYTE_SEQ_
        	WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*(ptmpkey3));
             WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*(ptmpkey3+1));
        #else
        	WRITE_REGISTER_ULONG(CIPHER_KEY1_OFFSET,*(ptmpkey1));
             WRITE_REGISTER_ULONG(CIPHER_KEY2_OFFSET,*(ptmpkey1+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY3_OFFSET,*(ptmpkey2));
             WRITE_REGISTER_ULONG(CIPHER_KEY4_OFFSET,*(ptmpkey2+1));
             WRITE_REGISTER_ULONG(CIPHER_KEY5_OFFSET,*(ptmpkey3));
             WRITE_REGISTER_ULONG(CIPHER_KEY6_OFFSET,*(ptmpkey3+1));
        #endif
        }
   }

    #ifdef DEBUG
       READ_REGISTER_ULONG(CIPHER_KEY1_OFFSET,stavalue);
       printf("key1:%8x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY2_OFFSET,stavalue);
       printf("key2:%8x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY3_OFFSET,stavalue);
       printf("key3:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY4_OFFSET,stavalue);
       printf("key4:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY5_OFFSET,stavalue);
       printf("key5:%x\n",stavalue);  
       READ_REGISTER_ULONG(CIPHER_KEY6_OFFSET,stavalue);
       printf("key6:%x\n",stavalue);  
    #endif
    return 0;
}

/*  
 *to set the ivin value to the register, no matter it is in des,3des,aes.
 * ecb and ctr mode don't need to set this value 
 */
static int des_config_ivin(unsigned char  * ivin)
{

   unsigned  int * ivin_tmp;
#ifdef _REVERSE_BYTE_SEQ_  
	unsigned char iv_char[8];
#endif
#ifdef DEBUG
   unsigned  int stavalue;
#endif 

	#ifdef _REVERSE_BYTE_SEQ_
		reverse_array(ivin, &iv_char[0]);
		ivin_tmp= (unsigned int *) (&iv_char[0]);
	#else
		ivin_tmp =  (unsigned int *) (ivin);
	#endif
	
   if(NULL==ivin)
   {
        printf("the ivin pointer is null!\n");
        return -1;
   }   
     
   if( ECB_MODE==(ctrl_word & ECB_MASK ))
   {
        printf(" ecb mode can't be set  ivin!\n");
        return -1;
   }

      WRITE_REGISTER_ULONG(CIPHER_IVIN1_OFFSET, *ivin_tmp++);
      WRITE_REGISTER_ULONG(CIPHER_IVIN2_OFFSET, *ivin_tmp);

   #ifdef DEBUG
       READ_REGISTER_ULONG(CIPHER_IVIN1_OFFSET,stavalue);
       printf("ivin1:%8x\n",stavalue);  

       READ_REGISTER_ULONG(CIPHER_IVIN2_OFFSET,stavalue);
       printf("ivin2:%8x\n",stavalue); 
   #endif 
   
   return 0;
}

/*
 * we here set the pplaintext to the register
 */
static int des_config_plaintext(unsigned char  * pplaintext)
{
#ifdef _REVERSE_BYTE_SEQ_
    unsigned char p_char[8];
#endif
    unsigned int *pplaintext_tmp;
#ifdef DEBUG    
    unsigned int stavalue;
#endif
    	
   if(NULL == pplaintext)
   {
        printf("the pplaintext pointer is null!\n");
        return -1;
   }   

	#ifdef _REVERSE_BYTE_SEQ_
		if ( ((ctrl_word & 0x30) == 0) || ((ctrl_word & 0x30) == 0x30) )
		{
			reverse_array((unsigned char*)pplaintext, &p_char[0]);
			pplaintext_tmp =  (unsigned int*) (&p_char[0]);
		}
		else
		{
			pplaintext_tmp =  (unsigned int*) (pplaintext);
		}
	#else
		pplaintext_tmp =  (unsigned int*) (pplaintext);
	#endif

   if(-1==require_loop())
   {
       printf("the busy signal is always 1!\n");
       return -1;
   }

    WRITE_REGISTER_ULONG(CIPHER_DIN1_OFFSET,*(unsigned int*)pplaintext_tmp++ );
    WRITE_REGISTER_ULONG(CIPHER_DIN2_OFFSET,*(unsigned int*)pplaintext_tmp );
   
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
       
       READ_REGISTER_ULONG(CIPHER_CTRL_OFFSET,stavalue);
       printf("ctrl_word is :0x%x\n",	stavalue); 
    #endif 

    return 0;

}


/*
 * get data processed by the module ,and clear the interrupt register
*/
static int des_get_data(unsigned char * cipher_out_tmp,unsigned char * feedback_ivin_tmp)
{
    unsigned int stavalue;
    unsigned int * cipher_out = (unsigned int *) cipher_out_tmp;
    unsigned int * feedback_ivin = (unsigned int*) feedback_ivin_tmp;

    if(NULL==cipher_out)
    {
        printf("the outtrans pointer is null !\n\n");
        return -1;
    }    

    if(-1== ioctl(fd_dev, SLAVE_GET_DATA, 0))
    {
        printf("in slave interface,the signal for getting data does not come!\n");
        return -1;
    } 
     
    READ_REGISTER_ULONG(INT_CIPHER__OFFSET, stavalue);   
    if(0x2==(stavalue & 0x02))
    {
        WRITE_REGISTER_ULONG(INT_CIPHER__OFFSET, 0x02);    
        printf("the interrupt is error,can't get the data!\n");
        return -1;
    }
   
    READ_REGISTER_ULONG(CIPHER_DOUT1_OFFSET, *cipher_out++);
    READ_REGISTER_ULONG(CIPHER_DOUT2_OFFSET, *cipher_out);
    
   if( ((ctrl_word & ECB_MASK) != ECB_MODE) && ((ctrl_word & ECB_MASK) != CTR_MODE))
   {
        READ_REGISTER_ULONG(CIPHER_IVOUT1_OFFSET,*feedback_ivin++);
        READ_REGISTER_ULONG(CIPHER_IVOUT2_OFFSET,*feedback_ivin);
   }

	#ifdef _REVERSE_BYTE_SEQ_
		reverse(cipher_out_tmp);
		if ( feedback_ivin_tmp != NULL )
			reverse(feedback_ivin_tmp);		//0930
	#endif
	
    return 0;
}


/*
 * non-ctr mode use this function to set key and ivin
 * value to register for the first time
 */
int 	des_set_key_ivin(struct key_ivin_des* key_ivin)
{
  
    if( NULL==key_ivin )
    {
        printf("the  pointer in set_keyivin is null !\n");
        return -1;
    }

   if(-1==require_loop())
   {
        printf("the busy signal is always 1!\n");
        return -1;
   }

/* 0930 del weimx
    if(ALG_TYPE_3DES==(ctrl_word & ALG_TYPE_MASK)) 
    {
       memcpy(key_ivin_tmp_des.cipher_key1,key_ivin->cipher_key1,8);
       memcpy(key_ivin_tmp_des.cipher_key2,key_ivin->cipher_key2,8);
       memcpy(key_ivin_tmp_des.cipher_key3,key_ivin->cipher_key3,8);
    }
    else if(ALG_TYPE_AES!=(ctrl_word & ALG_TYPE_MASK))
    {
       memcpy(key_ivin_tmp_des.cipher_key1,key_ivin->cipher_key1,8);
    }
    else
    {
    	memcpy(key_ivin_tmp_des.ivin,key_ivin->ivin,8);
    }
    
    // to let the register ivin bit to be disabled when it is ecb mod
    if(((ctrl_word & ALG_TYPE_MASK)==ALG_TYPE_AES))                     
    {
        printf("aes should not call this function \n !");
        return -1;
    }
*/

    //config key
    if(ALG_TYPE_3DES==(ctrl_word & ALG_TYPE_MASK)) 		//  3des mode
    {
       memcpy(key_ivin_tmp_des.cipher_key1,key_ivin->cipher_key1,8);
       memcpy(key_ivin_tmp_des.cipher_key2,key_ivin->cipher_key2,8);
       memcpy(key_ivin_tmp_des.cipher_key3,key_ivin->cipher_key3,8);
    }
    else	//des mode
    {
       memcpy(key_ivin_tmp_des.cipher_key1,key_ivin->cipher_key1,8);
    }

    //config ivin
    memcpy(key_ivin_tmp_des.ivin,key_ivin->ivin,8);
    
/*
    if( ECB_MODE==(ctrl_word & ECB_MASK ))
    {
        ctrl_word &=0xfbff;
    }
    else
    {
        if(-1== des_config_ivin(key_ivin_tmp_des.ivin))
        {
            printf("we set ivin error in set_keyivin!\n");
            return -1;
        }
    }
   */
   if( ECB_MODE != (ctrl_word & ECB_MASK ))
   {
	   if(-1== des_config_ivin(key_ivin_tmp_des.ivin))
	        {
	            printf("we set ivin error in set_keyivin!\n");
	            return -1;
	        }
   }
   
   if(-1== des_config_key(key_ivin_tmp_des.cipher_key1,key_ivin_tmp_des.cipher_key2,key_ivin_tmp_des.cipher_key3))
   {
       printf("we set key  error in set_keyivin!\n");
       return -1;
   }


   return 0;
}



/*
 * the encrypt entrance function  of des 
 */
int hi_des_crypt(unsigned char *src,unsigned char* dest,unsigned int  byte_length, struct key_ivin_des *key_ivin)
{
	unsigned int srcvalue;
	unsigned int destvalue;

	srcvalue=(unsigned int)src;
	destvalue=(unsigned int) dest;

	if( (NULL==src)|| (NULL==dest)  || (NULL == key_ivin))
	{
		printf("the  pointer in hi_des_crypt is null !\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	// to deal with the length in des,3des
	if((byte_length % 8) != 0)
	{
		 printf("des or 3des decrypt data byte_length is not 64 bits.\n");
		 pthread_mutex_unlock(&g_app_mutex);		//1011 del this two lines
		 return -1;	
	}

	cipher_info_tmp.src=src;
	cipher_info_tmp.byte_length=byte_length;  
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
	// set the register's  corresponding bit to crypt but not decrypt
	ctrl_word &=0xfffe;
	//printf("ctrl_word2:%x\n",ctrl_word);  

	#ifndef _REVERSE_BYTE_SEQ_	//软件不反字节序
		ctrl_word |=0x6000;
	#endif
	
	if (ALG_TYPE_AES==(ctrl_word & ALG_TYPE_MASK) )
	{
		printf("the name of the function is not the same with your input !\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	#ifdef DEBUG
		printf("Raw ctrl_word is:%d\n",  ctrl_word);
	#endif
	   
    	WRITE_REGISTER_ULONG(CIPHER_CTRL_OFFSET,ctrl_word);

	#ifdef DEBUG
	if(-1==require_loop())
	{
		printf("the busy signal is always 1!\n");
		return -1;
	}
	READ_REGISTER_ULONG(CIPHER_CTRL_OFFSET,tmp);
       printf("after WRITE_REGISTER_ULONG ctrl_word is :%d\n", tmp); 
	#endif
	
	if(des_set_key_ivin(key_ivin) != 0 )
	{
		printf("des_set_key_ivin error!\n");
		return -1;
	}

	if(-1 == des_console(src,dest,byte_length))
	{
		printf("sorry,your crypt in des failed!\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	pthread_mutex_unlock(&g_app_mutex);
	return 0;
}

/*
 * the decrypt entrance  of des
 */
int hi_des_decrypt(unsigned char *src,unsigned char* dest,unsigned int  byte_length, struct key_ivin_des *key_ivin)
{

	unsigned int srcvalue;
	unsigned int destvalue;

	srcvalue=(unsigned int)src;
	destvalue=(unsigned int) dest;

	if( (NULL==src)|| (NULL==dest) || (NULL == key_ivin))
	{
		printf("the  pointer in hi_des_crypt is null !\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	// to deal with the length in des,aes,3des
	if((byte_length % 8) != 0)
	{
		printf("des or 3des decrypt data byte_length is not 64 bits.\n");
		pthread_mutex_unlock(&g_app_mutex);		//20071011 del this two lines
		return -1;
	}

	cipher_info_tmp.src=src;
	cipher_info_tmp.byte_length=byte_length;
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
	// set the register's  corresponding bit to crypt but not decrypt
	ctrl_word |=0x0001;

	#ifndef _REVERSE_BYTE_SEQ_	//软件不反字节序
		ctrl_word |=0x6000;
	#endif

	if (ALG_TYPE_AES==(ctrl_word & ALG_TYPE_MASK) )
	{
		printf("the name of the function is not the same with your input !");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	WRITE_REGISTER_ULONG(CIPHER_CTRL_OFFSET,ctrl_word);

	if(des_set_key_ivin(key_ivin) != 0 )
	{
		printf("des_set_key_ivin error!\n");
		return -1;
	}

	if(-1 == des_console(src,dest,byte_length))
	{
		printf("sorry,your decrypt in des failed!\n");
		pthread_mutex_unlock(&g_app_mutex);
		return -1;
	}

	pthread_mutex_unlock(&g_app_mutex);
	return 0;
}

/*
 * the controlling  functions to determine  which functions we call
 */
static int des_console(unsigned char* src,unsigned char* dest,unsigned int byte_length)
{
   unsigned int mode;
   mode=ctrl_word & ECB_MASK;

   /*multi-group mode, use dma method*/
   if (MULTI_MODE == (ctrl_word & SIGLE_MASK))
   {
	if(-1 == dma_access(dest))
            return -1;
	
   	return 0;
   }

    switch(mode)
    {
        case ECB_MODE:
        {     
        	#ifdef DEBUG
        	printf("*******************come into ecb_module branch!\n");
        	#endif
                if(-1==ecb_module(src,dest,byte_length))
                {
                    printf("ecb decrypt module error!\n");
                    return -1;
                }
                break;
        }
       case CBC_MODE:
        {
        	#ifdef DEBUG
   	        printf("*******************come into cbc_module branch!\n");
        	#endif
                if(-1==cbc_module(src,dest,byte_length))
                {
                    printf("cbc decrypt module error!\n");
                    return -1;
                }
                break;
        }
       case OFB_MODE:
        {     
        	  if (SHIFT_1==(ctrl_word & SHIFT_MASK))
               {
               	#ifdef DEBUG
                   	printf("*******************come into ofb_module SHIFT_1 branch!\n");
               	#endif
                   if(-1==ofb_1_des_module(src,dest,byte_length))
                   {
                       printf("ofb_1_module  decrypt module error!");
                       return -1;
                   }
               }
               else if(SHIFT_8==(ctrl_word & SHIFT_MASK))
               {
               	#ifdef DEBUG
               	printf("*******************come into ofb_module SHIFT_8 branch!\n");
               	#endif
                   //printf("in  ofb_8_module !\n");
                   if(-1==ofb_8_des_module(src,dest,byte_length))
                   {
                       printf("ofb_8_module  decrypt module error!");
                       return -1;
                   }
               }
              else if(ALG_TYPE_AES!=(ctrl_word & ALG_TYPE_MASK))
               {
               	#ifdef DEBUG
               	printf("*******************come into ofb_module SHIFT_64 branch!\n");
               	#endif
                  // printf("in  ofb_des_module !\n");
                    if(-1==ofb_des_module(src,dest,byte_length))
                   {
                       printf("ofb_des_module  decrypt module error!");
                       return -1;
                   }
               }
                break;
        }
         case CFB_MODE:
        {
              if (SHIFT_1==(ctrl_word & SHIFT_MASK))
               {
               	#ifdef DEBUG
               	printf("*******************come into cfb_module SHIFT_1 branch!\n");
               	#endif
                       if(-1==cfb_1_des_module(src,dest,byte_length))
                        {
                           printf("cfb_1_des_module  decrypt module error!");
                           return -1;
                        }
               }
               else if(SHIFT_8==(ctrl_word & SHIFT_MASK))
               {
               	#ifdef DEBUG
               	  printf("*******************come into cfb_module SHIFT_8 branch!\n");
               	#endif
                      if(-1==cfb_8_des_module(src,dest,byte_length))
                       {
                           printf("ofb_8_module  decrypt module error!");
                           return -1;
                       }
               }
               else if(ALG_TYPE_AES!=(ctrl_word & ALG_TYPE_MASK))
               {
               	#ifdef DEBUG
               	printf("*******************come into cfb_module SHIFT_64 branch!\n");
               	#endif
                    if(-1==cfb_des_module(src,dest,byte_length))
                   {
                       printf("ofb_des_module  decrypt module error!");
                       return -1;
                   }
               }
               break;

        }
    }
 
   return 0;
}


/*
 * the functions below is the modules 
 */
static int ecb_module(unsigned char* src,unsigned char* dest,unsigned int byte_length)
{
    unsigned int i;
    unsigned char tmp_out_des[8];

    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!\n");
        return -1;
    }    

    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<byte_length;)
        {
           
               //hi_des_stop();
                if(-1==des_config_plaintext(&src[i]) )
                    return -1;
                if(-1 == des_run())
                    return -1;
                
                //printf("set start signal  ends!\n");
                if(-1==des_get_data(tmp_out_des,0))
                     return -1;

                memcpy(&dest[i],tmp_out_des,8);
               // printf("get data  ends!\n");
                i+=8;
        } 
    } 

    return 0;   
}

static int cbc_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i;
    unsigned char tmp_out_des[8];
    
    unsigned char  tmp_ivin_out_des[8];

    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {
                if(i != 0)
                {
                     if(-1 == des_config_ivin(tmp_ivin_out_des))
                     {
                         return -1;
                     }
                }                

                if(-1 == des_config_plaintext(&src[i]) )
                {
                    return -1;
                }
                if(-1 == des_run())
                {
                    return -1;
                }

                if(-1==des_get_data(tmp_out_des,tmp_ivin_out_des))
		  {
		      return -1;
		  }

                memcpy(&dest[i],tmp_out_des,8);

                i += 8;
        }

	  /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out_des, 8);
    }

    return 0;
}


static int cfb_1_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    
    unsigned int i,j;

    unsigned char  out[8]={0,0,0,0,0,0,0,0};
    unsigned char  rout[8];
    unsigned char  temp_plain[4];

    unsigned  char tmp_out[8];
    unsigned  char  tmp_ivin_out[8];


    printf(" i am in cfb_1_des!\n"); 
    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

    printf(" i am in cfb_1_des!\n"); 
    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {
            for(j=0;j<64;j++)
            {
                if(!((i == 0) && (j == 0)))
                {
                   if(-1==des_config_ivin(tmp_ivin_out)) 
                   return -1;
                }

                temp_plain[0] = src[i+j/8] >> j%8;                                                     
                if(des_config_plaintext(temp_plain) != 0)
                    return -1;
       
                if(des_run()!= 0)
                    return -1;

                if(-1==des_get_data(tmp_out, tmp_ivin_out))
                    return -1;

                rout[j/8] =  (tmp_out[0] & 0x01) << (j%8);
                out[j/8] |= rout[j/8];

            }
                    memcpy(&dest[i],out,8);
                    i=i+8;
        }

        /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);
        
    }

    return 0; 
}



/*
 * the processing module  for cfb&des&8
 */
static int cfb_8_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i,j;

    unsigned char out[8];
    unsigned char temp_plain[4];

    unsigned char  tmp_out[8];
    unsigned char  tmp_ivin_out[8];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

    if(SIGLE_MODE==(ctrl_word & SIGLE_MASK) )
    {
        for(i=0;i<length;)
        {
              for(j=0;j<8;j++)
              {
                    if(!((i == 0) && (j == 0)))
                    {
                        if(-1==des_config_ivin(tmp_ivin_out))
                            return -1;
                    }

                    temp_plain[0] = src[i+j] ;                                               

                    if(des_config_plaintext(temp_plain) != 0)
                        return -1;

                    if(des_run() != 0)
                        return -1;

                    if(des_get_data(tmp_out,tmp_ivin_out) != 0)
                        return -1;
                    out[j]= tmp_out[0];


              }
                
              memcpy(&dest[i],out,8);
              i+=8;
        }  

        /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);
    }

    return 0;
} 


/* 
 *the processing module for cfb&des
 */
static int cfb_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i;

    unsigned char  tmp_out[8];
    unsigned char  tmp_ivin_out[8];

    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {

            if(i!=0)
            {
                 if(-1==des_config_ivin(tmp_ivin_out))
                   return -1;
            }

            if(des_config_plaintext(&src[i]) != 0)
                   return -1;
            if(des_run() != 0)
                   return -1;
            if(des_get_data(tmp_out,tmp_ivin_out) != 0)
                   return -1;


            memcpy(&dest[i],tmp_out,8);
            i+=8;
        }

        /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);

    }    
    return 0;
}


/*
 * the ofb modules
 */
static int ofb_1_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i,j;

    unsigned char out[8]={0,0,0,0,0,0,0,0};
    unsigned char temp_plain[4];

    unsigned char  tmp_out[8];
    unsigned char  tmp_ivin_out[8];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }


    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {
            for(j=0;j<64;j++)
            {
                if(!((i == 0) && (j == 0)))
                {
                    if(-1==des_config_ivin(tmp_ivin_out))
                    return -1;
                }

                temp_plain[0] = src[i+j/8] >> j%8;                                                     
                if(des_config_plaintext(temp_plain) != 0)
                     return -1;
                if(des_run() != 0)
                    return -1;
                if(des_get_data(tmp_out,tmp_ivin_out) != 0)
                    return -1;
                out[j/8] |=  (tmp_out[0] & 0x01) << j%8;

            }
            memcpy(&dest[i],out,8); 
            i+=8;
        }

        /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);
    }

    return 0;
}


/*
 * the processing module for ofb&des&8
 */
static int ofb_8_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i,j;

    unsigned char out[8];
    unsigned char temp_plain[4];

    unsigned char  tmp_out[8];
    unsigned char  tmp_ivin_out[8];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }


    if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {
            
            for(j=0;j<8;j++)
            {

                 if(!((i == 0) && (j == 0)))
                 {
                     if(-1==des_config_ivin(tmp_ivin_out))
                      return -1;
                 }

                  temp_plain[0] = src[i+j] ;                                                   
                  if(des_config_plaintext(temp_plain) != 0)
                    return -1;
                  if(des_run()!= 0)
                    return -1;
                  if(des_get_data(tmp_out,tmp_ivin_out) != 0)
                    return -1;
                  out[j] = tmp_out[0];

            }
             memcpy(&dest[i],out,8);
             i+=8; 
       }

        /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);
    }
   
    return 0;
}

/*
 * the processing module for ofb&des
 */
static int ofb_des_module(unsigned char* src,unsigned char* dest,unsigned int length)
{
    unsigned int i;


    unsigned char  tmp_out[8];
    unsigned char  tmp_ivin_out[8];


    if((NULL==src)||(NULL==dest))
    {
        printf("the pointer  operate is null!");
        return -1;
    }

     if (SIGLE_MODE==(ctrl_word & SIGLE_MASK))
    {
        for(i=0;i<length;)
        {
            if(i!=0)
            {
                if(-1==des_config_ivin(tmp_ivin_out))
                 return -1;
            }

            if(des_config_plaintext(&src[i]) != 0)
                 return -1;

            if(des_run() != 0)
            {
                return -1;
            }

            if(des_get_data(tmp_out,tmp_ivin_out) != 0)
                 return -1;


		memcpy(&dest[i],tmp_out,8);
            i+=8;
        } 

	  /*get the final feedback iv value*/
	 memcpy(&feedback_iv[0], tmp_ivin_out, 8);
    }
 
    return 0;
}



int des_run()
{
	if(-1== ioctl(fd_dev,SET_RUN,0))
	{
		printf("set the run register failed!\n");
		return -1;
	}

	return 0;
}

