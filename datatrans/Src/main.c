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
#define InteRaction_TASK_PRIO    10                    //任务优先级
#define InteRaction_STK_SIZE     256                  //任务堆栈大小
TaskHandle_t InteRactionTaskHanhler;                  //任务句柄

/*升级任务定义*/
#define Upgrade_TASK_PRIO    8                    //任务优先级
#define Upgrade_STK_SIZE     256                  //任务堆栈大小
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
	 
		
	 MsgInfoConfig();																	/*配置Message 串口接收发送队列*/
	 gRemoteIpPortConfig("112.124.6.31","6800"); 			/*配置远端IP*/
	 gGprs.gGPRSConfig();									/*模组相关信息配置*/
	 
   InteracEventHandler = xEventGroupCreate();				/*创建交互的事件标志组*/

	 /*联网定时器---单次定时  2S*/
	 connectTimerHandler = xTimerCreate((const char *  )"OneShotTimer",
										(TickType_t    )2000,
										(UBaseType_t   )pdFALSE,
										(void *        )1,
										(TimerCallbackFunction_t)gGprs.gGPRSTCPConnect
										);
//	 /*网络连接状态检查定时器---循环定时  30S*/
//	 NetTimerHandler  = xTimerCreate(   (const char *  )"NetTimer",
//										(TickType_t    )30000,     
//										(UBaseType_t   )pdTRUE,
//										(void *        )3,
//										(TimerCallbackFunction_t)gGprs.gGPRSTCPConectStatusTest
//										);
//	 /*网络检查定时器  ----循环定时  5MIN*/
//	 CSQTimerHandler  = xTimerCreate(   (const char *  )"CSQTimer",
//										(TickType_t    )300000,     
//										(UBaseType_t   )pdTRUE,
//									    (void *        )4,
//										(TimerCallbackFunction_t)DeviceUploadCSQ
//										);	
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
							 (const char*   ) "MessageReceiveTask",                
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

		xTimerStart(connectTimerHandler,portMAX_DELAY);
//    xTimerStart(CSQTimerHandler,portMAX_DELAY);
//    xTimerStart(StoreTimerHandler,portMAX_DELAY);		
//    xTimerStart(PortInsertTimerHandler,portMAX_DELAY);	
							 
  //  xTimerStart(SearchCardTimerHandler,portMAX_DELAY);								 
	
	  vTaskDelete(StartTaskHanhler);
	  taskEXIT_CRITICAL();     					 
}

void TESTTask(void *pvParameter)
{
		
		LOG(LOG_DEBUG,"this is test task!\r\n");

	 uint8_t data[100]="this is my can send data task\0";
		while(1)
		{
			gCAN_SendData(0x14,0,0,data);
			vTaskDelay(2000);
		}
}



