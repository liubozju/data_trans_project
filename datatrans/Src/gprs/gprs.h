#ifndef	__GPRS_H__
#define __GPRS_H__


#include "sys.h"
#include "usart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

/*IP和端口号*/
typedef struct IPPort{
	uint8_t gremoteIP[20];				/*建立远端连接，支持五路连接*/
	uint8_t gremotePort[6];
}gIPPort;

/*标志*/
typedef struct FLAG{
	uint8_t gGPRSConnectFlag;				/*联网标志*/
	uint8_t gINFOFlag;							/*获取信息标志*/
	uint8_t gNetFlag;								/*网络监测标志*/
	uint8_t gTcpSendDataFlag;				/*TCP发送数据*/
  uint8_t gRegisterFlag;
}gGPRSFlag;

/*gprs模组的相关数据和函数*/
typedef struct GPRS{
		
		gGPRSFlag gprsFlag;						/*模组的FLAG标志*/
		volatile char gimei[20];			/*模组的IMEI号码*/
		volatile char gimsi[20];			/*卡号码*/
		volatile int csq;							/*GPRS的信号强度*/
		volatile char gprsIP[30];			/*模组连接的IP地址*/
		volatile char gprsPort[6];		/*模组连接的PORT端口*/
		volatile char gGPRSConnect[100];	/*模组连接使用的参数*/
		volatile uint8_t gTCPLinkNum[6];	/*模组可以建立好几路链路，记录链路号码*/
		int(*gGPRSInit)(void);				/*模组的初始化函数*/
		int(*gGPRSTCPConnect)(void);			/*模组的联网函数*/
		int(*gGPRSReset)(void);				/*模组的复位函数*/
		void (*gGPRSConfig)(void);		/*模组的配置函数*/
		int (*gGPRSTCPConectStatusTest)(void);	/*模组的联网状态测试函数*/
		int (*gGetGprsInfo)(const char * cmd,const char *rsp);/*模组获取其信息*/
		SemaphoreHandle_t GprsConnectBinarySemaphore;
		SemaphoreHandle_t GprsINFOBinarySemaphore;	/*信号量，用于获取模组的CSQ IMEI IMSI等信息*/
		SemaphoreHandle_t GprsTCPSendBinarySemaphore;				/*信号量，用于发送TCP数据*/
	
		QueueHandle_t GPRSINFORepQueue;
		QueueHandle_t GprsRepQueue;
		QueueHandle_t GprsTCPRepQueue;											/*接收队列，TCP消息期待回复 >*/
}gprs;


extern gprs gGprs;


extern int gDeviceConnect(void);
extern void gDeviceConfig(void);
static int pdeviceConnect(void);


/*自定义函数声明*/
static int sIpPortCheck(const char * remoteIp,const char * remotePort);
extern void gRemoteIpPortConfig(const char * remoteIp,const char * remotePort);
void gTCPIPConfig(void);
int gNetTest(void);
int gGetDeviceINFO(const char * cmd,const char *rsp);  
#endif




