#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "log.h"
#include "sys.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#define gPack_Data_Len 51*1024		/*��������1K�Ŀռ䣬��ֹ�������ݹ��࣬��������Խ���������֪��Ӱ��*/

typedef struct{
	uint32_t Pack_Data_len;
	uint8_t  Pack_Data[gPack_Data_Len];
}gPack_Data;

typedef enum{
	online = 0x01,
	offline,
}Method;

typedef enum{
	sametime = 0x01,
	diff,
}Measure;


typedef struct{
	uint8_t method;							/*������ʽ   0x01--ʹ����������  0x02---ʹ��SD������*/
	uint8_t measure;						/*���غ�ֱ�����������������أ�������FLASH���ٽ�������*/
	uint8_t data_status;				/*�����غ�������ʽ���Ƿ��Ѿ���ȡ��������*/
	uint16_t PackSize;					/*ȫ������С*/
	uint16_t Req_Pack;					/*��ǰ�����*/
	uint16_t Rev_Pack;					/*ƽ̨��ǰ�·��Ĺ̼�������*/
	uint32_t Rec_Data_Len;			/*���ڴ洢���յ������ݰ��ܳ���--�����ֽڵ�λ���м���*/
	uint8_t LinePack[200];			/*���ڴ洢ÿ�е����ݰ�*/
	char md5[35];						/*���ڴ洢���յ���md5У�����ݰ�*/
}Upgrade_info;

extern Upgrade_info upgrade_info;
extern SemaphoreHandle_t ConfigBinarySemaphore;
extern SemaphoreHandle_t GettingPackBinarySemaphore;

void UpgradeTask(void *pArg);
void getPackFromInternet(const uint8_t packnum);
uint8_t startUpgrade(void);

#endif


