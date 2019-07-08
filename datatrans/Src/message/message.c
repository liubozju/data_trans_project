#include "message.h"
#include "gprs.h"
#include "log.h"
#include "register.h"
#include "delay.h"
#include "conversion.h"
#include "cjson.h"
#include "can.h"
#include "upgrade.h"
#include "string.h"
#include "md5.h"

QueueHandle_t UsartRecMsgQueue;   //������Ϣ���о��
static QueueHandle_t UsartSenMsgQueue;   //������Ϣ���о��
SemaphoreHandle_t DeviceSendSemaphore;		//���յ�  '>'
extern gprs gGprs;
extern SemaphoreHandle_t NetBinarySemaphore;
extern SemaphoreHandle_t HeartBinarySemaphore;
extern SemaphoreHandle_t RigisterBinarySemaphore;
extern gPack_Data gpack_data;
extern SemaphoreHandle_t CanRevStatus;	
extern SemaphoreHandle_t Can2RevStatus;	
GETFIRM getfirm;

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
				printf("data rev :%d\r\n",gpack_data.Pack_Data_len);
				LOG(LOG_INFO,"MSG OUT:%s\r\n",msg);
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
			vPortFree(Msg);
			LOG(LOG_ERROR,"Message failed\r\n");
			return -1;
		}
	}
	vPortFree(Msg);
	return 1;
}


/*����ƽ̨�·���ÿ���̼���***/


/*���ݻ�ȡ����buf���ݽ������ݰ�����*/
static uint16_t sGetbufLen(char * msg)
{
	unsigned char datalen[5]={0};
	char * msg_start=NULL;
	uint16_t len = 0;
	uint8_t i = 0;
	uint8_t j = 0;
	if(NULL != strstr(msg,REV_DATA))					//���ݰ��д���TCPREV��ͷ
	{
		msg_start=strstr(msg,REV_DATA);			/*��ȡ��ͷ��Ϣ*/
		msg_start+=11;											/*֡��ʽ�� +TCPRECV: 0,1050,*/
		do{																			//�������ݳ���
			datalen[i] = *msg_start;
			//LOG(LOG_INFO,"datalen%d=%c \r\n",i,datalen[i]);
			if(datalen[i] == ','){
				break;
			}
			if(i>sizeof(datalen)){
				LOG(LOG_ERROR,"Too long length of data is %s \r\n",datalen);
				return 0;
			}
			if(datalen[i] < '0' && datalen[i] > '9'){
				LOG(LOG_ERROR,"Invalid len string!!!, datalen is %s \r\n", datalen);
				return 0;
			}
			i++;msg_start++;
		}while(1);
		for(j = 0;j<i;j++){
			len = len * 10 + datalen[j] - '0';
		}
		return len;
	}
	return 0;
}
/**
*	���й̼�������У�飬���ݽ��ܵȲ���
*/

static uint8_t sAnalyzePack(char *msg)
{
	uint8_t len_count = 0;
	char * msg_start=NULL;
	uint16_t buf_len = sGetbufLen(msg);			/*��ȡ���յ����ַ�����*/
	len_count = getIntNum(buf_len);					/*��ȡbuf_len����λ��*/
	//LOG(LOG_INFO,"buf_len : %d len_count: %d\r\n",buf_len,len_count);
	if(NULL != strstr(msg,REV_DATA))
	{
		msg_start=strstr(msg,REV_DATA);			/*��ȡ��ͷ��Ϣ*/
		if(gpack_data.Pack_Data_len > 50*1024)	/*���һ�����ݹ���*/
		{
			LOG(LOG_ERROR,"one pack is too long,longer than 50*1024\r\n");
		}
		memcpy(gpack_data.Pack_Data+gpack_data.Pack_Data_len,msg_start+11+len_count+1,buf_len);
		gpack_data.Pack_Data_len+=buf_len;
		return PackOK;
	}
  return PackFail; 	
}

/**����ƽ̨�·������ð�*/
static void sConfigAnalyze(char * msg)
{
	char * str = pvPortMalloc(600);
	memset(str,0,600);
	volatile char * msg_start=NULL;
	volatile char * msg_end=NULL;
	if(NULL != strstr(msg,LEFT_JSON) && NULL != strstr(msg,RIGHT_JSON))
	{
		msg_start=strstr(msg,LEFT_JSON);			/*��ȡ����JSON��Ϣ*/
		msg_end = strstr(msg,RIGHT_JSON);
		uint8_t len = msg_end - msg_start;
		for(int i =0;i<len+1;i++)
		{
			*(str+i) = *(msg_start+i);
		}
		LOG(LOG_INFO,"payload :%s\r\n",str);
		cJSON *res = cJSON_Parse(str);
		if(!res){LOG(LOG_ERROR,"THE Config is wrong!\r\n");sysReset();};
		cJSON * RecID = cJSON_GetObjectItem(res,"recID");
    can_id.RecID= RecID->valueint;
		cJSON * SendID = cJSON_GetObjectItem(res,"sendID");
    can_id.SendID= SendID->valueint;		
		cJSON * cannum = cJSON_GetObjectItem(res,"cannum");
    can_id.Can_num= cannum->valueint;
		cJSON * PaSize = cJSON_GetObjectItem(res,"packSize");
    upgrade_info.PackSize= PaSize->valueint;	
		cJSON * Measure = cJSON_GetObjectItem(res,"measure");
    upgrade_info.measure= Measure->valueint;			
		LOG(LOG_INFO,"%d %d %d %d \r\n",can_id.Can_num,can_id.RecID,can_id.SendID,upgrade_info.PackSize);
		cJSON_Delete(res);
	}
	vPortFree(str);
}

static uint8_t sMd5Check(void){
	unsigned char decrypt[16]={0}; 
  char decrypt_str[35]={0};
	memset(decrypt,0,16);memset(decrypt_str,0,35);
	MD5_CTX md5;
	MD5Init(&md5);
//	LOG(LOG_INFO,"gpack_data.Pack_Data �� \r\n");
//	for(uint32_t i = 0;i<gpack_data.Pack_Data_len;i++)
//	{
//		printf("%c",gpack_data.Pack_Data[i]);
//	}
	LOG(LOG_INFO,"\r\ngpack_data.Pack_Data_len �� %d\r\n",gpack_data.Pack_Data_len);
	MD5Update(&md5,gpack_data.Pack_Data,gpack_data.Pack_Data_len);
	MD5Final(&md5,decrypt);        
	HexToStrLow(decrypt,decrypt_str,16);
	LOG(LOG_INFO,"decrypt_str: %s\r\n",decrypt_str);
	LOG(LOG_INFO,"upgrade_info.md5: %s\r\n",upgrade_info.md5);
	/*У�����ȷ*/
	if(strcmp(decrypt_str,upgrade_info.md5) == 0){
		return PackOK;
	}else{				/*У�������*/
		return PackFail;
	}
}

/**����ƽ̨�·������ð�*/
static void sCheckAnalyze(char * msg)
{
	char * str = pvPortMalloc(2048);
	memset(str,0,2048);
	volatile char * msg_start=NULL;
	volatile char * msg_end=NULL;
  char * msg_tcp_rev = NULL;
	uint16_t sPackData_len = 0;
	uint16_t buf_len = sGetbufLen(msg);			/*��ȡ���յ����ַ�����*/
	uint8_t len_count = getIntNum(buf_len);					/*��ȡbuf_len����λ��*/
	if(NULL != strstr(msg,LEFT_JSON) && NULL != strstr(msg,RIGHT_JSON))
	{
		msg_start=strstr(msg,LEFT_JSON);			/*��ȡ����JSON��Ϣ*/
		msg_end = strstr(msg,RIGHT_JSON);
		if(*(msg_start-1)!= ',')							/*˵��������ճ������*/
		{
			msg_tcp_rev = strstr(msg,REV_DATA);
			sPackData_len = msg_start - (msg_tcp_rev+11+len_count+1);
			if(gpack_data.Pack_Data_len > 50*1024)	/*���һ�����ݹ���*/
			{
				LOG(LOG_ERROR,"one pack is too long,longer than 50*1024\r\n");
			}
			memcpy(gpack_data.Pack_Data+gpack_data.Pack_Data_len,msg_tcp_rev+11+len_count+1,sPackData_len);
			gpack_data.Pack_Data_len+=sPackData_len;
		}
		uint8_t len = msg_end - msg_start;
		for(int i =0;i<len+1;i++)
		{
			*(str+i) = *(msg_start+i);
		}
		LOG(LOG_INFO,"payload :%s\r\n",str);
		cJSON *res = cJSON_Parse(str);
		if(!res){LOG(LOG_ERROR,"THE Check pack is wrong!\r\n");sysReset();};
		cJSON * MD5 = cJSON_GetObjectItem(res,"md5");
		memset(upgrade_info.md5,0,35);
		char * string = MD5->valuestring;
		memcpy(upgrade_info.md5,string,strlen(string));			/*��ȡmd5У�����128λ��16���ֽڣ�32���ַ�*/
		cJSON * PackNum = cJSON_GetObjectItem(res,"packNum");
    upgrade_info.Rev_Pack= PackNum->valueint;		
		//LOG(LOG_INFO,"%s %d \r\n",upgrade_info.md5,upgrade_info.Rev_Pack);
		cJSON_Delete(res);
	}
	vPortFree(str);
	/*����md5У����ıȶ�*/
	if(sMd5Check()==PackOK){
		LOG(LOG_INFO,"CHECKOK\r\n");
		getfirm = GETFIRM_STOP;
		xSemaphoreGive(GettingPackBinarySemaphore);
	}else{		/*У�������*/
		LOG(LOG_INFO,"CHECKFAILED\r\n");
		getfirm = GETFIRM_START;
		memset(&gpack_data,0,sizeof(gpack_data));		/*���ݴ��󣬽��������*/
		getPackFromInternet(upgrade_info.Rev_Pack);	/*��������ð�����*/
	}	
}

void MessageReceiveTask(void *pArg)   //�����������
{
	char* buf = pvPortMalloc(2500);
	char* rep = pvPortMalloc(60);
	int buf_len = 0;
	while(1)
	{
		memset(buf,0,2500);
		xQueueReceive(UsartRecMsgQueue,buf,portMAX_DELAY);
		buf_len = strlen(buf);
		//LOG(LOG_DEBUG,"Get buf %s\r\n",buf);
		if(NULL != strstr(buf,CHECKSUM))																											/*���յ�md5У���*/
		{
			sCheckAnalyze(buf);
			/*�ͷ��ź���*/
			buf_len = 0;
		}
		if(NULL != strstr(buf,CONFIG))																												/*���յ����ð�*/
		{
			sConfigAnalyze(buf);
			getfirm = GETFIRM_START;
			buf_len = 0;
			/*�ͷ��ź���*/
			xSemaphoreGive(ConfigBinarySemaphore);
		}
		if(getfirm == GETFIRM_START && buf_len != 0 && NULL != strstr(buf,REV_DATA) && NULL ==strstr(buf,"OK"))												/*���յ�������*/
		{
			sAnalyzePack(buf);
			buf_len = 0;
		}
	  if(NULL != strstr(buf,PBREADY))																												/*���յ�PBREADY*/
	  {
			xQueueReceive(gGprs.GprsRepQueue,rep,0);
			xSemaphoreGive(gGprs.GprsConnectBinarySemaphore);
			buf_len = 0;
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
			buf_len = 0;
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
			buf_len = 0;
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
			buf_len = 0;
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
			buf_len = 0;
	  } 
		if(NULL != strstr(buf,HEART_REP))
		 {
			 if(HeartBinarySemaphore!=NULL)
			 {
				 buf_len = 0;
				 xSemaphoreGive(HeartBinarySemaphore);
			 }
		 }	
		if(NULL != strstr(buf,"OK"))
		{
		   xSemaphoreGive(Message.OKBinarySemaphore);
			 buf_len = 0;
		}
		//vTaskDelay(50);
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
	CanRevStatus = xSemaphoreCreateBinary();
	Can2RevStatus = xSemaphoreCreateBinary();
	GettingPackBinarySemaphore= xSemaphoreCreateBinary();
	DeviceSendSemaphore = xSemaphoreCreateBinary();
	HeartBinarySemaphore = xSemaphoreCreateBinary();
	ConfigBinarySemaphore = xSemaphoreCreateBinary();
	UsartRecMsgQueue = xQueueCreate(4,2500);		/*���ն���--4*2048  ��Ϊ��������Ҫ�������ſ��Խ����ļ�*/
	UsartSenMsgQueue = xQueueCreate(8,200);			/*���Ͷ���--8*200   ���������������٣����д�С����Сһ��*/   
	Message.OKBinarySemaphore = xSemaphoreCreateBinary();		/*�����ź��������ڽ���ģ��ظ���OK*/
	
	Message.send = deviceSend;									/*������Ϣ�ķ��ͺ���*/													
}

static char device_cmd[200] = {0};
static char device_sendmsg[200] = {0};

/*��Ϣ����ע�ắ��*/
void deviceSend(u8 head)
{
	char *len = pvPortMalloc(6);
	uint8_t slinknum[2]={0};uint8_t glinknum=0;
	memset(slinknum,0,2);
	slinknum[0] = glinknum+'0';
	
	if(head==1)					/*ʹ�������ҵ������*/
	{
		memset(device_cmd,0,200);
		memset(len,0,6);
		memset(device_sendmsg,0,200);
		strcpy(device_sendmsg,Message.payload);
		sprintf(len, "%d", strlen(Message.payload));
		strcpy(device_cmd,"AT+TCPSEND=0,");
		strcat(device_cmd,len);
		strcat(device_cmd,"\r\n");
		//LOG(LOG_DEBUG,"at_cmd: %s",device_cmd);
		Usart2Write((const char *)device_cmd);
		memset(device_cmd,0,200);
		//xSemaphoreTake(DeviceSendSemaphore,1000);
		vTaskDelay(100);
		//LOG(LOG_DEBUG,"sendmsg: %s %d\r\n",device_sendmsg,strlen(device_sendmsg));
		Usart2Write((const char *)device_sendmsg);
	}
	else								/*����ͨ�õ�ATָ��*/
	{
		memset(device_cmd,0,200);
		strcpy(device_cmd,Message.payload);
		Usart2Write((const char *)device_cmd);
	  LOG(LOG_DEBUG,"send message: %s",device_cmd);
	}
	vPortFree(len);
}

/*��Ϣ����ע�ắ��*/
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





