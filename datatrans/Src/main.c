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

/*�������񴴽�-��*/
#define TEST_TASK_PRIO    6          //�������ȼ�
#define TEST_STK_SIZE     128        //�����ջ��С
TaskHandle_t TESTTaskHanhler;        //������
void TESTTask(void *pvParameters);   //����������

/*��ʼ���񴴽�---���ڴ�����������Ҫ�Ķ�ʱ��  �����*/
#define START_TASK_PRIO    6          //�������ȼ�
#define START_STK_SIZE     128        //�����ջ��С
TaskHandle_t StartTaskHanhler;        //������
void StartTask(void *pvParameters);   //����������

/*����������*/
#define InteRaction_TASK_PRIO    10                    //�������ȼ�
#define InteRaction_STK_SIZE     256                  //�����ջ��С
TaskHandle_t InteRactionTaskHanhler;                  //������

/*����������*/
#define Upgrade_TASK_PRIO    8                    //�������ȼ�
#define Upgrade_STK_SIZE     256                  //�����ջ��С
TaskHandle_t UpgradeTaskHanhler;                  //������

/*�¼���־��Ķ���*/
EventGroupHandle_t InteracEventHandler;

/*���ڽ��ն��н�������*/
#define MsgRecTask_TASK_PRIO    7                    //�������ȼ�
#define MsgRecTask_STK_SIZE     512                  //�����ջ��С
TaskHandle_t MsgRecTaskHanhler;                  //������
//�ص�������message.c�ж��壬��������ʱע��          //������

/*���ݰ���װ���ڷ���*/
#define MsgSendTask_TASK_PRIO    5          //�������ȼ�
#define MsgSendTask_STK_SIZE     512        //�����ջ��С
TaskHandle_t MsgSendTaskHanhler;        //������

extern gprs gGprs;
TimerHandle_t connectTimerHandler;
xTimerHandle NetTimerHandler;




int main(void)
{
	platformInit();
	
	/*������ʼ����*/
	xTaskCreate(	(TaskFunction_t) StartTask,				/*������*/
								(const char *)   "StartTask",			/*��������*/
								(uint16_t		)			START_STK_SIZE,	/*�����ջ*/
								(void *)					NULL,						/*�������*/
								(UBaseType_t )		START_TASK_PRIO,/*�������ȼ�*/
								(TaskHandle_t* ) &StartTaskHanhler /*������*/
									);
	
	vTaskStartScheduler();													/*�����������*/
}

void StartTask(void *pvParameter)
{
	
	 taskENTER_CRITICAL();     
	 
		
	 MsgInfoConfig();																	/*����Message ���ڽ��շ��Ͷ���*/
	 gRemoteIpPortConfig("112.124.6.31","6800"); 			/*����Զ��IP*/
	 gGprs.gGPRSConfig();									/*ģ�������Ϣ����*/
	 
   InteracEventHandler = xEventGroupCreate();				/*�����������¼���־��*/

	 /*������ʱ��---���ζ�ʱ  2S*/
	 connectTimerHandler = xTimerCreate((const char *  )"OneShotTimer",
										(TickType_t    )2000,
										(UBaseType_t   )pdFALSE,
										(void *        )1,
										(TimerCallbackFunction_t)gGprs.gGPRSTCPConnect
										);
//	 /*��������״̬��鶨ʱ��---ѭ����ʱ  30S*/
//	 NetTimerHandler  = xTimerCreate(   (const char *  )"NetTimer",
//										(TickType_t    )30000,     
//										(UBaseType_t   )pdTRUE,
//										(void *        )3,
//										(TimerCallbackFunction_t)gGprs.gGPRSTCPConectStatusTest
//										);
//	 /*�����鶨ʱ��  ----ѭ����ʱ  5MIN*/
//	 CSQTimerHandler  = xTimerCreate(   (const char *  )"CSQTimer",
//										(TickType_t    )300000,     
//										(UBaseType_t   )pdTRUE,
//									    (void *        )4,
//										(TimerCallbackFunction_t)DeviceUploadCSQ
//										);	
		/*���ڽ������ݴ�������*/
		xTaskCreate( (TaskFunction_t) MessageReceiveTask,       
							 (const char*   ) "MessageReceiveTask",                
							 (uint16_t      ) MsgRecTask_STK_SIZE,       
							 (void *        ) NULL,                      
							 (UBaseType_t   ) MsgRecTask_TASK_PRIO,       
							 (TaskHandle_t* ) &MsgRecTaskHanhler          
	            );
		/*���ڷ�������*/
		xTaskCreate( (TaskFunction_t) MessageSendTask,          
							 (const char*   ) "MessageSendTask",                  
							 (uint16_t      ) MsgSendTask_STK_SIZE,       
							 (void *        ) NULL,                       
							 (UBaseType_t   ) MsgSendTask_TASK_PRIO,      
							 (TaskHandle_t* ) &MsgSendTaskHanhler         
							 );
		/*������������/���뿪���Լ�LED�ͷ���������ʾ��*/
		xTaskCreate( (TaskFunction_t) InteRactionTask,       
							 (const char*   ) "MessageReceiveTask",                
							 (uint16_t      ) InteRaction_STK_SIZE,       
							 (void *        ) NULL,                      
							 (UBaseType_t   ) InteRaction_TASK_PRIO,       
							 (TaskHandle_t* ) &InteRactionTaskHanhler          
	            );							 
	/*������ʼ����*/
//	xTaskCreate(	(TaskFunction_t) TESTTask,				/*������*/
//								(const char *)   "TESTTask",			/*��������*/
//								(uint16_t		)			TEST_STK_SIZE,	/*�����ջ*/
//								(void *)					NULL,						/*�������*/
//								(UBaseType_t )		TEST_TASK_PRIO,/*�������ȼ�*/
//								(TaskHandle_t* ) &TESTTaskHanhler /*������*/
//									);
								
	xTaskCreate(	(TaskFunction_t) UpgradeTask,				/*������*/
								(const char *)   "UpgradeTask",			/*��������*/
								(uint16_t		)			Upgrade_STK_SIZE,	/*�����ջ*/
								(void *)					NULL,						/*�������*/
								(UBaseType_t )		Upgrade_TASK_PRIO,/*�������ȼ�*/
								(TaskHandle_t* ) &UpgradeTaskHanhler /*������*/
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



