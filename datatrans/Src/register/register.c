#include "log.h"
#include "register.h"
#include "cjson.h"
#include "message.h"
#include <string.h>
#include "Interactive.h"
#include "iwdg.h"

extern gprs gGprs;
extern xTimerHandle NetTimerHandler;
SemaphoreHandle_t RigisterBinarySemaphore;		/*�豸ע��ʹ���ź���*/

#define DEVICE_REGISTER_PACK 	"{\"type\":\"register\",\"imsi\":\"%s\",\"imei\":\"%s\",\"csq\":\"%d\"}"

/*�豸�����Ժ�ע��ʹ�ú���*/
int gDeviceRegister(void)
{
	memset((void *)gGprs.gimei,0,20);
	memset((void *)gGprs.gimsi,0,20);
	HAL_IWDG_Refresh(&hiwdg);
	gGprs.gGetGprsInfo("AT+CSQ\r\n","+CSQ: ");						/*��ȡ�ź�ǿ��*/
	gGprs.gGetGprsInfo("AT+CGSN\r\n","+CGSN: \"");				/*��ȡIMEI*/
	gGprs.gGetGprsInfo("AT+CIMI\r\n","460");							/*��ȡIMSI*/
	HAL_IWDG_Refresh(&hiwdg);	
	taskENTER_CRITICAL();
	RigisterBinarySemaphore = xSemaphoreCreateBinary();
	
  char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,DEVICE_REGISTER_PACK,gGprs.gimsi,gGprs.gimei,gGprs.csq);
	LOG(LOG_INFO,"str:%s \r\n",str);	

	taskEXIT_CRITICAL();
  BaseType_t err;
  for(int i=0;i<3;i++)
  {
		HAL_IWDG_Refresh(&hiwdg);
		MessageSend(str,1);
		err = xSemaphoreTake(RigisterBinarySemaphore,10000);
	  if(err == pdTRUE)
		{
			vPortFree(str);
			vSemaphoreDelete(RigisterBinarySemaphore);
      LOG(LOG_DEBUG,"register ok\r\n");
			xEventGroupSetBits(InteracEventHandler,EventNetledOn);
			xEventGroupClearBits(InteracEventHandler,EventNetledFlick);
			//Net_LED_On;
			//xTimerStart(NetTimerHandler,portMAX_DELAY);
			return 1;
		}
		LOG(LOG_ERROR,"register %d times\r\n",i);
  }
	vPortFree(str);	
	LOG(LOG_ERROR,"register failed\r\n");
	LOG(LOG_ERROR,"System Rebooting\r\n");
	Net_LED_Off;
	NVIC_SystemReset();
	return -1;		
}








