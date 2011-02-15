#ifndef COMUSERVEXPORT_H_FGJIYE382ASD
#define COMUSERVEXPORT_H_FGJIYE382ASD

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stddef.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "typedef.h"

#ifdef XUN_DEBUG
#define XUN_ASSERT_VALID( boolval, strval ) { if(!(boolval)) assert(false); }
#else
#define XUN_ASSERT_VALID( boolval, strval ) {}
#endif

#define MSG_NOR		1
#define MSG_DBG		1
#define MSG_ERR		1

#define MSG_HEAD	("")

#define PRTMSG(enb, format, ...) \
if( enb ) \
{ \
	printf( "%s: ", MSG_HEAD ); \
	printf( format, ##__VA_ARGS__ ); \
}

// ���汾��,�ڹ鵵���������ͻ�ʹ�õ������Ҫע��汾�ŵ�����
#define CHK_VER			0	// 0,�����汾; 1,�������汾
#define USE_PROTOCOL	0	// 0,ǧ����ͨ�ð�Э��; 1,ǧ��������������Э��; 2,ǧ������һ������Э��(����˾������); 30,����ͨ�ð�Э��
#define USE_LANGUAGE	0	// 0,��������; 11,Ӣ��
#define RESET_ENABLE_ALL  1  //����Ӧ�ó����յ�NK�ĸ�λ��Ϣ�Ƿ���ɱ�� 0 ����ɱ; 1 ��ɱ (��ʽ�汾Ҫ����ɱ)

#define CFGSPEC	("YXYF3_��Ʒ����DVRǰ;����_2009") // �ֽ���ӦС�������еĴ洢����,��������
#if CHK_VER == 0
	#define VER_SFT		"app-yx001-160vw01-001"
#else
	#define VER_SFT		"app-t001-160vw01-001"
#endif


///////// { <1>ͨ�ñ��뿪�� (��������,��Ӧ������Ҫ���������)

// { ��������
#define USE_TELLIMIT	0		// 0,ǧ�������ĺ����������; 1,ǧ�������ĵ绰������; 2,�������ĵ绰������; 3,��һ������绰������
#define USE_TTS			1		// 1,TTS����֧��; 0,��֧��
#define USE_METER		0		// 1,֧��˰��,�Ƽ����복̨֮���ô���ͨ��,�������س�����; 0,��֧��˰��
#define JIJIA_HIGHHEAVY			// �򿪴˿���,��ʾ�ڵ���DeviceIoControl��ѯ�������ź�ʱ,���õ���ֵ(������ת������ź�)Ϊ1����ʾ�س�(����ʱ�Ƽ���ֱ��������ź��ǵ͵�ƽ)
#define LOWPOWAD		240		// ǷѹAD�ٽ�ֵ: 12v��,240; 24v��,�ݶ�480 (��ͬ����ע���޸�!!)
#define CP_CHS			936
#define USE_SOCKBINDIP	0		// 0,socket����(��ʽ�汾ʹ��); 1,��GPRS�����IP(�ڵ���ʱʹ��)

#define GPS_TEST			0	// 1, ��GPS���Թ���1; 2,��GPS���Թ���2; 0,�ر�
#define USE_UBUNTU_DEBUG	0	// 1, ֧��ʹ��Ubentu���е��ԣ�0����֧��
#define GPSLOG_PATH		L"\\Resflh4"
#define GPSLOG_FILENAME L"GpsLog.txt"

// ע�⣺������ȫһ��
#define DIR2NAME_C		("\\Resflh2\\")		// (char��)
#define DIR2NAME_W		(L"\\Resflh2\\")	// (wchar_t��)
#define DIR2NAME_W_2	(L"\\Resflh2")		// (wchar_t��)
#define DIR4NAME_W		(L"\\Resflh4")		// (wchar_t��)
// } ��������

// QianExeʹ��
#define UDP_ONLY		0		// 1,ֻʹ��UDPͨ��; 0,��ʹ��UDP,��ʹ��TCP
#define USE_BLK			0		// 1,֧�ֺ�ϻ��; 0,��֧��
#define USE_AUTORPT		0		// 1,֧�������ϱ�(0154); 0,��֧��
#define USE_DRVREC		0		// 1,ʹ��JG2�ɵ���ʻ��¼��ģ��; 0,��ʹ��
//�� ��USE_DRVRECΪ1�����⼸����Ҫ����
#define USE_DRVRECRPT	0		// 1,3745��ʻ��¼�ϴ�; 2,3746; 3,3747; 4,3748; 11,3745_2��; 0,��֧��	(1��4�ݲ�֧��)
#define USE_ACDENT		0		// 1,�¹��ɵ�,ǧ��������Э��; 2,2������Э��; 0,��֧��
#define USE_DRIVERCTRL	0		// 1,ʹ��˾������ҵ��; 0,��ʹ��
// ����USE_DRVRECΪ1�����⼸����Ҫ����
#define USE_OIL			0		// 1,�ͺ�����ʹ��IOֱ�Ӷ�ȡ; 2,ʹ���ͺļ���,��ʹ����һ�㷨; 0,��֧��
#define USE_REALNAV		0		// 1,����ʵʱָ��(Ŀǰ����2��ϵͳ���õ�); 0,������
#define USE_JIJIA		0		// 1,֧�ּƼ���ģ��(�������ڴ�����IO����,����˰�ع����޹�); 0,��֧��
#define USE_REALPOS		0		// 1,֧��ʵʱ��λģ��; 0,��֧��
#define USE_DISCONNOILELEC 0	// 1,�������Ͷϵ�; 0,������
#define USE_LEDTYPE		0		// 1,ʹ�ò���LED��; 2,ʹ���ൺ����LED; 0,��ʹ��LED
#define USE_HELPAUTODIAL 0		// 1,����ʱ�Զ��������ĵ������绰���豣֤�����绰������Ч��; 0,������
//#define DIS_SPD_USEIO			// �򿪴˿���,��ʻ��¼ģ��CDrvrecRptDlg�е���̺��ٶ�,ʹ��IO(�������)�źż���
#define UPLOAD_AFTERPHOTOALL 0	// 0,����1�ż��ϴ�1��; 1,ȫ��������ϲſ�ʼ�ϴ�
#define USE_PHOTOUPLD	0		// 0,ʹ�ô��ڷ�ʽ�ϴ�Э��; 1,ʹ��һ�����ϴ�������ƬЭ��
#define USE_PHOTOTHD	1		// 1,���մ���ʹ�õ������߳�; 0,��ʹ��

// ���¿���ҪС��ʹ��
#define USE_EXCEPTRPT	0		// 1,�쳣������ͱ������; 0,��֧��(ע��: �����汾������0)
//#define LOGSOCKDATA			// �򿪴˿��أ���Socket�շ����ݴ�ӡ�����Դ��� (ע�⣺�����汾����ر�)
//#define PHOTO_INVERSE			// �򿪴˿��أ���ʾ��Ƭ��Ҫ�������ҵߵ�����
//#define FOOTALERM_NORMOLOPEN	// �򿪴˿���,��ʾʹ�ó����پ����أ��رռ�ʹ�ó��ձ������أ�Ŀǰֻ����Ϣ�۰汾ʹ�ó������أ�
//#define LOGSAVEFILE			// ������ʾ��־��Ϊ�ļ�����ʽ�汾����رգ�
//#define PICOLDVER				// ��Ƭ����ʹ�þ�Э�飬�Ѳ���
//#define OLDIO					// �ɵ��ں��������Ѳ���

#if USE_DRVREC == 1
#define CAR_RUNNING			0
#define CAR_STOP_RUN		1
#define CAR_RUN_STOP		2
#define CAR_STOP			3
#define UnLog				0 
#define Logined				1
#define Inputing			2
#define MAX_DRIVER			10		//���˾����
#define MAX_PROMPT			3		//���˾�����Ѵ���
#define CONTINUE_TIME		3		//��ʻ��¼�������ʻʱ��(Сʱ),���ڴ�ֵ������
#define VT_LOW_RUN			2		//�жϳ���ֹͣ����С�ٶ�(����/ʱ)
#define RUN_MINTIME			5		//�жϳ����ƶ�����Сʱ��(��)
#define STOP_MINTIME		20		//�жϳ���ֹͣ����Сʱ��(��)
#define PATH				"\\ResFlh2\\DrvJg2"
#define SND_TYPE 1  // 1,ÿ֡����,��һ�����ڵȴ�Ӧ��   2,����һ�����ڲŷ���;
#endif



// ComuExeʹ��
// { �ֻ�ģ��ѡ��
#define NETWORK_YIDONG	66
#define NETWORK_LIANTONG	88
#define NETWORK_GPRS 0
#define NETWORK_CDMA 1
#define NETWORK_TD	2

#define NETWORK_TYPE  NETWORK_GPRS
#define NETWORK_GPRSTYPE NETWORK_YIDONG	// NETWORK_LIANTONG
//ע��: �����CDMAģ��Ҫѡ��ģ�������
#define CDMA_EM200 1
#define CDMA_CM320 2
#define CDMA_TYPE CDMA_EM200
// } �ֻ�ģ��ѡ��

#define GPSVALID_FASTFIXTIME 1	// 1����ʾһ��GPS��Ч������Уʱ��0��������ЧԼ15���Ӻ��Уʱ
#define USE_IRD			0		// 1,��ʾʹ�ú�������,��ʹ���ֱ��͵�����(��ʱӦȷ������TTS); 0,��ʹ�ú�������,ʹ���ֱ��������(����Ӧ)
//#define OUTPUT_GPS			// �򿪴˶��壬�յ���GPS���ݴӵ��Դ������ (��ʽ�汾ʱ�ر�)
//#define GPSGSA_ENB			// �Ƿ��ȡGSA��ʽ���� (PDOP��HDOP��VDOP)
#define USE_PING		0		// �Ƿ�ʹ��PING�Ļ��� 1:ʹ�� 0:��ʹ��
#if GPS_TEST && !defined(GPSGSA_ENB)
#define GPSGSA_ENB
#endif


// StreamMediaʹ��
#define SCREENMOD		true	// ����ģʽѡ��: true,����ģʽ(�����Ź��ſ���); false,����ģʽ(������ʱ�Ϳ���)
#define IMGSHOW			0		// �Ƿ�֧�ֲ��Ź��ǰ��ʾ˾����Ƭ:	0,��֧��; 1,֧��(Ŀǰֻ��VM1��NK֧��)
///////// } <1>ͨ�ñ��뿪��


#define NONE		0
#define TCPMP		1
#define WMEDIA		2

#define UDISK_NAME		(L"UDisk")
#define NO_CARID		1		// 1:��ֹ��дID		0:������дID
#define USE_SERIAL_GPS			// �Ƿ�ʹ�������ڶ�ȡGPS����
#define VEDIO			NONE	// ֧�ֵ���Ƶģ��,NONE,TCPMP,WMEDIA
#define PHOTOCHK_TYPE	2		// 1,����ͷ�����LCD��ʾ; 2,�����Ƭ�����U��; 3,���ͬ���ź�; 0,��֧��
#define USE_UDISK       1

#define DISK2_SIZETYPE	1		// 1,10M; 2,30M;
#define DISK3_SIZETYPE	1		// 1,60M; 2,80M;

const DWORD DISK2_CUSIZE = 1024;// ����2�Ĵش�С��ÿ���ļ���ռ�ÿռ��С��Ӧ���Ǵص�������
const DWORD DISK3_CUSIZE = 512;	// ����3�Ĵش�С

/// } KJ2��JG2��V7����



/// { �������ó�ʼ������,������������Ĭ������,�ҽ����ڸ����ö����Init������,��ͬ�İ汾��Ҫע���޸�!
/// !!ע���Ƿ�������Ҫ�޸ĵ�Ĭ������
#define APN			"CMNET"				// APN���ã�һ�㲻�øģ�
#define CDMA_NAME   "card"
#define CDMA_PASS   "card"

#define QTCPIP		"125.77.254.122"	// ǧ����TCP IP��ַ
#define QTCPPORT	9100				// ǧ����TCP�˿�
#define LUDPIP		"59.61.82.173"		// ��ý��(��Զ����������)UDP IP��ַ
#define LUDPPORT	18000				// ��ý��(��Զ����������)UDP�˿�
#define DUDPIP		"59.61.82.172"
#define DUDPPORT	11101				
#if USE_PROTOCOL == 0 || USE_PROTOCOL == 1 || USE_PROTOCOL == 2
#define QUDPIP		"125.77.254.122"	// ǧ����UDP IP��ַ // 59.61.82.170
#define QUDPPORT	9100				// ǧ����UDP�˿� // 6660
#endif
#if USE_PROTOCOL == 30
#define QUDPIP		"59.61.82.170"		// ǧ����UDP IP��ַ // 59.61.82.170
#define QUDPPORT	6660				// ǧ����UDP�˿� // 6660
#endif

#define SMSCENTID	""					// �������ĺ�,ע��"+86"���ܶ� // +8613800592500
#define SMSSERVCODE	""					// �ط���
#define CENTID1		0x35		// ����ID(�������)��һ�ֽ�,ע��ʹ��16����
#define CENTID2		0x00		// ����ID(�������)�ڶ��ֽ�,ע��ʹ��16����,0����ת��Ϊ7f

#define USERIDENCODE "111111"	// ������_��һЭ���е�GPRS��½֡�е��û�ʶ����(6�ַ�),ÿ�����ư汾��Ӧ���Լ��̶���ʶ����

#define EXCEP_RPTTEL "15985886953"		// �쳣������ŵ�Ŀ���ֻ���
/// } �������ó�ʼ������,������������Ĭ������,�ҽ����ڸ����ö����Init������,��ͬ�İ汾��Ҫע���޸�!

enum QUEELVL // �����1��ʼ����,����ֻ��16��,Խ��Խ����
{
	LV1 = 1,
		LV2,
		LV3,
		LV4,
		LV5,
		LV6,
		LV7,
		LV8,
		LV9,
		LV10,
		LV11,
		LV12,
		LV13,
		LV14,
		LV15,
		LV16
};

enum ERR_CODE
{
	ERR_SEMGET = 1,
	ERR_SEMCTL,
	ERR_SEMOP,
	
	ERR_SHMGET,
	ERR_SHMAT,
	
	ERR_MSGGET,
	ERR_MSGSND,
	ERR_MSGRCV,
	ERR_MSGSKIPBLOCK,

	ERR_THREAD,
	ERR_READCFGFAILED,

	ERR_MEMLACK,
	ERR_SPACELACK,
	ERR_BUFLACK,
	ERR_PTR,
	ERR_QUEUE_EMPTY,
	ERR_QUEUE_FULL,
	ERR_QUEUE,
	ERR_QUEUE_TOOSMALL,
	ERR_QUEUE_SYMBNOFIND,

	ERR_OPENFAIL,
	ERR_IOCTLFAIL,
	ERR_LSEEKFAIL,
	ERR_WRITEFAIL,
	ERR_READFAIL,

	ERR_SOCKLIB,
	ERR_SOCK,
	ERR_SOCK_TCPFBID,

	ERR_FILEMAP,
	ERR_FILE_FAILED,
	ERR_FILE,

	ERR_COM,
	ERR_IO,
	ERR_PHOTO,
	ERR_CFGBAD,		
	ERR_COMCHK,
	ERR_COMSND,
	ERR_COMSND_UNALL,

	ERR_DLL,
	ERR_PAR,	
	ERR_CFG,
	ERR_DATA_INVALID,
	ERR_BLACKGPS_INVALID,
	ERR_GPS_INVALID,
	ERR_INNERDATA_ERR,
	ERR_STA_ERR,

	ERR_AUTOLSN,
	ERR_AUTOLSN_NOTEL,	

	ERR_CODECVT,

	ERR_LED,
	ERR_LED_QUEE,

	ERR_SMS_DESCODE, // ���ŷ���Ŀ�ķ��ֻ��Ų���ȷ

	ERR_TM,

	ERR_SYSCALL,
};

enum PHONE_TYPE
{
	PHONE_BENQ = 0,
	PHONE_SIMCOM,
	PHONE_HWCDMA,
	PHONE_DTTD
};

#define VALIDYEAR_BEGIN		2009
#define VALIDMONTH_BEGIN	10 // ��VALIDYEAR_BEGIN�е�month

#define TCP_RECVSIZE	256
#define TCP_SENDSIZE	1024
#define UDP_RECVSIZE	1500
#define UDP_SENDSIZE	1500
#define SOCK_MAXSIZE	1500
#define COMUBUF_SIZE	2048
#define UDPDATA_SIZE	1200 // UDP���ݱ����������ݵ���󳤶�, Ҳ����Ƭ����֡һ�����Ĵ�С�޶�ֵ
// !!ע��,��ֵӦС��1500,��С��SOCKSND_MAXBYTES_PERSEC-QIANUPUDPFRAME_BASELEN

#define MAX_UNSIGNED_64		((unsigned long long)(-1))
#define MAX_UNSIGNED_32		((unsigned long)(-1))
#define MAX_UNSIGNED_16		((unsigned short)(-1))
#define MAX_UNSIGNED_8		((unsigned char)(-1))

const double BELIV_MAXSPEED = 100 * 1.852 / 3.6; // ��λ:��/��

const BYTE GPS_REAL		= 0x01;
const BYTE GPS_LSTVALID	= 0x02;

const WORD PHOTOEVT_FOOTALERM		= 0x8000; // �پ�
const WORD PHOTOEVT_OVERTURNALERM	= 0x4000; // �෭����
const WORD PHOTOEVT_BUMPALERM		= 0x2000; // ��ײ����
const WORD PHOTOEVT_FOOTALERM_BIG	= 0x1000; // �پ�ʱ���ӵĴ���Ƭ,�����������ϴ�
const WORD PHOTOEVT_TRAFFICHELP		= 0x0800; // ��ͨ����
const WORD PHOTOEVT_MEDICALHELP		= 0x0400; // ҽ������
const WORD PHOTOEVT_BOTHERHELP		= 0x0200; // ��������
const WORD PHOTOEVT_CLOSEDOOR		= 0x0100; // �س���
const WORD PHOTOEVT_AREA			= 0x0080; // ��������
const WORD PHOTOEVT_ACCON			= 0x0040; // ACC��Ч
const WORD PHOTOEVT_DRIVEALERM		= 0x0020; // �Ƿ�����������������
const WORD PHOTOEVT_CHK				= 0x0010; // ��������¼�
const WORD PHOTOEVT_JIJIADOWN		= 0x0008;
const WORD PHOTOEVT_JIJIAUP			= 0x0004;
const WORD PHOTOEVT_TIMER			= 0x0002; // ��ʱ����
const WORD PHOTOEVT_CENT			= 0x0001; // ����ץ��

const WORD PHOTOUPLOAD_FOOTALERM		= PHOTOEVT_FOOTALERM;
const WORD PHOTOUPLOAD_OVERTURNALERM	= PHOTOEVT_OVERTURNALERM;
const WORD PHOTOUPLOAD_BUMPALERM		= PHOTOEVT_BUMPALERM;
const WORD PHOTOUPLOAD_TRAFFICHELP		= PHOTOEVT_TRAFFICHELP;
const WORD PHOTOUPLOAD_MEDICALHELP		= PHOTOEVT_MEDICALHELP;
const WORD PHOTOUPLOAD_BOTHERHELP		= PHOTOEVT_BOTHERHELP;
const WORD PHOTOUPLOAD_CLOSEDOOR		= PHOTOEVT_CLOSEDOOR;
const WORD PHOTOUPLOAD_AREA				= PHOTOEVT_AREA;
const WORD PHOTOUPLOAD_ACCON			= PHOTOEVT_ACCON;
const WORD PHOTOUPLOAD_DRIVEALERM		= PHOTOEVT_DRIVEALERM;
const WORD PHOTOUPLOAD_JIJIADOWN		= PHOTOEVT_JIJIADOWN;
const WORD PHOTOUPLOAD_JIJIAUP			= PHOTOEVT_JIJIAUP;
const WORD PHOTOUPLOAD_TIMER			= PHOTOEVT_TIMER;
const WORD PHOTOUPLOAD_CENT				= PHOTOEVT_CENT;


/// ���¶���,������Ϊ1��bit�������� !!!
const DWORD DEV_SOCK	= 0x00000001;
const DWORD DEV_LED		= 0x00000002;
const DWORD DEV_QIAN	= 0X00000004;
const DWORD DEV_DVR		= 0x00000008;
const DWORD DEV_PHONE	= 0x00000010;
const DWORD DEV_DIAODU	= 0x00000020;
const DWORD DEV_LIU		= 0x00000040;
const DWORD DEV_IO		= 0x00000080; // ����Ҫ���ն���
const DWORD DEV_GPS		= 0x00000100; // ����Ҫ���ն���


// ������SYSIO_CFG[]�Ķ���˳��һ��!�ұ����0��ʼ���,ע����ȡֵ��Χ: 0~254 (255����Ϊ�����������ʹ��)
enum IOSymb
{
	// ����
	IOS_ACC = 0,
	IOS_ALARM, //�پ��ź�
	IOS_JIJIA, // ������
	IOS_FDOOR, //ǰ���ź�
	IOS_BDOOR, //�����ź�
	IOS_JIAOSHA,
	IOS_SHOUSHA,
	IOS_PASSENGER1, //�ؿ��ź�1
	IOS_PASSENGER2, //�ؿ��ź�2
	IOS_PASSENGER3, //�ؿ��ź�3
	
	
	// ���
	IOS_GPS,
	IOS_HUBPOW,
	IOS_PHONEPOW,
	IOS_PHONERST,
	IOS_HARDPOW, //Ӳ�̵�Դ
	IOS_TRUMPPOW, //���ŵ�Դ
	IOS_SCHEDULEPOW, //��������Դ
	IOS_VIDICONPOW, //����ͷ/TW2835��Դ
	IOS_OILELECCTL, //���͵����
	IOS_APPLEDPOW, //������ָʾ��
	IOS_SYSLEDPOW, //ϵͳ����ָʾ��
	IOS_TOPLEDPOW, //���Ƶ�Դ
	IOS_AUDIOSEL, //�ֻ���Ƶ/PC��Ƶѡ��
	IOS_EARPHONESEL, //����/����ѡ��
	IOS_TW2835RST, //TW2835��λ
};

// IOӦ�����嶨��,����ⲿIO�ź�,��CPU�ܽ��ź�
enum IO_APP
{
	// ����
	IO_ACC_ON = 1, // �����1��ʼ,��Ϊ�ڲ������õ���unsigned char,��Լ�ռ�
	IO_ACC_OFF,
	IO_ALARM_ON,
	IO_ALARM_OFF,
	IO_JIJIA_HEAVY,
	IO_JIJIA_LIGHT,
	IO_FDOOR_OPEN,
	IO_FDOOR_CLOSE,
	IO_BDOOR_OPEN,
	IO_BDOOR_CLOSE,
	IO_JIAOSHA_VALID,
	IO_JIAOSHA_INVALID,
	IO_SHOUSHA_VALID,
	IO_SHOUSHA_INVALID,
	IO_PASSENGER1_VALID,
	IO_PASSENGER1_INVALID,
	IO_PASSENGER2_VALID,
	IO_PASSENGER2_INVALID,
	IO_PASSENGER3_VALID,
	IO_PASSENGER3_INVALID,
	
	
	// ���
	IO_GPSPOW_ON,
	IO_GPSPOW_OFF,
	IO_HUBPOW_ON,
	IO_HUBPOW_OFF,
	IO_PHONEPOW_ON,
	IO_PHONEPOW_OFF,
	IO_PHONERST_HI,
	IO_PHONERST_LO,
	IO_HARDPOW_ON, 
	IO_HARDPOW_OFF, 
	IO_TRUMPPOW_ON,
	IO_TRUMPPOW_OFF,
	IO_SCHEDULEPOW_ON, 
	IO_SCHEDULEPOW_OFF, 
	IO_VIDICONPOW_ON, 
	IO_VIDICONPOW_OFF, 
	IO_OILELECCTL_CUT, 
	IO_OILELECCTL_RESUME, 
	IO_APPLEDPOW_ON, 
	IO_APPLEDPOW_OFF, 
	IO_SYSLEDPOW_ON, 
	IO_SYSLEDPOW_OFF, 
	IO_TOPLEDPOW_ON,
	IO_TOPLEDPOW_OFF,
	IO_AUDIOSEL_PHONE, //ѡ���ֻ���Ƶ
	IO_AUDIOSEL_PC, //ѡ��PC��Ƶ
	IO_EARPHONESEL_ON,//ѡ�����
	IO_EARPHONESEL_OFF,//ѡ������
	IO_TW2835RST_HI,
	IO_TW2835RST_LO,
};

struct tagIOCfg
{
	unsigned int m_uiPinNo; // �ܽű��
	unsigned int m_uiContPrid; // �ж��źų���ʱ��(ʹ���ж�ʱ��������),ȥ����,ms
	unsigned char m_ucInOut;	// 0,����; 1,���
	bool m_bBreak;	// true,ʹ���ж�(����ʱ��������); false,��ֹ�ж�
	char m_cBreakType; // �жϷ�ʽ(ʹ���ж�ʱ��������): 1,��ƽ����; 2,�����ش���; 3,˫���ش���
	char m_cBreakPow; // �жϴ�����ƽ(�ڵ�ƽ�����͵����ش���ʱ��������): 0, �͵�ƽ���½��ش���; 1,�ߵ�ƽ�������ش���
	unsigned char m_ucPinLowApp; // �ܽŵ͵�ƽ��Ӧ������
	unsigned char m_ucPinHigApp; // �ܽŸߵ�ƽ��Ӧ������
};


#pragma pack(8)

typedef struct tagGPSData
{
	double		latitude;         //γ��0��90
	double		longitude;        //����0��180
	float		speed;           //�ٶȣ���/��
	float		direction;        //����0��360
	float		mag_variation;    //�ű�Ƕȣ�0��180
	int			nYear; // 4�����ֱ�ʾ��ֵ,��2007,������7;
	int			nMonth;
	int			nDay;
	int			nHour;
	int			nMinute;
	int			nSecond;
	char		valid;             //��λ�����Ƿ���Ч
	BYTE		la_hemi;          //γ�Ȱ���'N':������'S':�ϰ���
    BYTE		lo_hemi;           //���Ȱ���'E':������'W':������
	BYTE		mag_direction;    //�ű䷽��E:����W:��
	BYTE		m_bytMoveType; // 1,�ƶ�; 2,��ֹ
	BYTE		m_bytPDop; // GPS������ӡ�10,��ͬ
	BYTE		m_bytVDop;
	BYTE		m_bytHDop;
	char		m_cFixType; // ��λ����: '1'-δ��λ; '2'-2D��λ; '3'-3D��λ
	
	tagGPSData(char c)
	{
		Init();
	}
	tagGPSData()
	{
		Init();
	}
	void Init()
	{
		// ��ʼ����ͬ,dll�Ĵ�����ܲ�ͬ,ע��!
		memset( this, 0, sizeof(*this) );
		nYear = VALIDYEAR_BEGIN, nMonth = VALIDMONTH_BEGIN, nDay = 15;
		valid = 'V', m_cFixType = '1';
	}
}GPSDATA;

#pragma pack()


#pragma pack(8)

/// ��������:
// (1)������Ҫ���õ�,ǰ׺��"1";����һ�����õ�,ǰ׺��"2"
// (2)��������,ǰ׺��"P";��ý������,ǰ׺��"L";ǧ��������,ǰ׺��"Q"
// (3)һ�����ö���,��"Cfg"��β; �������ö���,��"Par"��β

/// ���µĵ绰����,��������˵��,�������ǲ��ո�

struct tag1PComuCfg
{
	char		m_szDialCode[33];
	char		m_szSmsCentID[22];	// �������ĺ�,������0
	char		m_szTel[16];		// ��̨�ֻ���,��NULL��β
	int			m_iRegSimFlow;		// ע��SIM������
	byte		m_PhoneVol;			// �绰����
//	char		m_szUserIdentCode[6]; // �û�ʶ����	
	
	char        m_szCdmaUserName[50];
	char        m_szCdmaPassWord[30];

	void Init(const char *v_szCdmaUserName , const char *v_szCdmaPassWord)
	{
		memset( this, 0, sizeof(*this) );
		strncpy( m_szSmsCentID, SMSCENTID, sizeof(m_szSmsCentID) );
		m_szSmsCentID[ sizeof(m_szSmsCentID) - 1 ] = 0;
		strncpy( m_szCdmaUserName , v_szCdmaUserName, sizeof(m_szCdmaUserName) );
		m_szCdmaUserName[sizeof(m_szCdmaUserName) -1] = 0;
		strncpy( m_szCdmaPassWord , v_szCdmaPassWord, sizeof(m_szCdmaPassWord) );
		m_szCdmaPassWord[sizeof(m_szCdmaPassWord) -1] = 0;
		m_iRegSimFlow = 1;
		m_PhoneVol = 3;		//0-5
	}
};

struct tag1LComuCfg
{
	unsigned long m_ulLiuIP;
	unsigned long m_ulLiuIP2;
	unsigned short m_usLiuPort;
	unsigned short m_usLiuPort2;
	char	m_szLiuSmsTel[15];
	char	m_szVeIDKey[20]; // ʵ���Ѳ���,�ѵ����������
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_ulLiuIP = inet_addr(LUDPIP); 
		m_usLiuPort = htons(LUDPPORT);
	}
};

struct tag1LMediaCfg
{
	BYTE		m_bytPlayTimes; // ��ý�岥�Ŵ���
	//unsigned long m_aryDriverID[5]; // ����ע��˾��ID
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_bytPlayTimes = 1;
	}
};

struct tag1QIpCfg
{
	unsigned long m_ulQianTcpIP; // (inet_addr֮���,��ͬ)
	unsigned long m_ulLiuUdpIP;
	unsigned long m_ulDvrUdpIP;
	unsigned short m_usQianTcpPort; // (htons֮��ģ���ͬ) 
	unsigned short m_usLiuUdpPort;
	unsigned short m_usDvrUdpPort;
	char		m_szApn[20]; // ��NULL��β

	void Init( unsigned long v_ulQianTcpIP, unsigned short v_usQianTcpPort,
				unsigned long v_ulLiuUdpIP, unsigned short v_usLiuUdpPort,
				unsigned long v_ulDvrUdpIP, unsigned short v_usDvrUdpPort,
		const char *const v_szApn )
	{
		memset(this, 0, sizeof(*this));
		strncpy( m_szApn, v_szApn, sizeof(m_szApn) - 1 );

		m_ulQianTcpIP = v_ulQianTcpIP, m_usQianTcpPort = v_usQianTcpPort;
		m_ulLiuUdpIP = v_ulLiuUdpIP,   m_usLiuUdpPort = v_usLiuUdpPort;
		m_ulDvrUdpIP = v_ulDvrUdpIP, m_usDvrUdpPort = v_usDvrUdpPort;
	}
};

struct tag1QBiaoding
{
	WORD		m_wCyclePerKm; // ÿ����������
	//HXD 081120 �ͺ���صĲ�����
	BYTE		m_bReverse;
	BYTE	    m_bFiltrate;
	BYTE	    m_bytHiVal;
	BYTE		m_bytLoVal;

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

// ����20070914����,��Ϊ������Ӧ����ҵ����зֿ�,����ÿ���СӦ��40��������
struct tagImportantCfg
{
	char		m_szSpecCode[40];

	// ����ͨ�Ų���
	union
	{
		char		m_szReserved[160];
		tag1PComuCfg m_obj1PComuCfg;
	} m_uni1PComuCfg;

	// ��ý��ͨ�Ų���
	union
	{
		char		m_szReserved[160];
		tag1LComuCfg m_obj1LComuCfg;
	} m_uni1LComuCfg;

	// ��ý�岥������
	union
	{
		char		m_szReserved[120];
		tag1LMediaCfg m_obj1LMediaCfg;
	} m_uni1LMediaCfg;

	// ǧ����IP
	union
	{
		char		m_szReserved[200];
		tag1QIpCfg	m_ary1QIpCfg[3];
	} m_uni1QComuCfg;

	// IO���ݷ���ı궨ֵ
	union
	{
		char		m_szReserved[80];
		tag1QBiaoding m_obj1QBiaodingCfg;
	} m_uni1QBiaodingCfg;

	void InitDefault()
	{
		memset(this, 0, sizeof(*this));
		strncpy( m_szSpecCode, CFGSPEC, sizeof(m_szSpecCode) - 1 );

		m_uni1PComuCfg.m_obj1PComuCfg.Init(CDMA_NAME,CDMA_PASS);
		m_uni1LComuCfg.m_obj1LComuCfg.Init();
		m_uni1LMediaCfg.m_obj1LMediaCfg.Init();
		m_uni1QComuCfg.m_ary1QIpCfg[0].Init( inet_addr(QTCPIP), htons(QTCPPORT),
											inet_addr(LUDPIP), htons(LUDPPORT),
											inet_addr(DUDPIP), htons(DUDPPORT),
											APN );
		m_uni1QBiaodingCfg.m_obj1QBiaodingCfg.Init();
	}
};

struct tag2LMediaCfg
{
	DWORD		m_dwSysVolume; // ϵͳ����
	byte        m_bytInvPhoto; //090611 �Ƿ�Ҫ�ߵ���Ƭ
	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_dwSysVolume = 75;
	}
};

struct tag2QBlkCfg
{
	DWORD			m_dwBlackChkInterv; // ��λ:��

	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_dwBlackChkInterv = 30; // ��ȫ���
	}
};

struct tag2QRealPosCfg // 432
{
	double	m_dLstLon;
	double	m_dLstLat;
	double	m_dLstSendDis; // �ϴη�������֮���ۻ���ʻ�ľ��� (��)

	float	m_fSpeed; // ��/��, 0��ʾ��Ч

	DWORD	m_dwLstSendTime; // �ϴη���ʱ��

	long	m_aryArea[16][4]; // ÿ����Χ������: ���¾��ȡ�����γ�ȡ����Ͼ��ȡ�����γ�� (����10��-6�η���)
	DWORD	m_aryAreaContCt[16]; // ���������ļ��� (Ԫ�ظ���������һ��)
	BYTE	m_aryAreaProCode[16]; // ����Χ�ı�� (Ԫ�ظ���������һ��)
	BYTE	m_bytAreaCount;

	BYTE	m_bytGpsType;
	WORD	m_wSpaceInterv; // ����,0��ʾ��Ч
	WORD	m_wTimeInterv; // ����,0��ʾ��Ч

	char	m_szSendToHandtel[15];

	BYTE	m_aryTime[10][3]; // ÿ��ʱ��������: ���,ʱ,��
	BYTE	m_bytTimeCount;

	BYTE	m_aryStatus[6];
	// �̶�Ϊ6��, BYTE	m_bytStaCount; 

	BYTE	m_bytCondStatus; // 0	1	I	T	V	A	S	0
						//	�ɣ��ر�/�������������0-�رգ�1-����
						//	�ԣ��ر�/����ʱ��������0-�رգ�1-����
						//	�֣��ر�/�����ٶ�������0-�رգ�1-����
						//	�����ر�/������Χ������0-�رգ�1-����
						//	�ӣ��ر�/����״̬������0-�رգ�1-����

	BYTE	m_bytLstTimeHour; // �ϴη���ʱ��������ʱ��,��ʼ��Ϊ��Чֵ����0xff,
	BYTE	m_bytLstTimeMin;

	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_bytLstTimeHour = m_bytLstTimeMin = 0xff;
		m_wTimeInterv = 0, m_wSpaceInterv = 0, m_bytCondStatus = 0x40;
	}
};

struct tag2QGpsUpPar // �������ļ���
{
	WORD		m_wMonSpace; // ��ض������ ����λ��10�ף�
	WORD		m_wMonTime; // ���ʱ�� �����ӣ���0xffff��ʾ���ü��
	WORD		m_wMonInterval; // ��ؼ�� ���룩������Ϊ1
	BYTE		m_bytStaType; // ע��: ����Э�����״̬����
	BYTE		m_bytMonType; // Э���ж���ļ������

	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_wMonInterval = 0xffff;
		m_wMonSpace = 0xffff;
	}
};

struct tag2QGprsCfg	// 42
{
	tag2QGpsUpPar	m_aryGpsUpPar[4];
	BYTE		m_bytGprsOnlineType; // ���߷�ʽ
	BYTE		m_bytOnlineTip; // ���߷�ʽ��ʾ
	BYTE		m_bytGpsUpType; // GPS�����ϴ���ʶ 1,��������0154֡��2,����������
	BYTE		m_bytGpsUpParCount; // GPS���ݲ�������
	BYTE		m_bytChannelBkType_1; // ���ű�������1
		// 0	H1	H2	H3	H4	H5	H6	1
		// H1: ���, H2: ʵʱ��λ, H3: λ�ò�ѯ, H4: ����, H5: ����, H6: ����
	BYTE		m_bytChannelBkType_2; // ���ű�������2
		// 0	L1	L2	L3	L4	L5	L6	1
		// L1: ��ϻ��, L2: ˰�ش���

	BYTE		m_bytRptBeginHour;	// �����ϱ�ʱ���,����ʼʱ��==����ʱ��,���ʾ��ʱ�������
	BYTE		m_bytRptBeginMin;
	BYTE		m_bytRptEndHour;
	BYTE		m_bytRptEndMin;

	void Init()
	{
		memset( this, 0, sizeof(*this) );

		// �ر����ж��ű��ݣ����ű��ݹ������ڳ�����̶�����ע�������޸Ķ��ű��ݵ�ָ��Ҫ����
		m_bytChannelBkType_1 = m_bytChannelBkType_2 = 0;
		
		m_bytGpsUpType = 2;
		for( unsigned int ui = 0; ui < sizeof(m_aryGpsUpPar) / sizeof(m_aryGpsUpPar[0]); ui ++ )
			m_aryGpsUpPar[ui].Init();
	}
};

struct tag2QServCodeCfg
{
	char		m_szAreaCode[2]; // �����(����ID)
	char		m_szQianSmsTel[15]; // �����ط���(�����ط���),������0
	char		m_szTaxHandtel[15]; // ˰���ط���
	char		m_szDiaoduHandtel[15]; // �����ط���
	char		m_szHelpTel[15]; // ��������
	
	BYTE		m_bytListenTelCount;
	char		m_aryLsnHandtel[8][15]; // ����������������ĵ绰 (Э���ﶨ�������Ϊ5��)

	BYTE		m_bytCentServTelCount;
	char		m_aryCentServTel[5][15]; // ���ķ���绰

	BYTE		m_bytUdpAtvTelCount; // UDP����绰����
	char		m_aryUdpAtvTel[10][15];

	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_szAreaCode[0] = CENTID1;
		m_szAreaCode[1] = CENTID2;
		strncpy( m_szQianSmsTel, SMSSERVCODE, sizeof(m_szQianSmsTel) );
		m_szQianSmsTel[ sizeof(m_szQianSmsTel) - 1 ] = 0;
	}
};

struct tag2QListenCfg
{
	BYTE		m_bytListenTelCount;
	char		m_aryLsnHandtel[8][15]; // ����������������ĵ绰 (Э���ﶨ�������Ϊ5��)

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

struct tag2QFbidTelCfg
{
	BYTE		m_bytFbidInTelCount;
	BYTE		m_bytAllowInTelCount;
	BYTE		m_bytFbidOutTelCount;
	BYTE		m_bytAllowOutTelCount;
	BYTE		m_bytFbidType; // ��Э����Ķ��巽ʽһ��
	char		m_aryFbidInHandtel[12][15]; // ��ֹ����ĵ绰 (Э���ﶨ�������Ϊ8��)
	char		m_aryAllowInHandtel[12][15]; // �������ĵ绰
	char		m_aryFbidOutHandtel[12][15]; // ��ֹ�����ĵ绰
	char		m_aryAllowOutHandtel[12][15]; // ��������ĵ绰
		// ��4λ,0�������к���,1��ֹ���к���,2�����ֺ������ֽ�ֹĳЩ����,3��ֹ���ֺ���,��������ĳЩ����
		// ��4λ,0�������к���,1��ֹ���к���,2�����ֺ��뵫�ֽ�ֹĳЩ����,3��ֹ���ֺ���,��������ĳЩ����

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

struct tag2QPhotoPar
{
	BYTE		m_bytTime; // ���մ���
	BYTE		m_bytSizeType; // ���շֱ��� (��Э�鶨�岻ͬ)
							// 1,��С
							// 2,�е�
							// 3,���
	BYTE		m_bytQualityType; // �������� (��Э�鶨����ͬ)
							// 0x01���㶨�����ȼ����ߣ�
							// 0x02���㶨�����ȼ����У���Ĭ�ϣ�
							// 0x03���㶨�����ȼ����ͣ�
	BYTE		m_bytChannel; // ͨ�� 0x01,ͨ��1; 0x02,ͨ��2; 0x03,ͨ��1��ͨ��2
	BYTE		m_bytInterval; // ���ռ��,s
	BYTE		m_bytBright; // ����
	BYTE		m_bytContrast; // �Աȶ�
	BYTE		m_bytHue; // ɫ��
	BYTE		m_bytBlueSaturation; // ������
	BYTE		m_bytRedSaturation; // �챥��
	BYTE		m_bytDoorPar; // �����ڹس�������,����λ��ʾ�ٶ����ƣ�0-15������/Сʱ��,����λ��ʾ���ٶȳ���ʱ�䣺0-15�����ӣ�
	//090611 hxd
	BYTE		m_bytIntervHour; // ���ռ����Сʱ��JG2��Сʱ�ͷ���ȫ0,��Ϊ����ʱ����
	BYTE		m_bytIntervMin; // ���ռ��,����(JG2)
	char		m_szReserve[7];

	void Init( BYTE v_bytTime, BYTE v_bytSizeType, BYTE v_bytQualityType, BYTE v_bytChannel )
	{
		memset( this, 0, sizeof(*this) );
		m_bytBright = m_bytContrast = m_bytHue = m_bytBlueSaturation = m_bytRedSaturation = 8;
		m_bytTime = v_bytTime;
		m_bytSizeType = v_bytSizeType;
		m_bytQualityType = v_bytQualityType;
		m_bytChannel = v_bytChannel;
	}
};

struct tag2QAlertCfg
{
	// ��������
	BYTE		m_bytAlert_1; // 0	S1	S2	S3	S4	S5	S6	1
		// JG2��JG3��VM1Э�飺
		// S1��0-�ر�Ƿѹ����            1-����Ƿѹ����
		// S2��0-�ر����ٱ���            1-�������ٱ���
		// S3��0-�ر��𶯱���            1-�����𶯱���
		// S4��0-�رշǷ��򿪳��ű���    1-����Ƿ��򿪳��ű���
		// S5��0-�رն�·����            1-�����·����
		// S6����������

		// KJ2Э�飺
		// S1��0-�ر�Ƿѹ����            1-����Ƿѹ���� (Э����û��,����ΪKJ-01���Ա������д˹���,���Բ���)
		// S2��0-�ر����ٱ���            1-�������ٱ���
		// S3��0-�ر���ײ����            1-������ײ����
		// S4��0-�رղ෭����            1-����෭����
		// S5��0-�رն�·����            1-�����·���� (Э����û��,����ΪKJ-01���Ա������д˹���,���Բ���)

	BYTE		m_bytAlert_2; // 0	A1	A2	A3	A4	A5	A6	1
		// JG2��JG3��VM1Э�飺
		// A1��0-�ر�Խ�籨��            1-����Խ�籨��
		// A2��0-�رճ��ٱ���            1-�����ٱ���
		// A3��0-�ر�ͣ����������        1-����ͣ��ʱ���������
		// A4��0-�ر�ʱ��μ�鱨��      1-����ʱ��μ�鱨��
		// A5��A6, ��������

		// KJ2Э�飺
		// A5��0-�رշǷ���������        1-����Ƿ���������

	BYTE		m_bytAlermSpd; // ���ٱ����ٶ� (����/Сʱ)
	BYTE		m_bytSpdAlermPrid; // ���ٱ����ĳ��ٳ���ʱ�� (��)

	void Init()
	{
		// Ĭ�ϱ���ȫ����
		m_bytAlert_1 = 0x7f;
		m_bytAlert_2 = 0x7f;

		// ���ٱ�������Ĭ����Ϊ������
		m_bytAlermSpd = 0xff;
		m_bytSpdAlermPrid = 0xff;
	}
};

// ��ʻ��¼���������
// (����������Ϊ0, ��ʾ������; ����������Ϊ0,��ʾ������; �����ڴ�СΪ0,��ʾ����Ҫȷ�ϲ���Ҫ�ɿ�����)
struct tag2QDriveRecCfg
{
	WORD	m_wOnPeriod;	// ACC ON ����ʱ�� (����) (ffff��ʾ���÷���) (ǧ����Э��ʱʹ��)
	WORD	m_wOffPeriod;	// ACC OFF ����ʱ�� (����) (ffff��ʾ���÷���) (ǧ����Э��ʱʹ��)
	WORD	m_wOnSampCycle; // ACC ON ��������,��
	WORD	m_wOffSampCycle; // ACC OFF ��������,��
	BYTE	m_bytSndWinSize; // ���ʹ��ڴ�С
	BYTE	m_bytOnSndCycle; // ACC ON,��������,��
	BYTE	m_bytOffSndCycle; // ACC OFF,��������,��

	void Init()
	{
		memset( this, 0, sizeof(*this) );
#if USE_PROTOCOL == 30
		m_wOnPeriod = 0xffff;
		m_wOffPeriod = 0xffff;
		m_wOnSampCycle = 15;
		m_wOffSampCycle = 30;
		m_bytSndWinSize = 5;
		m_bytOnSndCycle = 2;
		m_bytOffSndCycle = 1;
#endif

#if USE_PROTOCOL == 0 || USE_PROTOCOL == 1 || USE_PROTOCOL == 2
		m_wOnPeriod = 0xffff;
		m_wOffPeriod = 0xffff;
		m_wOnSampCycle = 60;
		m_wOffSampCycle = 1800;
		m_bytSndWinSize = 4;
		m_bytOnSndCycle = 1;
		m_bytOffSndCycle = 1;
#endif
	}
};

// �绰����Ŀ����
struct tag2QTelBookPar
{
	WORD	m_wLmtPeriod; // ͨ��ʱ��(����),0��ʾ����ʱ
	BYTE	m_bytType; // Ȩ������,ͬЭ�鶨��,��
		// 00H  ���������ͺ���
		// 01H  ��������룬�������
		// 02H  ������룬�������
		// 03H  �������ͺ���
	char	m_szTel[15]; // ���Ϊ��,��ʾ�����绰��δ����
	char	m_szName[20];
};

// �绰������
struct tag2QCallCfg
{
	BYTE	m_bytSetType; // ������
						// �ֽڶ������£�
						// 0	1	CL	CR	D	D	R	R
						// 
						// CL��1-������е绰�����ã� 0-����֮ǰ�绰������
						// DD�������绰����
						// 00���������к����绰
						// 01����ֹ���к����绰
						// 10���绰������
						// 11������
						// RR������绰����
						// 00���������к���绰
						// 01����ֹ���к���绰
						// 10���绰������ 
						// 11������
	tag2QTelBookPar	m_aryTelBook[30];

	void Init()
	{
		memset( this, 0, sizeof(*this) ); // �������к������
	}
};

// ���ķ���绰��������
struct tag2QCentServTelCfg
{
	BYTE	m_bytTelCount;
	char	m_aryCentServTel[5][15];

	void Init() { memset( this, 0, sizeof(*this) ); }
};

// UDP����绰��������
struct tag2QUdpAtvTelCfg
{
	BYTE	m_bytTelCount;
	char	m_aryUdpAtvTel[10][15];

	void Init()
	{
		memset(this, 0, sizeof(*this));
		//strcpy( m_aryUdpAtvTel[0], "05922956773" );
	}
};

// ��ʻԱ��Ϣ (ǧ����ϵͳ)
struct tag2QDriverPar_QIAN
{
	WORD	m_wDriverNo; // ��� 0~126, 0��ʾ����ϢΪ�� (ʵ��ֻ��1���ֽ�)
	char	m_szIdentID[7]; // ��ݴ���,���㲹0,Ҳ����ȫ����
	char	m_szDrivingLic[18]; // ��ʻ֤��,���㲹0,Ҳ����ȫ����
};

// ��ʻԱ��Ϣ (2��ϵͳ)
struct tag2QDriverPar_KE
{
	WORD	m_wDriverNo; // ��� 0~65534, 0��ʾ����ϢΪ��
	char	m_szIdentID[8]; // ��ݴ���, �����0��Ҳ����ȫ����
};

// ��ʻԱ�������
struct tag2QDriverCfg
{
#if USE_PROTOCOL == 0 || USE_PROTOCOL == 1 || USE_PROTOCOL == 2
	tag2QDriverPar_QIAN	m_aryDriver[10];
#endif

#if USE_PROTOCOL == 30
	tag2QDriverPar_KE	m_aryDriver[10];
#endif

	void Init() { memset(this, 0, sizeof(*this)); }
};

// ʵʱָ��������Ŀ (�պ�4�ֽڶ���,�����׵���)
struct tag2QRealNavPar
{
	long		m_lCentLon;
	long		m_lCentLat;
	WORD		m_wRadius; // ��
	BYTE		m_bytSpeed; // �����ٶ�,����/Сʱ,0��ʾ������
	BYTE		m_bytDir; // ��,ʵ�ʶ���/3
	BYTE		m_bytType; // ����
		// 000TD000 (����Э��)
        //T��0--��ͨ����
        //   1--�������ͣ���ʾ�����⣩��Ҫ����ʵʱָ�������ģ�����ֻ�����ٶ����ƣ�
        //D��0--�����ж��г�����
        //   1--���ж��г�����
	char		m_szTip[51]; // ��ʾ����,������0,�ұض���NULL��β,��ΪЭ���ﶨ�����50�ֽ�,���ֶ����ñ�Э�����1���ֽ�
};


// ʵʱָ������
struct tag2QRealNavCfg
{
	tag2QRealNavPar		m_aryRealNavPar[50];

	void Init() { memset(this, 0, sizeof(*this)); }
};


// ��������
struct tag2QPhotoCfg
{
	WORD		m_wPhotoEvtSymb; // ���մ���״̬�� (��QianExeDef.h�еĹ涨)
	WORD		m_wPhotoUploadSymb; // ���������ϴ�״̬�� (��QianExeDef.h�еĹ涨)

	tag2QPhotoPar	m_objAlermPhotoPar; // ������������
	tag2QPhotoPar	m_objJijiaPhotoPar; // �ճ�/�س��仯ʱ����������
	tag2QPhotoPar	m_objHelpPhotoPar; // ������������
	tag2QPhotoPar	m_objDoorPhotoPar; // ���س�������
	tag2QPhotoPar	m_objAreaPhotoPar; // ������������
	tag2QPhotoPar	m_objTimerPhotoPar; // ��ʱ����
	tag2QPhotoPar	m_objAccPhotoPar; // ACC�仯����

	void Init()
	{
		memset( this, 0, sizeof(*this));
		m_wPhotoEvtSymb = PHOTOEVT_FOOTALERM;
		m_wPhotoUploadSymb = PHOTOUPLOAD_FOOTALERM;//| PHOTOUPLOAD_OVERTURNALERM | PHOTOUPLOAD_BUMPALERM
// 			| PHOTOUPLOAD_TRAFFICHELP | PHOTOUPLOAD_MEDICALHELP | PHOTOUPLOAD_BOTHERHELP
// 			| PHOTOUPLOAD_AREA | PHOTOUPLOAD_TIMER; // �Ƿ��������������ݲ��ϴ�����ΪЭ��δ����
		m_objAlermPhotoPar.Init( 1, 1, 2, 3 ); // v_bytTime, v_bytSizeType, v_bytQualityType, v_bytChannel
		m_objJijiaPhotoPar.Init( 1, 1, 2, 3 );
		m_objHelpPhotoPar.Init( 1, 1, 2, 3 );
		m_objDoorPhotoPar.Init( 1, 1, 1, 3 );
		m_objAreaPhotoPar.Init( 1, 1, 2, 3 );
		m_objTimerPhotoPar.Init( 1, 1, 1, 3 );
		m_objAccPhotoPar.Init( 1, 1, 1, 3 );
	}
};

//LED���� 07.9.26 hong
struct tag2QLedCfg 
{
	unsigned short m_usLedParamID;  //LED����ID
	unsigned short m_usTURNID;		//LEDת����ϢID add by kzy 20090819
	unsigned short m_usNTICID;		//LED֪ͨ��ϢID add by kzy 20090819
	BYTE           m_bytLedPwrWaitTm; //LED�ϵ�ȴ�ʱ��
	BYTE           m_bytLedConfig;   //LED���ܶ��� 07.9.12 hong // 0 A1 A2 A3 A4 A5 A6 0
	//A6: 1: ��̨�ƶ���ֹʱ��LED���������ڿ��� 0�������п���
	//A5: 1:LED��ͨ�Ŵ���ʱ����ֹ���ع��     0��ͨ�Ŵ���ʱ��ֹ���ع��
	//A4��1������MD5                          0��������MD5
	unsigned short m_usLedBlkWaitTm; //������ֹ������ȴ�ʱ�䣬��λ����,Ĭ��Ϊ2����
	//#ifdef DELAYLEDTIME
	//	unsigned short m_usLedMenuID;   //��Ŀ��ID 07.9.30
	//#endif
	char			m_szLedVer[64]; // LED����汾��
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		
		m_usLedParamID = 0;
		m_bytLedPwrWaitTm = 5;
		m_bytLedConfig = 0;
		m_usLedBlkWaitTm = 60;
		m_usNTICID = 0;
		m_usTURNID = 0;
		
		//#ifdef DELAYLEDTIME
		//		m_usLedMenuID = 0;
		//#endif
	}
};


//�ֱ����� caoyi 07.11.29
struct tag2QHstCfg{
    char		m_bytcallring;
    char		m_bytsmring;
    bool		m_HandsfreeChannel;
    bool		m_EnableKeyPrompt;
    bool		m_EnableAutoPickup;
	//char		m_bytListenTel[3];	//�����绰�Ĵ���  //cyh add:del
	char		m_szListenTel[20];	//cyh add:add
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		
		m_HandsfreeChannel = false;
		m_EnableKeyPrompt = true;
		m_EnableAutoPickup = true;
	}
};

#undef MAX_DRIVER
#define MAX_DRIVER 10

// ��ʻ����, 090817 xun move here
struct DrvCfg 
{
	byte inited;		//�Ƿ��ʼ�����ı�־
	
	ushort factor;		//��������ϵ��(ÿ�����������)
	char vin[17];		//����VIN�ţ�ռ��1��17���ֽ�
	char num[12];		//�������룺ռ��1��12���ֽ�
	char type[12];		//���Ʒ��ࣺռ��1��12���ֽ�	
	
	byte total;			//˾������
	byte max_tire;		//���õ����ƣ��ʱ��(Сʱ)
	byte min_sleep;		//���õ���С��Ϣʱ��(����)
	
	struct Driver 
	{		
		bool valid;			//��Ч��־λ
		byte num;			//˾����ţ�ȡֵ��Χ0~126
		char ascii[7];		//��ݴ���(1~7)
		char code[18];		//��ʻ֤��(1~18)
	} inf[MAX_DRIVER];
	
	byte max_speed;		//�����ٶ�(����/ʱ)
	byte min_over;		//���ټ���ʱ��
	
	byte winsize;		//���ڴ�С
	byte sta_id;		//״̬��ʶ
	ushort uptime[6];	//�ϱ���ʱ��
	ushort cltcyc[6];	//��������
	ushort sndcyc[6];	//��������
	
	void init() {
		memset(this, 0, sizeof(*this));
		inited = 123; // ��ʼ����־���Ƚ����⣩
		winsize = 4;
		
		// 090229 xun: ����Ĭ�ϲ���
		uptime[0] = 0xffff; // �ƶ� (ʵ�ʴ���ʱ��ΪACC ON)
		cltcyc[0] = 60;
		sndcyc[0] = 1;
		uptime[1] = 0xffff; // ��ֹ (ʵ�ʴ���ʱ��ΪACC OFF)
		cltcyc[1] = 600;
		sndcyc[1] = 1;
		sta_id = 2; // 090817 xun add
		
		max_speed = byte(105 / 1.852);
		min_over = 20;
	}
};

struct tagSpecCtrlCfg
{
	bool			m_bOilCut;

	void Init()
	{
		m_bOilCut = false;
	}
};


//--------------------����ΪDvrExe����--------------------//

//����
struct tag2DPwdPar
{
	char	AdminPassWord[11];
	char	UserPassWord[11];
	char	m_szReserved[26]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

	  strcpy(AdminPassWord, "0123456789");
	  strcpy(UserPassWord, "0000000000");
	}
};

//�豸��Ϣ
struct tag2DDevInfoPar
{
	char	DriverID[13]; 
	char	CarNumber[13];
	char	SoftVersion[13]; 
	char	m_szReserved[25]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

	  strcpy(DriverID, "000000000000");
	  strcpy(CarNumber, "000000000000");
	  strcpy(SoftVersion, "MVR1600-V1.0");
	}
};

//������Ϣ
struct tag2DIpPortPar
{
	DWORD m_ulTcpIP;//ǰ�û�IP
	WORD  m_usTcpPort;//ǰ�û��˿�
	char	m_szTel[16];			//��������
	BYTE	m_uszsizeTel;			//�������볤��
	char	m_szAreaCode[2]; 	// �����(����ID)
	char	m_szReserved[31]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���

	void Init()
	{
		memset( this, 0, sizeof(*this) );

		strcpy(m_szTel, "13000000000");
		m_uszsizeTel = 11;
		m_ulTcpIP = inet_addr("59.61.82.170");
		m_usTcpPort = htons(9020);
		m_szAreaCode[0] = 0x27;
		m_szAreaCode[1] = 0x00;
	}
};

//����Ƶ��������
struct tag2DTrigPar 
{
	UINT TrigType;
	char m_szReserved[20]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		TrigType = 1;//TRIG_EVE;
	}
};

//ʱ�δ�������
struct tag2DPerTrigPar
{
	BOOL	PeriodStart[6];
	char	StartTime[6][9];
	char	EndTime[6][9];
	char	m_szReserved[60]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		int i;
		for(i = 0; i < 6; i++)
		{
			PeriodStart[i] = FALSE;
			strcpy(StartTime[i], "00:00:00");
			strcpy(EndTime[i], "23:59:59");
		}
	}
};

//�¼���������
struct tag2DEveTrigPar
{
	BOOL	AccStart, MoveStart, OverlayStart, S1Start, S2Start, S3Start, S4Start;
	UINT	AccDelay, MoveDelay, OverlayDelay, S1Delay, S2Delay, S3Delay, S4Delay;
	char	m_szReserved[40]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		AccStart = MoveStart = OverlayStart = S1Start = S2Start = S3Start = S4Start = FALSE;
		AccDelay = MoveDelay = OverlayDelay = S1Delay = S2Delay = S3Delay = S4Delay = 20;
	}
};

//��Ƶ�������
struct tag2DVIPar
{
	UINT	VMode;
	UINT	VNormal;	
	char	m_szReserved[16]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		VMode = 1;//CIF;
		VNormal = 0;//PAL;
	}
};

//��Ƶ�������
struct tag2DVOPar
{
	UINT  VMode;
	UINT  VNormal;
	char	m_szReserved[16]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		VMode = 1;//CIF;
		VNormal = 0;//PAL;
	}
};

//��ƵԤ������
struct tag2DVPrevPar
{
	BOOL  VStart[4];
	char	m_szReserved[16]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		int i;
		for(i = 0; i < 4; i++)
		{
			VStart[i] = TRUE;
		}
	}
};

//����Ƶ����¼�����
struct tag2DAVRecPar
{
	BOOL  AStart[4];
	BOOL  VStart[4];
	UINT 	VMode;
	UINT 	VBitrate[4];
	UINT 	VFramerate[4];
	char  DiskType[10];
	char	m_szReserved[34]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		strcpy(DiskType, "SDisk");
		VMode = 1;//CIF;
		
		int i;
		for(i = 0; i < 4; i++)
		{
			AStart[i] = TRUE;
			VStart[i] = TRUE;
			VBitrate[i] = 409600;//BITRATE_REC_3;
			VFramerate[i] = 20;
		}
	}
};

//����Ƶ�����ϴ�����
struct tag2DAVUpdPar
{
	BOOL  AStart[4];
	BOOL  VStart[4];
	UINT 	VMode;
	UINT 	VBitrate[4];
	UINT 	VFramerate[4];
	int 	UploadType;
	int 	UploadSecond;
	char	m_szReserved[36]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		VMode = 2;//QCIF;
		UploadType = 0;
		UploadSecond = 300;
		
		int i;
		for(i = 0; i < 4; i++)
		{
			AStart[i] = FALSE;
			VStart[i] = FALSE;
			VBitrate[i] = 16000;//BITRATE_UPL_0;
			VFramerate[i] = 10;
		}
	}
};

//����Ƶ����طŲ���
struct tag2DAVPbPar
{
	BOOL  AStart[4];
	BOOL  VStart[4];
	UINT	DecNum;
	//	UINT	VdecPlaybackMode[4];
	UINT  VFramerate[4];
	char	CreatDate[11];
	char	EncChn[4][2];
	char	AFilePath[4][50];
	char	VFilePath[4][50];
	char	IFilePath[4][50];
	char	m_szReserved[73]; // ����Ԥ���ռ�,��ʹ�������ṹ��ߴ�Ϊ8�ı���
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		int i;
		for(i = 0; i < 4; i++)
		{
			AStart[i] = FALSE;
			VStart[i] = FALSE;
			VFramerate[i] = 0;
			memset(EncChn[i], 0, sizeof(EncChn[i]));
			memset(AFilePath[i], 0, sizeof(AFilePath[i]));
			memset(VFilePath[i], 0, sizeof(VFilePath[i]));
			memset(IFilePath[i], 0, sizeof(IFilePath[i]));
		}
		DecNum = 1;
		memset(CreatDate, 0, sizeof(CreatDate));
	}
};

//DVR���ò���
struct tag2DDvrCfg
{
	tag2DPwdPar	    PasswordPara;
	tag2DDevInfoPar	DeviceInfo;
	tag2DIpPortPar	IpPortPara;
	tag2DTrigPar		TrigPara;
	tag2DPerTrigPar	PeriodTrig;
	tag2DEveTrigPar EventTrig;
	tag2DVIPar			VInput;
	tag2DVOPar 			VOutput;
	tag2DVPrevPar		VPreview;
	tag2DAVRecPar		AVRecord;
	tag2DAVUpdPar		AVUpload;
	tag2DAVPbPar		AVPlayback;
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		PasswordPara.Init();
		DeviceInfo.Init();
		IpPortPara.Init();
		TrigPara.Init();
		PeriodTrig.Init();
		EventTrig.Init();
		VInput.Init();
	 	VOutput.Init();
		VPreview.Init();
		AVRecord.Init();
		AVUpload.Init();
		AVPlayback.Init();
	}
};

//--------------------����ΪDvrExe����--------------------//


// ����20070914����,��Ϊ������Ӧ����ҵ����зֿ�,����ÿ���СӦ��80��������
struct tagSecondCfg// ע��ṹ���ڴ����
{
	char		m_szSpecCode[80];

	// ��ý������
	union
	{
		char		m_szReserved[80];
		tag2LMediaCfg m_obj2LMediaCfg;
	} m_uni2LMediaCfg;

	// ��ϻ�ӱ�������
	union
	{
		char		m_szReservered[80];
		tag2QBlkCfg	m_obj2QBlkCfg;
	} m_uni2QBlkCfg;

	// ��������
	union
	{
		char		m_szReservered[80];
		tag2QAlertCfg	m_obj2QAlertCfg;
	} m_uni2QAlertCfg;

	// ʵʱ��λ��������
	union
	{
		char		m_szReservered[560];
		tag2QRealPosCfg	m_obj2QRealPosCfg;
	} m_uni2QRealPosCfg;

	// GPRS��������
	union
	{
		char		m_szReservered[160];
		tag2QGprsCfg	m_obj2QGprsCfg;
	} m_uni2QGprsCfg;

	// ���ķ����������
	union 
	{
		char		m_szReservered[560];
		tag2QServCodeCfg	m_obj2QServCodeCfg;
	} m_uni2QServCodeCfg;

	// ��������
	union
	{
		char		m_szReservered[480];
		tag2QPhotoCfg	m_obj2QPhotoCfg;
	} m_uni2QPhotoCfg;
	
	// LED����
	union
	{
		char		m_szReservered[160];
		tag2QLedCfg m_obj2QLedCfg;
	}m_uni2QLedCfg;
	
	//�ֱ�����
	union
	{
		char		m_szReservered[80];
		tag2QHstCfg	m_obj2QHstCfg;
	}m_uni2QHstCfg;

	// ���������������
	union
	{
		char		m_szReservered[880];
		tag2QFbidTelCfg m_obj2QFbidTelCfg;
	} m_uni2QFbidTelCfg;
	
	// �绰������
	union
	{
		char		m_szReservered[960];
		tag2QCallCfg	m_obj2QCallCfg;
	} m_uni2QCallCfg;

	// ��ʻ��¼�ϱ�����
	union
	{
		char		m_szReservered[80];
		tag2QDriveRecCfg	m_obj2QDriveRecCfg;
	} m_uni2QDriveRecCfg;

	// ��ʻԱ�������
	union
	{
		char		m_szReservered[480];
		tag2QDriverCfg		m_obj2QDriverCfg;
	} m_uni2QDriverCfg;

	// ʵʱָ������
	union
	{
		char		m_szReservered[3200];
		tag2QRealNavCfg		m_obj2QRealNavCfg;
	} m_uni2QRealNavCfg;
	

	/// { JG2�ɵ���ʽ��¼��ģ������
	// �绰�� (JG2)
	union
	{
		char		m_szReservered[1600];	//�Ѿ�ʹ��1245
		
	} m_uniBookCfg;
	
	// �г���¼�� (JG2)
	union
	{
		char		m_szReservered[480];	//�Ѿ�ʹ��318	
	} m_uniDrvCfg;
	/// } JG2�ɵ���ʽ��¼��ģ������

	union
	{
		char		m_szReservered[80];
		tagSpecCtrlCfg m_obj2QSpecCtrlCfg;
	} m_uni2QSpecCtrlCfg;

	// Dvr����
	union
	{
		tag2DDvrCfg m_obj2DDvrCfg;
		char		m_szReservered[1760]; //�Ѿ�ʹ��1530����
	}m_uni2DDvrCfg;

	void InitDefault()
	{
		memset(this, 0, sizeof(*this));
		strncpy( m_szSpecCode, CFGSPEC, sizeof(m_szSpecCode) - 1 );

		m_uni2LMediaCfg.m_obj2LMediaCfg.Init();
		m_uni2QBlkCfg.m_obj2QBlkCfg.Init();
		m_uni2QAlertCfg.m_obj2QAlertCfg.Init();
		m_uni2QRealPosCfg.m_obj2QRealPosCfg.Init();
		m_uni2QGprsCfg.m_obj2QGprsCfg.Init();
		m_uni2QServCodeCfg.m_obj2QServCodeCfg.Init();
		m_uni2QPhotoCfg.m_obj2QPhotoCfg.Init();
		m_uni2QLedCfg.m_obj2QLedCfg.Init();
		m_uni2QHstCfg.m_obj2QHstCfg.Init();
		m_uni2QFbidTelCfg.m_obj2QFbidTelCfg.Init();
		m_uni2QCallCfg.m_obj2QCallCfg.Init();
		m_uni2QDriveRecCfg.m_obj2QDriveRecCfg.Init();
		m_uni2QDriverCfg.m_obj2QDriverCfg.Init();
		m_uni2QRealNavCfg.m_obj2QRealNavCfg.Init();
		m_uni2QSpecCtrlCfg.m_obj2QSpecCtrlCfg.Init();

 		memset(m_uniBookCfg.m_szReservered, 0, sizeof(m_uniBookCfg.m_szReservered));
 		memset(m_uniDrvCfg.m_szReservered, 0, sizeof(m_uniDrvCfg.m_szReservered));

		// 090817 xun add
		DrvCfg objDrvCfg;
		objDrvCfg.init();
		memcpy( m_uniDrvCfg.m_szReservered, &objDrvCfg, sizeof(objDrvCfg) );

		m_uni2DDvrCfg.m_obj2DDvrCfg.Init();
	}
};

#pragma pack()

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

int DataPush( void* v_pPushData, DWORD v_dwPushLen, DWORD v_dwPushSymb, DWORD v_dwPopSymb, BYTE v_bytLvl ); // �˷�����ͬʱָ�����PopSymb,�����ն���
int DataPop( void* v_pPopData, DWORD v_dwPopSize, DWORD *v_p_dwPopLen, DWORD *v_p_dwPushSymb, DWORD v_dwPopSymb, BYTE *v_p_bytLvl );
int DataDel( DWORD v_dwPopSymb, void* v_pCmpDataFunc );
int DataWaitPop( DWORD v_dwPopSymb );
int DataSkipWait( DWORD v_dwPopSymb );

int GetImpCfg( void *v_pDes, unsigned int v_uiDesSize, unsigned int v_uiGetBegin, unsigned int v_uiGetSize );
int SetImpCfg( void *v_pSrc, unsigned int v_uiSaveBegin, unsigned int v_uiSaveSize );
int GetSecCfg( void *v_pDes, unsigned int v_uiDesSize, unsigned int v_uiGetBegin, unsigned int v_uiGetSize );
int SetSecCfg( void *v_pSrc, unsigned int v_uiSaveBegin, unsigned int v_uiSaveSize );
int ResumeCfg( bool v_bReservTel );

int IOGet( unsigned char v_ucIOSymb, unsigned char *v_p_ucIOSta );
int IOSet( unsigned char v_ucIOSymb, unsigned char v_ucIOSta,  void* v_pReserv, unsigned int v_uiReservSiz );
int IOCfgGet( unsigned char v_ucIOSymb, void *v_pCfg, DWORD v_dwCfgSize );
int IOIntInit( unsigned char v_ucIOSymb, void* v_pIntFunc ); // ��v_ucIOSymbֵΪ255,���ʾ����ȫ������IO���ж�
//int IOIntFuncSet( unsigned char v_ucIOSymb, void* v_pIntFunc ); // ��v_ucIOSymbֵΪ255,���ʾ����ȫ��IO�Ļص�����

int GetCurGps( void* v_pGps, const unsigned int v_uiSize, BYTE v_bytGetType );
int SetCurGps( const void* v_pGps, BYTE v_bytSetType );

bool GetDevID( char* v_szDes, unsigned int v_uiDesSize );
bool GetAppVer( char* v_szDes, unsigned int v_uiDesSize );

int SetCurTime( void* v_pTmSet ); // �������Ӧ��tm���Ͷ����ָ�� (UTCʱ��)
unsigned int GetTickCount(); // ����ֵ: ��1970.1.1�賿������ʱ��,ms

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
