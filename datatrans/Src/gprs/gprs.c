#include "gprs.h"
#include "message.h"
#include "log.h"
#include <string.h>
#include "timers.h"
#include "register.h"
#include "Interactive.h"
#include "upgrade.h"
#include "iwdg.h"

#define HEARTBEAT_PACK 	"{\"type\":\"heartbeat\",\"imei\":\"%s\"}"

extern TaskHandle_t MsgRecTaskHanhler;        /*消息解析任务*/
extern QueueHandle_t UsartRecMsgQueue;        /*消息接收队列*/
extern TaskHandle_t MsgSendTaskHanhler;       /*消息封装发送任务*/
extern TimerHandle_t connectTimerHandler;
extern xTimerHandle NetTimerHandler;
extern gFlag_Data gFlag;

SemaphoreHandle_t HeartBinarySemaphore;		/*设备注册使用信号量*/
SemaphoreHandle_t NetBinarySemaphore;					/*联网的信号量*/
gIPPort	gRemoteIpPor;													/*远端IP定义*/

uint8_t gNetConnectFlag = 0;									/*是否已经连接网络标记*/

/*GPRS模组的定义*/
gprs gGprs={
	.gGPRSTCPConnect = gDeviceConnect,							/*设备联网函数*/
	.gGPRSConfig = gDeviceConfig,										/*设备的配置函数*/
	.gGPRSTCPConectStatusTest = gNetTest,						/*联网配置函数*/
	.gGetGprsInfo = gGetDeviceINFO,												/*获取信息*/
};


/*设备TCP联网函数*/
int gDeviceConnect(void)
{
	LOG(LOG_TRACE,"gprs connect start\r\n");
  gGprs.gprsFlag.gGPRSConnectFlag = 1;
	for(int i=0;i<5;i++)
	{
		if( pdeviceConnect()>0)														
		{
     	LOG(LOG_TRACE,"GPRS Connecet to cloud successful\r\n");
			gGprs.gprsFlag.gGPRSConnectFlag = 0;
			if(gFlag.ModeFlag == NET_MODE_FLAG)
			{
				HAL_IWDG_Refresh(&hiwdg);
				gGprs.gprsFlag.gRegisterFlag =1;
				gDeviceRegister();
				HAL_IWDG_Refresh(&hiwdg);
				gGprs.gprsFlag.gRegisterFlag =0;
				gNetConnectFlag = NetConnected;
				LOG(LOG_DEBUG,"start timer NetTimerHandler\r\n");
				HAL_IWDG_Refresh(&hiwdg);
				xTimerStart(NetTimerHandler,portMAX_DELAY);					
			}
			return 1;
		}
    	LOG(LOG_TRACE,"Connect to cloud failed %d.....\r\n",i+1);
	}
	LOG(LOG_TRACE,"Device Rebooting\r\n");
	vTaskSuspendAll();
	sysReset();
	return 0;
}

/*发送数据到模组，指定发送和接收的回复值*/
static int sSendRspData(const char *cmd,const char * rsp)
{
		u8 count = 0;
		BaseType_t err;
		if(gGprs.GprsRepQueue!=NULL)
	  {
			err = xQueueOverwrite(gGprs.GprsRepQueue,rsp);		/*将期望回复写入到GPRS回复队列中去*/
			if(err!=pdTRUE)
			{
				LOG(LOG_ERROR,"Rep Send failed\r\n");
				return -1;
			}
  	}
	  MessageSend(cmd,0);
		while(1)
		{
			count++;
			if(gGprs.GprsConnectBinarySemaphore!=NULL)
			{
					HAL_IWDG_Refresh(&hiwdg);
			    err = xSemaphoreTake(gGprs.GprsConnectBinarySemaphore,5000);
					if(err == pdTRUE)
					{
						break;
					}
					else
					{
						LOG(LOG_DEBUG,"MessageSend count :%d\r\n",count);
						MessageSend(cmd,0);
						if(gGprs.GprsRepQueue!=NULL)
						{
							err = xQueueOverwrite(gGprs.GprsRepQueue,rsp);
							if(err!=pdTRUE)
							{
								LOG(LOG_ERROR,"Rep Sen failed\r\n");
								return -1;
							}
						}
					}
			}
			else
			{
				LOG(LOG_ERROR,"No GPRS semaphore \r\n");
			}
			if(count>=5)
			{
				if(gFlag.ModeFlag == NET_MODE_FLAG){
					MessageSend("AT+CFUN=1,1\r\n",0);
					LOG(LOG_ERROR,"GPRS not responding .Please check\r\n");
					sysReset();
				}else{
					LOG(LOG_ERROR,"GPRS not responding .Please check\r\n");
					return 1;
				}
			}
		}
	return 1;
}

/*判断信号强度是否适合联网*/
static int sSignalJudge(void){
	gGprs.csq = 0;
	gGprs.gGetGprsInfo("AT+CSQ\r\n","+CSQ: ");						/*获取信号强度*/
	LOG(LOG_INFO,"CSQ : %d \r\n",gGprs.csq);
	if(gGprs.csq <0 || gGprs.csq >31){
		LOG(LOG_ERROR,"CSQ IS wrong\r\n.");
		sysReset();
	}
	if(gGprs.csq < 5){
		LOG(LOG_CRIT,"The csq is too weak,please check.and reset\r\n");
		xEventGroupSetBits(InteracEventHandler,EventNetledFlick);		//信号强度较差，闪烁信号差灯
		sysReset();
	}
}

/*AT指令连接网络*/
static int pdeviceConnect(void)
{
	LOG(LOG_TRACE,"Signal Strangth Testing\r\n");
	HAL_Delay(2000);
	sSendRspData("AT\r\n","OK");
	HAL_IWDG_Refresh(&hiwdg);
	HAL_Delay(3000);
	HAL_IWDG_Refresh(&hiwdg);
	sSendRspData("ATE0\r\n","OK");
	sSendRspData("AT+COPS?\r\n","OK");
	sSendRspData("AT+IPR=460800\r\n","OK");
	MX_USART2_UART_Init(460800);			//提升串口波特率，重新初始化
	sSendRspData("AT+CMEE=2\r\n","OK");
	sSendRspData("AT+CPIN?\r\n","+CPIN: READY");
	sSendRspData("AT+CGREG?\r\n","+CGREG: 0,1");
	if(gFlag.ModeFlag == NET_MODE_FLAG)
	{
		sSignalJudge();										//在此处加上信号强度的判断		
	}
	HAL_IWDG_Refresh(&hiwdg);
	sSendRspData("AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n","OK");
	sSendRspData("AT+XIIC=1\r\n","OK");
	HAL_Delay(1000);
	sSendRspData("AT+XIIC?\r\n","+XIIC:    1");
	gTCPIPConfig();
	if(sSendRspData((const char *)gGprs.gGPRSConnect,"TCPSETUP: 0") != 1)
		return -1;
	return 1;
}

/*将GPRS内部的IP和PORT组装成TCP连接命令*/
void gTCPIPConfig(void)
{
	uint8_t slinkNumtostr[2]={0};
	memset(slinkNumtostr,0,2);
	memset((void *)gGprs.gTCPLinkNum,0,5);
//	gGprs.gTCPLinkNum[0]=1;					/*TCP链路1   G510支持4路链接 1,2,3,4*/
	gGprs.gTCPLinkNum[0]=0;
	slinkNumtostr[0] =gGprs.gTCPLinkNum[0]+'0';
	memset((void *)gGprs.gGPRSConnect,0,100);
	memcpy((void *)gGprs.gGPRSConnect,"AT+TCPSETUP=",strlen("AT+TCPSETUP="));
	strcat((char *)gGprs.gGPRSConnect,(const char *)slinkNumtostr);
	strcat((char *)gGprs.gGPRSConnect,(const char *)",");
	strcat((char *)gGprs.gGPRSConnect,(const char *)gGprs.gprsIP);
	strcat((char *)gGprs.gGPRSConnect,(const char *)",");
	strcat((char *)gGprs.gGPRSConnect,(const char *)gGprs.gprsPort);
	strcat((char *)gGprs.gGPRSConnect,(const char *)"\r\n");
	// memcpy((void *)gGprs.gGPRSConnect,"AT+MIPOPEN=",strlen("AT+MIPOPEN="));
	// strcat((char *)gGprs.gGPRSConnect,(const char *)slinkNumtostr);
	// strcat((char *)gGprs.gGPRSConnect,(const char *)",1,\"");
	// strcat((char *)gGprs.gGPRSConnect,(const char *)gGprs.gprsIP);
	// strcat((char *)gGprs.gGPRSConnect,(const char *)"\",");
	// strcat((char *)gGprs.gGPRSConnect,(const char *)gGprs.gprsPort);
	// strcat((char *)gGprs.gGPRSConnect,(const char *)"0,\r\n");			//打开一路TCP连接
	//LOG(LOG_DEBUG,"gGprs.gGPRSConnect: %s\r\n",gGprs.gGPRSConnect);
}

/*GPRS模组的配置函数---用于初始化相关参数以及建立队列*/
void gDeviceConfig(void)
{
	gGprs.gprsFlag.gGPRSConnectFlag = 0;		/*GPRS模组连接参数设置为0*/
	gGprs.gprsFlag.gINFOFlag = 0;				
	gGprs.gprsFlag.gRegisterFlag =0;			/*GPRS的注册标记设置为0*/
	char * pcmd = pvPortMalloc(160);
	memset(pcmd,0,160);

	LOG(LOG_DEBUG,"GPRS Config\r\n");
	
	memset((void *)gGprs.gprsIP,0,30);			/*设置GPRS的远端IP地址*/
	memcpy((void *)gGprs.gprsIP,gRemoteIpPor.gremoteIP,20);
	memcpy((void *)gGprs.gprsPort,gRemoteIpPor.gremotePort,6);
	LOG(LOG_INFO,"gGprs.gprsIP : %s\r\n",gGprs.gprsIP);
	LOG(LOG_INFO,"gGprs.gprsPort : %s\r\n",gGprs.gprsPort);
	
	gGprs.GprsConnectBinarySemaphore = xSemaphoreCreateBinary();		/*GPRS连接二值信号量建立*/
	gGprs.GprsINFOBinarySemaphore = xSemaphoreCreateBinary();
	gGprs.GprsTCPSendBinarySemaphore = xSemaphoreCreateBinary();
	
	gGprs.GprsRepQueue = xQueueCreate(1,30);							/*GPRS联网队列建立*/
	gGprs.GPRSINFORepQueue = xQueueCreate(1,120);
	gGprs.GprsTCPRepQueue = xQueueCreate(1,10);		
}


/*检查远端IP 和端口号的长度是否合法
	-1	不合法
	1   合法
*/
static int sIpPortCheck(const char * remoteIp,const char * remotePort)
{
	if(strlen(remoteIp) < 7 || strlen(remoteIp)>16)
	{
		LOG(LOG_ERROR,"IP is wrong .Please Check\r\n");
		return -1;
	}
	if(strlen(remotePort) < 1 || strlen(remotePort)>5)
	{
		LOG(LOG_ERROR,"Port is wrong .Please Check\r\n");
		return -1;
	}
		return 1;
}

/*配置远端要连接的IP*/
void gRemoteIpPortConfig(const char * remoteIp,const char * remotePort)
{
	memset(gRemoteIpPor.gremoteIP,0,20);
	memset(gRemoteIpPor.gremotePort,0,6);
	if(sIpPortCheck(remoteIp,remotePort)<0)
	{
		LOG(LOG_INFO,"the remote ip is wrong: %s %s",remoteIp,remotePort);	
		return;
	}
	memcpy(gRemoteIpPor.gremoteIP,remoteIp,strlen(remoteIp));
	memcpy(gRemoteIpPor.gremotePort,remotePort,strlen(remotePort));
	LOG(LOG_INFO,"remoteIp : %s  ",remoteIp);
	LOG(LOG_INFO,"remotrPort : %s  \r\n",remotePort);
}




/*联网测试函数*/
int gNetTest(void)
{
	uint8_t glinkNum =0;
	static 	uint8_t slinkNumtostr[2]={0};
	memset(slinkNumtostr,0,2);
	slinkNumtostr[0]= glinkNum+'0';
  NetBinarySemaphore = xSemaphoreCreateBinary();
  gGprs.gprsFlag.gNetFlag = 1;
  BaseType_t err;
	char * sNetCheckCmd = pvPortMalloc(30);
	memset(sNetCheckCmd,0,30);
	memcpy(sNetCheckCmd,"AT+IPSTATUS=",strlen("AT+IPSTATUS="));
	strcat(sNetCheckCmd,(const char *)slinkNumtostr);
	strcat(sNetCheckCmd,"\r\n");
  for(int i=0;i<3;i++)
  {
		HAL_IWDG_Refresh(&hiwdg);
		MessageSend(sNetCheckCmd,0);
		err = xSemaphoreTake(NetBinarySemaphore,10000);
	  if(err == pdTRUE)
		{
			gGprs.gprsFlag.gNetFlag = 0;
			vSemaphoreDelete(NetBinarySemaphore);
      LOG(LOG_DEBUG,"Net Check ok\r\n");
			vPortFree(sNetCheckCmd);
			return 1;
		}
		LOG(LOG_TRACE,"NetCheck %d times\r\n",i);
  }
	LOG(LOG_ERROR,"Net Check failed\r\n");
	xTimerStart(connectTimerHandler,10000);				//GPRS没有返回，重新打开联网定时器。5S后开始
	return -1;		
}

static int sAnalysisRsp(char * rspbuf,const char *rsp)
{
	if(NULL != strstr(rspbuf,"CSQ"))   /*Receive csq*/
	{
		 char * result;
		 result = strstr(rspbuf,rsp);
		 result+=strlen(rsp);
		 if((*(result+1))!=',')
		 {
			 gGprs.csq = ((*result)-'0')*10+(*(result+1))-'0';
		 }
		 else 
		 {
			 gGprs.csq = (*result)-'0';
		 }
		 LOG(LOG_INFO,"GPRS CSQ Value :%d\r\n",gGprs.csq);
		 return 1;
	 }
	 else if(NULL != strstr(rspbuf,"CGSN"))		/*获取IMEI*/
	 {
		 char * result;
		 result = strstr(rspbuf,rsp);
		 result+=strlen(rsp);
		 memcpy((void *)gGprs.gimei,result,15);
		 LOG(LOG_INFO,"GPRS IMEI: %s\r\n",gGprs.gimei);
		 return 1;
	 }
	 else if(NULL != strstr(rspbuf,rsp))		/*获取IMSI*/
	 {
		 char * result;
		 result = strstr(rspbuf,rsp);
		 memcpy((void *)gGprs.gimsi,result,15);
		 LOG(LOG_INFO,"GPRS IMSI: %s\r\n",gGprs.gimsi);
		 return 1;		 
	 }
	 else
	 {
		 LOG(LOG_ERROR,"the rsp buf is wrong.\r\n");
		 return -1;
	 }
}

static int sGetDeviceINFO(const char *cmd,const char *rsp)
{
	  u8 count = 0;
	  BaseType_t err;
	  if(gGprs.GprsRepQueue!=NULL)
	  {
			err = xQueueOverwrite(gGprs.GprsRepQueue,rsp);
			if(err!=pdTRUE)
			{
				LOG(LOG_ERROR,"Rep Send failed\r\n");
				return -1;
			}
  	}
	  MessageSend(cmd,0);
		while(1)
		{
			count++;
			if(gGprs.GprsINFOBinarySemaphore!=NULL)
			{
					HAL_IWDG_Refresh(&hiwdg);
			    err = xSemaphoreTake(gGprs.GprsINFOBinarySemaphore,5000);		/*收到返回时会发送信号量*/
					if(err == pdTRUE)
					{
							char * rspbuf = pvPortMalloc(120);
							memset(rspbuf,0,120);
							xQueueReceive(gGprs.GPRSINFORepQueue,rspbuf,10);
							if(sAnalysisRsp(rspbuf,rsp) == 1)		/*成功接收到数据*/
							{
								vPortFree(rspbuf);
								return 1;													/*接收成功  返回1*/
							}
							vPortFree(rspbuf);
					}
					else
					{
						LOG(LOG_DEBUG,"MessageSend :%d\r\n",count);
						MessageSend(cmd,0);
						if(gGprs.GprsRepQueue!=NULL)
						{
							err = xQueueOverwrite(gGprs.GprsRepQueue,rsp);
							if(err!=pdTRUE)
							{
								printf("Rep Sen failed\r\n");
								return -1;
							}
						}
					}
			}
			else
			{
				LOG(LOG_ERROR,"No GPRS semaphore \r\n");
			}
			if(count>=5)
			{
				LOG(LOG_ERROR,"getting gprs failed. \r\n");
				return -1; 
			}
		}
}

/*获取设备CSQ*/
int gGetDeviceINFO(const char * cmd,const char *rsp)
{
  gGprs.gprsFlag.gINFOFlag = 1;			/*置位CSQ标记，用于串口命令解析*/
	for(int i=0;i<5;i++)
	{
		HAL_IWDG_Refresh(&hiwdg);
		if(sGetDeviceINFO(cmd,rsp)>0)
		{
			gGprs.gprsFlag.gINFOFlag = 0;
			return 1;
		}
   	 LOG(LOG_DEBUG,"Get Device info failed %d.....\r\n",i+1);
	}
	LOG(LOG_ERROR,"Device Rebooting\r\n");
	vTaskSuspendAll();
	sysReset();
	return -1;
}

int HeartBeat(void)
{
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,HEARTBEAT_PACK,gGprs.gimei);
	LOG(LOG_INFO,"str:%s \r\n",str);
  BaseType_t err;
  for(int i=0;i<3;i++)
  {
		HAL_IWDG_Refresh(&hiwdg);
		memset(str,0,200);
		sprintf(str,HEARTBEAT_PACK,gGprs.gimei);
		MessageSend(str,1);
		err = xSemaphoreTake(HeartBinarySemaphore,5000);
	  if(err == pdTRUE)
		{
			vPortFree(str);
      LOG(LOG_DEBUG,"heartrsp ok\r\n");
			xEventGroupSetBits(InteracEventHandler,EventNetledOn);
			xEventGroupClearBits(InteracEventHandler,EventNetledFlick);
			gNetConnectFlag= NetConnected;
			return 1;
		}
		LOG(LOG_ERROR,"send heart %d times\r\n",i);
  }
	vPortFree(str);	
	LOG(LOG_ERROR,"send heart failed\r\n");
	sysReset();
	return -1;		
}



