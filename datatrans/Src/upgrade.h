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
	uint8_t measure;
	uint16_t PackSize;					/*ȫ������С*/
	uint16_t Req_Pack;					/*��ǰ�����*/
	uint8_t Pack[300];					/*���ڴ洢ÿ�ν��յ��İ�*/
}Upgrade_info;

extern Upgrade_info upgrade_info;
extern SemaphoreHandle_t ConfigBinarySemaphore;
extern SemaphoreHandle_t GettingPackBinarySemaphore;

void UpgradeTask(void *pArg);
uint8_t sametimeUpgrade(void);

#endif


