#ifndef _HI_DES_H_
#define _HI_DES_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef unsigned char DES_cblock[8];
typedef /* const */ unsigned char const_DES_cblock[8];
/* With "const", gcc 2.8.1 on Solaris thinks that DES_cblock *
 * and const_DES_cblock * are incompatible pointer types. */

#ifndef DES_LONG
#define DES_LONG unsigned long
#endif

typedef struct DES_ks
{
	union
	{
	DES_cblock cblock;
	/* make sure things are correct size on machines with
	 * 8 byte longs */
	DES_LONG deslong[2];
	} ks[16];
} DES_key_schedule;

#define DES_KEY_SZ 	(sizeof(DES_cblock))
#define DES_SCHEDULE_SZ (sizeof(DES_key_schedule))

#define DES_ENCRYPT	1
#define DES_DECRYPT	0

#define DES_CBC_MODE	0
#define DES_PCBC_MODE	1

#define DES_ecb2_encrypt(i,o,k1,k2,e) \
	DES_ecb3_encrypt((i),(o),(k1),(k2),(k1),(e))

#define DES_ede2_cbc_encrypt(i,o,l,k1,k2,iv,e) \
	DES_ede3_cbc_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(e))

#define DES_ede2_cfb64_encrypt(i,o,l,k1,k2,iv,n,e) \
	DES_ede3_cfb64_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(n),(e))

#define DES_ede2_ofb64_encrypt(i,o,l,k1,k2,iv,n) \
	DES_ede3_ofb64_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(n))

#define DES_ncbc_encrypt(i,o,l,k1,iv,n)	\
		DES_cbc_encrypt((i),(o),(l),(k1),(iv),(n))
		
/*below are des interface*/
void DES_ecb_encrypt(const_DES_cblock *input,DES_cblock *output,
		     DES_key_schedule *ks,int enc);

/* DES_cbc_encrypt does not update the IV!  Use DES_ncbc_encrypt instead. */
void DES_cbc_encrypt(const unsigned char *input,unsigned char *output,
		     long length,DES_key_schedule *schedule,DES_cblock *ivec,
		     int enc);

/*can't understand this interface's role, not implement*/
void DES_pcbc_encrypt(const unsigned char *input,unsigned char *output,
		      long length,DES_key_schedule *schedule,DES_cblock *ivec,
		      int enc);

/*para: numbits is encrypt feedback's bits length ,such as 8 ,1 bits in hisilicon's cipher implemention;
	     lentgh   is the multiply of numbits len	*/
void DES_cfb_encrypt(const unsigned char *in,unsigned char *out,int numbits,
		     long length,DES_key_schedule *schedule,DES_cblock *ivec,
		     int enc);

void DES_cfb64_encrypt(const unsigned char *in,unsigned char *out,long length,
		       DES_key_schedule *schedule,DES_cblock *ivec,int *num,
		       int enc);

/*para: numbits is encrypt feedback's bits length ,such as 8 ,1 bits in hisilicon's cipher implemention;
	     lentgh   is the multiply of numbits len	*/
void DES_ofb_encrypt(const unsigned char *in,unsigned char *out,int numbits,
		     long length,DES_key_schedule *schedule,DES_cblock *ivec);
void DES_ofb64_encrypt(const unsigned char *in,unsigned char *out,long length,
		       DES_key_schedule *schedule,DES_cblock *ivec,int *num);

/*below are 3des interface*/
void DES_ecb3_encrypt(const_DES_cblock *input, DES_cblock *output,
		      DES_key_schedule *ks1,DES_key_schedule *ks2,
		      DES_key_schedule *ks3, int enc);

void DES_ede3_cbc_encrypt(const unsigned char *input,unsigned char *output, 
			  long length,
			  DES_key_schedule *ks1,DES_key_schedule *ks2,
			  DES_key_schedule *ks3,DES_cblock *ivec,int enc);

//not implement
void DES_ede3_cbcm_encrypt(const unsigned char *in,unsigned char *out,
			   long length,
			   DES_key_schedule *ks1,DES_key_schedule *ks2,
			   DES_key_schedule *ks3,
			   DES_cblock *ivec1,DES_cblock *ivec2,
			   int enc);

void DES_ede3_cfb64_encrypt(const unsigned char *in,unsigned char *out,
			    long length,DES_key_schedule *ks1,
			    DES_key_schedule *ks2,DES_key_schedule *ks3,
			    DES_cblock *ivec,int *num,int enc);
void DES_ede3_cfb_encrypt(const unsigned char *in,unsigned char *out,
			  int numbits,long length,DES_key_schedule *ks1,
			  DES_key_schedule *ks2,DES_key_schedule *ks3,
			  DES_cblock *ivec,int enc);
void DES_ede3_ofb64_encrypt(const unsigned char *in,unsigned char *out,
			    long length,DES_key_schedule *ks1,
			    DES_key_schedule *ks2,DES_key_schedule *ks3,
			    DES_cblock *ivec,int *num);

int DES_set_key_checked(const_DES_cblock *key,DES_key_schedule *schedule);
void DES_set_key_unchecked(const_DES_cblock *key,DES_key_schedule *schedule);

#ifndef OPENSSL_DES_LIBDES_COMPATIBILITY
	#define des_cblock DES_cblock
	#define const_des_cblock const_DES_cblock
	#define des_key_schedule DES_key_schedule
	#define des_ecb2_encrypt(i,o,k1,k2,e) \
		des_ecb3_encrypt((i),(o),(k1),(k2),(k1),(e))
	#define des_ecb3_encrypt(i,o,k1,k2,k3,e)\
		DES_ecb3_encrypt((i),(o),&(k1),&(k2),&(k3),(e))
	#define des_ede3_cbc_encrypt(i,o,l,k1,k2,k3,iv,e)\
		DES_ede3_cbc_encrypt((i),(o),(l),&(k1),&(k2),&(k3),(iv),(e))
	#define des_ede3_cbcm_encrypt(i,o,l,k1,k2,k3,iv1,iv2,e)\
		DES_ede3_cbcm_encrypt((i),(o),(l),&(k1),&(k2),&(k3),(iv1),(iv2),(e))
	#define des_ede3_cfb64_encrypt(i,o,l,k1,k2,k3,iv,n,e)\
		DES_ede3_cfb64_encrypt((i),(o),(l),&(k1),&(k2),&(k3),(iv),(n),(e))
	#define des_ede3_ofb64_encrypt(i,o,l,k1,k2,k3,iv,n)\
		DES_ede3_ofb64_encrypt((i),(o),(l),&(k1),&(k2),&(k3),(iv),(n))
	#define des_cbc_encrypt(i,o,l,k,iv,e)\
		DES_cbc_encrypt((i),(o),(l),&(k),(iv),(e))
	#define des_ncbc_encrypt(i,o,l,k,iv,e)\
		DES_ncbc_encrypt((i),(o),(l),&(k),(iv),(e))
	#define des_cfb_encrypt(i,o,n,l,k,iv,e)\
		DES_cfb_encrypt((i),(o),(n),(l),&(k),(iv),(e))
	#define des_ecb_encrypt(i,o,k,e)\
		DES_ecb_encrypt((i),(o),&(k),(e))
#endif

#ifdef  __cplusplus
}
#endif

#endif

