#include <string.h>
#include <stdio.h>
#include "hi_des.h"
#include "../app/hi_daes_api.h"

/*below are ctr_word related CODE*/
#define ECB_WORK_MODE 0
#define CBC_WORK_MODE 1
#define CFB_WORK_MODE 2
#define OFB_WORK_MODE 3
#define CTR_WORK_MODE 4

#define STANDARD_BIT_WIDTH 	0	/*defaul value,des/3des is 64 bits,Aes is 128 bits*/
#define EIGHT_BIT_WIDTH 		1	/*8 bits*/
#define ONE_BIT_WIDTH 			2	/*1 bit*/

#define AES_KEY_128_BITS		0	/*Aes key lengths is 128 bits*/
#define AES_KEY_192_BITS		1	/*Aes key lengths is 128 bits*/
#define AES_KEY_256_BITS		2	/*Aes key lengths is 128 bits*/
#define _3DES_THREE_KEY		0	/*3des 3 keys*/
#define _3DES_TWO_KEY			3	/*3des 2 keys*/
#define DES_KEY_LENGTH			0	/*des default keys length*/

#define DES_ALG					0	/*Des alg*/
#define _3DES_ALG				1	/*3des alg*/
#define AES_ALG					2	/*Aes alg*/

#define SINGLE_ENCRYPT			0	/*single group mode*/
#define MULTI_ENCRYPT			1	/*multi group mode*/

struct key_ivin_des des_key_iv;

struct daes_ctrl ctr_word;

void DES_ecb_encrypt(const_DES_cblock *input, DES_cblock *output,
		     					DES_key_schedule *ks, int enc)
{
	if((NULL == input) || (NULL == output) || (NULL == ks))
	{
		printf("DES_ecb_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_ecb_encrypt para enc invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ecb_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = ECB_WORK_MODE; 	 //模式ecb;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

  	memcpy(des_key_iv.cipher_key1, (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_ecb_encrypt cipher_key1 NULL error!\n");
	}
	
	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)input, (unsigned char *)output, 8, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)input, (unsigned char *)output, 8, &des_key_iv);
	}

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ecb_encrypt hi_daes_init error!\n");
	}
 }

void DES_cbc_encrypt(const unsigned char *input,unsigned char *output,
		     long length,DES_key_schedule *ks,DES_cblock *ivec,
		     int enc)
{
	if((NULL == input) || (NULL == output) || (NULL == ks) || (NULL == ivec) )
	{
		printf("DES_cbc_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_cbc_encrypt para enc invalid!\n");
		return;
	}
	
	if( length%8 != 0)
	{
		printf("DES_cbc_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_cbc_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CBC_WORK_MODE; 		//模式cbc;
	ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_cbc_encrypt cipher_key1 NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)input, (unsigned char *)output, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)input, (unsigned char *)output, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("DES_cbc_encrypt hi_daes_init error!\n");
	}
	
}

void DES_cfb64_encrypt(const unsigned char *in,unsigned char *out,long length,
		       DES_key_schedule *ks,DES_cblock *ivec,int *num,
		       int enc)
{
	if((NULL == in) || (NULL == out) || (NULL == ks) ||
								(NULL == ivec)  || (NULL == num) )
	{
		printf("DES_cfb64_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_cfb64_encrypt para enc invalid!\n");
		return;
	}

	if( length%8 != 0)
	{
		printf("DES_cfb64_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_cfb64_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 		//模式cbc;
	ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_cfb64_encrypt cipher_key1 NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_cfb64_encrypt hi_daes_init error!\n");
	}
}

void DES_cfb_encrypt(const unsigned char *in,unsigned char *out,int numbits,
		     long length,DES_key_schedule *ks,DES_cblock *ivec,
		     int enc)
{
	if((NULL == in) || (NULL == out) || (NULL == ks) ||
								(NULL == ivec) ) 
	{
		printf("DES_cfb_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_cfb_encrypt para enc invalid!\n");
		return;
	}

 	if(!((numbits == 1) ||(numbits == 8) || (numbits == 64)))
	{
		printf("DES_cfb_encrypt para numbits invalid!\n");
		return;
	}
 	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_cfb_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 		//模式cbc;
	if( numbits == 8 )
	{
		ctr_word.daes_bit_width  = EIGHT_BIT_WIDTH; 	//位宽8bits;
	}
	else if ( numbits == 1 )
	{
		ctr_word.daes_bit_width  = ONE_BIT_WIDTH; 		//位宽1bits;
	}
	else if ( numbits == 64 )
	{
		ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	}
	else 
	{
		hi_daes_exit();
		return;
	}
	
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_cfb_encrypt cipher_key1 NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_cfb_encrypt hi_daes_init error!\n");
	}
}

void DES_ofb_encrypt(const unsigned char *in,unsigned char *out,int numbits,
		     long length,DES_key_schedule *ks,DES_cblock *ivec)
{
	int l = length * numbits/8;

	if((NULL == in) || (NULL == out) || (NULL == ks) ||
								(NULL == ivec) ) 
	{
		printf("DES_ofb_encrypt para null pointer error!\n");
		return;
	}

	if(!((numbits == 1) ||(numbits == 8) || (numbits == 64)))
	{
		printf("DES_ofb_encrypt para numbits invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ofb_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = OFB_WORK_MODE; 		//模式cbc;
	if( numbits == 8 )
	{
		ctr_word.daes_bit_width  = EIGHT_BIT_WIDTH; 	//位宽8bits;
	}
	else if ( numbits == 1 )
	{
		ctr_word.daes_bit_width  = ONE_BIT_WIDTH; 		//位宽1bits;
	}
	else if ( numbits == 64 )
	{
		ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	}
	else 
	{
		hi_daes_exit();
		return;
	}
	
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_ofb_encrypt cipher_key1 NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	hi_des_crypt((unsigned char *)in, (unsigned char *)out, l, &des_key_iv);

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ofb_encrypt hi_daes_init error!\n");
	}
}

/*not implement the para:num's function*/
void DES_ofb64_encrypt(const unsigned char *in,unsigned char *out,long length,
		       DES_key_schedule *ks,DES_cblock *ivec,int *num)
{
	if((NULL == in) || (NULL == out) || (NULL == ks) ||
								(NULL == ivec) || (NULL == num) ) 
	{
		printf("DES_ofb64_encrypt para null pointer error!\n");
		return;
	}

 	if((length % 8) != 0)
	{
		printf("DES_ofb64_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_cfb64_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = OFB_WORK_MODE; 		//模式cbc;
	ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks, 8);
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_ofb64_encrypt cipher_key1 NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ofb64_encrypt hi_daes_init error!\n");
	}
}

void DES_ecb3_encrypt(const_DES_cblock *input, DES_cblock *output,
		      DES_key_schedule *ks1,DES_key_schedule *ks2,
		      DES_key_schedule *ks3, int enc)
{
	if((NULL == input) || (NULL == output) || (NULL == ks1) ||
								(NULL == ks2) || (NULL == ks3) ) 
	{
		printf("DES_ecb3_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_ecb3_encrypt para enc invalid!\n");
		return;
	} 
 	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ecb3_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = ECB_WORK_MODE; 	 //模式ecb;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = _3DES_ALG;		//_3Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

  	memcpy(des_key_iv.cipher_key1, (unsigned char *)ks1, 8);
  	memcpy(des_key_iv.cipher_key2, (unsigned char *)ks2, 8);
  	memcpy(des_key_iv.cipher_key3, (unsigned char *)ks3, 8);
  	
	if(des_key_iv.cipher_key1 == NULL)
	{
	  	printf("DES_ecb3_encrypt cipher_key1 NULL error!\n");
	}
	
	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)input, (unsigned char *)output, 8, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)input, (unsigned char *)output, 8, &des_key_iv);
	}

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ecb3_encrypt hi_daes_init error!\n");
	}
	
}

void DES_ede3_cbc_encrypt(const unsigned char *input,unsigned char *output, 
			  long length,
			  DES_key_schedule *ks1,DES_key_schedule *ks2,
			  DES_key_schedule *ks3,DES_cblock *ivec,int enc)
{
	if((NULL == input) || (NULL == output) || (NULL == ks1) ||
						(NULL == ks2) || (NULL == ks3) || (NULL == ivec)) 
	{
		printf("DES_ede3_cbc_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_ede3_cbc_encrypt para enc invalid!\n");
		return;
	} 

	if((length % 8) != 0)
	{
		printf("DES_ede3_cbc_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ede3_cbc_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CBC_WORK_MODE; 	 //模式cbc;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = _3DES_ALG;		//_3Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

  	memcpy(des_key_iv.cipher_key1, (unsigned char *)ks1, 8);
  	memcpy(des_key_iv.cipher_key2, (unsigned char *)ks2, 8);
  	memcpy(des_key_iv.cipher_key3, (unsigned char *)ks3, 8);
  	
	if(des_key_iv.cipher_key1 == NULL || des_key_iv.cipher_key2 == NULL || 
		des_key_iv.cipher_key3 == NULL )
	{
	  	printf("DES_ede3_cbc_encrypt cipher_key NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);
	
	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)input, (unsigned char *)output, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)input, (unsigned char *)output, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ede3_cbc_encrypt hi_daes_exit error!\n");
	}
}

void DES_ede3_cfb_encrypt(const unsigned char *in,unsigned char *out,
			  int numbits,long length,DES_key_schedule *ks1,
			  DES_key_schedule *ks2,DES_key_schedule *ks3,
			  DES_cblock *ivec,int enc)
{
	if((NULL == in) || (NULL == out) || (NULL == ks1) ||
						(NULL == ks2) || (NULL == ks3) || (NULL == ivec)) 
	{
		printf("DES_ede3_cfb_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_ede3_cfb_encrypt para enc invalid!\n");
		return;
	} 

	if(!((numbits == 1) ||(numbits == 8) || (numbits == 64)))
	{
		printf("DES_ede3_cfb_encrypt para numbits invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ede3_cfb_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 		//模式cfb;
	if( numbits == 8 )
	{
		ctr_word.daes_bit_width  = EIGHT_BIT_WIDTH; 	//位宽8bits;
	}
	else if ( numbits == 1 )
	{
		ctr_word.daes_bit_width  = ONE_BIT_WIDTH; 		//位宽1bits;
	}
	else if ( numbits == 64 )
	{
		ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	}
	else 
	{
		hi_daes_exit();
		return;
	}
	
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = _3DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks1, 8);
	memcpy(&des_key_iv.cipher_key2[0], (unsigned char *)ks2, 8);
	memcpy(&des_key_iv.cipher_key3[0], (unsigned char *)ks3, 8);
	
	if(des_key_iv.cipher_key1 == NULL || des_key_iv.cipher_key2 == NULL || 
		des_key_iv.cipher_key3 == NULL )
	{
	  	printf("DES_ede3_cfb_encrypt cipher_key NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ede3_cfb_encrypt hi_daes_exit error!\n");
	}
}

void DES_ede3_cfb64_encrypt(const unsigned char *in,unsigned char *out,
			    long length,DES_key_schedule *ks1,
			    DES_key_schedule *ks2,DES_key_schedule *ks3,
			    DES_cblock *ivec,int *num,int enc)
{
	if((NULL == in) || (NULL == out) || (NULL == ks1) ||
			(NULL == num) || (NULL == ks2) || (NULL == ks3) || (NULL == ivec)) 
	{
		printf("DES_ede3_cfb64_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == DES_ENCRYPT ) || (enc == DES_DECRYPT )))
	{
		printf("DES_ede3_cfb64_encrypt para enc invalid!\n");
		return;
	}

	if( length%8 != 0)
	{
		printf("DES_ede3_cfb64_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ede3_cfb64_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 		//模式cfb;
	ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;	//
	ctr_word.daes_alg = _3DES_ALG;		//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;	//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks1, 8);
	memcpy(&des_key_iv.cipher_key2[0], (unsigned char *)ks2, 8);
	memcpy(&des_key_iv.cipher_key3[0], (unsigned char *)ks3, 8);
	
	if(des_key_iv.cipher_key1 == NULL || des_key_iv.cipher_key2 == NULL || 
		des_key_iv.cipher_key3 == NULL )
	{
	  	printf("DES_ede3_cfb64_encrypt cipher_key NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	if ( enc == DES_ENCRYPT )
	{
		hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}
	else
	{
		hi_des_decrypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);

	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ede3_cfb64_encrypt hi_daes_exit error!\n");
	}
}

void DES_ede3_ofb64_encrypt(const unsigned char *in,unsigned char *out,
			    long length,DES_key_schedule *ks1,
			    DES_key_schedule *ks2,DES_key_schedule *ks3,
			    DES_cblock *ivec,int *num)
{
	if((NULL == in) || (NULL == out) || (NULL == ks1) ||
			(NULL == num) || (NULL == ks2) || (NULL == ks3) || (NULL == ivec)) 
	{
		printf("DES_ede3_ofb64_encrypt para null pointer error!\n");
		return;
	}

	if( length%8 != 0)
	{
		printf("DES_ede3_ofb64_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("DES_ede3_ofb64_encrypt hi_daes_init error!\n");
	}
	
	ctr_word.daes_work_mod = OFB_WORK_MODE; 		//模式cfb;
	ctr_word.daes_bit_width  = STANDARD_BIT_WIDTH; 	//位宽64bits;
	ctr_word.daes_key_length = DES_KEY_LENGTH;		//
	ctr_word.daes_alg = _3DES_ALG;					//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;				//single mode

	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;			//DMA mode
	#endif

	memcpy(&des_key_iv.cipher_key1[0], (unsigned char *)ks1, 8);
	memcpy(&des_key_iv.cipher_key2[0], (unsigned char *)ks2, 8);
	memcpy(&des_key_iv.cipher_key3[0], (unsigned char *)ks3, 8);
	
	if(des_key_iv.cipher_key1 == NULL || des_key_iv.cipher_key2 == NULL || 
		des_key_iv.cipher_key3 == NULL )
	{
	  	printf("DES_ede3_ofb64_encrypt cipher_key NULL error!\n");
	}

	memcpy(&des_key_iv.ivin[0],  ivec, 8);

	hi_daes_config(&ctr_word);

	hi_des_crypt((unsigned char *)in, (unsigned char *)out, length, &des_key_iv);

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 8);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("DES_ede3_ofb64_encrypt hi_daes_exit error!\n");
	}
}


