#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"
#include "myflash.h"

Upgrade_info upgrade_info;
gPack_Data  gpack_data;
SemaphoreHandle_t ConfigBinarySemaphore;					/*���õ��ź���*/
SemaphoreHandle_t GettingPackBinarySemaphore;			/*��ȡ�̼������ź���*/

/*��������***
**���ڽ��й̼��ĸ���
*	
**/

#define CONFIG_REPLY_PACK 	"{\"type\":\"configok\",\"imei\":\"%s\"}"
#define GET_PACK_FROM_INTERNET 	"{\"type\":\"reqpack\",\"imei\":\"%s\",\"packnum\":%d}"


/*�ظ�CONFIG��*/
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

/*�������ȡ�̼���--�ϱ���ȡ��*/
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
	/*3�λ�ȡ��ʧ�ܻ���3�λ�ȡ�İ�������ʧ���ˣ���ô֪ͨƽ̨�����ݳ������¿�ʼ*/
	/*�ϱ�ERRCODE��*/
}

/*��ȡ�̼�����д��FLASH����ȥ*/
static void gGetPackToFlash(void)
{
	BaseType_t err;
	upgrade_info.Rec_Data_Len = 0;
	/*����ÿһ����50K��ȥ��ȡ�����������ڿ����޸�*/
	for(;upgrade_info.Req_Pack<upgrade_info.PackSize;upgrade_info.Req_Pack++)
	{
		/*�ȴ����յ�����50K��ȷ�̼�������ź��� */
		err = xSemaphoreTake(GettingPackBinarySemaphore,portMAX_DELAY);
		upgrade_info.Req_Pack = upgrade_info.Rev_Pack+1;		/*�Ѿ���ȡ���İ�����*/
		/*�ɹ���ȡ���ź���*/
		if(err == pdTRUE)
		{
			LOG(LOG_INFO,"upgrade_info.Rev_Pack:%d!\r\n",upgrade_info.Rev_Pack);
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
		getPackFromInternet(upgrade_info.Req_Pack);	
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
				startUpgrade();
		
			/*ͬ����ҪдFLASH����������Ϣ������������������ɺ�ֱ�����*/
				break;
		/*�����غ�����*/
		case diff:
			
			/*��ҪдFLASH����������Ϣ*/
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

static void sReadPackFromFlash(uint8_t * str,uint8_t * des){
	
	
	
	
}


/*��������*/
uint8_t startUpgrade(void)
{
	int ret = -1;
	int i = 0;
	uint16_t sPackNum = upgrade_info.Rec_Data_Len/gPack_Data_Len;		//����50K�ְ�������ݰ�����
	uint32_t sRemain = upgrade_info.Rec_Data_Len % gPack_Data_Len;	//���һ��ʣ����ֽ�����
	char * scopyStart = NULL;
	char * scopyEnd = NULL;
	uint32_t copyLen = 0;
	for(i = 0;i<sPackNum;i++)
	{
		/*���ȴ�FLASH�ж�ȡ50K����*/
		memset(&gpack_data,0,sizeof(gpack_data));
		STMFLASH_Read(PACK_ADDR_START,gpack_data.Pack_Data,gPack_Data_Len);	
		/*��50K�����ݰ��н�����ÿһ�У�������У��*/
		memset(upgrade_info.LinePack,0,sizeof(upgrade_info.LinePack));
		
		
	}

	
	
	/*��ѭ��ʹ��CAN���и���*/
	
	
	/**ѭ�����½������ϱ�������*/
	
	
	return ret;
}

/*****��������*****/
/******/
void UpgradeTask(void *pArg)   
{
	BaseType_t err;
	while(1){
			/*ʹ�������������*/
		if(1)  //upgrade_info.method == online)																													/*�����翪ʼ����*/
		{
			/*�ж�upgradinfo�Ƿ��Ѿ���д���ˣ�Ȼ�����ж���ֱ�Ӷ�ȡFLASH���������Ƚ�����������*/
			/*�ȴ���������ź��� */
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			/*�ɹ���ȡ���ź���*/
			if(err == pdTRUE)
			{
				sConfig_reply();						/*�ظ����ð�*/	
				memset(&gpack_data,0,sizeof(gpack_data));			/*�洢�����*/
				sInterUpgradeProcess();			/*������������������غ�ֱ����������ô��ֱ������������������ر��棬��ô�������غ�����*/
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

