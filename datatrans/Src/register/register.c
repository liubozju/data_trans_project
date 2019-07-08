#include "log.h"
#include "register.h"
#include "cjson.h"
#include "message.h"
#include <string.h>
#include "Interactive.h"
#include "iwdg.h"

extern gprs gGprs;
extern xTimerHandle NetTimerHandler;
SemaphoreHandle_t RigisterBinarySemaphore;		/*设备注册使用信号量*/

#define DEVICE_REGISTER_PACK 	"{\"type\":\"register\",\"imsi\":\"%s\",\"imei\":\"%s\",\"csq\":\"%d\"}"

/*设备上线以后注册使用函数*/
int gDeviceRegister(void)
{
	memset((void *)gGprs.gimei,0,20);
	memset((void *)gGprs.gimsi,0,20);
	HAL_IWDG_Refresh(&hiwdg);
	gGprs.gGetGprsInfo("AT+CSQ\r\n","+CSQ: ");						/*获取信号强度*/
	gGprs.gGetGprsInfo("AT+CGSN\r\n","+CGSN: \"");				/*获取IMEI*/
	gGprs.gGetGprsInfo("AT+CIMI\r\n","460");							/*获取IMSI*/
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








