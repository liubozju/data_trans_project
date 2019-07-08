/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "include.h"
#include "stdlib.h"
#include "string.h"
#include "timers.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "log.h"
#include "can.h"
#include "crc.h"
#include "md5.h"
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "stdio.h"
#include "upgrade.h"

extern gFlag_Data gFlag;

/*测试任务创建-等*/
#define TEST_TASK_PRIO    6          //任务优先级
#define TEST_STK_SIZE     128        //任务堆栈大小
TaskHandle_t TESTTaskHanhler;        //任务句柄
void TESTTask(void *pvParameters);   //任务函数声明

/*开始任务创建---用于创建工程所需要的定时器  任务等*/
#define START_TASK_PRIO    6          //任务优先级
#define START_STK_SIZE     128        //任务堆栈大小
TaskHandle_t StartTaskHanhler;        //任务句柄
void StartTask(void *pvParameters);   //任务函数声明

/*交互任务定义*/
#define InteRaction_TASK_PRIO    4                    //任务优先级
#define InteRaction_STK_SIZE     256                  //任务堆栈大小
TaskHandle_t InteRactionTaskHanhler;                  //任务句柄

/*升级任务定义*/
#define Upgrade_TASK_PRIO    6                    //任务优先级
#define Upgrade_STK_SIZE     512                  //任务堆栈大小
TaskHandle_t UpgradeTaskHanhler;                  //任务句柄

/*事件标志组的定义*/
EventGroupHandle_t InteracEventHandler;

/*串口接收队列解析任务*/
#define MsgRecTask_TASK_PRIO    7                    //任务优先级
#define MsgRecTask_STK_SIZE     512                  //任务堆栈大小
TaskHandle_t MsgRecTaskHanhler;                  //任务句柄
//回调函数在message.c中定义，创建任务时注册          //任务函数

/*数据包封装串口发送*/
#define MsgSendTask_TASK_PRIO    5          //任务优先级
#define MsgSendTask_STK_SIZE     512        //任务堆栈大小
TaskHandle_t MsgSendTaskHanhler;        //任务句柄


/*CAN数据发送*/
#define CANSendTask_TASK_PRIO    4          //任务优先级
#define CANSendTask_STK_SIZE     512        //任务堆栈大小
TaskHandle_t CANSendTaskHanhler;        //任务句柄

extern gprs gGprs;
TimerHandle_t connectTimerHandler;
xTimerHandle NetTimerHandler;

int main(void)
{
	platformInit();

	/*创建开始任务*/
	xTaskCreate(	(TaskFunction_t) StartTask,				/*任务函数*/
								(const char *)   "StartTask",			/*任务名称*/
								(uint16_t		)			START_STK_SIZE,	/*任务堆栈*/
								(void *)					NULL,						/*任务参数*/
								(UBaseType_t )		START_TASK_PRIO,/*任务优先级*/
								(TaskHandle_t* ) &StartTaskHanhler /*任务句柄*/
									);
	
	vTaskStartScheduler();													/*开启任务调度*/
}

void StartTask(void *pvParameter)
{	
		 taskENTER_CRITICAL();
		 HAL_IWDG_Refresh(&hiwdg);
		 MsgInfoConfig();																												/*配置Message 串口接收发送队列*/
		 gRemoteIpPortConfig("47.111.9.209","9090"); 			/*配置远端IP*/
		 gGprs.gGPRSConfig();									/*模组相关信息配置*/
		 
		 InteracEventHandler = xEventGroupCreate();				/*创建交互的事件标志组*/

		 /*联网定时器---单次定时  2S*/
		 connectTimerHandler = xTimerCreate((const char *  )"OneShotTimer",
											(TickType_t    )2000,
											(UBaseType_t   )pdFALSE,
											(void *        )1,
											(TimerCallbackFunction_t)gGprs.gGPRSTCPConnect
											);
		 /*网络连接状态检查定时器---循环定时  120S*/
		 NetTimerHandler  = xTimerCreate(   (const char *  )"NetTimer",
											(TickType_t    )90000,     
											(UBaseType_t   )pdTRUE,
											(void *        )2,
											(TimerCallbackFunction_t)HeartBeat
											);
			/*串口接收数据处理任务*/
			xTaskCreate( (TaskFunction_t) MessageReceiveTask,       
								 (const char*   ) "MessageReceiveTask",                
								 (uint16_t      ) MsgRecTask_STK_SIZE,       
								 (void *        ) NULL,                      
								 (UBaseType_t   ) MsgRecTask_TASK_PRIO,       
								 (TaskHandle_t* ) &MsgRecTaskHanhler          
								);
			/*串口发送任务*/
			xTaskCreate( (TaskFunction_t) MessageSendTask,          
								 (const char*   ) "MessageSendTask",                  
								 (uint16_t      ) MsgSendTask_STK_SIZE,       
								 (void *        ) NULL,                       
								 (UBaseType_t   ) MsgSendTask_TASK_PRIO,      
								 (TaskHandle_t* ) &MsgSendTaskHanhler         
								 );		 
		/*创建交互任务/拨码开关以及LED和蜂鸣器的提示音*/
		xTaskCreate( (TaskFunction_t) InteRactionTask,       
							 (const char*   ) "InteRactionTaskTask",                
							 (uint16_t      ) InteRaction_STK_SIZE,       
							 (void *        ) NULL,                      
							 (UBaseType_t   ) InteRaction_TASK_PRIO,       
							 (TaskHandle_t* ) &InteRactionTaskHanhler          
	            );							 
	/*创建开始任务*/
//	xTaskCreate(	(TaskFunction_t) TESTTask,				/*任务函数*/
//								(const char *)   "TESTTask",			/*任务名称*/
//								(uint16_t		)			TEST_STK_SIZE,	/*任务堆栈*/
//								(void *)					NULL,						/*任务参数*/
//								(UBaseType_t )		TEST_TASK_PRIO,/*任务优先级*/
//								(TaskHandle_t* ) &TESTTaskHanhler /*任务句柄*/
//									);
								
		xTaskCreate(	(TaskFunction_t) UpgradeTask,				/*任务函数*/
								(const char *)   "UpgradeTask",			/*任务名称*/
								(uint16_t		)			Upgrade_STK_SIZE,	/*任务堆栈*/
								(void *)					NULL,						/*任务参数*/
								(UBaseType_t )		Upgrade_TASK_PRIO,/*任务优先级*/
								(TaskHandle_t* ) &UpgradeTaskHanhler /*任务句柄*/
									);
		if(gFlag.ModeFlag == NET_MODE_FLAG)
		{
			upgrade_info.method = online;
			xTimerStart(connectTimerHandler,portMAX_DELAY);				//网络模式，打开联网定时器
			xEventGroupSetBits(InteracEventHandler,EventNETModeLedOn);
			xEventGroupClearBits(InteracEventHandler,EventSDModeLedOn);
		}else if(gFlag.ModeFlag == SD_MODE_FLAG)
		{
			upgrade_info.method = offline;
			xEventGroupSetBits(InteracEventHandler,EventSDModeLedOn);
			xEventGroupClearBits(InteracEventHandler,EventNETModeLedOn);
		}
		HAL_IWDG_Refresh(&hiwdg);
	  vTaskDelete(StartTaskHanhler);
	  taskEXIT_CRITICAL();     					 
}

void TESTTask(void *pvParameter)
{
		
		LOG(LOG_DEBUG,"this is test task!\r\n");
		memset(&can_id,0,sizeof(can_id));
		can_id.Can_num = Can_num2;
		can_id.RecID = 0xc8;
	  can_id.SendID = 0x64;

		uint8_t data[100]="this is my can send data task\0";
		MX_CAN2_Init(can_id.RecID);
		while(1)
		{
			LOG(LOG_INFO,"start sending\r\n");
			CanPre();
			
//			CanSendLinePack(data);
//			CanSendEndPack();
			HAL_Delay(10000);
		}
}



