#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"
#include "myflash.h"
#include "conversion.h"
#include "can.h"

Upgrade_info upgrade_info;
s19_msg msg_s19_line;
gPack_Data  gpack_data;
SemaphoreHandle_t ConfigBinarySemaphore;					/*配置的信号量*/
SemaphoreHandle_t GettingPackBinarySemaphore;			/*获取固件包的信号量*/

/*交互任务***
**用于进行固件的更新
*	
**/

const char supgradeOK[10]="updateok\0";
const char supgradeFAIL[15]="updatefail\0";
const char sdownloadOK[15]="downloadok\0";
const char sdownloadFAIL[15]="downloadfail\0";

#define CONFIG_REPLY_PACK 	"{\"type\":\"configok\",\"imei\":\"%s\"}"
#define GET_PACK_FROM_INTERNET 	"{\"type\":\"reqpack\",\"imei\":\"%s\",\"packnum\":%d}"
#define UPGRADE_REPLY_PACK	"{\"type\":\"%s\",\"imei\":\"%s\"}"

/*升级回复完成包*/
static void upgrade_reply(const char * result_str)
{
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,UPGRADE_REPLY_PACK,result_str,gGprs.gimei);
	LOG(LOG_INFO,"str:%s \r\n",str);
	MessageSend(str,1);
	vPortFree(str);
}

/*回复CONFIG包*/
static uint8_t sConfig_reply(void)
{
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,CONFIG_REPLY_PACK,gGprs.gimei);
	LOG(LOG_INFO,"str:%s \r\n",str);
	MessageSend(str,1);
	vPortFree(str);
	//vSemaphoreDelete(ConfigBinarySemaphore);
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
//  for(int i=0;i<3;i++)
//  {
		MessageSend(getPacStr,1);
//		err = xSemaphoreTake(GettingPackBinarySemaphore,10000);
//	  if(err == pdTRUE)
//		{
//			return;
//		}
//		Log(LOG_ERROR,"getPackFromInternet %d times\r\n",i);
//  }
	/*3次获取都失败或者3次获取的包解析都失败了，那么通知平台，数据出错，重新开始*/
	/*上报ERRCODE包*/
}

/*获取固件包并写入FLASH当中去*/
static void gGetPackToFlash(void)
{
	BaseType_t err;
	upgrade_info.Rec_Data_Len = 0;
	/*按照每一包（50K）去获取并升级，后期可能修改*/
	for(;upgrade_info.Req_Pack<=upgrade_info.PackSize+1;upgrade_info.Req_Pack++)
	{
		/*等待接收到完整50K正确固件包完成信号量 */
		err = xSemaphoreTake(GettingPackBinarySemaphore,portMAX_DELAY);
		upgrade_info.Req_Pack = upgrade_info.Rev_Pack+1;		/*已经获取到的包号码*/
		/*成功获取到信号量*/
		if(err == pdTRUE)
		{
			//LOG(LOG_INFO,"upgrade_info.Rev_Pack:%d!\r\n",upgrade_info.Rev_Pack);
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
		if(upgrade_info.Req_Pack<=upgrade_info.PackSize)
		{
			getfirm = GETFIRM_START;
			getPackFromInternet(upgrade_info.Req_Pack);
		}	
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
			upgrade_reply(sdownloadOK);
			vTaskDelay(500);
			upgrade_reply(supgradeOK);
//				ret = startUpgrade();
//				if(ret == upgradeOK)
//				{
//					upgrade_reply(supgradeOK);
//					break;
//				}
//				upgrade_reply(supgradeFAIL);
			/*同样需要写FLASH区域配置信息，不过这里是升级完成后，直接清空*/
				break;
		/*先下载后升级*/
		case diff:
			vTaskDelay(100);
			upgrade_reply(sdownloadOK);
			break;
			/*需要写FLASH区域配置信息*/
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

static void sReadPackFromFlash(uint8_t * str,uint8_t * des){
	
	
	
	
}


static s19_end slineJudge(uint8_t type[])
{
	if(type[1]=='7' || type[1]=='8' ||type[1]=='9' )				//是S7/S8/S9 结束行
	{
		return endline;
	}else{
		return middleline;
	}
}

/*升级函数*/
uint8_t startUpgrade(void)
{
	int ret = -1;
	int i = 0;
	uint8_t sLinelen = 0;
	uint32_t sReadFlashDataLen = 0;
			can_id.Can_num = Can_num1;
		can_id.RecID = 0xc8;
	  can_id.SendID = 0x64;
	if(CanPre() != CAN_PRE_OK)
		LOG(LOG_ERROR,"can pre failed .Please check can connection.\r\n");
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//获取数据长度
	sReadFlashDataLen+=sLinelen+1;																								//+1代表有一个换行符
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		LOG(LOG_ERROR,"start line format is not correct.\r\n");	
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//获取数据长度
	sReadFlashDataLen+=sLinelen+1;	
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
	{
		LOG(LOG_ERROR,"middle line format is not correct.\r\n");	
	}
	while(slineJudge(msg_s19_line.type)==middleline)
	{
		//LOG(LOG_INFO,"upgrade_info.LinePack: %s \r\n sLinelen:%d\r\n",upgrade_info.LinePack,sLinelen);
		if(CanSendLinePack(upgrade_info.LinePack)!=CAN_PRE_OK)
		{
			LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");	
			break;			
		}
		memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
		sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//获取数据长度
		sReadFlashDataLen+=sLinelen+1;	
		memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
		if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		{
			LOG(LOG_ERROR,"middle line format is not correct.\r\n");	
			break;
		}
	}		
	/*已经到最后一行*/
	if(CanSendEndPack()!=CAN_PRE_OK)
	{
		LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");	
	}
	ret = upgradeOK;
	return ret;
}

/*****升级任务*****/
/******/
extern xTimerHandle NetTimerHandler;
void UpgradeTask(void *pArg)   
{
	BaseType_t err;
	while(1){
			/*使用网络进行升级*/
		if(1)  //upgrade_info.method == online)																													/*从网络开始升级*/
		{
			/*判断upgradinfo是否已经被写入了，然后再判断是直接读取FLASH升级还是先进行网络下载*/
			/*等待配置完成信号量 */
//			sInterUpgradeProcess();
//			while(1);
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			xTimerStop(NetTimerHandler,0);
			/*成功获取到信号量*/
			if(err == pdTRUE)
			{
				sConfig_reply();						/*回复配置包*/	
				memset(&gpack_data,0,sizeof(gpack_data));			/*存储区清空*/
				sInterUpgradeProcess();			/*进行升级，如果是下载后直接升级，那么就直接升级，如果是先下载保存，那么就先下载后升级*/
				memset(&upgrade_info,0,sizeof(upgrade_info));			/*存储区清空*/
				xTimerStart(NetTimerHandler,0);
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

