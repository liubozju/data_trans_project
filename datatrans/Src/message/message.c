#include "message.h"
#include "gprs.h"
#include "log.h"
#include "register.h"
#include "delay.h"
#include "conversion.h"
#include "cjson.h"
#include "can.h"
#include "upgrade.h"

QueueHandle_t UsartRecMsgQueue;   //接收信息队列句柄
static QueueHandle_t UsartSenMsgQueue;   //发送信息队列句柄
extern gprs gGprs;
extern SemaphoreHandle_t NetBinarySemaphore;
extern SemaphoreHandle_t RigisterBinarySemaphore;

static Msg Message;
int MessageReceiveFromISR(char *msg)
{
	  BaseType_t err;
	  BaseType_t xHighPriorityTaskWoken;
		if(UsartRecMsgQueue!=NULL)
		{
			err = xQueueSendFromISR(UsartRecMsgQueue,msg,&xHighPriorityTaskWoken);
			if(err!=pdTRUE)
			{
				LOG(LOG_ERROR,"usart idel add queue failed\r\n");
			}
			portYIELD_FROM_ISR(xHighPriorityTaskWoken);
			return 1;
		}
		else
		{
			LOG(LOG_ERROR,"this is wrong .\r\n");
			return 0;
		}
}

/*将PayLoad数据包发送到发送队列中*/
int MessageSend(const char *msg,u8 head)
{
	BaseType_t err;
	char * Msg = pvPortMalloc(200);
	memset(Msg,0,200);
	if(head==0)
	{
	  strcpy(Msg,"AT-");
		strcat(Msg,msg);
	}
	else if(head==1)
	{
		strcpy(Msg,msg);
	}
	if(UsartSenMsgQueue!=NULL)
	{
		err = xQueueSend(UsartSenMsgQueue,Msg,portMAX_DELAY);
		if(err!=pdTRUE)
		{
			LOG(LOG_ERROR,"Message failed\r\n");
			return -1;
		}
	}
	vPortFree(Msg);
	return 1;
}


/*解析平台下发的每个固件包***/
/**
*	进行固件完整性校验，数据解密等操作
*/
static uint8_t sAnalyzePack(char *msg)
{
	char * str = pvPortMalloc(600);
	memset(str,0,600);
	volatile char * msg_start=NULL;
	volatile char * msg_end=NULL;
	uint16_t str_len = 0;
	if(NULL != strstr(msg,PACKSTART) && NULL != strstr(msg,PACKEND))
	{
		msg_start=strstr(msg,PACKSTART);			/*获取每一包的PACK信息*/
		msg_end = strstr(msg,PACKEND);
		uint8_t len = msg_end - msg_start;
		memcpy(str,msg+9,len+1);
		LOG(LOG_INFO,"payload :%s\r\n",str);
		str_len = len +1;
		if(gPackCheck(str,str_len)==PackOK)
			return PackOK;
		else
			return PackFail;
	}		
}

/**解析平台下发的配置包*/
static void sConfigAnalyze(char * msg)
{
	char * str = pvPortMalloc(600);
	memset(str,0,600);
	volatile char * msg_start=NULL;
	volatile char * msg_end=NULL;
	if(NULL != strstr(msg,LEFT_JSON) && NULL != strstr(msg,RIGHT_JSON))
	{
		msg_start=strstr(msg,LEFT_JSON);			/*获取配置JSON信息*/
		msg_end = strstr(msg,RIGHT_JSON);
		uint8_t len = msg_end - msg_start;
		for(int i =0;i<len+1;i++)
		{
			*(str+i) = *(msg_start+i);
		}
		LOG(LOG_INFO,"payload :%s\r\n",str);
		cJSON *res = cJSON_Parse(str);
		if(!res){LOG(LOG_ERROR,"THE Config is wrong!\r\n");sysReset();};
		cJSON * RecID = cJSON_GetObjectItem(res,"RecID");
    can_id.RecID= RecID->valueint;
		cJSON * SendID = cJSON_GetObjectItem(res,"SendID");
    can_id.SendID= SendID->valueint;
		cJSON * cannum = cJSON_GetObjectItem(res,"cannum");
    can_id.Can_num= cannum->valueint;
		cJSON * PaSize = cJSON_GetObjectItem(res,"PackSize");
    upgrade_info.PackSize= PaSize->valueint;	
		cJSON * Measure = cJSON_GetObjectItem(res,"measure");
    upgrade_info.measure= Measure->valueint;			
		LOG(LOG_INFO,"%d %d %d %d \r\n",can_id.Can_num,can_id.RecID,can_id.SendID,upgrade_info.PackSize);
		cJSON_Delete(res);
	}
	vPortFree(str);
}

void MessageReceiveTask(void *pArg)   //命令解析任务
{
	char* buf = pvPortMalloc(512);
	char* rep = pvPortMalloc(60);
	while(1)
	{
		xQueueReceive(UsartRecMsgQueue,buf,portMAX_DELAY);

		LOG(LOG_DEBUG,"Get buf %s\r\n",buf);
		if(NULL != strstr(buf,CONFIG))																												/*接收到配置包*/
		{
			sConfigAnalyze(buf);
			/*释放信号量*/
			xSemaphoreGive(ConfigBinarySemaphore);
		}
		if(NULL != strstr(buf,PACKSTART) && NULL != strstr(buf,PACKEND))												/*接收到升级包*/
		{
			if(sAnalyzePack(buf)==PackOK)
			{
				 xSemaphoreGive(GettingPackBinarySemaphore);
			}
		}
	  if(NULL != strstr(buf,PBREADY))																												/*接收到PBREADY*/
	  {
			xQueueReceive(gGprs.GprsRepQueue,rep,0);
			xSemaphoreGive(gGprs.GprsConnectBinarySemaphore);
	  }
		if(gGprs.gprsFlag.gGPRSConnectFlag == 1 && gGprs.gprsFlag.gINFOFlag == 0)							/*联网命令解析*/
		{
			if(xQueuePeek(gGprs.GprsRepQueue,rep,0)==pdTRUE)
			{
				if(NULL != strstr(buf,rep))
				{
					xQueueReceive(gGprs.GprsRepQueue,rep,0);
					xSemaphoreGive(gGprs.GprsConnectBinarySemaphore);
				}
		 	}
	  }
		if(gGprs.gprsFlag.gINFOFlag == 1)											/*INFO命令解析*/
		{
			if(xQueuePeek(gGprs.GprsRepQueue,rep,0)==pdTRUE)
			{
				if(NULL != strstr(buf,rep))
				{
					xQueueReceive(gGprs.GprsRepQueue,rep,0);
					if(gGprs.GprsRepQueue!=NULL)
	        {
			       xQueueOverwrite(gGprs.GPRSINFORepQueue,buf);
  	      }
					xSemaphoreGive(gGprs.GprsINFOBinarySemaphore);
				}
			}
	  }
		if(gGprs.gprsFlag.gNetFlag == 1)                               /*网络检查*/
		{
			if(NULL != strstr(buf,"CONNECT"))
			{
				if(NetBinarySemaphore!=NULL)
				{
					xSemaphoreGive(NetBinarySemaphore);
					xSemaphoreGive(Message.OKBinarySemaphore);
				}
			}
	   }		
		if(gGprs.gprsFlag.gRegisterFlag == 1)  
		{			
			if(RigisterBinarySemaphore!=NULL)                    /*注册回复*/
			{  
				 if(NULL != strstr(buf,REGISTER_REP))
				 {
					 if(RigisterBinarySemaphore!=NULL)
					 xSemaphoreGive(RigisterBinarySemaphore);
				 }
			}
	  }
//		if(CSQBinarySemaphore!=NULL)                          /*CSQ回复*/
//		{  
//			 if(NULL != strstr(buf,CSQ_REP))
//		   {
//				 if(CSQBinarySemaphore!=NULL)
//		     xSemaphoreGive(CSQBinarySemaphore);
//			 }
//		}		
		if(NULL != strstr(buf,"OK"))
		{
		   xSemaphoreGive(Message.OKBinarySemaphore);
		}
		vTaskDelay(100);
	}
	vPortFree(buf);
	vPortFree(rep);
}

/*串口发送任务*/
void MessageSendTask(void *pArg)
{
	char* buf = pvPortMalloc(200);
	u8 i = 0;
	while(1)
	{
		memset(buf,0,200);
		xQueueReceive(UsartSenMsgQueue,buf,portMAX_DELAY);			/*查询发送队列有无数据，有就取出一帧数据发送*/

		if(NULL!=strstr(buf,"AT-"))
		{
			char * result = strstr(buf,"AT-");
			result+=3;
			Message.payload = result;
			Message.payloadLen = strlen(Message.payload);
			Message.send(0);
		}
		else
		{
			Message.payload = buf;
			Message.payloadLen = strlen(Message.payload);
			Message.send(1);
		}
		BaseType_t err = pdFALSE;
		i = 0;
		while(1)
		{
			i++;
			if(Message.OKBinarySemaphore!=NULL)
			{
			   err = xSemaphoreTake(Message.OKBinarySemaphore,1000);				/*等待模组返回OK。任何命令模组都会有一个OK返回*/
				{
					if(err == pdTRUE)																						/*收到OK，说明发送成功。退出循环*/
					{
						break;
					}
					LOG(LOG_DEBUG,"OKBinarySemaphore :%d\r\n",i);
				}
			}
			if(i>=3)												/*重复发送3次*/
			{
				break;
			}
		}
	}
	vPortFree(buf);
}

/*串口接收、发送队列创建*/
void MsgInfoConfig(void)
{
	GettingPackBinarySemaphore= xSemaphoreCreateBinary();
	ConfigBinarySemaphore = xSemaphoreCreateBinary();
	UsartRecMsgQueue = xQueueCreate(10,600);		/*接收队列--10*600  因为接收数据要尽量长才可以接收文件*/
	UsartSenMsgQueue = xQueueCreate(8,200);			/*发送队列--8*200   发送数据数据量少，队列大小可以小一点*/   
	Message.OKBinarySemaphore = xSemaphoreCreateBinary();		/*创建信号量，用于接收模组回复的OK*/
	
	Message.send = deviceSend;									/*配置消息的发送函数*/													
}


/*消息发送注册函数*/
void deviceSend(u8 head)
{
	char *cmd = pvPortMalloc(200);
	char *len = pvPortMalloc(6);
	char *sendmsg = pvPortMalloc(200);
	uint8_t slinknum[2]={0};uint8_t glinknum=0;
	memset(slinknum,0,2);
	slinknum[0] = glinknum+'0';
	
	if(head==1)					/*使用命令发送业务数据*/
	{
		memset(cmd,0,200);
		memset(len,0,6);
		memset(sendmsg,0,200);
		strcpy(sendmsg,Message.payload);
		sprintf(len, "%d", strlen(Message.payload));
		strcpy(cmd,"AT+TCPSEND=0,");
		strcat(cmd,len);
		strcat(cmd,"\r\n");
		LOG(LOG_DEBUG,"at_cmd: %s",cmd);
		Usart2Write((const char *)cmd);
		memset(cmd,0,200);
		vTaskDelay(100);
		LOG(LOG_DEBUG,"sendmsg: %s %d\r\n",sendmsg,strlen(sendmsg));
		Usart2Write((const char *)sendmsg);
	}
	else								/*发送通用的AT指令*/
	{
		memset(cmd,0,200);
		strcpy(cmd,Message.payload);
		Usart2Write((const char *)cmd);
	  LOG(LOG_DEBUG,"send message: %s",cmd);
	}
	vPortFree(len);
	vPortFree(cmd);
}

/*消息发送注册函数*/
void deviceDirectSend(char * str)
{
	char *cmd = pvPortMalloc(200);
	char *len = pvPortMalloc(6);
	char *sendmsg = pvPortMalloc(200);
	uint8_t slinknum[2]={0};uint8_t glinknum=0;
	memset(slinknum,0,2);
	slinknum[0] = glinknum+'0';
	
	memset(cmd,0,200);
	memset(len,0,6);
	memset(sendmsg,0,200);
	strcpy(sendmsg,str);
	sprintf(len, "%d", strlen(str));
	strcpy(cmd,"AT+TCPSEND=0,");
	strcat(cmd,len);
	strcat(cmd,"\r\n");
	LOG(LOG_DEBUG,"at_cmd: %s",cmd);
	Usart2Write((const char *)cmd);
	memset(cmd,0,200);
	vTaskDelay(100);
	LOG(LOG_DEBUG,"sendmsg: %s %d\r\n",sendmsg,strlen(sendmsg));
	Usart2Write((const char *)sendmsg);

	vPortFree(len);
	vPortFree(cmd);
}





