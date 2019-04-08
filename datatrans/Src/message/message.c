#include "message.h"
#include "gprs.h"
#include "log.h"
#include "register.h"
#include "delay.h"

QueueHandle_t UsartRecMsgQueue;   //������Ϣ���о��
static QueueHandle_t UsartSenMsgQueue;   //������Ϣ���о��
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

/*��PayLoad���ݰ����͵����Ͷ�����*/
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

void MessageReceiveTask(void *pArg)   //�����������
{
	char* buf = pvPortMalloc(120);
	char* rep = pvPortMalloc(60);
	while(1)
	{
		xQueueReceive(UsartRecMsgQueue,buf,portMAX_DELAY);

		LOG(LOG_DEBUG,"Get buf %s\r\n",buf);
	  if(NULL != strstr(buf,PBREADY))
	  {
			xQueueReceive(gGprs.GprsRepQueue,rep,0);
			xSemaphoreGive(gGprs.GprsConnectBinarySemaphore);
	  }
		if(gGprs.gprsFlag.gGPRSConnectFlag == 1 && gGprs.gprsFlag.gINFOFlag == 0)							/*�����������*/
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
		if(gGprs.gprsFlag.gINFOFlag == 1)											/*INFO�������*/
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
		if(gGprs.gprsFlag.gNetFlag == 1)                               /*������*/
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
			if(RigisterBinarySemaphore!=NULL)                    /*ע��ظ�*/
			{  
				 if(NULL != strstr(buf,REGISTER_REP))
				 {
					 if(RigisterBinarySemaphore!=NULL)
					 xSemaphoreGive(RigisterBinarySemaphore);
				 }
			}
	  }
//		if(CSQBinarySemaphore!=NULL)                          /*CSQ�ظ�*/
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

/*���ڷ�������*/
void MessageSendTask(void *pArg)
{
	char* buf = pvPortMalloc(200);
	u8 i = 0;
	while(1)
	{
		memset(buf,0,200);
		xQueueReceive(UsartSenMsgQueue,buf,portMAX_DELAY);			/*��ѯ���Ͷ����������ݣ��о�ȡ��һ֡���ݷ���*/

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
			   err = xSemaphoreTake(Message.OKBinarySemaphore,1000);				/*�ȴ�ģ�鷵��OK���κ�����ģ�鶼����һ��OK����*/
				{
					if(err == pdTRUE)																						/*�յ�OK��˵�����ͳɹ����˳�ѭ��*/
					{
						break;
					}
					LOG(LOG_DEBUG,"OKBinarySemaphore :%d\r\n",i);
				}
			}
			if(i>=3)												/*�ظ�����3��*/
			{
				break;
			}
		}
	}
	vPortFree(buf);
}

/*���ڽ��ա����Ͷ��д���*/
void MsgInfoConfig(void)
{
	UsartRecMsgQueue = xQueueCreate(10,600);		/*���ն���--10*600  ��Ϊ��������Ҫ�������ſ��Խ����ļ�*/
	UsartSenMsgQueue = xQueueCreate(8,200);			/*���Ͷ���--8*200   ���������������٣����д�С����Сһ��*/   
	Message.OKBinarySemaphore = xSemaphoreCreateBinary();		/*�����ź��������ڽ���ģ��ظ���OK*/
	Message.send = deviceSend;									/*������Ϣ�ķ��ͺ���*/													
}


/*��Ϣ����ע�ắ��*/
void deviceSend(u8 head)
{
	char *cmd = pvPortMalloc(200);
	char *len = pvPortMalloc(6);
	char *sendmsg = pvPortMalloc(200);
	uint8_t slinknum[2]={0};uint8_t glinknum=0;
	memset(slinknum,0,2);
	slinknum[0] = glinknum+'0';
	
	if(head==1)					/*ʹ�������ҵ������*/
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
	else								/*����ͨ�õ�ATָ��*/
	{
		memset(cmd,0,200);
		strcpy(cmd,Message.payload);
		Usart2Write((const char *)cmd);
	  LOG(LOG_DEBUG,"send message: %s",cmd);
	}
	vPortFree(len);
	vPortFree(cmd);
}





