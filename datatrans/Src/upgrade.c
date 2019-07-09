#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"
#include "myflash.h"
#include "conversion.h"
#include "can.h"
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "stdio.h"
#include "cjson.h"
#include "myflag.h"
#include "iwdg.h"

gFlag_Data gFlag;
Upgrade_info upgrade_info;
s19_msg msg_s19_line;
gPack_Data  gpack_data;
SemaphoreHandle_t ConfigBinarySemaphore;					/*配置的信号量*/
SemaphoreHandle_t GettingPackBinarySemaphore;			/*获取固件包的信号量*/
extern xTimerHandle NetTimerHandler;

FATFS fs;                 // Work area (file system object) for logical drive
FIL fil;                  // file objects
uint32_t byteswritten;                /* File write counts */
uint32_t bytesread;                   /* File read counts */
uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
uint8_t rtext[200];                     /* File read buffers */
char filename[] = "STM32cube.txt";
char config_filename[]="config.txt";
char fota_filename[50];									/*文件名长度：<=50*/

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
	HAL_IWDG_Refresh(&hiwdg);
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
	HAL_IWDG_Refresh(&hiwdg);
	char * str = pvPortMalloc(100);
	memset(str,0,100);
	sprintf(str,CONFIG_REPLY_PACK,gGprs.gimei);
	LOG(LOG_INFO,"str:%s \r\n",str);
	MessageSend(str,1);
	vPortFree(str);
	return 1;
}

static char getPacStr[100] = {0};

/*从网络获取固件包--上报获取包*/
void getPackFromInternet(const uint8_t packnum)
{
	HAL_IWDG_Refresh(&hiwdg);
	memset(getPacStr,0,100);
	sprintf(getPacStr,GET_PACK_FROM_INTERNET,gGprs.gimei,packnum);
	LOG(LOG_INFO,"str:%s \r\n",getPacStr);	

	BaseType_t err;
	MessageSend(getPacStr,1);
}

/*获取固件包并写入FLASH当中去*/
static void gGetPackToFlash(void)
{
	HAL_IWDG_Refresh(&hiwdg);
	BaseType_t err;
	upgrade_info.Rec_Data_Len = 0;
	/*按照每一包（50K）去获取并升级，后期可能修改*/
	for(;upgrade_info.Req_Pack<=upgrade_info.PackSize+1;upgrade_info.Req_Pack++)
	{
		HAL_IWDG_Refresh(&hiwdg);
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
	HAL_IWDG_Refresh(&hiwdg);
	int ret = -1;
	/*首先获取升级包存储到FLASH*/
	if(upgrade_info.measure == diff)
	{
		/*离线模式开始存储*/
		gWriteDiffStartFlag(DIFF_START_FLAG);
	}
	gGetPackToFlash();
	/*使用网络升级的两种措施*/
	switch(upgrade_info.measure)
	{
		/*边升级边下载*/
		case sametime:
			upgrade_reply(sdownloadOK);
			//xEventGroupSetBits(InteracEventHandler,EventDownOk);
			ret = upgradeFAIL;//startUpgrade();
			HAL_Delay(2000);
			if(ret == upgradeOK)
			{
				upgrade_reply(supgradeOK);
				xEventGroupSetBits(InteracEventHandler,EventFirmOk);
				break;
			}
			/*更新失败，灯全部熄灭*/
			Down_OK_Led_Off;
			Job_OK_Led_Off;
			gUploadErrorCode(CAN_SEND_ERR);
			xEventGroupSetBits(InteracEventHandler,EventFirmFail);
			LOG(LOG_ERROR,"online upgrade FAIL!\r\n");

			/*同样需要写FLASH区域配置信息，不过这里是升级完成后，直接清空*/
				break;
		/*先下载后升级*/
		case diff:
			xEventGroupSetBits(InteracEventHandler,EventDownOk);
			/*离线模式结束存储*/
			gWriteDiffEndFlag(DIFF_END_FLAG);
			/*写FLASH*/
			upgrade_reply(sdownloadOK);
			break;
			/*需要写FLASH区域配置信息*/
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
	return ret;
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

static uint8_t sGetLength(uint8_t num_high,uint8_t num_low){
	uint8_t high = 0;uint8_t low = 0;
	if('0'<=num_high && num_high<='9')
	{
		high = (num_high-'0')*16;
	}else if('a'<=num_high && num_high<='z'){
		high = (num_high-'0'-39)*16;
	}else if('A'<=num_high && num_high<='Z'){
		high = (num_high-'0'-7)*16;
	}
	if('0'<=num_low && num_low<='9')
	{
		low = num_low-'0';
	}else if('a'<=num_low && num_low<='z'){
		low = num_low-'0'-39;
	}else if('A'<=num_low && num_low<='Z'){
		low = num_low-'0'-7;
	}
	return (high+low);
}


/*升级函数 ----测试版本printf*/
uint8_t startUpgrade(void)
{
	HAL_IWDG_Refresh(&hiwdg);
	int ret = -1;
	int i = 0;
	uint8_t sLinelen = 0;
	uint32_t sReadFlashDataLen = 0;
	HAL_IWDG_Refresh(&hiwdg);
	if(CanPre() != CAN_PRE_OK)
	{
		LOG(LOG_ERROR,"can pre failed .Please check can connection.\r\n");
		ret = upgradeFAIL;
		return ret;	
	}
	HAL_IWDG_Refresh(&hiwdg);
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
	sReadFlashDataLen+=sLinelen+1;																								//+1代表有一个换行符
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK){
		LOG(LOG_ERROR,"start line format is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;			
	}
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
	sReadFlashDataLen+=sLinelen+1;	
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
	{
		LOG(LOG_ERROR,"middle line format is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;			
	}
	HAL_IWDG_Refresh(&hiwdg);
	while(slineJudge(msg_s19_line.type)==middleline)
	{
		HAL_IWDG_Refresh(&hiwdg);
		//LOG(LOG_INFO,"upgrade_info.LinePack: %s \r\n sLinelen:%d\r\n",upgrade_info.LinePack,sLinelen);
		//printf("%s\n",upgrade_info.LinePack);
		if(CanSendLinePack(upgrade_info.LinePack)!=CAN_PRE_OK)
		{
			LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");
			ret = upgradeFAIL;
			return ret;					
		}
		memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
		sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
		sReadFlashDataLen+=sLinelen+1;	
		memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
		if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		{
			LOG(LOG_ERROR,"The middle line format is not correct.\r\n");
			ret = upgradeFAIL;
			return ret;				
		}
	}
	HAL_IWDG_Refresh(&hiwdg);	
	if(CanSendEndPack()!=CAN_PRE_OK)
	{
		LOG(LOG_ERROR,"can last line upgrade is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;			
	}	
	/*已经到最后一行*/
	ret = upgradeOK;
	return ret;
}

/*网络升级*/
static void sOnlineUpgrade(void){
	int ret = -1;
	/*防止按键误触发*/
	HAL_IWDG_Refresh(&hiwdg);
	if(upgrade_info.measure != sametime && upgrade_info.measure != diff && gFlag.DiffUpgradeStartFlag != DIFF_START_FLAG){
		/*不属于工作模式任何一种，为按键误触发，直接返回*/
		LOG(LOG_ERROR,"wrong button!\r\n");
		return;
	}
	/**网络在线升级模式*/
	if(upgrade_info.measure == sametime || upgrade_info.measure == diff)
	{
		xTimerStop(NetTimerHandler,0);		/*心跳包暂停*/
		sConfig_reply();						/*回复配置包*/	
		memset(&gpack_data,0,sizeof(gpack_data));			/*存储区清空*/
		sInterUpgradeProcess();			/*进行升级，如果是下载后直接升级，那么就直接升级，如果是先下载保存，那么就先下载后升级*/
		xTimerReset(NetTimerHandler,0);
		//xTimerStart(NetTimerHandler,0);			
	}
	/*离线下载后再升级  ---离线下载升级情况下，是不会有升级模式标识的*/
	if(upgrade_info.measure != sametime && upgrade_info.measure != diff && gFlag.DiffUpgradeEndFlag == DIFF_END_FLAG)
	{
			xEventGroupSetBits(InteracEventHandler,EventDownOk);
			STMFLASH_Read(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
			can_id.Can_num = gFlag.DiffCannum;
			can_id.RecID = gFlag.DiffCanRevid;
		  can_id.SendID = gFlag.DiffCanSendid;
			printf("%x %x %x\r\n",can_id.Can_num,can_id.RecID,can_id.SendID);
			ret = startUpgrade();
			if(ret == upgradeOK)
			{
				xEventGroupSetBits(InteracEventHandler,EventFirmOk);
				/*升级成功---清除标记*/
				gClearDiffFlag();
				return ;
			}
			/*更新失败，灯全部熄灭*/
			Down_OK_Led_Off;
			Job_OK_Led_Off;
			xEventGroupSetBits(InteracEventHandler,EventFirmFail);
			LOG(LOG_ERROR,"online upgrade FAIL!\r\n");
	}
}

/*SD卡输入升级函数，用于读取固件文件，并开始CAN升级**
***输入参数：无
***返回参数：升级成功
						 升级失败
*/
static uint8_t sSDUpgrade(void){
	int ret = -1;
	int i = 0;
	retSD = f_open(&fil,fota_filename,FA_READ);
	if(retSD)
	{
		printf("open file error: %d\r\n",retSD);
		ret = upgradeFAIL;
		return ret;
	}
	else{
		printf("open file success!\r\n");
	}
	HAL_IWDG_Refresh(&hiwdg);
	if(CanPre() != CAN_PRE_OK){
		LOG(LOG_ERROR,"can pre failed .Please check can connection.\r\n");
		ret = upgradeFAIL;
		return ret;		
	}
	HAL_IWDG_Refresh(&hiwdg);
	uint8_t sLinelen = 0;
	uint32_t sReadFlashDataLen = 0;
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	f_gets((char *)&msg_s19_line,sizeof(msg_s19_line),&fil);
	sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK){
		LOG(LOG_ERROR,"start line format is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;		
	}
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	f_gets((char *)&msg_s19_line,sizeof(msg_s19_line),&fil);
	sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
	{
		LOG(LOG_ERROR,"middle line format is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;		
	}
	while(slineJudge(msg_s19_line.type)==middleline)
	{
		HAL_IWDG_Refresh(&hiwdg);
		//printf("%s\n",upgrade_info.LinePack);
		
		if(CanSendLinePack(upgrade_info.LinePack)!=CAN_PRE_OK)
		{
			LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");
			ret = upgradeFAIL;
			return ret;				
		}
		memset(&msg_s19_line,0,sizeof(msg_s19_line));										//用于从FLASH中读取固件包
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		f_gets((char *)&msg_s19_line,sizeof(msg_s19_line),&fil);
		sLinelen=(sGetLength(msg_s19_line.msg_len[0],msg_s19_line.msg_len[1]))*2+4;
		memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
		if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		{
			LOG(LOG_ERROR,"middle line format is not correct.\r\n");
			ret = upgradeFAIL;
			return ret;			
		}
	}
	HAL_IWDG_Refresh(&hiwdg);
	/*已经到最后一行*/
	if(CanSendEndPack()!=CAN_PRE_OK)
	{
		LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");
		ret = upgradeFAIL;
		return ret;	
	}
	ret = upgradeOK;
	return ret;	
}

/*读取config文件，读取固件名称，CAN发送与接收ID*/
static void sReadConfigFile(void)
{
	retSD = f_open(&fil,config_filename,FA_READ);
	if(retSD)
	{
		printf("open file error: %d\r\n",retSD);
	}
	else{
		printf("open file success!\r\n");
	}
	HAL_IWDG_Refresh(&hiwdg);
	memset(rtext,0,sizeof(rtext));
	retSD = f_read(&fil,rtext,sizeof(rtext),(UINT *)&bytesread);
	if(retSD)
			printf(" read error!!! %d\r\n",retSD);
	else
	{
			printf(" read sucess!!! \r\n");
			printf(" read Data : %s\r\n",rtext);
	}	
	cJSON *res = cJSON_Parse((const char *)rtext);
	if(!res){printf("there is wrong!\r\n");}
	memset(fota_filename,0,sizeof(fota_filename));
	cJSON * RecID = cJSON_GetObjectItem(res,"recid");
  can_id.RecID= RecID->valueint;
	cJSON * SendID = cJSON_GetObjectItem(res,"sendid");
  can_id.SendID= SendID->valueint;
	cJSON * cannum = cJSON_GetObjectItem(res,"cannum");
  can_id.Can_num= cannum->valueint;
	cJSON * FILENAME = cJSON_GetObjectItem(res,"filename");
	char * string = FILENAME->valuestring;
	memcpy(fota_filename,string,strlen(string));			/*获取文件名称*/
	printf("%d %d %d %s\r\n",can_id.Can_num,can_id.RecID,can_id.SendID,fota_filename);
	cJSON_Delete(res);
	retSD = f_close(&fil);
  if(retSD)  
        printf(" close error!!! %d\r\n",retSD);
    else
        printf(" close sucess!!! \r\n");	
}

/*文件系统测试函数---后续待删除*/
void Fatfs_RW_test(void)
{
	  printf("\r\n ****** FatFs Example ******\r\n\r\n");
 
    /*##-1- Register the file system object to the FatFs module ##############*/
    retSD = f_mount(&fs, "", 1);
    if(retSD)
    {
        printf(" mount error : %d \r\n",retSD);
        Error_Handler();
    }
    else
        printf(" mount sucess!!! \r\n");
     
    /*##-2- Create and Open new text file objects with write access ######*/
    retSD = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(retSD)
        printf(" open file error : %d\r\n",retSD);
    else
        printf(" open file sucess!!! \r\n");
     
    /*##-3- Write data to the text files ###############################*/
    retSD = f_write(&fil, wtext, sizeof(wtext), (void *)&byteswritten);
    if(retSD)
        printf(" write file error : %d\r\n",retSD);
    else
    {
        printf(" write file sucess!!! \r\n");
        printf(" write Data : %s\r\n",wtext);
    }
     
    /*##-4- Close the open text files ################################*/
    retSD = f_close(&fil);
    if(retSD)
        printf(" close error : %d\r\n",retSD);
    else
        printf(" close sucess!!! \r\n");
     
    /*##-5- Open the text files object with read access ##############*/
    retSD = f_open(&fil, filename, FA_READ);
    if(retSD)
        printf(" open file error : %d\r\n",retSD);
    else
        printf(" open file sucess!!! \r\n");
     
    /*##-6- Read data from the text files ##########################*/
    retSD = f_read(&fil, rtext, sizeof(rtext), (UINT*)&bytesread);
    if(retSD)
        printf(" read error!!! %d\r\n",retSD);
    else
    {
        printf(" read sucess!!! \r\n");
        printf(" read Data : %s\r\n",rtext);
    }
     
    /*##-7- Close the open text files ############################*/
    retSD = f_close(&fil);
    if(retSD)  
        printf(" close error!!! %d\r\n",retSD);
    else
        printf(" close sucess!!! \r\n");
     
    /*##-8- Compare read data with the expected data ############*/
    if(bytesread == byteswritten)
    { 
        printf(" FatFs is working well!!!\r\n");
    }
}


/*SD卡升级---测试版*/
static void sOfflineUpgrade(void){
	int ret  = -1;
	HAL_IWDG_Refresh(&hiwdg);
	MX_FATFS_Init();
	LOG(LOG_INFO,"*******start upgrade sd ********\r\n");
	/*挂接文件系统，重新挂接相当于初始化*/
	retSD = f_mount(&fs,"",0);
	if(retSD)
	{
		printf("file mount failed!\r\n mount error:%d\r\n",retSD);
		return;
	}
	else
	{
		printf("file mount success!\r\n");
	}
	/*读取config文件*/
	HAL_IWDG_Refresh(&hiwdg);
	sReadConfigFile();
	/*服务固件文件，开始进行升级*/
	HAL_IWDG_Refresh(&hiwdg);
	ret = sSDUpgrade();
	/*升级成功*/
	if(ret == upgradeOK)
	{
		Log(LOG_INFO,"sd upgrade success!\r\n");
		xEventGroupSetBits(InteracEventHandler,EventFirmOk);
		return;
	}else{
		//升级失败提醒
		xEventGroupSetBits(InteracEventHandler,EventFirmFail);
		LOG(LOG_ERROR,"sd upgrade FAIL!\r\n");
		return;
	}	
}



/*****升级任务*****/
/******/
void UpgradeTask(void *pArg)   
{
	BaseType_t err;
	while(1){
		/*等待信号同步*/
		Down_OK_Led_Off;
		Job_OK_Led_Off;
		HAL_IWDG_Refresh(&hiwdg);
		err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
		/*成功获取到信号量*/
		if(err == pdTRUE)
		{
			switch(upgrade_info.method){
				/*网络升级方式*/
				case online:
					Log(LOG_INFO,"online\r\n");
					sOnlineUpgrade();
					break;
				/*SD卡升级方式*/
				case offline:
					Log(LOG_INFO,"offline\r\n");
					sOfflineUpgrade();
					break;
				default:
					break;
			}
				/*清除标记*/
				memset(&upgrade_info,0,sizeof(upgrade_info));
				switch(gFlag.ModeFlag){
					case NET_MODE_FLAG:
						upgrade_info.method =online;
						break;
					case SD_MODE_FLAG:
						upgrade_info.method = offline;
						break;
					default:
						upgrade_info.method = online;break;
				}
		}
		else
		{
			LOG(LOG_ERROR,"getting Config failed. Please Check!\r\n");
			sysReset();
		}		
		vTaskDelay(50);
	}
}

