#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"

Upgrade_info upgrade_info;
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
	//deviceDirectSend(str);
	vPortFree(str);
	vSemaphoreDelete(ConfigBinarySemaphore);
	return 1;
}

/*�������ȡ�̼���--�ϱ���ȡ��*/
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
	/*3�λ�ȡ��ʧ�ܻ���3�λ�ȡ�İ�������ʧ���ˣ���ô֪ͨƽ̨�����ݳ������¿�ʼ*/
	/*�ϱ�ERRCODE��*/
	
	
}

/*������������*/
static uint8_t sInterUpgradeProcess()
{
	int ret = -1;
	/*ʹ���������������ִ�ʩ*/
	switch(upgrade_info.measure)
	{
		/*������������*/
		case sametime:
				sametimeUpgrade();
				break;
		/*�����غ�����*/
		case diff:
			
		default:
			LOG(LOG_ERROR,"upgrade_info.measure is wrong ! Please Check!\r\n");
			break;		
	}
}

/*�����������ش�����*/
uint8_t sametimeUpgrade(void)
{
	int ret = -1;
	upgrade_info.Req_Pack = 0;
	/*����ÿһ��ȥ��ȡ�����������ڿ����޸�*/
	for(;upgrade_info.Req_Pack<upgrade_info.PackSize;upgrade_info.Req_Pack++)
	{
		getPackFromInternet(upgrade_info.Req_Pack);
		
	}
	
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
			/*�ȴ���������ź��� */
			err = xSemaphoreTake(ConfigBinarySemaphore,portMAX_DELAY);
			/*�ɹ���ȡ���ź���*/
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
		/*ʹ��SD����������*/
		else if(upgrade_info.method == offline)  																									/*��SD��ʼ��ȡ���ݣ����������������漰������������Ҳ�����жϣ�ֻ��ֱ����������*/
		{
			
		}
		vTaskDelay(50);
	}
}
