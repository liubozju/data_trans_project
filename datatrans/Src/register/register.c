#include "log.h"
#include "register.h"
#include "cjson.h"
#include "message.h"
#include <string.h>

extern gprs gGprs;
SemaphoreHandle_t RigisterBinarySemaphore;		/*�豸ע��ʹ���ź���*/

#define DEVICE_REGISTER_PACK 	"{\"type\":\"register\",\"imsi\":\"%s\",\"imei\":\"%s\",\"csq\":\"%d\"}"

/*�豸�����Ժ�ע��ʹ�ú���*/
int gDeviceRegister(void)
{
	memset((void *)gGprs.gimei,0,20);
	memset((void *)gGprs.gimsi,0,20);
	gGprs.gGetGprsInfo("AT+CSQ\r\n","+CSQ: ");						/*��ȡ�ź�ǿ��*/
	gGprs.gGetGprsInfo("AT+CGSN\r\n","+CGSN: \"");				/*��ȡIMEI*/
	gGprs.gGetGprsInfo("AT+CIMI\r\n","460");							/*��ȡIMSI*/
		
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








