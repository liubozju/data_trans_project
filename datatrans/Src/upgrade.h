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
	uint8_t method;							/*升级方式   0x01--使用网络升级  0x02---使用SD卡升级*/
	uint8_t measure;
	uint16_t PackSize;					/*全部包大小*/
	uint16_t Req_Pack;					/*当前请求包*/
	uint8_t Pack[300];					/*用于存储每次接收到的包*/
}Upgrade_info;

extern Upgrade_info upgrade_info;
extern SemaphoreHandle_t ConfigBinarySemaphore;
extern SemaphoreHandle_t GettingPackBinarySemaphore;

void UpgradeTask(void *pArg);
uint8_t sametimeUpgrade(void);

#endif


