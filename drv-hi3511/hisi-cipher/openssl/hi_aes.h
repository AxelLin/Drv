
#ifndef HEADER_AES_H
#define HEADER_AES_H

//#include <openssl/opensslconf.h>

#ifdef OPENSSL_NO_AES
#error AES is disabled.
#endif

#define AES_ENCRYPT	1
#define AES_DECRYPT	0

/* Because array size can't be a const in C, the following two are macros.
   Both sizes are in bytes. */
#define AES_MAXNR 14
#define AES_BLOCK_SIZE 16

#define AES_set_decrypt_key(userKey, bits, key)		\
				AES_set_encrypt_key((userKey), (bits), (key))
				
#ifdef  __cplusplus
extern "C" {
#endif

/* This should be a hidden type, but EVP requires that the size be known */
struct aes_key_st {
	unsigned char rd_key[32];
	unsigned int key_bits;
};
typedef struct aes_key_st AES_KEY;

//const char *AES_options(void);


				
int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
	AES_KEY *key);

//int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
//	AES_KEY *key);
/*
void AES_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key);
*/
void AES_ecb_encrypt(const unsigned char *in, unsigned char *out,
	const AES_KEY *key, const int enc);
void AES_cbc_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, const int enc);
void AES_cfb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc);
void AES_cfb1_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc);
void AES_cfb8_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc);
void AES_ofb128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *ivec, int *num);
void AES_ctr128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char ivec[AES_BLOCK_SIZE],
	unsigned char ecount_buf[AES_BLOCK_SIZE],
	unsigned int *num);

/* For IGE, see also http://www.links.org/files/openssl-ige.pdf */
/* NB: the IV is _two_ blocks long */
//void AES_ige_encrypt(const unsigned char *in, unsigned char *out,
//		     const unsigned long length, const AES_KEY *key,
//		     unsigned char *ivec, const int enc);
/* NB: the IV is _four_ blocks long */
//void AES_bi_ige_encrypt(const unsigned char *in, unsigned char *out,
//			const unsigned long length, const AES_KEY *key,
//			const AES_KEY *key2, const unsigned char *ivec,
//			const int enc);


#ifdef  __cplusplus
}
#endif

#endif /* !HEADER_AES_H */


