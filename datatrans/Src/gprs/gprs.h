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

/*IP�Ͷ˿ں�*/
typedef struct IPPort{
	uint8_t gremoteIP[20];				/*����Զ�����ӣ�֧����·����*/
	uint8_t gremotePort[6];
}gIPPort;

/*��־*/
typedef struct FLAG{
	uint8_t gGPRSConnectFlag;				/*������־*/
	uint8_t gINFOFlag;							/*��ȡ��Ϣ��־*/
	uint8_t gNetFlag;								/*�������־*/
	uint8_t gTcpSendDataFlag;				/*TCP��������*/
  uint8_t gRegisterFlag;
}gGPRSFlag;

/*gprsģ���������ݺͺ���*/
typedef struct GPRS{
		
		gGPRSFlag gprsFlag;						/*ģ���FLAG��־*/
		volatile char gimei[20];			/*ģ���IMEI����*/
		volatile char gimsi[20];			/*������*/
		volatile int csq;							/*GPRS���ź�ǿ��*/
		volatile char gprsIP[30];			/*ģ�����ӵ�IP��ַ*/
		volatile char gprsPort[6];		/*ģ�����ӵ�PORT�˿�*/
		volatile char gGPRSConnect[100];	/*ģ������ʹ�õĲ���*/
		volatile uint8_t gTCPLinkNum[6];	/*ģ����Խ����ü�·��·����¼��·����*/
		int(*gGPRSInit)(void);				/*ģ��ĳ�ʼ������*/
		int(*gGPRSTCPConnect)(void);			/*ģ�����������*/
		int(*gGPRSReset)(void);				/*ģ��ĸ�λ����*/
		void (*gGPRSConfig)(void);		/*ģ������ú���*/
		int (*gGPRSTCPConectStatusTest)(void);	/*ģ�������״̬���Ժ���*/
		int (*gGetGprsInfo)(const char * cmd,const char *rsp);/*ģ���ȡ����Ϣ*/
		SemaphoreHandle_t GprsConnectBinarySemaphore;
		SemaphoreHandle_t GprsINFOBinarySemaphore;	/*�ź��������ڻ�ȡģ���CSQ IMEI IMSI����Ϣ*/
		SemaphoreHandle_t GprsTCPSendBinarySemaphore;				/*�ź��������ڷ���TCP����*/
	
		QueueHandle_t GPRSINFORepQueue;
		QueueHandle_t GprsRepQueue;
		QueueHandle_t GprsTCPRepQueue;											/*���ն��У�TCP��Ϣ�ڴ��ظ� >*/
}gprs;


extern gprs gGprs;


extern int gDeviceConnect(void);
extern void gDeviceConfig(void);
static int pdeviceConnect(void);


/*�Զ��庯������*/
static int sIpPortCheck(const char * remoteIp,const char * remotePort);
extern void gRemoteIpPortConfig(const char * remoteIp,const char * remotePort);
void gTCPIPConfig(void);
int gNetTest(void);
int gGetDeviceINFO(const char * cmd,const char *rsp);  
#endif




