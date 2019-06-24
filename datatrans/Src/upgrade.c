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
SemaphoreHandle_t ConfigBinarySemaphore;					/*���õ��ź���*/
SemaphoreHandle_t GettingPackBinarySemaphore;			/*��ȡ�̼������ź���*/

/*��������***
**���ڽ��й̼��ĸ���
*	
**/

const char supgradeOK[10]="updateok\0";
const char supgradeFAIL[15]="updatefail\0";
const char sdownloadOK[15]="downloadok\0";
const char sdownloadFAIL[15]="downloadfail\0";

#define CONFIG_REPLY_PACK 	"{\"type\":\"configok\",\"imei\":\"%s\"}"
#define GET_PACK_FROM_INTERNET 	"{\"type\":\"reqpack\",\"imei\":\"%s\",\"packnum\":%d}"
#define UPGRADE_REPLY_PACK	"{\"type\":\"%s\",\"imei\":\"%s\"}"

/*�����ظ���ɰ�*/
static void upgrade_reply(const char * result_str)
{
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,UPGRADE_REPLY_PACK,result_str,gGprs.gimei);
	LOG(LOG_INFO,"str:%s \r\n",str);
	MessageSend(str,1);
	vPortFree(str);
}

/*�ظ�CONFIG��*/
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

/*�������ȡ�̼���--�ϱ���ȡ��*/
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
	/*3�λ�ȡ��ʧ�ܻ���3�λ�ȡ�İ�������ʧ���ˣ���ô֪ͨƽ̨�����ݳ������¿�ʼ*/
	/*�ϱ�ERRCODE��*/
}

/*��ȡ�̼�����д��FLASH����ȥ*/
static void gGetPackToFlash(void)
{
	BaseType_t err;
	upgrade_info.Rec_Data_Len = 0;
	/*����ÿһ����50K��ȥ��ȡ�����������ڿ����޸�*/
	for(;upgrade_info.Req_Pack<=upgrade_info.PackSize+1;upgrade_info.Req_Pack++)
	{
		/*�ȴ����յ�����50K��ȷ�̼�������ź��� */
		err = xSemaphoreTake(GettingPackBinarySemaphore,portMAX_DELAY);
		upgrade_info.Req_Pack = upgrade_info.Rev_Pack+1;		/*�Ѿ���ȡ���İ�����*/
		/*�ɹ���ȡ���ź���*/
		if(err == pdTRUE)
		{
			//LOG(LOG_INFO,"upgrade_info.Rev_Pack:%d!\r\n",upgrade_info.Rev_Pack);
			/*��50K������д�뵽FLASH����ȥ��ע���·��ı�Ŵ�0����1��ʼ*/
			STMFLASH_Write(PACK_ADDR_START+upgrade_info.Rec_Data_Len,gpack_data.Pack_Data,gpack_data.Pack_Data_len);
			upgrade_info.Rec_Data_Len+=gpack_data.Pack_Data_len;
			memset(&gpack_data,0,sizeof(gpack_data));			/*�洢�����*/
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

/*������������*/
static uint8_t sInterUpgradeProcess()
{
	int ret = -1;
	/*���Ȼ�ȡ�������洢��FLASH*/
	gGetPackToFlash();
	/*ʹ���������������ִ�ʩ*/
	switch(upgrade_info.measure)
	{
		/*������������*/
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
			/*ͬ����ҪдFLASH����������Ϣ������������������ɺ�ֱ�����*/
				break;
		/*�����غ�����*/
		case diff:
			vTaskDelay(100);
			upgrade_reply(sdownloadOK);
			break;
			/*��ҪдFLASH����������Ϣ*/
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

static void sReadPackFromFlash(uint8_t * str,uint8_t * des){
	
	
	
	
}


static s19_end slineJudge(uint8_t type[])
{
	if(type[1]=='7' || type[1]=='8' ||type[1]=='9' )				//��S7/S8/S9 ������
	{
		return endline;
	}else{
		return middleline;
	}
}

/*��������*/
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
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//���ڴ�FLASH�ж�ȡ�̼���
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//��ȡ���ݳ���
	sReadFlashDataLen+=sLinelen+1;																								//+1������һ�����з�
	memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
	if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		LOG(LOG_ERROR,"start line format is not correct.\r\n");	
	memset(&msg_s19_line,0,sizeof(msg_s19_line));										//���ڴ�FLASH�ж�ȡ�̼���
	memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
	STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
	sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//��ȡ���ݳ���
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
		memset(&msg_s19_line,0,sizeof(msg_s19_line));										//���ڴ�FLASH�ж�ȡ�̼���
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		STMFLASH_Read(PACK_ADDR_START+sReadFlashDataLen,(uint8_t*)&msg_s19_line,sizeof(msg_s19_line));	
		sLinelen = ((msg_s19_line.msg_len[0]-'0')*16+(msg_s19_line.msg_len[1]-'0'))*2+4;	//��ȡ���ݳ���
		sReadFlashDataLen+=sLinelen+1;	
		memcpy(upgrade_info.LinePack,&msg_s19_line,sLinelen);
		if(gPackCheck((char *)upgrade_info.LinePack,sLinelen)!=PackOK)
		{
			LOG(LOG_ERROR,"middle line format is not correct.\r\n");	
			break;
		}
	}		
	/*�Ѿ������һ��*/
	if(CanSendEndPack()!=CAN_PRE_OK)
	{
		LOG(LOG_ERROR,"can line upgrade is not correct.\r\n");	
	}
	ret = upgradeOK;
	return ret;
}

/*****��������*****/
/******/
extern xTimerHandle NetTimerHandler;
void UpgradeTask(void *pArg)   
{
	BaseType_t err;
	while(1){
			/*ʹ�������������*/
		if(1)  //upgrade_info.method == online)																													/*�����翪ʼ����*/
		{
			/*�ж�upgradinfo�Ƿ��Ѿ���д���ˣ�Ȼ�����ж���ֱ�Ӷ�ȡFLASH���������Ƚ�����������*/
			/*�ȴ���������ź��� */
//			sInterUpgradeProcess();
//			while(1);
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			xTimerStop(NetTimerHandler,0);
			/*�ɹ���ȡ���ź���*/
			if(err == pdTRUE)
			{
				sConfig_reply();						/*�ظ����ð�*/	
				memset(&gpack_data,0,sizeof(gpack_data));			/*�洢�����*/
				sInterUpgradeProcess();			/*������������������غ�ֱ����������ô��ֱ������������������ر��棬��ô�������غ�����*/
				memset(&upgrade_info,0,sizeof(upgrade_info));			/*�洢�����*/
				xTimerStart(NetTimerHandler,0);
			}
			else
			{
				LOG(LOG_ERROR,"getting Config failed. Please Check!\r\n");
				sysReset();
			}
		}
		/*ʹ��SD����������*/
		else if(upgrade_info.method == offline)  																									/*��SD��ʼ��ȡ���ݣ����������������漰������������Ҳ�����жϣ�ֻ��ֱ����������*/
		{
			
		}
		vTaskDelay(50);
	}
}

