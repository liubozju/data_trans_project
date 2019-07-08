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
#define InteRaction_TASK_PRIO    4                    //�������ȼ�
#define InteRaction_STK_SIZE     256                  //�����ջ��С
TaskHandle_t InteRactionTaskHanhler;                  //������

/*����������*/
#define Upgrade_TASK_PRIO    6                    //�������ȼ�
#define Upgrade_STK_SIZE     512                  //�����ջ��С
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


/*CAN���ݷ���*/
#define CANSendTask_TASK_PRIO    4          //�������ȼ�
#define CANSendTask_STK_SIZE     512        //�����ջ��С
TaskHandle_t CANSendTaskHanhler;        //������

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
		 HAL_IWDG_Refresh(&hiwdg);
		 MsgInfoConfig();																												/*����Message ���ڽ��շ��Ͷ���*/
		 gRemoteIpPortConfig("47.111.9.209","9090"); 			/*����Զ��IP*/
		 gGprs.gGPRSConfig();									/*ģ�������Ϣ����*/
		 
		 InteracEventHandler = xEventGroupCreate();				/*�����������¼���־��*/

		 /*������ʱ��---���ζ�ʱ  2S*/
		 connectTimerHandler = xTimerCreate((const char *  )"OneShotTimer",
											(TickType_t    )2000,
											(UBaseType_t   )pdFALSE,
											(void *        )1,
											(TimerCallbackFunction_t)gGprs.gGPRSTCPConnect
											);
		 /*��������״̬��鶨ʱ��---ѭ����ʱ  120S*/
		 NetTimerHandler  = xTimerCreate(   (const char *  )"NetTimer",
											(TickType_t    )90000,     
											(UBaseType_t   )pdTRUE,
											(void *        )2,
											(TimerCallbackFunction_t)HeartBeat
											);
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
							 (const char*   ) "InteRactionTaskTask",                
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
		if(gFlag.ModeFlag == NET_MODE_FLAG)
		{
			upgrade_info.method = online;
			xTimerStart(connectTimerHandler,portMAX_DELAY);				//����ģʽ����������ʱ��
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



