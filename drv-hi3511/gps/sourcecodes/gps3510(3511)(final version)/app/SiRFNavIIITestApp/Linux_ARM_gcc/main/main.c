/***************************************************************************
 *                                                                         *
 *                   SiRF Technology, Inc. GPS Software                    *
 *                                                                         *
 *    Copyright (c) 1996-2009 by SiRF Technology, Inc. All rights reserved.*
 *                                                                         *
 *    This Software is protected by United States copyright laws and       *
 *    international treaties.  You may not reverse engineer, decompile     *
 *    or disassemble this Software.                                        *
 *                                                                         *
 *    WARNING:                                                             *
 *    This Software contains SiRF Technology Inc.s confidential and        *
 *    proprietary information. UNAUTHORIZED COPYING, USE, DISTRIBUTION,    *
 *    PUBLICATION, TRANSFER, SALE, RENTAL OR DISCLOSURE IS PROHIBITED      *
 *    AND MAY RESULT IN SERIOUS LEGAL CONSEQUENCES.  Do not copy this      *
 *    Software without SiRF Technology, Inc.s  express written             *
 *    permission.   Use of any portion of the contents of this Software    *
 *    is subject to and restricted by your signed written agreement with   *
 *    SiRF Technology, Inc.                                                *
 *                                                                         *
 **************************************************************************/
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>


//++the head files blow is added++//
	#include<ctype.h>
	#include<sys/ioctl.h>
	#include<sys/ipc.h>
	#include<sys/sem.h>
	#include <sys/shm.h>
	#include <unistd.h>
	#include<sys/wait.h>
	#include<signal.h>
	#include<setjmp.h>
//===========over==============//

//--//
#include<ComuServExport.h>
//--//

#include "SiRFNav.h"

#include "sirf_ext_aux.h"
#include "sirf_ext_log.h"

#include "sirf_codec_nmea.h"
#include "sirf_demo.h"

#define POWERPULLDOWN    4
#define POWERPULLUP      5 
#define RESETPULLDOWN    0
#define ONOFFPULLDOWN    2 
#define UARTDISABLE_     7
#define UARTENABLE_      6
//--//
#define DATA_LEGAL 0x0f
#define DATA_UNLEGAL 0xf0
//#define GPS_PATH  "pathname"
//==//

//++the macro blow is added++//
	#define SHM_SIZE 4096
	#define PATH   "/usr/for_key"

	#define data_unit_buff 1
	#define length  150
//===========over=========//

//++the new strucure is added++//

	typedef struct{
		char gga_data[ length];//for gga format.
		char rmc_data[ length];//for rmc format.
		char gsa_data[ length];//for gsa format.
		char vtg_data[ length];//for vtg format.
		//
		char gsv_data[512];//for gsv format.
	}data_unit;//the  basic data uint.

	typedef struct {
		//int gga_write_pointer;
		//int rmc_write_pointer;
		//int gsa_write_pointer;
		//int vtg_write_pointer;
		//int mem_set_flag;
		int flag;//this flag is used to show that whether the share_data is initialized or not.
		//int sb1;
		//int gsv_data_pointer;
		//int data_pointer;//a virtual pointer used to point to the basic data unit we just write to.
		data_unit combine_data[data_unit_buff];//data_unit buffer space used to store data of different format. 
		//char gga_data[heigth][length];
		//char rmc_data[heigth][length];
		//char gsa_data[heigth][length];
		//char vtg_data[heigth][length];	
		struct timeval time;
		unsigned n1_time;
		struct timezone time_zone;
	}gps_data_share;//the gps data controller

	typedef union{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
		 struct seminfo  *__buf;
	}sem_un;

//==========over=============//

//#define SIRFNAV_UI_CTRL_MODE_HW_ON_GPIO 0
//#include "sirf_demo.h"

//#include "sirf_pal_hw_open_close.h"


//=============================================================================
//
//-----------------------------------------------------------------------------
//--//
#if 0
static int test_cunt;
#endif
//--//

static tSIRF_CHAR	buf[512];
static tSIRF_CHAR bufdata1[]="GPS_data_start";
static tSIRF_CHAR bufdata2[]="GPS_data_end";	
static sigjmp_buf jmpbuf;						

//++ 11 functions blow is added++//
	int y_sem_create_or_cite(void)
	{

		int flag1,flag2,key;
		int semid;
		int tmperrno;
		struct semid_ds sem_info;////
		//union semun arg; ////			//
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key=ftok(PATH,'a');
		if (key == -1){
			printf("failed to get a key for IPC\n");
			return -1;
		}
		semid=semget(key,1,flag1);//create a semaphore set.
		if(semid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				//printf("the semphore is exist,recreate a semphore please\n");
				//return -1;
			
				semid=semget(key,1,flag2);
				if(semid == -1){
					printf("semget error\n");
					return 0;			
				}
		
				/*
				arg.val=1;
				if(semctl(semid,0,SETVAL,arg)==-1) {
					printf("failed to set the semphore value!\n");
					return 0;			
				}
				*/
			}else{
				printf("semget error\n");
				return -1;
			}
		}
/*
		else //semid>=0; do some initializing 	
		{
			arg.val=1;
			if(semctl(semid,0,SETVAL,arg) == -1){
				printf("failed to set the semphore value!\n");
				return 0;
			}
		}
*/
		return semid;		
	}	

	
	int y_sem_create(void)
	{
	//DESCRIPTION:only create a new semphore set.
	//IN:none.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun nonzero.if not,return -1.

		int flag1,flag2,key;
		int semid;
		int tmperrno;
		struct semid_ds sem_info;////
		//union semun arg; ////			//
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key=ftok(PATH,'a');
		if (key == -1){
			printf("failed to get a key for IPC\n");
			return -1;
		}
		semid=semget(key,1,flag1);//create a semaphore set.
		if(semid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				printf("the semphore is exist,recreate a semphore please\n");
				return -1;
				/*
				semid=semget(key,1,flag2);
				if(semid == -1){
					printf("semget error\n");
					return 0;			
				}
				*/
				/*
				arg.val=1;
				if(semctl(semid,0,SETVAL,arg)==-1) {
					printf("failed to set the semphore value!\n");
					return 0;			
				}
				*/
			}else{
				printf("semget error\n");
				return -1;
			}
		}
		/*
		else //semid>=0; do some initializing 	
		{
			arg.val=1;
			if(semctl(semid,0,SETVAL,arg) == -1){
				printf("failed to set the semphore value!\n");
				return 0;
			}
		}
		*/
		return semid;
	}

	int y_sem_cite(void)
	{
	//DESCRIPTION:only cite  a existed semphore.
	//IN:none.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun nonzero.if not,return -1.

		int flag1,flag2,key;
		int semid;
		int tmperrno;
		struct semid_ds sem_info;////
		//union semun arg; ////			//
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key=ftok(PATH,'a');
		if (key == -1){
			printf("failed to get a key for IPC\n");
			return -1;
		}
		semid=semget(key,1,flag1);//create a semaphore set that only includes one semphore.
		if(semid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				semid=semget(key,1,flag2);
				if(semid == -1){
					printf("semget error:flag2");
					return -1;
				}
				//printf("get the semid successfully from a existed semphore\n");
				return semid;
				/*
				semid=semget(key,1,flag2);
				if(semid == -1){
					printf("semget error\n");
					return 0;			
				}
				*/
				/*
				arg.val=1;
				if(semctl(semid,0,SETVAL,arg)==-1) {
					printf("failed to set the semphore value!\n");
					return 0;			
				}
				*/
			}else{
				printf("cite a sem error:unkown error\n");
				return -1;
			}
		}
		else{	
			//printf("failed to cite a semphore,the semphore you select does not exist\n");
			semctl(semid, 0, IPC_RMID);// delete the semphore created just now
			return -1;
		/*
			arg.val=1;
			if(semctl(semid,0,SETVAL,arg) == -1){
				printf("failed to set the semphore value!\n");
				return 0;
			}
		*/
		
		}
		
		
		//return semid;
	}
/*
	int y_sem_setvalue(int semid)
	{
	//DESCIPTION:this is function is used to set the value of the sem you select from the sem_set.
	//IN:semid.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun nonzero.if not,return zero.

		int i = 5;
		union semun arg;
		arg.val = 1;
		i = semctl(semid, 0, SETVAL,arg);
		if (i == -1){
			printf("failed to set the semphore value!\n");
			return 0;
		}
		return 1;
	}
*/
	int y_sem_setvalue(int semid)
	{
	//DESCIPTION:this is function is used to set the value of the sem you select from the sem_set.
	//IN:semid.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun nonzero.if not,return zero.

		int i = 5;
		//union semun arg;
		sem_un arg;
		arg.val = 1;
		i = semctl(semid, 0, SETVAL,arg);
		if (i == -1){
			printf("failed to set the semphore value!\n");
			return 0;
		}
		return 1;
	}

	int y_sem_get(int semid, short sem_flag)
	{
	//DESCRIPTION:this function is used to apply for a sem.
	//IN:semid defined.
	//IN:sem_flag.
		//if you want to suspend youself during the sem_get(),just set sem_flag zero.
		//if you do not want to suspend youeself during the semget(),just set_flag IPC_NOWAITE.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:	if success,return 1.if not ,return 0.

		struct sembuf askfor_res;
		int i;
		askfor_res.sem_num = 0;
		askfor_res.sem_op = -1;
		askfor_res.sem_flg = SEM_UNDO | sem_flag;
		i = semop(semid,&askfor_res,1);
		if (i == -1){
			printf("failed to get resource!\n");
			return 0;
		}
		return 1;
	}

	int y_sem_free(int semid)
	{
	//DESCRIPTION:this function is used to free the sem.
	//IN:semid
	//OUT:none
	//IN/OUT:none
	//RETURN VALUE:if success,return 1.if not ,return 0.
		int i;
		struct sembuf free_res;
		free_res.sem_num = 0;
		free_res.sem_op = 1;
		free_res.sem_flg = SEM_UNDO;
		i = semop(semid, &free_res, 1);
		if (i == -1){
			printf("failed to free resource!\n");
			return 0;	
		}else{
			//printf("free sem success\n");	
		}	
		return 1;

	}
	int y_sem_destroy(int semid)
	{
	//DESCRIPTION:this function is used to destroy a sem in the kernel.
	//IN:semid
	//OUT:none
	//IN/OUT:none
	//RETURN VALUE:if success,return 1.if not ,return 0.
		if(semctl(semid, 0, IPC_RMID)==-1){
			printf("failed to destroy sem\n");
			return 0;	
		}else{
			//printf("remove sem ok\n");
			return 1;
		}
	}
	
	int y_shm_create_or_cite(void)
	{
		int shmid;
		int tmperrno;
		char *name = PATH;
		key_t key;
		int i;
		int flag1, flag2;
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key = ftok(name, 0);
		if (key == -1){
			printf("failed to get a key from ftok(name, 0)");
			return -1;	
		}
		shmid = shmget(key,SHM_SIZE,flag1);
		if(shmid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				shmid=shmget(key,SHM_SIZE,flag2);
				if(shmid == -1){
					printf("shmget error:flag2");
					return -1;
				}
				//printf("get the shmid successfully from a existed shm\n");
				return shmid;
			}else{
				printf("cite a shm error:unkown error\n");
				return -1;
			}
		}else{	
/*
			printf("failed to cite a shm,the shm you select does not exist\n");
			shmctl(shmid, IPC_RMID, NULL);// delete the shm created just now
			return -1;
*/	
		}
	return shmid;
	
	}

	int y_shm_create(void)
	{
	//DESCRIPTION:only create a new shm
	//IN:none.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun SHMID.if not,return -1.
		int shmid;
		char *name = PATH;
		key_t key;
		int i;
		int flag1,flag2;
		int tmperrno;
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key = ftok(name, 0);
		if (key == -1){
			printf("failed to get a key from ftok(name, 0)\n");
			return -1;	
		}
		shmid = shmget(key,SHM_SIZE,flag1);
		if(shmid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				printf("the shmphore is exist,recreate a shm please\n");
				return -1;
			}else{
				printf("shm creation error:unknown error\n");
				return -1;
			}
		}
		return shmid;
		
	}

	int y_shm_cite(void)
	{
	//DESCRIPTION:only cite a shm
	//IN:none.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun SHMID.if not,return -1.
		int shmid;
		int tmperrno;
		char *name = PATH;
		key_t key;
		int i;
		int flag1, flag2;
		flag1=IPC_CREAT|IPC_EXCL|00666;
		flag2=IPC_CREAT|00666;
		key = ftok(name, 0);
		if (key == -1){
			printf("failed to get a key from ftok(name, 0)");
			return -1;	
		}
		shmid = shmget(key,SHM_SIZE,flag1);
		if(shmid<0)
		{
			tmperrno=errno;
			if(tmperrno==EEXIST){
				shmid=shmget(key,SHM_SIZE,flag2);
				if(shmid == -1){
					printf("shmget error:flag2");
					return -1;
				}
				//printf("get the shmid successfully from a existed shm\n");
				return shmid;
			}else{
				printf("cite a shm error:unkown error\n");
				return -1;
			}
		}else{	
			//printf("failed to cite a shm,the shm you select does not exist\n");
			shmctl(shmid, IPC_RMID, NULL);// delete the shm created just now
			return -1;
		
		}

	}

	void * y_shm_get(int shmid)
	{
	//DESCRIPTION:this  function is used to get a void pointer to the address mapped to the share_memory space in the kernel.
	//IN:shmid.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun the address mapped to the share_memory space in the kernel.if not,return zero.
		void *p;
		p =(void *)shmat(shmid, NULL, 0);
		//printf("get_shm shmid is %d\n",shmid);
		//p = (people*)shmat(shmid,NULL,0);
		//printf("p is :%d\n",p);
		if (p == (void *)-1){
			printf("failed to map the address to the process memory space\n");
			return NULL;
		}
		return p;
	}

	int y_shm_disconnect(void *addr)
	{
	//DESCRIPTION:this  function is used to disconnect the memory space in user space to the share_memory in the kernel. 
	//IN:addr.
	//OUT:none.
	//IN/OUT:none.
	//RETURN VALUE:if success retrun 1.if not,return zero.
		int i = 5;
		i = shmdt(addr);
		if (i == -1){
			printf("failed to disconect the process memory space to share_memory space in the kernel\n");
			return 0;
		}
		return 1;
	}

	int y_shm_destroy(int shmid)
	{
	//DECIPTION:this function is used to destroy the share_memory.
	//IN:semid
	//OUT:none
	//IN/OUT:none
	//RETURN VALUE:if success,return 1.if not ,return 0.

		int i = 5;
		i = shmctl(shmid, IPC_RMID, NULL);
		if (i == -1){
			printf("failed to destroy the share_memory\n");
			return 0;
		}
		return 1;
	}
	
	static void sig_usr1_handler(int signo)
	{
		printf("enter the sig_usr1 handler\n");
		siglongjmp(jmpbuf,1);
	}

//===========over============//

tSIRF_VOID SiRFNav_Output( tSIRF_UINT32 message_id, tSIRF_VOID *message_structure, tSIRF_UINT32 message_length )
{
//++the new variable definition blow is added++//
		int semid;
		int shmid;
	    gps_data_share *share_pointer;
	//printf("HELLO WORLD.\n");
//======================================// 


   // This is the main callback function for GPS output. Typical user code will do something like this:
#if 0   
   FILE * fp;
   fp=fopen("/sirfNav_Output_Data","a+");
   if(fp==NULL)
   {
       printf( "open file SiRFNav_Output_Data erro\n");
       return;
   }
   if(fp)
   {
       fwrite((char *)message_structure,1 , message_length*sizeof(tSIRF_UINT32) ,fp);
       //printf( "sirfNav_Output_Data write .....\n");	
       fclose(fp);
   }   
#endif  
	semid = y_sem_cite();
	shmid = y_shm_cite();
   switch ( message_id )
   {
      case SIRF_MSG_SSB_GEODETIC_NAVIGATION:
      {
         
         //SIRF_CODEC_NMEA_Export(message_id,message_structure,message_length,(tSIRF_UINT8*)&(g_packet[0]), &g_packet_length);
         //if(g_packet_length<=511){
		//memcpy(g_pGpsHead->MidBuf,g_packet,g_packet_length);
					//printf( "GPS NMEA:%s \n",g_packet);					
			//}

//++//
			if (y_sem_get(semid, IPC_NOWAIT) == 0){
				printf("+++++++have not got the sem++++++++++\n");		
			}else{
				//shmid = init_shm();
				//printf("--------shmid iud d d is %d\n",shmid);
				//p_map = (people *) get_shm(shmid);
				share_pointer = (gps_data_share *) y_shm_get(shmid);////


				 tSIRF_MSG_SSB_GEODETIC_NAVIGATION *msg = (tSIRF_MSG_SSB_GEODETIC_NAVIGATION*)message_structure;
				 SIRF_CODEC_NMEA_Encode_GGA(msg, buf);
			    	 strcpy((share_pointer->combine_data[0]).gga_data,buf);
				//printf((share_pointer->combine_data[0]).gga_data);     
				//printf(buf);

				SIRF_CODEC_NMEA_Encode_RMC(msg, buf);
				strcpy((share_pointer->combine_data[0]).rmc_data,buf);
				//printf((share_pointer->combine_data[0]).rmc_data);
				// printf(buf);

				SIRF_CODEC_NMEA_Encode_GSA(msg, buf);
				strcpy((share_pointer->combine_data[0]).gsa_data,buf);
				//printf((share_pointer->combine_data[0]).gsa_data);
				//printf(buf);

				SIRF_CODEC_NMEA_Encode_VTG(msg, buf);
				strcpy((share_pointer->combine_data[0]).vtg_data,buf);
				//printf((share_pointer->combine_data[0]).vtg_data);
				//printf(buf);
//--
				share_pointer->n1_time = GetTickCount();
				//printf("the time stab is:%d\n",share_pointer->n1_time);
//--
				gettimeofday(&(share_pointer->time),&(share_pointer->time_zone));
				//printf("the related time is %ld s,%ld us\n",(share_pointer->time).tv_sec,(share_pointer->time).tv_usec);

				share_pointer->flag = DATA_LEGAL;//this show that the gps data in the share memory is valid
				//printf("share_pointer->flag is :%d\n",share_pointer->flag);
				//printf("share_pointer->data_pointer is %d\n",share_pointer->data_pointer);

				y_shm_disconnect((void *) share_pointer);
				//y_shm_destroy(shmid);
				//sleep(2);
				y_sem_free(semid);

#if 0      //just for debug
				if (test_cunt++  ==  10){
					printf("child process is over");
					abort();
					//raise(SIGKILL);			
				}
#endif
			}
              
#if 0         
         if ( msg->nav_mode & SIRF_MSG_SSB_MODE_MASK )
            printf( "%u-%02u-%02u %u:%02u:%.3f  Lat=%.7f, Lon=%.7f, Alt(MSL)=%.1f\n",
                    msg->utc_year,
                    msg->utc_month,
                    msg->utc_day,
                    msg->utc_hour,
                    msg->utc_min,
                    msg->utc_sec / 1e3,    // Note the value scaling
                    msg->lat     / 1e7,
                    msg->lon     / 1e7,
                    msg->alt_msl / 1e2 );
         else
            printf( "%u-%02u-%02u %u:%02u:%.3f  Fix not available\n",
                    msg->utc_year,
                    msg->utc_month,
                    msg->utc_day,
                    msg->utc_hour,
                    msg->utc_min,
                    msg->utc_sec / 1e3 );
#endif                    
      }
      break;

      case SIRF_MSG_SSB_MEASURED_TRACKER:
      {

			if ((y_sem_get(semid,IPC_NOWAIT)) == 0){
				printf("have not get sem from kennel\n");			
			}else{
				//shmid = init_shm();
				//printf("shmid iud d d is %d\n",shmid);
				share_pointer = (gps_data_share *) y_shm_get(shmid);////
	
				/*
				if (share_pointer->flag != DATA_LEGAL){
							memset(share_pointer->combine_data,0,sizeof(share_pointer->combine_data));	
							//share_pointer->flag = 1;
				}
				*/

				tSIRF_MSG_SSB_MEASURED_TRACKER *msg = (tSIRF_MSG_SSB_MEASURED_TRACKER*)message_structure;
				SIRF_CODEC_NMEA_Encode_GSV(msg, buf);
				strcpy((share_pointer->combine_data[0]).gsv_data,buf);
				//printf((share_pointer->combine_data[0]).gsv_data);

//--
				share_pointer->n1_time = GetTickCount();
				//printf("the time stab is:%d\n",share_pointer->n1_time);
//--

				gettimeofday(&(share_pointer->time),&(share_pointer->time_zone));
				//printf("the related time is %ld s,%ld us\n",(share_pointer->time).tv_sec,(share_pointer->time).tv_usec);


				share_pointer->flag = DATA_LEGAL;
				//printf("share_pointer->flag is :%d\n",share_pointer->flag);
				//printf((share_pointer->combine_data[share_pointer->gsv_data_pointer]).gsv_data);
				//printf("gsv_data is %d",strlen(share_pointer->combine_data[share_pointer->gsv_data_pointer]).gsv_data));

				/*
				for(k = 0,t = 0;k < 512; k++){
					if(buf[k] == '\0'){
						break;					
					}			
					t++;
				}
				printf("the raw data in buf is %d\n",t);
				for(k = 0,t = 0;k < 512; k++){
					if((share_pointer->combine_data[share_pointer->gsv_data_pointer]).gsv_data[k] == '\0'){
						break;
					}
					t++;
				}
				*/
				
				//printf("rexeive_gsv_data is %d\n",t);				
				//printf(buf);
				//printf("share_pointer->gsv_data_pointer is %d\n",share_pointer->gsv_data_pointer);

				y_shm_disconnect((void *) share_pointer);
					//y_shm_destroy(shmid);
					//sleep(2);
				y_sem_free(semid);
				 //printf( "GPS  MEASURED_TRACKER\n");
			 	// Use tracker information here
			}	
      }
      break;
      default:
               //printf( "GPS default\n");
               break;

   } // switch()
   

   // SIRFNAV_DEMO_Send() forwards every message to AUX serial port, local log file,
   // TCP/IP port and other components
	//printf("out_put funciotn is over\n");
   	SIRFNAV_DEMO_Send( message_id, message_structure, message_length );

} // SiRFNav_Output()


//=============================================================================
//
//-----------------------------------------------------------------------------
int main ( int argc, char* argv[] )
{
   tSIRF_RESULT result;
   tSIRF_UINT8 input_flags = 0;
   int i, n, ch;

   short aux_port_enabled    = 0;             /* 0: disabled, 1: enabled */
   short tcpip_port_enabled  = 0;
   short logging_enabled     = 0;

   unsigned long tracker_port       = 1;             /* default tracker port */
   //unsigned long reset_port         = 2;             /* default reset port */
   unsigned long reset_port         = 1;             /* default reset port */
   unsigned long aux_port           = 3;             /* default auxiliary port */

   unsigned long tracker_baud       = 115200;        /* default tracker port baud rate*/
   unsigned long aux_baud           = 115200;        /* default auxiliary port baud rate*/
   unsigned short tcpip_port        = 7555;          /* default TCP/IP port */

   char          log_filename[256]  = "SiRFNavIIITestApp";  /* default log file name */

   short log_type         = SIRF_EXT_LOG_SIRF_ASCII_TEXT;
   int   force_cold_start = 0;

   char *token;
//++//those variables are used to create a semphore and initialize it
	int semid;
	int shmid;	
	gps_data_share *share_pointer;
//==//
   
   int fd=-1;
   int result1=-1; 
   sigset_t  newmask;
//++// set a signal handler function for the signal sigusr1
	sigprocmask(0,NULL,&newmask);
	sigdelset(&newmask,SIGUSR1);
	sigprocmask(SIG_SETMASK,&newmask,NULL);
	sigprocmask(0,NULL,&newmask);
	if (sigismember(&newmask,SIGUSR1) != 1){
		printf("SIGUSR1 is not in the newmask set\n");	
	}

	if (signal(SIGUSR1,sig_usr1_handler) ==SIG_ERR){
		printf("failed to set the SIG_CHLD handler function\n");
		return 0;
	}
 
//==//
//++//  create a file in order to make prepariton for ftok(PATH,--)
	int fdd;
	fdd = creat(PATH,0666);
	if (fdd == -1){	
		printf("creat file failed\n");
		return 0;
	}else{
		//printf("creat file named ---/usr/for_key success\n");	
	}
//==//

//++//
	if((semid = y_sem_cite())  != -1){
		y_sem_destroy(semid);	
	}
	if((shmid = y_shm_cite()) != -1){
		y_shm_destroy(shmid);	
	}
//==//

//++//create a semphore and initialize it 
	semid = y_sem_create();
	if (semid == -1){
		printf("y_sem_create()fialed:in customer main\n");
		return 0;
	}
	if((y_sem_setvalue(semid)) == 0){
		printf("y_sem_create() failed:in customer main\n");
		return 0;
	}
//printf("hello world\n");
//==//
//++//create a share memory and initialze it 
	shmid = y_shm_create();
	if (shmid == -1){
		printf("y_shm_create() failed:in customer main\n");
		return 0;
	}
//printf("hello world\n");
	if ((share_pointer =(gps_data_share *)(y_shm_get(shmid))) == NULL){
		printf("y_shm_get()failed:in customer main \n");
		return 0;
	}else{
		//printf("hello world\n");
		//printf("the address is :%d\n",share_pointer);
		memset(share_pointer,0,sizeof(gps_data_share));
		share_pointer->flag = DATA_UNLEGAL;
		//printf("hello world--\n");
		//return 0;
	}
//	printf("hello world\n");
//==//

/* gps power on */
   fd = open("/dev/misc/misc_gpio", O_RDWR);
   if(-1 == fd) printf("open misc_gpio error!\n");
   if(fd)
   {
       result1=ioctl(fd, POWERPULLUP, 0);
       if(result1 != 0)
       {
           printf("POWERIO ioctl err \n");	
       }	
       close(fd); 
       printf("GPS POWER ON");
    }
   // Process command-line options ----------------------------------
   for( i=1; i<argc; i++ )
   {
      if ( argv[i][0] == '-' )
         switch( toupper(argv[i][1]) )
         {
            case 'T': /* Tracker serial port */
               token = strchr( argv[i]+2, ':' );
               if(token)
               {
                  n = strtoul( argv[i]+2, &token, 10 );
                  tracker_baud = strtoul( token+1, NULL, 10 );
               }
               else
                  n = strtoul( argv[i]+2, NULL, 10 );

               if ( n )
                  tracker_port = n;
               break;

            case 'X': /* Tracker reset port */
               n = strtoul( argv[i]+2, NULL, 10 );
               if ( n )
                 reset_port = n;
               break;
#if 0
            case 'A': /* AUX serial port */
               input_flags |= SIRF_AUX_ENABLED;
               token = strchr( argv[i]+2, ':' );
               if(token)
               {
                  n = strtoul( argv[i]+2, &token, 10 );
                  aux_baud = strtoul( token+1, NULL, 10 );
               }
               else
                  n = strtoul( argv[i]+2, NULL, 10 );

               if ( n )
                  aux_port = n;
               break;

            case 'N': /* TCP/IP server port */
               input_flags |= SIRF_TCPIP_ENABLED;
               n = strtoul( argv[i]+2, NULL, 10 );
               if ( n )
                 tcpip_port = n;
               break;
#endif 
            case 'F': /* Log type: SiRF ASCII text */
               input_flags |= SIRF_LOG_ENABLED;
               log_type        = SIRF_EXT_LOG_SIRF_ASCII_TEXT;
               if ( argv[i][2] )
                  strncpy ( log_filename, argv[i]+2, sizeof(log_filename) );
               if ( !strchr(log_filename, '.') )
                  strncat(log_filename, ".gps", ( sizeof(log_filename) - strlen(log_filename) - 1 ) );
               break;
#if 0
            case 'B': /* Log type: SiRF binary stream */
               input_flags |= SIRF_LOG_ENABLED;
               log_type = SIRF_EXT_LOG_SIRF_BINARY_STREAM;
               if ( argv[i][2] )
                  strncpy( log_filename, argv[i]+2, sizeof(log_filename) );
               if ( !strchr( log_filename, '.' ) )
                  strncat( log_filename, ".sb", ( sizeof(log_filename) - strlen(log_filename) - 1 ) );
               break;
#endif
            case 'C': /* Force GPS cold start */
               force_cold_start = SIRFNAV_UI_CTRL_MODE_COLD;
               break;

            case 'H': /* Display command-line usage */
            default:
               printf( "Usage: %s [-ttracker_port[:baudrate]] [-a[aux_port][:baudrate]] [-x[reset_port]] [-n[tcpip_port]] [-f|-b[log_file]] [-c]\n", argv[0] );
               exit(0);
               break;
         } // switch()
   } // for()
   //printf( "test---\n");

   // Start Test Application ----------------------------------------
//   printf( "SiRFNavIII Test App v%s started\n", SIRFNAV_DEMO_VERSION );
//   printf( "GSD3tw tracker port: COM%lu @ %lu bps\n", tracker_port, tracker_baud );
//   printf( "GSD3tw tracker reset port: COM%lu\n", reset_port );

   result = SIRFNAV_DEMO_Create( aux_port, 115200, tcpip_port,
                                 log_filename, input_flags );
   if ( result != SIRF_SUCCESS )
   {
      printf( "ERROR: SIRFNAV_DEMO_Create() return value: 0x%lX. Program terminated.\n", result );
      exit(-1);
   }

//   if ( input_flags & SIRF_AUX_ENABLED )
//   {
//      printf( "AUX port: COM%lu @ %lu bps\n", aux_port, aux_baud );
//      result = SIRF_EXT_AUX_Open( 0, aux_port, aux_baud );
//      if ( result != SIRF_SUCCESS )
//         printf( "ERROR: SIRF_EXT_AUX_Open() return value: 0x%lX.\n", result );
//   }
#if 1
   if ( input_flags & SIRF_LOG_ENABLED )
   {
      printf( "Log file............: %s\n", log_filename );
   }
#endif
   // Initialize 3tw reset port -------------------------------------
   //result = SIRF_PAL_HW_TrackerResetPortOpen( reset_port );
   //if ( result != SIRF_SUCCESS )
   //{
   //   printf( "ERROR: SIRF_PAL_HW_TrackerResetPortOpen() return value: 0x%lX. Program terminated.\n", result );
   //   exit(1);
   //}


   // Start GPS -----------------------------------------------------
   result = SiRFNav_Start(   SIRFNAV_UI_CTRL_MODE_STORAGE_ON_STOP
                           | SIRFNAV_UI_CTRL_MODE_USE_TRACKER_RTC
                           | SIRFNAV_UI_CTRL_MODE_TEXT_ENABLE         //
                           | SIRFNAV_UI_CTRL_MODE_RAW_MSG_ENABLE      //
                           | SIRFNAV_UI_CTRL_MODE_HW_FLOW_CTRL_OFF
                           | SIRFNAV_UI_CTRL_MODE_AUTO
                           | SIRFNAV_UI_CTRL_CTRL_TCXO_DELAY_NONE
                           | SIRFNAV_UI_CTRL_MODE_HW_ON_GPIO,
                           //| force_cold_start,                      //
                           0,
                           tracker_port,
                           tracker_baud );

   if ( result != SIRF_SUCCESS )
   {
      printf( "ERROR: SiRFNav_Start() return value: 0x%lX. Program terminated.\n", result );
      exit(-1);
   }

#if 0
    printf( "SiRFNavIII started. Enter Q to stop the application.\n\n");
   do
   {
      ch = toupper( getchar() );
   }
   while ( ch != 'Q' );


   // Stop GPS ------------------------------------------------------
   result = SiRFNav_Stop( SIRFNAV_UI_CTRL_STOP_MODE_TERMINATE );

   if (  result != SIRF_SUCCESS )
      printf( "ERROR: SiRFNav_Stop() return value: 0x%lX.\n", result );
#endif

   // Stop GPS ------------------------------------------------------

#if 1
/*
	while(1){
		if(sigsetjmp(jmpbuf,1) != 0 ){
			printf("stop the gps process\n");
		        result = SiRFNav_Stop( SIRFNAV_UI_CTRL_STOP_MODE_TERMINATE );	
	  		if (  result != SIRF_SUCCESS ){
      				printf("ERROR: SiRFNav_Stop() return value: 0x%lX.\n", result );
				printf("there may be some bugs,please contact sirf to learn when (  tSIRF_UINT32 stop_mode ) would fail.\n")
			}
			break;		
		}

	}
*/
	while(1){
		if(sigsetjmp(jmpbuf,1) == 0 ){
			printf("suceed in calling sigsetjmp(jmpbuf,1)\n");
			pause();
		}else{
			printf("stopping the gps process...\n");	
			result = SiRFNav_Stop( SIRFNAV_UI_CTRL_STOP_MODE_TERMINATE );	
  			if (  result != SIRF_SUCCESS ){
      				printf("ERROR: SiRFNav_Stop() return value: 0x%lX.\n", result );
				printf("there may be some bugs,please contact sirf to learn when (  tSIRF_UINT32 stop_mode ) would fail\n");
			}
			break;
		}
	}
#endif

   // Close 3tw reset port ------------------------------------------
   //result = SIRF_PAL_HW_TrackerResetPortClose();
   //if ( result != SIRF_SUCCESS )
   //{
   //   printf( "SIRF_PAL_HW_TrackerResetPortClose() error: 0x%lX\n", result );
  // }


   // Stop Test Application -----------------------------------------
   //SIRFNAV_DEMO_Delete();

		y_sem_destroy(semid);
		y_shm_destroy(shmid);

   printf( "SiRFNavIII Test App finished.\n" );
/*reset pull down */   
   fd = open("/dev/misc/misc_gpio", O_RDWR);
   if(-1 == fd) printf("open misc_gpio error!\n");
   if(fd)
   {
       result1=ioctl(fd, RESETPULLDOWN, 0); 
       if(result1 != 0)
       {
           printf("reset ioctl err \n");	
       }	
       close(fd); 
       printf("reset pull down\n");
    } 

/*onoff  pull down */   
   fd = open("/dev/misc/misc_gpio", O_RDWR);
   if(-1 == fd) printf("open misc_gpio error!\n");
   if(fd)
   {
       result1=ioctl(fd, ONOFFPULLDOWN, 0); 
       if(result1 != 0)
       {
           printf("onoff ioctl err \n");	
       }	
       close(fd); 
       printf("on_off pull down\n");
    } 

/*close GPS power */   
   fd = open("/dev/misc/misc_gpio", O_RDWR);
   if(-1 == fd) printf("open misc_gpio error!\n");
   if(fd)
   {
       result1=ioctl(fd, POWERPULLDOWN, 0);
       if(result1 != 0)
       {
           printf("POWERIO ioctl err \n");	
       }	
       close(fd); 
       printf("GPS POWER OFF\n");
    }
   

   return 0;

} // main()

