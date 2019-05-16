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

#define gPack_Data_Len 51*1024		/*多留出来1K的空间，防止接收数据过多，导致数组越界产生不可知的影响*/

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
	uint8_t method;							/*升级方式   0x01--使用网络升级  0x02---使用SD卡升级*/
	uint8_t measure;						/*下载后直接升级，或者先下载，保存在FLASH中再进行升级*/
	uint8_t data_status;				/*先下载后升级方式，是否已经获取到升级包*/
	uint16_t PackSize;					/*全部包大小*/
	uint16_t Req_Pack;					/*当前请求包*/
	uint16_t Rev_Pack;					/*平台当前下发的固件包号码*/
	uint32_t Rec_Data_Len;			/*用于存储接收到的数据包总长度--按照字节单位进行计数*/
	uint8_t LinePack[200];			/*用于存储每行的数据包*/
	char md5[35];						/*用于存储接收到的md5校验数据包*/
}Upgrade_info;

extern Upgrade_info upgrade_info;
extern SemaphoreHandle_t ConfigBinarySemaphore;
extern SemaphoreHandle_t GettingPackBinarySemaphore;

void UpgradeTask(void *pArg);
void getPackFromInternet(const uint8_t packnum);
uint8_t startUpgrade(void);

#endif


