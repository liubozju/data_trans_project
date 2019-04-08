
#include "Interactive.h"
/*交互接口----LED   拨码开关  蜂鸣器等*/


/*init接口*/

void sInteractive_Init(void){

    GPIO_InitTypeDef GPIO_InitStruct = {0};


    /*Net 相关LED*/
    GPIO_InitStruct.Pin = Net_LED|Net_Led_Flick_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, Net_LED | Net_Led_Flick_Pin, Led_Off);

    /*Can_Data_Led_Pin  Job_OK 的引脚*/
    GPIO_InitStruct.Pin = Can_Data_Led_Pin|Job_OK_Led_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, Can_Data_Led_Pin | Job_OK_Led_Pin, Led_Off);

    /*拨码开关对应的初始化*/
    GPIO_InitStruct.Pin = USER_BUTTON1_Pin|USER_BUTTON2_Pin |USER_BUTTON3_Pin|USER_BUTTON4_Pin ;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

    /*蜂鸣器对应的初始化*/
    GPIO_InitStruct.Pin = BUZZER_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct); 
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, Buzzer_Off); 

}

/*交互任务*/
void InteRactionTask(void *pArg)   
{
    EventBits_t sGetEventBit ;
    while(1){
        if(InteracEventHandler != NULL){
            sGetEventBit = xEventGroupGetBits(InteracEventHandler);
            if(sGetEventBit & EventNetledOn)
                Net_LED_On;
            else
                Net_LED_Off;
            if(sGetEventBit & EventNetledFlick){
                HAL_GPIO_TogglePin(Net_Led_Flick_GPIO_Port,Net_Led_Flick_Pin);
                HAL_Delay(1000);
            }else
                Net_LED_Flick_Off;
            if(sGetEventBit & EventJobOk){
                BEEP_On;
                Job_OK_Led_On;
            }else{
                BEEP_Off;
                Job_OK_Led_Off;                
            }
            sGetEventBit = 0;   //清空
        }
				vTaskDelay(50);
    }
}







