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
//#define gPack_Data_Len 1*1024		/*多留出来1K的空间，防止接收数据过多，导致数组越界产生不可知的影响*/

#define SD_MODE_FLAG 	  0x55
#define NET_MODE_FLAG 	0x66

#define DIFF_START_FLAG	0x33
#define DIFF_END_FLAG	  0x77

#define DIFF_CLEAR_FLAG	0x00

typedef struct{
	uint8_t ModeFlag;					/*模式标记*/
	uint8_t DiffUpgradeStartFlag;			/*离线升级标记*/
	uint8_t DiffUpgradeEndFlag;			/*离线升级标记*/
	uint8_t DiffCannum;							/*离线升级使用的CAN接口*/
	uint8_t DiffCanSendid;
	uint8_t DiffCanRevid;
}gFlag_Data;


typedef struct{
	uint32_t Pack_Data_len;
	uint8_t  Pack_Data[gPack_Data_Len];
}gPack_Data;

typedef enum{
	online = 0x01,
	offline,
}Method;

typedef enum{
	upgradeOK = 0x00,
	upgradeFAIL,
	downloadOK,
	downloadFAIL,
}upgrade_result;

typedef enum{
	endline = 0x01,
	middleline,
}s19_end;

typedef enum{
	sametime = 66,
	diff = 23,
}Measure;

typedef struct{
	uint8_t type[2];
	uint8_t msg_len[2];
	uint8_t data[280];				//地址域+数据域+校验和  最大长度为38个字节，76个字符，为避免少读，多读几个字节		
}s19_msg;


typedef struct{
	uint8_t method;							/*升级方式   0x01--使用网络升级  0x02---使用SD卡升级*/
	uint8_t measure;						/*下载后直接升级，或者先下载，保存在FLASH中再进行升级*/
	uint8_t data_status;				/*先下载后升级方式，是否已经获取到升级包*/
	uint16_t PackSize;					/*全部包大小*/
	uint16_t Req_Pack;					/*当前请求包*/
	uint16_t Rev_Pack;					/*平台当前下发的固件包号码*/
	uint32_t Rec_Data_Len;			/*用于存储接收到的数据包总长度--按照字节单位进行计数*/
	uint8_t LinePack[100];			/*用于存储每行的数据包*/
	char md5[35];						/*用于存储接收到的md5校验数据包*/
}Upgrade_info;

extern Upgrade_info upgrade_info;
extern SemaphoreHandle_t ConfigBinarySemaphore;
extern SemaphoreHandle_t GettingPackBinarySemaphore;

void UpgradeTask(void *pArg);
void getPackFromInternet(const uint8_t packnum);
uint8_t startUpgrade(void);

#endif


