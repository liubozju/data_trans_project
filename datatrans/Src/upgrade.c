#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"
#include "myflash.h"

Upgrade_info upgrade_info;
gPack_Data  gpack_data;
SemaphoreHandle_t ConfigBinarySemaphore;					/*配置的信号量*/
SemaphoreHandle_t GettingPackBinarySemaphore;			/*获取固件包的信号量*/

/*交互任务***
**用于进行固件的更新
*	
**/

#define CONFIG_REPLY_PACK 	"{\"type\":\"configok\",\"imei\":\"%s\"}"
#define GET_PACK_FROM_INTERNET 	"{\"type\":\"reqpack\",\"imei\":\"%s\",\"packnum\":%d}"


/*回复CONFIG包*/
static uint8_t sConfig_reply(void)
{
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,CONFIG_REPLY_PACK,gGprs.gimei);
	MessageSend(str,1);
	vPortFree(str);
	vSemaphoreDelete(ConfigBinarySemaphore);
	return 1;
}

static char getPacStr[100] = {0};

/*从网络获取固件包--上报获取包*/
void getPackFromInternet(const uint8_t packnum)
{
	memset(getPacStr,0,100);
	sprintf(getPacStr,GET_PACK_FROM_INTERNET,gGprs.gimei,packnum);
	LOG(LOG_INFO,"str:%s \r\n",getPacStr);	

	BaseType_t err;
  for(int i=0;i<3;i++)
  {
		MessageSend(getPacStr,1);
		err = xSemaphoreTake(GettingPackBinarySemaphore,10000);
	  if(err == pdTRUE)
		{
			return;
		}
		Log(LOG_ERROR,"getPackFromInternet %d times\r\n",i);
  }
	/*3次获取都失败或者3次获取的包解析都失败了，那么通知平台，数据出错，重新开始*/
	/*上报ERRCODE包*/
}

/*获取固件包并写入FLASH当中去*/
static void gGetPackToFlash(void)
{
	BaseType_t err;
	upgrade_info.Rec_Data_Len = 0;
	/*按照每一包（50K）去获取并升级，后期可能修改*/
	for(;upgrade_info.Req_Pack<upgrade_info.PackSize;upgrade_info.Req_Pack++)
	{
		/*等待接收到完整50K正确固件包完成信号量 */
		err = xSemaphoreTake(GettingPackBinarySemaphore,portMAX_DELAY);
		upgrade_info.Req_Pack = upgrade_info.Rev_Pack+1;		/*已经获取到的包号码*/
		/*成功获取到信号量*/
		if(err == pdTRUE)
		{
			LOG(LOG_INFO,"upgrade_info.Rev_Pack:%d!\r\n",upgrade_info.Rev_Pack);
			/*将50K的数据写入到FLASH当中去，注意下发的编号从0还是1开始*/
			STMFLASH_Write(PACK_ADDR_START+upgrade_info.Rec_Data_Len,gpack_data.Pack_Data,gpack_data.Pack_Data_len);
			upgrade_info.Rec_Data_Len+=gpack_data.Pack_Data_len;
			memset(&gpack_data,0,sizeof(gpack_data));			/*存储区清空*/
		}
		else
		{
			LOG(LOG_ERROR,"getting Pack failed. Please Check!\r\n");
			sysReset();
		}
		getPackFromInternet(upgrade_info.Req_Pack);	
	}
}

/*升级处理任务*/
static uint8_t sInterUpgradeProcess()
{
	int ret = -1;
	/*首先获取升级包存储到FLASH*/
	gGetPackToFlash();
	/*使用网络升级的两种措施*/
	switch(upgrade_info.measure)
	{
		/*边升级边下载*/
		case sametime:
				startUpgrade();
		
			/*同样需要写FLASH区域配置信息，不过这里是升级完成后，直接清空*/
				break;
		/*先下载后升级*/
		case diff:
			
			/*需要写FLASH区域配置信息*/
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

static void sReadPackFromFlash(uint8_t * str,uint8_t * des){
	
	
	
	
}


/*升级函数*/
uint8_t startUpgrade(void)
{
	int ret = -1;
	int i = 0;
	uint16_t sPackNum = upgrade_info.Rec_Data_Len/gPack_Data_Len;		//按照50K分包后的数据包数量
	uint32_t sRemain = upgrade_info.Rec_Data_Len % gPack_Data_Len;	//最后一包剩余的字节数量
	char * scopyStart = NULL;
	char * scopyEnd = NULL;
	uint32_t copyLen = 0;
	for(i = 0;i<sPackNum;i++)
	{
		/*首先从FLASH中读取50K数据*/
		memset(&gpack_data,0,sizeof(gpack_data));
		STMFLASH_Read(PACK_ADDR_START,gpack_data.Pack_Data,gPack_Data_Len);	
		/*从50K的数据包中解析出每一行，并进行校验*/
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		
		
	}

	
	
	/*大循环使用CAN进行更新*/
	
	
	/**循环更新结束，上报结束包*/
	
	
	return ret;
}

/*****升级任务*****/
/******/
void UpgradeTask(void *pArg)   
{
	BaseType_t err;
	while(1){
			/*使用网络进行升级*/
		if(1)  //upgrade_info.method == online)																													/*从网络开始升级*/
		{
			/*判断upgradinfo是否已经被写入了，然后再判断是直接读取FLASH升级还是先进行网络下载*/
			/*等待配置完成信号量 */
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			/*成功获取到信号量*/
			if(err == pdTRUE)
			{
				sConfig_reply();						/*回复配置包*/	
				memset(&gpack_data,0,sizeof(gpack_data));			/*存储区清空*/
				sInterUpgradeProcess();			/*进行升级，如果是下载后直接升级，那么就直接升级，如果是先下载保存，那么就先下载后升级*/
			}
			else
			{
				LOG(LOG_ERROR,"getting Config failed. Please Check!\r\n");
				sysReset();
			}
		}
		/*使用SD卡进行升级*/
		else if(upgrade_info.method == offline)  																									/*从SD开始读取数据，进行升级，不会涉及到其他操作，也不会中断，只需直接升级即可*/
		{
			
		}
		vTaskDelay(50);
	}
}

