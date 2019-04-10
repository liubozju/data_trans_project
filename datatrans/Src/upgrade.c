#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"

Upgrade_info upgrade_info;
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
	//deviceDirectSend(str);
	vPortFree(str);
	vSemaphoreDelete(ConfigBinarySemaphore);
	return 1;
}

/*从网络获取固件包--上报获取包*/
static void getPackFromInternet(const uint8_t packnum)
{
	char * str = pvPortMalloc(100);
	memset(str,0,100);
	sprintf(str,GET_PACK_FROM_INTERNET,gGprs.gimei,packnum);
	LOG(LOG_INFO,"str:%s \r\n",str);	

	BaseType_t err;
  for(int i=0;i<3;i++)
  {
		MessageSend(str,1);
		err = xSemaphoreTake(GettingPackBinarySemaphore,10000);
	  if(err == pdTRUE)
		{
			vPortFree(str);
			break;
			return;
		}
		Log(LOG_ERROR,"getPackFromInternet %d times\r\n",i);
  }
	/*3次获取都失败或者3次获取的包解析都失败了，那么通知平台，数据出错，重新开始*/
	/*上报ERRCODE包*/
	
	
}

/*升级处理任务*/
static uint8_t sInterUpgradeProcess()
{
	int ret = -1;
	/*使用网络升级的两种措施*/
	switch(upgrade_info.measure)
	{
		/*边升级边下载*/
		case sametime:
				sametimeUpgrade();
				break;
		/*先下载后升级*/
		case diff:
			
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

/*边升级边下载处理函数*/
uint8_t sametimeUpgrade(void)
{
	int ret = -1;
	upgrade_info.Req_Pack = 0;
	/*按照每一行去获取并升级，后期可能修改*/
	for(;upgrade_info.Req_Pack<upgrade_info.PackSize;upgrade_info.Req_Pack++)
	{
		getPackFromInternet(upgrade_info.Req_Pack);
		
	}
	
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
			/*等待配置完成信号量 */
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			/*成功获取到信号量*/
			if(err == pdTRUE)
			{
				sConfig_reply();			
				sInterUpgradeProcess();
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
