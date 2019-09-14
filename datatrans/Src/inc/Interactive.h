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

typedef enum{
	LEDBLINK_FALSE = 0,
	LEDBLINK_TRUE,
}ledblink;

extern EventGroupHandle_t InteracEventHandler;

/*事件标志位*/
#define EventNetledOn   (1<<0)      //NetLed点亮
#define EventNetledFlick (1<<1)     //信号差（连接网络失败），闪烁LED灯
#define EventSDModeLedOn   (1<<2)      //SD卡状态模式
#define EventNETModeLedOn   (1<<3)      //网络信号模式
#define EventDownOk   (1<<4)         //下载完成信号灯		--LED灯点亮 蜂鸣器提示
#define EventFirmOk     (1<<5)      //升级完成信号灯		---LED灯点亮，蜂鸣器提示
#define EventFirmFail     (1<<6)      //升级失败信号灯		---LED灯熄灭，蜂鸣器闪烁提示


#define NetConnected	1
#define NetDisConnect	0

#define Net_LED_On  HAL_GPIO_WritePin(Net_STATUS_LED_GPIO_Port, Net_STATUS_Pin , Led_On)
#define Net_LED_Off  HAL_GPIO_WritePin(Net_STATUS_LED_GPIO_Port, Net_STATUS_Pin , Led_Off)

#define Net_MODE_LED_On  HAL_GPIO_WritePin(Net_MODE_LED_GPIO_Port, Net_MODE , Led_On)
#define Net_MODE_LED_Off  HAL_GPIO_WritePin(Net_MODE_LED_GPIO_Port, Net_MODE , Led_Off)

#define SD_MODE_LED_On  HAL_GPIO_WritePin(SD_MODE_LED_GPIO_Port, SD_MODE , Led_On)
#define SD_MODE_LED_Off  HAL_GPIO_WritePin(SD_MODE_LED_GPIO_Port, SD_MODE , Led_Off)

#define Down_OK_Led_On  HAL_GPIO_WritePin(Down_OK_Led_GPIO_Port, Down_OK_Led_Pin , Led_On)
#define Down_OK_Led_Off  HAL_GPIO_WritePin(Down_OK_Led_GPIO_Port, Down_OK_Led_Pin , Led_Off)

#define Job_OK_Led_On  HAL_GPIO_WritePin(Job_OK_Led_GPIO_Port, Job_OK_Led_Pin , Led_On)
#define Job_OK_Led_Off  HAL_GPIO_WritePin(Job_OK_Led_GPIO_Port, Job_OK_Led_Pin , Led_Off)

#define Net_LED_Flick_Off  HAL_GPIO_WritePin(Net_STATUS_LED_GPIO_Port, Net_STATUS_Pin , Led_Off)

#define BEEP_On HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin , Buzzer_On)
#define BEEP_Off HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin , Buzzer_Off)

#define Led_On  GPIO_PIN_RESET
#define Led_Off GPIO_PIN_SET

#define Buzzer_On  GPIO_PIN_SET
#define Buzzer_Off GPIO_PIN_RESET

#define MOZU_POWER_On  GPIO_PIN_SET
#define MOZU_POWER_Off GPIO_PIN_RESET

#define MOZU_POWER_START()	HAL_GPIO_WritePin(MOZU_POWER_GPIO_Port, MOZU_POWER , MOZU_POWER_On)
#define MOZU_POWER_END()	HAL_GPIO_WritePin(MOZU_POWER_GPIO_Port, MOZU_POWER , MOZU_POWER_Off)


#define MOZU_POWER GPIO_PIN_6
#define MOZU_POWER_GPIO_Port GPIOC

/*交互接口的引脚定义功能*/
#define MODE_BUTTON GPIO_PIN_4							//PA4----唯一按钮，用于选择模式。短按：升级  长按：切换模式
#define START_UPGRADE_BUTTON GPIO_PIN_5

#define USER_BUTTON_GPIO_Port GPIOA

#define Net_MODE  GPIO_PIN_6								//PA6----显示当前模式为网络模式
#define Net_MODE_LED_GPIO_Port GPIOA

#define SD_MODE GPIO_PIN_4									//PC4----显示当前模式为SD卡模式LED
#define SD_MODE_LED_GPIO_Port GPIOC


#define Net_STATUS_Pin GPIO_PIN_5						//PC5----显示当前网络状态，如果网络连接成功，常亮。如果网络连接失败，闪烁；如果信号强度较差（<12)，闪烁
#define Net_STATUS_LED_GPIO_Port GPIOC

#define Down_OK_Led_Pin GPIO_PIN_1					//PB0----下载成功LED，固件成功下载到本地，网络模式下使用
#define Down_OK_Led_GPIO_Port GPIOB

#define Job_OK_Led_Pin GPIO_PIN_0						//PB1----升级成功LED，固件通过CAN总线升级成功，SD和网络模式下均使用
#define Job_OK_Led_GPIO_Port GPIOB

#define BUZZER_Pin  GPIO_PIN_2
#define BUZZER_GPIO_Port  GPIOC

#define Down_OK         PBout(1)

#define Down_OK_OFF(x)  (Down_OK = x) 
#define Down_OK_CONV()  (Down_OK = ~Down_OK)

#define Job_OK         PBout(0)
#define Job_OK_OFF(x)  (Job_OK = x)
#define Job_OK_CONV()  (Job_OK = ~Job_OK)



void sInteractive_Init(void);
void InteRactionTask(void *pArg);
uint8_t Read_Button_Level(void);
void ModeBt_Dowm_CallBack(void *btn);
void ModeBt_Long_CallBack(void *btn);


#endif


