#ifndef __Interactive_H__
#define __Interactive_H__

#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "log.h"
#include "sys.h"
#include "gpio.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


typedef enum{
	SD_BUTTON_ON = 0,
	SD_BUTTON_OFF,
}sdbutton_status;

extern EventGroupHandle_t InteracEventHandler;

/*事件标志位*/
#define EventNetledOn   (1<<0)      //NetLed点亮
#define EventNetledFlick (1<<1)     //信号差（连接网络失败），闪烁LED灯
#define EventCanLedOn   (1<<2)      //CAN总线信号灯
#define EventJobOk   (1<<3)         //点亮升级完成信号灯，蜂鸣器长鸣
#define EventFirmOk     (1<<4)      //蜂鸣器短鸣，固件下载完成


#define Net_LED_On  HAL_GPIO_WritePin(Net_LED_GPIO_Port, Net_LED , Led_On)
#define Net_LED_Off  HAL_GPIO_WritePin(Net_LED_GPIO_Port, Net_LED , Led_Off)

#define Job_OK_Led_On  HAL_GPIO_WritePin(Job_OK_Led_GPIO_Port, Job_OK_Led_Pin , Led_On)
#define Job_OK_Led_Off  HAL_GPIO_WritePin(Job_OK_Led_GPIO_Port, Job_OK_Led_Pin , Led_Off)

#define Net_LED_Flick_Off  HAL_GPIO_WritePin(Net_Led_Flick_GPIO_Port, Net_Led_Flick_Pin , Led_Off)

#define BEEP_On HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin , Buzzer_On)
#define BEEP_Off HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin , Buzzer_Off)

#define Led_On  GPIO_PIN_RESET
#define Led_Off GPIO_PIN_SET

#define Buzzer_On  GPIO_PIN_SET
#define Buzzer_Off GPIO_PIN_RESET



/*交互接口的引脚定义功能*/
#define SD_BUTTON GPIO_PIN_4
#define START_UPGRADE_BUTTON GPIO_PIN_5
#define USER_BUTTON3_Pin GPIO_PIN_6
#define USER_BUTTON4_Pin GPIO_PIN_7
#define USER_BUTTON_GPIO_Port GPIOA


#define Net_LED GPIO_PIN_4
#define Net_LED_GPIO_Port GPIOC
#define Net_Led_Flick_Pin GPIO_PIN_5
#define Net_Led_Flick_GPIO_Port GPIOC
#define Down_OK_Led_Pin GPIO_PIN_0
#define Down_OK_Led_GPIO_Port GPIOB
#define Job_OK_Led_Pin GPIO_PIN_1
#define Job_OK_Led_GPIO_Port GPIOB

#define BUZZER_Pin  GPIO_PIN_2
#define BUZZER_GPIO_Port  GPIOC


void sInteractive_Init(void);
void InteRactionTask(void *pArg);

#endif


