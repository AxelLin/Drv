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

// 各版本号,在归档、发布给客户使用等情况后，要注意版本号的升级
#define CHK_VER			0	// 0,发布版本; 1,生产检测版本
#define USE_PROTOCOL	0	// 0,千里眼通用版协议; 1,千里眼研三公开版协议; 2,千里眼研一公开版协议(即公司公开版); 30,两客通用版协议
#define USE_LANGUAGE	0	// 0,简体中文; 11,英文
#define RESET_ENABLE_ALL  1  //控制应用程序收到NK的复位消息是否自杀。 0 不自杀; 1 自杀 (正式版本要用自杀)

#define CFGSPEC	("YXYF3_产品大麦DVR前途无限_2009") // 字节数应小于配置中的存储容量,永不更改
#if CHK_VER == 0
	#define VER_SFT		"app-yx001-160vw01-001"
#else
	#define VER_SFT		"app-t001-160vw01-001"
#endif


///////// { <1>通用编译开关 (下述开关,都应根据需要来具体控制)

// { 公共开关
#define USE_TELLIMIT	0		// 0,千里眼中心呼入呼出限制; 1,千里眼中心电话本控制; 2,两客中心电话本控制; 3,研一公开版电话本控制
#define USE_TTS			1		// 1,TTS语音支持; 0,不支持
#define USE_METER		0		// 1,支持税控,计价器与车台之间用串口通信,包括空重车报告; 0,不支持税控
#define JIJIA_HIGHHEAVY			// 打开此开关,表示在调用DeviceIoControl查询空重载信号时,若得到的值(即光耦转换后的信号)为1，表示重车(但此时计价器直接输出的信号是低电平)
#define LOWPOWAD		240		// 欠压AD临界值: 12v车,240; 24v车,暂定480 (不同车型注意修改!!)
#define CP_CHS			936
#define USE_SOCKBINDIP	0		// 0,socket不绑定(正式版本使用); 1,绑定GPRS分配的IP(在调试时使用)

#define GPS_TEST			0	// 1, 打开GPS测试功能1; 2,打开GPS测试功能2; 0,关闭
#define USE_UBUNTU_DEBUG	0	// 1, 支持使用Ubentu进行调试；0，不支持
#define GPSLOG_PATH		L"\\Resflh4"
#define GPSLOG_FILENAME L"GpsLog.txt"

// 注意：必须完全一致
#define DIR2NAME_C		("\\Resflh2\\")		// (char型)
#define DIR2NAME_W		(L"\\Resflh2\\")	// (wchar_t型)
#define DIR2NAME_W_2	(L"\\Resflh2")		// (wchar_t型)
#define DIR4NAME_W		(L"\\Resflh4")		// (wchar_t型)
// } 公共开关

// QianExe使用
#define UDP_ONLY		0		// 1,只使用UDP通信; 0,即使用UDP,又使用TCP
#define USE_BLK			0		// 1,支持黑匣子; 0,不支持
#define USE_AUTORPT		0		// 1,支持主动上报(0154); 0,不支持
#define USE_DRVREC		0		// 1,使用JG2旧的行驶记录仪模块; 0,不使用
//【 若USE_DRVREC为1，则这几条不要启用
#define USE_DRVRECRPT	0		// 1,3745行驶记录上传; 2,3746; 3,3747; 4,3748; 11,3745_2客; 0,不支持	(1和4暂不支持)
#define USE_ACDENT		0		// 1,事故疑点,千里眼中心协议; 2,2客中心协议; 0,不支持
#define USE_DRIVERCTRL	0		// 1,使用司机管理业务; 0,不使用
// 】若USE_DRVREC为1，则这几条不要启用
#define USE_OIL			0		// 1,油耗数据使用IO直接读取; 2,使用油耗检测盒,并使用研一算法; 0,不支持
#define USE_REALNAV		0		// 1,启用实时指引(目前仅在2客系统中用到); 0,不启用
#define USE_JIJIA		0		// 1,支持计价器模块(包括串口触发和IO触发,但与税控功能无关); 0,不支持
#define USE_REALPOS		0		// 1,支持实时定位模块; 0,不支持
#define USE_DISCONNOILELEC 0	// 1,开启断油断电; 0,不开启
#define USE_LEDTYPE		0		// 1,使用博海LED屏; 2,使用青岛三杰LED; 0,不使用LED
#define USE_HELPAUTODIAL 0		// 1,求助时自动拨打中心的求助电话（需保证求助电话设置有效）; 0,不拨打
//#define DIS_SPD_USEIO			// 打开此开关,行驶记录模块CDrvrecRptDlg中的里程和速度,使用IO(里程脉冲)信号计算
#define UPLOAD_AFTERPHOTOALL 0	// 0,拍照1张即上传1张; 1,全部拍照完毕才开始上传
#define USE_PHOTOUPLD	0		// 0,使用窗口方式上传协议; 1,使用一次性上传完整照片协议
#define USE_PHOTOTHD	1		// 1,拍照处理使用单独子线程; 0,不使用

// 以下开关要小心使用
#define USE_EXCEPTRPT	0		// 1,异常情况发送报告短信; 0,不支持(注意: 发布版本必须是0)
//#define LOGSOCKDATA			// 打开此开关，则Socket收发数据打印到调试串口 (注意：发布版本必须关闭)
//#define PHOTO_INVERSE			// 打开此开关，表示照片需要上下左右颠倒处理
//#define FOOTALERM_NORMOLOPEN	// 打开此开关,表示使用常开劫警开关，关闭即使用常闭报警开关（目前只有信息港版本使用常开开关）
//#define LOGSAVEFILE			// 界面显示日志存为文件（正式版本必须关闭）
//#define PICOLDVER				// 照片传输使用旧协议，已不用
//#define OLDIO					// 旧的内核驱动，已不用

#if USE_DRVREC == 1
#define CAR_RUNNING			0
#define CAR_STOP_RUN		1
#define CAR_RUN_STOP		2
#define CAR_STOP			3
#define UnLog				0 
#define Logined				1
#define Inputing			2
#define MAX_DRIVER			10		//最大司机数
#define MAX_PROMPT			3		//最大司机提醒次数
#define CONTINUE_TIME		3		//行驶记录的最短行驶时间(小时),低于此值不保存
#define VT_LOW_RUN			2		//判断车辆停止的最小速度(海里/时)
#define RUN_MINTIME			5		//判断车辆移动的最小时间(秒)
#define STOP_MINTIME		20		//判断车辆停止的最小时间(秒)
#define PATH				"\\ResFlh2\\DrvJg2"
#define SND_TYPE 1  // 1,每帧发送,满一个窗口等待应答   2,积满一个窗口才发送;
#endif



// ComuExe使用
// { 手机模块选择
#define NETWORK_YIDONG	66
#define NETWORK_LIANTONG	88
#define NETWORK_GPRS 0
#define NETWORK_CDMA 1
#define NETWORK_TD	2

#define NETWORK_TYPE  NETWORK_GPRS
#define NETWORK_GPRSTYPE NETWORK_YIDONG	// NETWORK_LIANTONG
//注意: 如果是CDMA模块要选择模块的类型
#define CDMA_EM200 1
#define CDMA_CM320 2
#define CDMA_TYPE CDMA_EM200
// } 手机模块选择

#define GPSVALID_FASTFIXTIME 1	// 1，表示一旦GPS有效则立刻校时；0，连续有效约15分钟后才校时
#define USE_IRD			0		// 1,表示使用红外语音,不使用手柄和调度屏(此时应确保启用TTS); 0,不使用红外语音,使用手柄或调度屏(自适应)
//#define OUTPUT_GPS			// 打开此定义，收到的GPS数据从调试串口输出 (正式版本时关闭)
//#define GPSGSA_ENB			// 是否读取GSA格式数据 (PDOP、HDOP、VDOP)
#define USE_PING		0		// 是否使用PING的机制 1:使用 0:不使用
#if GPS_TEST && !defined(GPSGSA_ENB)
#define GPSGSA_ENB
#endif


// StreamMedia使用
#define SCREENMOD		true	// 开机模式选择: true,正常模式(即播放广告才开屏); false,工程模式(即启动时就开屏)
#define IMGSHOW			0		// 是否支持播放广告前显示司机照片:	0,不支持; 1,支持(目前只有VM1的NK支持)
///////// } <1>通用编译开关


#define NONE		0
#define TCPMP		1
#define WMEDIA		2

#define UDISK_NAME		(L"UDisk")
#define NO_CARID		1		// 1:禁止烧写ID		0:可以烧写ID
#define USE_SERIAL_GPS			// 是否使用物理串口读取GPS数据
#define VEDIO			NONE	// 支持的视频模块,NONE,TCPMP,WMEDIA
#define PHOTOCHK_TYPE	2		// 1,摄像头检测用LCD显示; 2,检测照片输出到U盘; 3,检测同步信号; 0,不支持
#define USE_UDISK       1

#define DISK2_SIZETYPE	1		// 1,10M; 2,30M;
#define DISK3_SIZETYPE	1		// 1,60M; 2,80M;

const DWORD DISK2_CUSIZE = 1024;// 分区2的簇大小，每个文件的占用空间大小，应该是簇的整数倍
const DWORD DISK3_CUSIZE = 512;	// 分区3的簇大小

/// } KJ2、JG2、V7开关



/// { 部分配置初始化定义,仅用于配置区默认配置,且仅用于各配置对象的Init方法中,不同的版本都要注意修改!
/// !!注意是否还有其他要修改的默认配置
#define APN			"CMNET"				// APN设置（一般不用改）
#define CDMA_NAME   "card"
#define CDMA_PASS   "card"

#define QTCPIP		"125.77.254.122"	// 千里眼TCP IP地址
#define QTCPPORT	9100				// 千里眼TCP端口
#define LUDPIP		"59.61.82.173"		// 流媒体(或远程升级中心)UDP IP地址
#define LUDPPORT	18000				// 流媒体(或远程升级中心)UDP端口
#define DUDPIP		"59.61.82.172"
#define DUDPPORT	11101				
#if USE_PROTOCOL == 0 || USE_PROTOCOL == 1 || USE_PROTOCOL == 2
#define QUDPIP		"125.77.254.122"	// 千里眼UDP IP地址 // 59.61.82.170
#define QUDPPORT	9100				// 千里眼UDP端口 // 6660
#endif
#if USE_PROTOCOL == 30
#define QUDPIP		"59.61.82.170"		// 千里眼UDP IP地址 // 59.61.82.170
#define QUDPPORT	6660				// 千里眼UDP端口 // 6660
#endif

#define SMSCENTID	""					// 短信中心号,注意"+86"不能丢 // +8613800592500
#define SMSSERVCODE	""					// 特服号
#define CENTID1		0x35		// 中心ID(即区域号)第一字节,注意使用16进制
#define CENTID2		0x00		// 中心ID(即区域号)第二字节,注意使用16进制,0无需转义为7f

#define USERIDENCODE "111111"	// 公开版_研一协议中的GPRS登陆帧中的用户识别码(6字符),每个定制版本都应有自己固定的识别码

#define EXCEP_RPTTEL "15985886953"		// 异常报告短信的目的手机号
/// } 部分配置初始化定义,仅用于配置区默认配置,且仅用于各配置对象的Init方法中,不同的版本都要注意修改!

enum QUEELVL // 必须从1开始递增,有且只有16个,越大越优先
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

	ERR_SMS_DESCODE, // 短信发送目的方手机号不正确

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
#define VALIDMONTH_BEGIN	10 // 在VALIDYEAR_BEGIN中的month

#define TCP_RECVSIZE	256
#define TCP_SENDSIZE	1024
#define UDP_RECVSIZE	1500
#define UDP_SENDSIZE	1500
#define SOCK_MAXSIZE	1500
#define COMUBUF_SIZE	2048
#define UDPDATA_SIZE	1200 // UDP数据报中数据内容的最大长度, 也即照片数据帧一个包的大小限定值
// !!注意,该值应小于1500,且小于SOCKSND_MAXBYTES_PERSEC-QIANUPUDPFRAME_BASELEN

#define MAX_UNSIGNED_64		((unsigned long long)(-1))
#define MAX_UNSIGNED_32		((unsigned long)(-1))
#define MAX_UNSIGNED_16		((unsigned short)(-1))
#define MAX_UNSIGNED_8		((unsigned char)(-1))

const double BELIV_MAXSPEED = 100 * 1.852 / 3.6; // 单位:米/秒

const BYTE GPS_REAL		= 0x01;
const BYTE GPS_LSTVALID	= 0x02;

const WORD PHOTOEVT_FOOTALERM		= 0x8000; // 劫警
const WORD PHOTOEVT_OVERTURNALERM	= 0x4000; // 侧翻报警
const WORD PHOTOEVT_BUMPALERM		= 0x2000; // 碰撞报警
const WORD PHOTOEVT_FOOTALERM_BIG	= 0x1000; // 劫警时附加的大照片,但并不立即上传
const WORD PHOTOEVT_TRAFFICHELP		= 0x0800; // 交通求助
const WORD PHOTOEVT_MEDICALHELP		= 0x0400; // 医疗求助
const WORD PHOTOEVT_BOTHERHELP		= 0x0200; // 纠纷求助
const WORD PHOTOEVT_CLOSEDOOR		= 0x0100; // 关车门
const WORD PHOTOEVT_AREA			= 0x0080; // 进出区域
const WORD PHOTOEVT_ACCON			= 0x0040; // ACC有效
const WORD PHOTOEVT_DRIVEALERM		= 0x0020; // 非法发动车辆报警拍照
const WORD PHOTOEVT_CHK				= 0x0010; // 生产检测事件
const WORD PHOTOEVT_JIJIADOWN		= 0x0008;
const WORD PHOTOEVT_JIJIAUP			= 0x0004;
const WORD PHOTOEVT_TIMER			= 0x0002; // 定时拍照
const WORD PHOTOEVT_CENT			= 0x0001; // 中心抓拍

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


/// 如下定义,二进制为1的bit依次左移 !!!
const DWORD DEV_SOCK	= 0x00000001;
const DWORD DEV_LED		= 0x00000002;
const DWORD DEV_QIAN	= 0X00000004;
const DWORD DEV_DVR		= 0x00000008;
const DWORD DEV_PHONE	= 0x00000010;
const DWORD DEV_DIAODU	= 0x00000020;
const DWORD DEV_LIU		= 0x00000040;
const DWORD DEV_IO		= 0x00000080; // 不需要接收队列
const DWORD DEV_GPS		= 0x00000100; // 不需要接收队列


// 必须与SYSIO_CFG[]的定义顺序一致!且必须从0开始编号,注意编号取值范围: 0~254 (255已作为函数特殊参数使用)
enum IOSymb
{
	// 输入
	IOS_ACC = 0,
	IOS_ALARM, //劫警信号
	IOS_JIJIA, // 空重载
	IOS_FDOOR, //前门信号
	IOS_BDOOR, //后门信号
	IOS_JIAOSHA,
	IOS_SHOUSHA,
	IOS_PASSENGER1, //载客信号1
	IOS_PASSENGER2, //载客信号2
	IOS_PASSENGER3, //载客信号3
	
	
	// 输出
	IOS_GPS,
	IOS_HUBPOW,
	IOS_PHONEPOW,
	IOS_PHONERST,
	IOS_HARDPOW, //硬盘电源
	IOS_TRUMPPOW, //功放电源
	IOS_SCHEDULEPOW, //调度屏电源
	IOS_VIDICONPOW, //摄像头/TW2835电源
	IOS_OILELECCTL, //断油电控制
	IOS_APPLEDPOW, //程序工作指示灯
	IOS_SYSLEDPOW, //系统工作指示灯
	IOS_TOPLEDPOW, //顶灯电源
	IOS_AUDIOSEL, //手机音频/PC音频选择
	IOS_EARPHONESEL, //耳机/免提选择
	IOS_TW2835RST, //TW2835复位
};

// IO应用意义定义,针对外部IO信号,非CPU管脚信号
enum IO_APP
{
	// 输入
	IO_ACC_ON = 1, // 必须从1开始,因为内部数组用的是unsigned char,节约空间
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
	
	
	// 输出
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
	IO_AUDIOSEL_PHONE, //选择手机音频
	IO_AUDIOSEL_PC, //选择PC音频
	IO_EARPHONESEL_ON,//选择耳机
	IO_EARPHONESEL_OFF,//选择免提
	IO_TW2835RST_HI,
	IO_TW2835RST_LO,
};

struct tagIOCfg
{
	unsigned int m_uiPinNo; // 管脚编号
	unsigned int m_uiContPrid; // 中断信号持续时间(使能中断时才有意义),去抖用,ms
	unsigned char m_ucInOut;	// 0,输入; 1,输出
	bool m_bBreak;	// true,使能中断(输入时才有意义); false,禁止中断
	char m_cBreakType; // 中断方式(使能中断时才有意义): 1,电平触发; 2,单边沿触发; 3,双边沿触发
	char m_cBreakPow; // 中断触发电平(在电平触发和单边沿触发时才有意义): 0, 低电平或下降沿触发; 1,高电平或上升沿触发
	unsigned char m_ucPinLowApp; // 管脚低电平的应用意义
	unsigned char m_ucPinHigApp; // 管脚高电平的应用意义
};


#pragma pack(8)

typedef struct tagGPSData
{
	double		latitude;         //纬度0－90
	double		longitude;        //经度0－180
	float		speed;           //速度，米/秒
	float		direction;        //方向，0－360
	float		mag_variation;    //磁变角度，0－180
	int			nYear; // 4个数字表示的值,如2007,而不是7;
	int			nMonth;
	int			nDay;
	int			nHour;
	int			nMinute;
	int			nSecond;
	char		valid;             //定位数据是否有效
	BYTE		la_hemi;          //纬度半球'N':北半球，'S':南半球
    BYTE		lo_hemi;           //经度半球'E':东半球，'W':西半球
	BYTE		mag_direction;    //磁变方向，E:东，W:西
	BYTE		m_bytMoveType; // 1,移动; 2,静止
	BYTE		m_bytPDop; // GPS误差因子×10,下同
	BYTE		m_bytVDop;
	BYTE		m_bytHDop;
	char		m_cFixType; // 定位类型: '1'-未定位; '2'-2D定位; '3'-3D定位
	
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
		// 初始化不同,dll的处理可能不同,注意!
		memset( this, 0, sizeof(*this) );
		nYear = VALIDYEAR_BEGIN, nMonth = VALIDMONTH_BEGIN, nDay = 15;
		valid = 'V', m_cFixType = '1';
	}
}GPSDATA;

#pragma pack()


#pragma pack(8)

/// 命名规则:
// (1)属于重要配置的,前缀用"1";属于一般配置的,前缀用"2"
// (2)公共配置,前缀用"P";流媒体配置,前缀用"L";千里眼配置,前缀用"Q"
// (3)一级配置定义,用"Cfg"结尾; 二级配置定义,用"Par"结尾

/// 以下的电话号码,若无特殊说明,不满都是补空格

struct tag1PComuCfg
{
	char		m_szDialCode[33];
	char		m_szSmsCentID[22];	// 短信中心号,不满填0
	char		m_szTel[16];		// 车台手机号,以NULL结尾
	int			m_iRegSimFlow;		// 注册SIM卡流程
	byte		m_PhoneVol;			// 电话音量
//	char		m_szUserIdentCode[6]; // 用户识别码	
	
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
	char	m_szVeIDKey[20]; // 实际已不用,已单独另外放置
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_ulLiuIP = inet_addr(LUDPIP); 
		m_usLiuPort = htons(LUDPPORT);
	}
};

struct tag1LMediaCfg
{
	BYTE		m_bytPlayTimes; // 流媒体播放次数
	//unsigned long m_aryDriverID[5]; // 本车注册司机ID
	void Init()
	{
		memset( this, 0, sizeof(*this) );
		m_bytPlayTimes = 1;
	}
};

struct tag1QIpCfg
{
	unsigned long m_ulQianTcpIP; // (inet_addr之后的,下同)
	unsigned long m_ulLiuUdpIP;
	unsigned long m_ulDvrUdpIP;
	unsigned short m_usQianTcpPort; // (htons之后的，下同) 
	unsigned short m_usLiuUdpPort;
	unsigned short m_usDvrUdpPort;
	char		m_szApn[20]; // 以NULL结尾

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
	WORD		m_wCyclePerKm; // 每公里脉冲数
	//HXD 081120 油耗相关的参数。
	BYTE		m_bReverse;
	BYTE	    m_bFiltrate;
	BYTE	    m_bytHiVal;
	BYTE		m_bytLoVal;

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

// 根据20070914讨论,认为配置区应按照业务进行分块,并且每块大小应是40的整数倍
struct tagImportantCfg
{
	char		m_szSpecCode[40];

	// 公共通信参数
	union
	{
		char		m_szReserved[160];
		tag1PComuCfg m_obj1PComuCfg;
	} m_uni1PComuCfg;

	// 流媒体通信参数
	union
	{
		char		m_szReserved[160];
		tag1LComuCfg m_obj1LComuCfg;
	} m_uni1LComuCfg;

	// 流媒体播放配置
	union
	{
		char		m_szReserved[120];
		tag1LMediaCfg m_obj1LMediaCfg;
	} m_uni1LMediaCfg;

	// 千里眼IP
	union
	{
		char		m_szReserved[200];
		tag1QIpCfg	m_ary1QIpCfg[3];
	} m_uni1QComuCfg;

	// IO数据方面的标定值
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
	DWORD		m_dwSysVolume; // 系统音量
	byte        m_bytInvPhoto; //090611 是否要颠倒照片
	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_dwSysVolume = 75;
	}
};

struct tag2QBlkCfg
{
	DWORD			m_dwBlackChkInterv; // 单位:秒

	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_dwBlackChkInterv = 30; // 安全起见
	}
};

struct tag2QRealPosCfg // 432
{
	double	m_dLstLon;
	double	m_dLstLat;
	double	m_dLstSendDis; // 上次发送数据之后累积行驶的距离 (米)

	float	m_fSpeed; // 米/秒, 0表示无效

	DWORD	m_dwLstSendTime; // 上次发送时刻

	long	m_aryArea[16][4]; // 每个范围依次是: 左下经度、左下纬度、右上经度、右上纬度 (都是10的-6次方度)
	DWORD	m_aryAreaContCt[16]; // 满足条件的计数 (元素个数与上面一致)
	BYTE	m_aryAreaProCode[16]; // 各范围的编号 (元素个数与上面一致)
	BYTE	m_bytAreaCount;

	BYTE	m_bytGpsType;
	WORD	m_wSpaceInterv; // 公里,0表示无效
	WORD	m_wTimeInterv; // 分钟,0表示无效

	char	m_szSendToHandtel[15];

	BYTE	m_aryTime[10][3]; // 每个时刻依次是: 序号,时,分
	BYTE	m_bytTimeCount;

	BYTE	m_aryStatus[6];
	// 固定为6个, BYTE	m_bytStaCount; 

	BYTE	m_bytCondStatus; // 0	1	I	T	V	A	S	0
						//	Ｉ：关闭/开启间隔条件，0-关闭，1-开启
						//	Ｔ：关闭/开启时刻条件，0-关闭，1-开启
						//	Ｖ：关闭/开启速度条件，0-关闭，1-开启
						//	Ａ：关闭/开启范围条件，0-关闭，1-开启
						//	Ｓ：关闭/开启状态条件，0-关闭，1-开启

	BYTE	m_bytLstTimeHour; // 上次符合时刻条件的时刻,初始化为无效值——0xff,
	BYTE	m_bytLstTimeMin;

	void Init()
	{
		memset(this, 0, sizeof(*this));
		m_bytLstTimeHour = m_bytLstTimeMin = 0xff;
		m_wTimeInterv = 0, m_wSpaceInterv = 0, m_bytCondStatus = 0x40;
	}
};

struct tag2QGpsUpPar // 保存在文件中
{
	WORD		m_wMonSpace; // 监控定距距离 （单位：10米）
	WORD		m_wMonTime; // 监控时间 （分钟），0xffff表示永久监控
	WORD		m_wMonInterval; // 监控间隔 （秒），至少为1
	BYTE		m_bytStaType; // 注意: 按照协议里的状态定义
	BYTE		m_bytMonType; // 协议中定义的监控类型

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
	BYTE		m_bytGprsOnlineType; // 在线方式
	BYTE		m_bytOnlineTip; // 在线方式提示
	BYTE		m_bytGpsUpType; // GPS数据上传标识 1,主动发送0154帧；2,不主动发送
	BYTE		m_bytGpsUpParCount; // GPS数据参数个数
	BYTE		m_bytChannelBkType_1; // 短信备份设置1
		// 0	H1	H2	H3	H4	H5	H6	1
		// H1: 监控, H2: 实时定位, H3: 位置查询, H4: 报警, H5: 求助, H6: 调度
	BYTE		m_bytChannelBkType_2; // 短信备份设置2
		// 0	L1	L2	L3	L4	L5	L6	1
		// L1: 黑匣子, L2: 税控传输

	BYTE		m_bytRptBeginHour;	// 主动上报时间段,若开始时间==结束时间,则表示无时间段限制
	BYTE		m_bytRptBeginMin;
	BYTE		m_bytRptEndHour;
	BYTE		m_bytRptEndMin;

	void Init()
	{
		memset( this, 0, sizeof(*this) );

		// 关闭所有短信备份，短信备份功能已在程序里固定，并注意中心修改短信备份的指令要屏蔽
		m_bytChannelBkType_1 = m_bytChannelBkType_2 = 0;
		
		m_bytGpsUpType = 2;
		for( unsigned int ui = 0; ui < sizeof(m_aryGpsUpPar) / sizeof(m_aryGpsUpPar[0]); ui ++ )
			m_aryGpsUpPar[ui].Init();
	}
};

struct tag2QServCodeCfg
{
	char		m_szAreaCode[2]; // 区域号(中心ID)
	char		m_szQianSmsTel[15]; // 短信特服号(报警特服号),不满填0
	char		m_szTaxHandtel[15]; // 税控特服号
	char		m_szDiaoduHandtel[15]; // 调度特服号
	char		m_szHelpTel[15]; // 求助号码
	
	BYTE		m_bytListenTelCount;
	char		m_aryLsnHandtel[8][15]; // 中心主动发起监听的电话 (协议里定义是最多为5个)

	BYTE		m_bytCentServTelCount;
	char		m_aryCentServTel[5][15]; // 中心服务电话

	BYTE		m_bytUdpAtvTelCount; // UDP激活电话号码
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
	char		m_aryLsnHandtel[8][15]; // 中心主动发起监听的电话 (协议里定义是最多为5个)

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
	BYTE		m_bytFbidType; // 和协议里的定义方式一致
	char		m_aryFbidInHandtel[12][15]; // 禁止呼入的电话 (协议里定义是最多为8个)
	char		m_aryAllowInHandtel[12][15]; // 允许呼入的电话
	char		m_aryFbidOutHandtel[12][15]; // 禁止呼出的电话
	char		m_aryAllowOutHandtel[12][15]; // 允许呼出的电话
		// 高4位,0允许所有呼出,1禁止所有呼出,2允许部分呼出但又禁止某些呼出,3禁止部分呼出,但又允许某些呼出
		// 低4位,0允许所有呼入,1禁止所有呼入,2允许部分呼入但又禁止某些呼入,3禁止部分呼入,但又允许某些呼入

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

struct tag2QPhotoPar
{
	BYTE		m_bytTime; // 拍照次数
	BYTE		m_bytSizeType; // 拍照分辨率 (与协议定义不同)
							// 1,最小
							// 2,中等
							// 3,最大
	BYTE		m_bytQualityType; // 拍照质量 (与协议定义相同)
							// 0x01：恒定质量等级（高）
							// 0x02：恒定质量等级（中）（默认）
							// 0x03：恒定质量等级（低）
	BYTE		m_bytChannel; // 通道 0x01,通道1; 0x02,通道2; 0x03,通道1和通道2
	BYTE		m_bytInterval; // 拍照间隔,s
	BYTE		m_bytBright; // 亮度
	BYTE		m_bytContrast; // 对比度
	BYTE		m_bytHue; // 色调
	BYTE		m_bytBlueSaturation; // 蓝饱和
	BYTE		m_bytRedSaturation; // 红饱和
	BYTE		m_bytDoorPar; // 仅用于关车门拍照,低四位表示速度限制：0-15（公里/小时）,高四位表示该速度持续时间：0-15（分钟）
	//090611 hxd
	BYTE		m_bytIntervHour; // 拍照间隔，小时（JG2）小时和分钟全0,视为不定时拍照
	BYTE		m_bytIntervMin; // 拍照间隔,分钟(JG2)
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
	// 报警设置
	BYTE		m_bytAlert_1; // 0	S1	S2	S3	S4	S5	S6	1
		// JG2、JG3、VM1协议：
		// S1：0-关闭欠压报警            1-允许欠压报警
		// S2：0-关闭抢劫报警            1-允许抢劫报警
		// S3；0-关闭震动报警            1-允许震动报警
		// S4：0-关闭非法打开车门报警    1-允许非法打开车门报警
		// S5：0-关闭断路报警            1-允许断路报警
		// S6，保留待定

		// KJ2协议：
		// S1：0-关闭欠压报警            1-允许欠压报警 (协议上没有,但因为KJ-01测试报告上有此功能,所以补上)
		// S2：0-关闭抢劫报警            1-允许抢劫报警
		// S3；0-关闭碰撞报警            1-允许碰撞报警
		// S4：0-关闭侧翻报警            1-允许侧翻报警
		// S5：0-关闭断路报警            1-允许断路报警 (协议上没有,但因为KJ-01测试报告上有此功能,所以补上)

	BYTE		m_bytAlert_2; // 0	A1	A2	A3	A4	A5	A6	1
		// JG2、JG3、VM1协议：
		// A1：0-关闭越界报警            1-允许越界报警
		// A2：0-关闭超速报警            1-允许超速报警
		// A3：0-关闭停留过长报警        1-允许停留时间过长报警
		// A4：0-关闭时间段检查报警      1-允许时间段检查报警
		// A5，A6, 保留待定

		// KJ2协议：
		// A5：0-关闭非法启动报警        1-允许非法启动报警

	BYTE		m_bytAlermSpd; // 超速报警速度 (海里/小时)
	BYTE		m_bytSpdAlermPrid; // 超速报警的超速持续时间 (秒)

	void Init()
	{
		// 默认报警全部打开
		m_bytAlert_1 = 0x7f;
		m_bytAlert_2 = 0x7f;

		// 超速报警参数默认设为不报警
		m_bytAlermSpd = 0xff;
		m_bytSpdAlermPrid = 0xff;
	}
};

// 行驶记录仪相关配置
// (若采样周期为0, 表示不采样; 若发送周期为0,表示不发送; 若窗口大小为0,表示不需要确认不需要可靠传输)
struct tag2QDriveRecCfg
{
	WORD	m_wOnPeriod;	// ACC ON 发送时长 (分钟) (ffff表示永久发送) (千里眼协议时使用)
	WORD	m_wOffPeriod;	// ACC OFF 发送时长 (分钟) (ffff表示永久发送) (千里眼协议时使用)
	WORD	m_wOnSampCycle; // ACC ON 采样周期,秒
	WORD	m_wOffSampCycle; // ACC OFF 采样周期,秒
	BYTE	m_bytSndWinSize; // 发送窗口大小
	BYTE	m_bytOnSndCycle; // ACC ON,发送周期,个
	BYTE	m_bytOffSndCycle; // ACC OFF,发送周期,个

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

// 电话本条目设置
struct tag2QTelBookPar
{
	WORD	m_wLmtPeriod; // 通话时长(分钟),0表示不限时
	BYTE	m_bytType; // 权限类型,同协议定义,即
		// 00H  不允许呼入和呼出
		// 01H  不允许呼入，允许呼出
		// 02H  允许呼入，不许呼出
		// 03H  允许呼入和呼出
	char	m_szTel[15]; // 如果为空,表示该条电话本未启用
	char	m_szName[20];
};

// 电话本配置
struct tag2QCallCfg
{
	BYTE	m_bytSetType; // 设置字
						// 字节定义如下：
						// 0	1	CL	CR	D	D	R	R
						// 
						// CL：1-清除所有电话本设置， 0-保留之前电话本设置
						// DD：呼出电话限制
						// 00：允许所有呼出电话
						// 01：禁止所有呼出电话
						// 10：电话本控制
						// 11：保留
						// RR：呼入电话限制
						// 00：允许所有呼入电话
						// 01：禁止所有呼入电话
						// 10：电话本控制 
						// 11：保留
	tag2QTelBookPar	m_aryTelBook[30];

	void Init()
	{
		memset( this, 0, sizeof(*this) ); // 允许所有呼入呼出
	}
};

// 中心服务电话号码设置
struct tag2QCentServTelCfg
{
	BYTE	m_bytTelCount;
	char	m_aryCentServTel[5][15];

	void Init() { memset( this, 0, sizeof(*this) ); }
};

// UDP激活电话号码设置
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

// 驾驶员信息 (千里眼系统)
struct tag2QDriverPar_QIAN
{
	WORD	m_wDriverNo; // 编号 0~126, 0表示本信息为空 (实际只用1个字节)
	char	m_szIdentID[7]; // 身份代码,不足补0,也可能全填满
	char	m_szDrivingLic[18]; // 驾驶证号,不足补0,也可能全填满
};

// 驾驶员信息 (2客系统)
struct tag2QDriverPar_KE
{
	WORD	m_wDriverNo; // 编号 0~65534, 0表示本信息为空
	char	m_szIdentID[8]; // 身份代码, 不足后补0；也可能全填满
};

// 驾驶员身份设置
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

// 实时指引设置条目 (刚好4字节对齐,别轻易调整)
struct tag2QRealNavPar
{
	long		m_lCentLon;
	long		m_lCentLat;
	WORD		m_wRadius; // 米
	BYTE		m_bytSpeed; // 上限速度,海里/小时,0表示无限制
	BYTE		m_bytDir; // 度,实际度数/3
	BYTE		m_bytType; // 类型
		// 000TD000 (参照协议)
        //T：0--普通类型
        //   1--特殊类型（提示后需检测）（要进行实时指引报警的，现在只用于速度限制）
        //D：0--无需判断行车方向
        //   1--需判断行车方向
	char		m_szTip[51]; // 提示内容,不足填0,且必定以NULL结尾,因为协议里定义的是50字节,该字段正好比协议里多1个字节
};


// 实时指引设置
struct tag2QRealNavCfg
{
	tag2QRealNavPar		m_aryRealNavPar[50];

	void Init() { memset(this, 0, sizeof(*this)); }
};


// 拍照配置
struct tag2QPhotoCfg
{
	WORD		m_wPhotoEvtSymb; // 拍照触发状态字 (见QianExeDef.h中的规定)
	WORD		m_wPhotoUploadSymb; // 拍照主动上传状态字 (见QianExeDef.h中的规定)

	tag2QPhotoPar	m_objAlermPhotoPar; // 报警拍照设置
	tag2QPhotoPar	m_objJijiaPhotoPar; // 空车/重车变化时的拍照设置
	tag2QPhotoPar	m_objHelpPhotoPar; // 求助拍照设置
	tag2QPhotoPar	m_objDoorPhotoPar; // 开关车门拍照
	tag2QPhotoPar	m_objAreaPhotoPar; // 进出区域拍照
	tag2QPhotoPar	m_objTimerPhotoPar; // 定时拍照
	tag2QPhotoPar	m_objAccPhotoPar; // ACC变化拍照

	void Init()
	{
		memset( this, 0, sizeof(*this));
		m_wPhotoEvtSymb = PHOTOEVT_FOOTALERM;
		m_wPhotoUploadSymb = PHOTOUPLOAD_FOOTALERM;//| PHOTOUPLOAD_OVERTURNALERM | PHOTOUPLOAD_BUMPALERM
// 			| PHOTOUPLOAD_TRAFFICHELP | PHOTOUPLOAD_MEDICALHELP | PHOTOUPLOAD_BOTHERHELP
// 			| PHOTOUPLOAD_AREA | PHOTOUPLOAD_TIMER; // 非法启动车辆拍照暂不上传，因为协议未定义
		m_objAlermPhotoPar.Init( 1, 1, 2, 3 ); // v_bytTime, v_bytSizeType, v_bytQualityType, v_bytChannel
		m_objJijiaPhotoPar.Init( 1, 1, 2, 3 );
		m_objHelpPhotoPar.Init( 1, 1, 2, 3 );
		m_objDoorPhotoPar.Init( 1, 1, 1, 3 );
		m_objAreaPhotoPar.Init( 1, 1, 2, 3 );
		m_objTimerPhotoPar.Init( 1, 1, 1, 3 );
		m_objAccPhotoPar.Init( 1, 1, 1, 3 );
	}
};

//LED配置 07.9.26 hong
struct tag2QLedCfg 
{
	unsigned short m_usLedParamID;  //LED配置ID
	unsigned short m_usTURNID;		//LED转场信息ID add by kzy 20090819
	unsigned short m_usNTICID;		//LED通知信息ID add by kzy 20090819
	BYTE           m_bytLedPwrWaitTm; //LED上电等待时间
	BYTE           m_bytLedConfig;   //LED功能定制 07.9.12 hong // 0 A1 A2 A3 A4 A5 A6 0
	//A6: 1: 车台移动静止时对LED屏进行亮黑控制 0：不进行控制
	//A5: 1:LED屏通信错误时不禁止下载广告     0：通信错误时禁止下载广告
	//A4：1：启用MD5                          0：不启用MD5
	unsigned short m_usLedBlkWaitTm; //车辆静止后黑屏等待时间，单位：秒,默认为2分钟
	//#ifdef DELAYLEDTIME
	//	unsigned short m_usLedMenuID;   //节目单ID 07.9.30
	//#endif
	char			m_szLedVer[64]; // LED程序版本号
	
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


//手柄配置 caoyi 07.11.29
struct tag2QHstCfg{
    char		m_bytcallring;
    char		m_bytsmring;
    bool		m_HandsfreeChannel;
    bool		m_EnableKeyPrompt;
    bool		m_EnableAutoPickup;
	//char		m_bytListenTel[3];	//监听电话的代号  //cyh add:del
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

// 行驶配置, 090817 xun move here
struct DrvCfg 
{
	byte inited;		//是否初始化过的标志
	
	ushort factor;		//车辆特征系数(每公里的脉冲数)
	char vin[17];		//车辆VIN号：占用1～17个字节
	char num[12];		//车辆号码：占用1～12个字节
	char type[12];		//车牌分类：占用1～12个字节	
	
	byte total;			//司机个数
	byte max_tire;		//设置的最大疲劳时间(小时)
	byte min_sleep;		//设置的最小休息时间(分钟)
	
	struct Driver 
	{		
		bool valid;			//有效标志位
		byte num;			//司机编号：取值范围0~126
		char ascii[7];		//身份代码(1~7)
		char code[18];		//驾驶证号(1~18)
	} inf[MAX_DRIVER];
	
	byte max_speed;		//上限速度(海里/时)
	byte min_over;		//超速计量时间
	
	byte winsize;		//窗口大小
	byte sta_id;		//状态标识
	ushort uptime[6];	//上报总时长
	ushort cltcyc[6];	//采样周期
	ushort sndcyc[6];	//发送周期
	
	void init() {
		memset(this, 0, sizeof(*this));
		inited = 123; // 初始化标志（比较特殊）
		winsize = 4;
		
		// 090229 xun: 增加默认参数
		uptime[0] = 0xffff; // 移动 (实际处理时是为ACC ON)
		cltcyc[0] = 60;
		sndcyc[0] = 1;
		uptime[1] = 0xffff; // 静止 (实际处理时是为ACC OFF)
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


//--------------------以下为DvrExe参数--------------------//

//密码
struct tag2DPwdPar
{
	char	AdminPassWord[11];
	char	UserPassWord[11];
	char	m_szReserved[26]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

	  strcpy(AdminPassWord, "0123456789");
	  strcpy(UserPassWord, "0000000000");
	}
};

//设备信息
struct tag2DDevInfoPar
{
	char	DriverID[13]; 
	char	CarNumber[13];
	char	SoftVersion[13]; 
	char	m_szReserved[25]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

	  strcpy(DriverID, "000000000000");
	  strcpy(CarNumber, "000000000000");
	  strcpy(SoftVersion, "MVR1600-V1.0");
	}
};

//网络信息
struct tag2DIpPortPar
{
	DWORD m_ulTcpIP;//前置机IP
	WORD  m_usTcpPort;//前置机端口
	char	m_szTel[16];			//本机号码
	BYTE	m_uszsizeTel;			//本机号码长度
	char	m_szAreaCode[2]; 	// 区域号(中心ID)
	char	m_szReserved[31]; // 留出预留空间,并使得整个结构体尺寸为8的倍数

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

//音视频触发类型
struct tag2DTrigPar 
{
	UINT TrigType;
	char m_szReserved[20]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		TrigType = 1;//TRIG_EVE;
	}
};

//时段触发参数
struct tag2DPerTrigPar
{
	BOOL	PeriodStart[6];
	char	StartTime[6][9];
	char	EndTime[6][9];
	char	m_szReserved[60]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
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

//事件触发参数
struct tag2DEveTrigPar
{
	BOOL	AccStart, MoveStart, OverlayStart, S1Start, S2Start, S3Start, S4Start;
	UINT	AccDelay, MoveDelay, OverlayDelay, S1Delay, S2Delay, S3Delay, S4Delay;
	char	m_szReserved[40]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		AccStart = MoveStart = OverlayStart = S1Start = S2Start = S3Start = S4Start = FALSE;
		AccDelay = MoveDelay = OverlayDelay = S1Delay = S2Delay = S3Delay = S4Delay = 20;
	}
};

//视频输入参数
struct tag2DVIPar
{
	UINT	VMode;
	UINT	VNormal;	
	char	m_szReserved[16]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		VMode = 1;//CIF;
		VNormal = 0;//PAL;
	}
};

//视频输出参数
struct tag2DVOPar
{
	UINT  VMode;
	UINT  VNormal;
	char	m_szReserved[16]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
	void Init()
	{
		memset( this, 0, sizeof(*this) );

		VMode = 1;//CIF;
		VNormal = 0;//PAL;
	}
};

//视频预览参数
struct tag2DVPrevPar
{
	BOOL  VStart[4];
	char	m_szReserved[16]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
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

//音视频编码录像参数
struct tag2DAVRecPar
{
	BOOL  AStart[4];
	BOOL  VStart[4];
	UINT 	VMode;
	UINT 	VBitrate[4];
	UINT 	VFramerate[4];
	char  DiskType[10];
	char	m_szReserved[34]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
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

//音视频编码上传参数
struct tag2DAVUpdPar
{
	BOOL  AStart[4];
	BOOL  VStart[4];
	UINT 	VMode;
	UINT 	VBitrate[4];
	UINT 	VFramerate[4];
	int 	UploadType;
	int 	UploadSecond;
	char	m_szReserved[36]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
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

//音视频解码回放参数
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
	char	m_szReserved[73]; // 留出预留空间,并使得整个结构体尺寸为8的倍数
	
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

//DVR配置参数
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

//--------------------以上为DvrExe参数--------------------//


// 根据20070914讨论,认为配置区应按照业务进行分块,并且每块大小应是80的整数倍
struct tagSecondCfg// 注意结构体内存对齐
{
	char		m_szSpecCode[80];

	// 流媒体设置
	union
	{
		char		m_szReserved[80];
		tag2LMediaCfg m_obj2LMediaCfg;
	} m_uni2LMediaCfg;

	// 黑匣子保存设置
	union
	{
		char		m_szReservered[80];
		tag2QBlkCfg	m_obj2QBlkCfg;
	} m_uni2QBlkCfg;

	// 报警设置
	union
	{
		char		m_szReservered[80];
		tag2QAlertCfg	m_obj2QAlertCfg;
	} m_uni2QAlertCfg;

	// 实时定位条件设置
	union
	{
		char		m_szReservered[560];
		tag2QRealPosCfg	m_obj2QRealPosCfg;
	} m_uni2QRealPosCfg;

	// GPRS参数设置
	union
	{
		char		m_szReservered[160];
		tag2QGprsCfg	m_obj2QGprsCfg;
	} m_uni2QGprsCfg;

	// 中心服务号码设置
	union 
	{
		char		m_szReservered[560];
		tag2QServCodeCfg	m_obj2QServCodeCfg;
	} m_uni2QServCodeCfg;

	// 拍照设置
	union
	{
		char		m_szReservered[480];
		tag2QPhotoCfg	m_obj2QPhotoCfg;
	} m_uni2QPhotoCfg;
	
	// LED配置
	union
	{
		char		m_szReservered[160];
		tag2QLedCfg m_obj2QLedCfg;
	}m_uni2QLedCfg;
	
	//手柄配置
	union
	{
		char		m_szReservered[80];
		tag2QHstCfg	m_obj2QHstCfg;
	}m_uni2QHstCfg;

	// 呼入呼出限制设置
	union
	{
		char		m_szReservered[880];
		tag2QFbidTelCfg m_obj2QFbidTelCfg;
	} m_uni2QFbidTelCfg;
	
	// 电话本设置
	union
	{
		char		m_szReservered[960];
		tag2QCallCfg	m_obj2QCallCfg;
	} m_uni2QCallCfg;

	// 行驶记录上报设置
	union
	{
		char		m_szReservered[80];
		tag2QDriveRecCfg	m_obj2QDriveRecCfg;
	} m_uni2QDriveRecCfg;

	// 驾驶员身份设置
	union
	{
		char		m_szReservered[480];
		tag2QDriverCfg		m_obj2QDriverCfg;
	} m_uni2QDriverCfg;

	// 实时指引设置
	union
	{
		char		m_szReservered[3200];
		tag2QRealNavCfg		m_obj2QRealNavCfg;
	} m_uni2QRealNavCfg;
	

	/// { JG2旧的形式记录仪模块配置
	// 电话本 (JG2)
	union
	{
		char		m_szReservered[1600];	//已经使用1245
		
	} m_uniBookCfg;
	
	// 行车记录仪 (JG2)
	union
	{
		char		m_szReservered[480];	//已经使用318	
	} m_uniDrvCfg;
	/// } JG2旧的形式记录仪模块配置

	union
	{
		char		m_szReservered[80];
		tagSpecCtrlCfg m_obj2QSpecCtrlCfg;
	} m_uni2QSpecCtrlCfg;

	// Dvr设置
	union
	{
		tag2DDvrCfg m_obj2DDvrCfg;
		char		m_szReservered[1760]; //已经使用1530左右
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

int DataPush( void* v_pPushData, DWORD v_dwPushLen, DWORD v_dwPushSymb, DWORD v_dwPopSymb, BYTE v_bytLvl ); // 此方法可同时指定多个PopSymb,即接收对象
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
int IOIntInit( unsigned char v_ucIOSymb, void* v_pIntFunc ); // 若v_ucIOSymb值为255,则表示设置全部输入IO的中断
//int IOIntFuncSet( unsigned char v_ucIOSymb, void* v_pIntFunc ); // 若v_ucIOSymb值为255,则表示设置全部IO的回调函数

int GetCurGps( void* v_pGps, const unsigned int v_uiSize, BYTE v_bytGetType );
int SetCurGps( const void* v_pGps, BYTE v_bytSetType );

bool GetDevID( char* v_szDes, unsigned int v_uiDesSize );
bool GetAppVer( char* v_szDes, unsigned int v_uiDesSize );

int SetCurTime( void* v_pTmSet ); // 传入参数应是tm类型对象的指针 (UTC时间)
unsigned int GetTickCount(); // 返回值: 从1970.1.1凌晨起经历的时间,ms

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
