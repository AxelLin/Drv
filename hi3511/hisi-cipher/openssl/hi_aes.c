#include <string.h>
#include <stdio.h>
#include "../app/hi_daes_api.h"
#include "hi_aes.h"

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

#define AES_ENCRYPT	1
#define AES_DECRYPT	0

#define AES_INIV_LENGTH 16

struct key_ivin_aes aes_key_iv;

struct daes_ctrl ctr_word;

//extern unsigned char feedback_iv[16];

int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key)
{
	if ( !(bits == 128 || bits == 192 || bits == 256) )
	{
		printf("AES_set_encrypt_key bits is invalid!\n");
		return -1;
	}

	if (userKey == NULL)
	{
		printf("AES_set_encrypt_key userKey is NULL!\n");
		return -1;
	}

	if (key == NULL)
	{
		printf("AES_set_encrypt_key key is NULL!\n");
		return -1;
	}
	
	memcpy(key->rd_key, userKey, bits/8);
	key->key_bits = bits;

	return 0;
}

void AES_ecb_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key, const int enc)
{
	if((NULL == in) || (NULL == out) || (NULL == key))
	{
		printf("AES_ecb_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == AES_ENCRYPT ) || (enc == AES_DECRYPT )))
	{
		printf("AES_ecb_encrypt para enc invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("AES_ecb_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = ECB_WORK_MODE; 	 //模式ecb;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽128bits;
	ctr_word.daes_alg = AES_ALG;		
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;

	ctr_word.daes_group = SINGLE_ENCRYPT;		//single mode
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	if ( key == NULL )
	{
	  	printf("AES_ecb_encrypt key NULL error!\n");
	  	return;
	}
  	memcpy(&aes_key_iv.cipher_key[0], key->rd_key, key->key_bits/8);
		
	hi_daes_config(&ctr_word);

	if ( enc == AES_ENCRYPT )
	{
		hi_aes_crypt((unsigned char *)in, (unsigned char *)out, 16, &aes_key_iv);
	}
	else
	{
		hi_aes_decrypt((unsigned char *)in, (unsigned char *)out, 16, &aes_key_iv);
	}
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_ecb_encrypt hi_daes_exit error!\n");
	}
}

void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, const int enc)
{
	int len = length;
	if((NULL == in) || (NULL == out) || (NULL == key) || (NULL == ivec))
	{
		printf("AES_cbc_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == AES_ENCRYPT ) || (enc == AES_DECRYPT )))
	{
		printf("AES_cbc_encrypt para enc invalid!\n");
		return;
	}

	if( length%16 != 0)
	{
		printf("AES_cbc_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("AES_cbc_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = CBC_WORK_MODE; 	 //模式cbc;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽128bits;
	ctr_word.daes_alg = AES_ALG;				
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;

	ctr_word.daes_group = SINGLE_ENCRYPT;		//single mode
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	if ( key == NULL )
	{
	  	printf("AES_cbc_encrypt para key is NULL error!\n");
	  	return;
	}
  	memcpy(&aes_key_iv.cipher_key[0], key->rd_key, key->key_bits/8);

	if ( ivec == NULL )
	{
	  	printf("AES_cbc_encrypt para ivec is NULL error!\n");
	  	return;
	}
	memcpy(&aes_key_iv.ivin[0], ivec, AES_INIV_LENGTH);
		
	hi_daes_config(&ctr_word);

	if ( enc == AES_ENCRYPT )
	{
		hi_aes_crypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}
	else
	{
		hi_aes_decrypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}

	
	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 16);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_cbc_encrypt hi_daes_exit error!\n");
	}
}

void AES_cfb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc)
{
	int len = length;
	if((NULL == in) || (NULL == out) || (NULL == key) || 
							(NULL == ivec) || (NULL == num))
	{
		printf("AES_cfb128_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == AES_ENCRYPT ) || (enc == AES_DECRYPT )))
	{
		printf("AES_cfb128_encrypt para enc invalid!\n");
		return;
	}

	if( length%16 != 0)
	{
		printf("AES_cfb128_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("AES_cfb128_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 	 //模式cfb128;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽128bits;
	ctr_word.daes_alg = AES_ALG;		
	ctr_word.daes_group = SINGLE_ENCRYPT;		//single mode
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;
	
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	if ( key == NULL )
	{
	  	printf("AES_cfb128_encrypt para key NULL error!\n");
	  	return;
	}
  	memcpy(aes_key_iv.cipher_key, key->rd_key, key->key_bits/8);
	
	if ( ivec == NULL )
	{
	  	printf("AES_cfb128_encrypt para ivec is NULL error!\n");
	  	return;
	}
	memcpy(&aes_key_iv.ivin[0], ivec, AES_INIV_LENGTH);
	
	hi_daes_config(&ctr_word);

	if ( enc == AES_ENCRYPT )
	{
		hi_aes_crypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}
	else
	{
		hi_aes_decrypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 16);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_cfb128_encrypt hi_daes_exit error!\n");
	}
}

void AES_cfb8_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc)
{
	int len = length;
	if((NULL == in) || (NULL == out) || (NULL == key) || 
							(NULL == ivec) || (NULL == num))
	{
		printf("AES_cfb8_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == AES_ENCRYPT ) || (enc == AES_DECRYPT )))
	{
		printf("AES_cfb8_encrypt para enc invalid!\n");
		return;
	}

	if ( hi_daes_init() != 0 )
	{
		printf("AES_cfb8_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 	//模式cfb8;
	ctr_word.daes_bit_width = EIGHT_BIT_WIDTH; 	//位宽8bits;
	ctr_word.daes_alg = AES_ALG;					//Des alg
	ctr_word.daes_group = SINGLE_ENCRYPT;			//single mode
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;
	
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	if( key == NULL )
	{
	  	printf("AES_cfb8_encrypt para key NULL error!\n");
	  	return;
	}
  	memcpy(aes_key_iv.cipher_key, key->rd_key, key->key_bits/8);
	
	if ( ivec == NULL )
	{
	  	printf("AES_cfb8_encrypt para ivec is NULL error!\n");
	  	return;
	}
	memcpy(&aes_key_iv.ivin[0], ivec, AES_INIV_LENGTH);
	
	hi_daes_config(&ctr_word);

	if ( enc == AES_ENCRYPT )
	{
		hi_aes_crypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}
	else
	{
		hi_aes_decrypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 16);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_cfb8_encrypt hi_daes_init error!\n");
	}
}

void AES_cfb1_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc)
{
	int len = length;
	
	if((NULL == in) || (NULL == out) || (NULL == key) || 
							(NULL == ivec) || (NULL == num))
	{
		printf("AES_cfb1_encrypt para null pointer error!\n");
		return;
	}

	if( !((enc == AES_ENCRYPT ) || (enc == AES_DECRYPT )))
	{
		printf("AES_cfb1_encrypt para enc invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("AES_cfb1_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = CFB_WORK_MODE; 	//模式cfb1;
	ctr_word.daes_bit_width = ONE_BIT_WIDTH; 		//位宽8bits;
	ctr_word.daes_alg = AES_ALG;		
	ctr_word.daes_group = SINGLE_ENCRYPT;			//single mode
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;
	
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;		//DMA mode
	#endif

	if( key == NULL )
	{
	  	printf("AES_cfb1_encrypt para key NULL error!\n");
	  	return;
	}
  	memcpy(aes_key_iv.cipher_key, key->rd_key, key->key_bits/8);

	if ( ivec == NULL )
	{
	  	printf("AES_cfb1_encrypt para ivec is NULL error!\n");
	  	return;
	}
	memcpy(&aes_key_iv.ivin[0], ivec, AES_INIV_LENGTH);
	
	hi_daes_config(&ctr_word);

	if ( enc == AES_ENCRYPT )
	{
		hi_aes_crypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}
	else
	{
		hi_aes_decrypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);
	}

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 16);

	if ( hi_daes_exit() != 0 )
	{
		printf("AES_cfb1_encrypt hi_daes_exit error!\n");
	}
}

void AES_ofb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num)
{
	int len = length;
	if((NULL == in) || (NULL == out) || (NULL == key) || 
							(NULL == ivec) || (NULL == num))
	{
		printf("AES_ofb128_encrypt para null pointer error!\n");
		return;
	}

	if( length%16 != 0)
	{
		printf("AES_ofb128_encrypt para length invalid!\n");
		return;
	}
	
	if ( hi_daes_init() != 0 )
	{
		printf("AES_ofb128_encrypt hi_daes_init error!\n");
		return;
	}
	
	ctr_word.daes_work_mod = OFB_WORK_MODE; 	 //模式ofb;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽128bits;
	ctr_word.daes_alg = AES_ALG;		
	ctr_word.daes_group = SINGLE_ENCRYPT;			//single mode
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;
	
	#ifdef _DMA_MODE_
		ctr_word.daes_group = MULTI_ENCRYPT;	//DMA mode
	#endif

	if( key == NULL )
	{
	  	printf("AES_ofb128_encrypt para key NULL error!\n");
	  	return;
	}
  	memcpy(aes_key_iv.cipher_key, key->rd_key, key->key_bits/8);
	
	if ( ivec == NULL )
	{
	  	printf("AES_ofb128_encrypt para ivec is NULL error!\n");
	  	return;
	}
	memcpy(&aes_key_iv.ivin[0], ivec, AES_INIV_LENGTH);
	
	hi_daes_config(&ctr_word);

	hi_aes_crypt((unsigned char *)in, (unsigned char *)out, len, &aes_key_iv);

	/*get the feedback iv*/
	memcpy((unsigned char *)ivec, &feedback_iv[0], 16);
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_ofb128_encrypt hi_daes_exit error!\n");
	}
}

void AES_ctr128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char ivec[AES_BLOCK_SIZE],
	unsigned char ecount_buf[AES_BLOCK_SIZE],
	unsigned int *num)
{
	int i;
	int len = length;
	if((NULL == in) || (NULL == out) || (NULL == key) || 
							(NULL == ivec) || (NULL == num))
	{
		printf("AES_ctr128_encrypt para null pointer error!\n");
		return;
	}

	if ( (len%16 != 0) || (len/16 > AES_BLOCK_SIZE))
	{
		printf("AES_ctr128_encrypt length is not regular!\n");
		return;
	}
	if ( hi_daes_init() != 0 )
	{
		printf("AES_ctr128_encrypt hi_daes_init error!\n");
		return;
	}
	ctr_word.daes_work_mod = CTR_WORK_MODE; 	 //模式ctr;
	ctr_word.daes_bit_width = STANDARD_BIT_WIDTH; //位宽128bits;
	ctr_word.daes_alg = AES_ALG;		
	ctr_word.daes_group = SINGLE_ENCRYPT;			//single mode
	if ( key->key_bits == 128 )
		ctr_word.daes_key_length = AES_KEY_128_BITS;	
	else if ( key->key_bits == 192 )
		ctr_word.daes_key_length = AES_KEY_192_BITS;
	else
		ctr_word.daes_key_length = AES_KEY_256_BITS;

	if( key == NULL )
	{
	  	printf("AES_ctr128_encrypt para key NULL error!\n");
	  	return;
	}
  	memcpy(aes_key_iv.cipher_key, key->rd_key, key->key_bits/8);
	for(i=0; i<len; i+=16)
	{
		memcpy(&aes_key_iv.ivin[0], &ivec[i], AES_INIV_LENGTH);
		
		hi_daes_config(&ctr_word);
		hi_aes_crypt((unsigned char *)in+i, (unsigned char *)out+i, 
						16, &aes_key_iv);
	}
	
	if ( hi_daes_exit() != 0 )
	{
		printf("AES_ctr128_encrypt hi_daes_exit error!\n");
	}
}

