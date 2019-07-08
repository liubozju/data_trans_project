
#include "Interactive.h"
#include "button.h"
#include  "myflag.h"
#include  "string.h"
#include "upgrade.h"
#include "tim.h"
extern TIM_HandleTypeDef htim4;
extern gFlag_Data gFlag;
extern TimerHandle_t connectTimerHandler;
extern xTimerHandle NetTimerHandler;
extern uint8_t gNetConnectFlag;
extern Upgrade_info upgrade_info;


/*交互接口----LED   拨码开关  蜂鸣器等*/


Button_t ModeButton;

/*init接口*/

void sInteractive_Init(void){

    GPIO_InitTypeDef GPIO_InitStruct = {0};

		/*模组供电引脚使能*/
		GPIO_InitStruct.Pin = MOZU_POWER;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOZU_POWER_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(MOZU_POWER_GPIO_Port, MOZU_POWER, MOZU_POWER_Off);
		
		/*模组3.8V供电*/
		MOZU_POWER_END();
		HAL_Delay(2000);
		MOZU_POWER_START();	
		//while(1){};

    /*Net 相关LED*/
    GPIO_InitStruct.Pin = SD_MODE|Net_STATUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SD_MODE_LED_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SD_MODE_LED_GPIO_Port, SD_MODE | Net_STATUS_Pin, Led_Off);
	
		GPIO_InitStruct.Pin = Net_MODE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Net_MODE_LED_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(Net_MODE_LED_GPIO_Port, Net_MODE, Led_Off);

    /*Can_Data_Led_Pin  Job_OK 的引脚*/
    GPIO_InitStruct.Pin = Down_OK_Led_Pin|Job_OK_Led_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(Down_OK_Led_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(Down_OK_Led_GPIO_Port, Down_OK_Led_Pin | Job_OK_Led_Pin, Led_Off);

    /*拨码开关对应的初始化*/
    GPIO_InitStruct.Pin = MODE_BUTTON;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);
	
	  HAL_NVIC_SetPriority(EXTI4_IRQn, 8, 0);
		HAL_NVIC_EnableIRQ(EXTI4_IRQn);
		
		Button_Create("Button1",
              &ModeButton, 
              Read_Button_Level, 
              KEY_ON);

		Button_Attach(&ModeButton,BUTTON_DOWM,ModeBt_Dowm_CallBack);                       //单击 
		Button_Attach(&ModeButton,BUTTON_LONG,ModeBt_Long_CallBack);                       //长按

    /*蜂鸣器对应的初始化*/
    GPIO_InitStruct.Pin = BUZZER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct); 
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, Buzzer_Off); 

}

/*单击进行升级*/
void ModeBt_Dowm_CallBack(void *btn)
{
	BEEP_Off;
	printf("one button\r\n");
	xSemaphoreGiveFromISR(ConfigBinarySemaphore,0);
	HAL_TIM_Base_Stop_IT(&htim4);
}

/*长按切换模式*/
void ModeBt_Long_CallBack(void *btn)
{
	BEEP_Off;
	printf("change mode \r\n");
	HAL_TIM_Base_Stop_IT(&htim4);
	/*网络模式切换到SD卡模式*/
	if(gFlag.ModeFlag ==NET_MODE_FLAG)
	{
		gFlag.ModeFlag = SD_MODE_FLAG;
		STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
		upgrade_info.method = offline;
		xTimerStopFromISR(NetTimerHandler,0);
		xTimerStopFromISR(connectTimerHandler,0);
		xEventGroupSetBitsFromISR(InteracEventHandler,EventSDModeLedOn,0);
		xEventGroupClearBitsFromISR(InteracEventHandler,EventNETModeLedOn);
	}else if(gFlag.ModeFlag ==SD_MODE_FLAG){
		gFlag.ModeFlag = NET_MODE_FLAG;
		STMFLASH_Write_WithBuf(GLOBAL_FLAG,(uint8_t *)&gFlag,sizeof(gFlag));
		upgrade_info.method = online;
		xEventGroupSetBitsFromISR(InteracEventHandler,EventNETModeLedOn,0);
		xEventGroupClearBitsFromISR(InteracEventHandler,EventSDModeLedOn);
		if(gNetConnectFlag == NetDisConnect)
		{
			xTimerStartFromISR(connectTimerHandler,0);
		}
		xTimerStartFromISR(NetTimerHandler,0);
	}
	printf("gFlag.ModeFlag: %x \r\n",gFlag.ModeFlag);
}

static void sBeepOn(uint8_t second)
{
	BEEP_On;
	HAL_Delay(second*1000);
	BEEP_Off;
}

static void sBeepFlink(void)
{
	BEEP_On;
	Job_OK_Led_On;
	HAL_Delay(300);
	BEEP_Off;
	Job_OK_Led_Off;
	HAL_Delay(300);
	BEEP_On;
	Job_OK_Led_On;
	HAL_Delay(300);
	BEEP_Off;
	Job_OK_Led_Off;
	HAL_Delay(300);
	BEEP_On;
	Job_OK_Led_On;
	HAL_Delay(300);
	BEEP_Off;
	Job_OK_Led_Off;
	HAL_Delay(300);
}

/*交互任务*/
void InteRactionTask(void *pArg)   
{
    EventBits_t sGetEventBit ;
    while(1){
        if(InteracEventHandler != NULL){
						/*获取事件标志组*/
            sGetEventBit = xEventGroupGetBits(InteracEventHandler);
						/*NET 状态LED展示*/
            if(sGetEventBit & EventNetledOn)													/*联网成功*/
						{
							xEventGroupClearBits(InteracEventHandler,EventNetledFlick);
              Net_LED_On;
						}
            else if(sGetEventBit & EventNetledFlick){									/*联网失败或者网络信号差，闪烁LED*/
								xEventGroupClearBits(InteracEventHandler,EventNetledOn);
                HAL_GPIO_TogglePin(Net_STATUS_LED_GPIO_Port,Net_STATUS_Pin);
            }else																											/*不使用网络信号灯*/
                Net_LED_Off;
						/*工作模式状态提示*/
						if(sGetEventBit & EventSDModeLedOn){
							SD_MODE_LED_On;
							Net_MODE_LED_Off;
						}else if(sGetEventBit & EventNETModeLedOn){
							Net_MODE_LED_On;
							SD_MODE_LED_Off;
						}
						/*下载与升级完成信号灯*/
            if(sGetEventBit & EventDownOk){														/*下载完成LED灯*/
								Down_OK_Led_On;
                sBeepOn(2);
								xEventGroupClearBits(InteracEventHandler,EventDownOk);
            }
						if(sGetEventBit & EventFirmOk){
								Job_OK_Led_On;
                sBeepOn(2);	
								xEventGroupClearBits(InteracEventHandler,EventFirmOk);
						}
						if(sGetEventBit & EventFirmFail){
								sBeepFlink();
								xEventGroupClearBits(InteracEventHandler,EventFirmFail);
						}
        }
				vTaskDelay(500);
    }
}

/*PA4---外部中断函数*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	BEEP_On;
	HAL_TIM_Base_Start_IT(&htim4);
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);	
}


uint8_t Read_Button_Level(void)
{
  return HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port,MODE_BUTTON);
}
 








