#include "log.h"
#include "register.h"
#include "cjson.h"
#include "message.h"
#include <string.h>

extern gprs gGprs;
SemaphoreHandle_t RigisterBinarySemaphore;		/*设备注册使用信号量*/

#define DEVICE_REGISTER_PACK 	"{\"type\":\"register\",\"imsi\":\"%s\",\"imei\":\"%s\",\"csq\":\"%d\"}"

/*设备上线以后注册使用函数*/
int gDeviceRegister(void)
{
	memset((void *)gGprs.gimei,0,20);
	memset((void *)gGprs.gimsi,0,20);
	gGprs.gGetGprsInfo("AT+CSQ\r\n","+CSQ: ");						/*获取信号强度*/
	gGprs.gGetGprsInfo("AT+CGSN\r\n","+CGSN: \"");				/*获取IMEI*/
	gGprs.gGetGprsInfo("AT+CIMI\r\n","460");							/*获取IMSI*/
		
	taskENTER_CRITICAL();
	RigisterBinarySemaphore = xSemaphoreCreateBinary();
	
//	cJSON *res = cJSON_CreateObject();
//	cJSON_AddStringToObject(res, "type", "register");
//	cJSON_AddStringToObject(res, "imsi", (const char *)"460022201575463");
//		cJSON_AddStringToObject(res, "imei", (const char *)"460022201575463");
//	cJSON_AddStringToObject(res, "imei",  (const char *)gGprs.gimei);
//	cJSON_AddNumberToObject(res, "csq", gGprs.csq);
//	cJSON_AddNumberToObject(res, "hardv", 10);
//	cJSON_AddStringToObject(res, "softv", "0102\0");
  char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,DEVICE_REGISTER_PACK,gGprs.gimsi,gGprs.gimei,gGprs.csq);
	LOG(LOG_INFO,"str:%s \r\n",str);	

	//str = cJSON_Print(res);
	taskEXIT_CRITICAL();
  BaseType_t err;
  for(int i=0;i<3;i++)
  {
		MessageSend(str,1);
		err = xSemaphoreTake(RigisterBinarySemaphore,10000);
	  if(err == pdTRUE)
		{
	    //cJSON_Delete(res);
			vPortFree(str);
			vSemaphoreDelete(RigisterBinarySemaphore);
      LOG(LOG_DEBUG,"register ok\r\n");
			return 1;
		}
		LOG(LOG_ERROR,"register %d times\r\n",i);
  }
	//cJSON_Delete(res); 
	vPortFree(str);	
	LOG(LOG_ERROR,"register failed\r\n");
	LOG(LOG_ERROR,"System Rebooting\r\n");
	NVIC_SystemReset();
	return -1;		
}








