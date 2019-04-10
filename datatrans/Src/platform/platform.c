#include "include.h"

/*平台相关初始化*/
void platformInit(void)
{
  HAL_Init();
  SystemClock_Config();
//	delayInit();
  MX_GPIO_Init();									//打开AB端口时钟
  sInteractive_Init();						//交互端口IO初始化
	LogInit(115200);								//DEBUG串口初始化
	LOG(LOG_INFO,"SYSTEM START\r\n");
	MX_CAN1_Init();
	//  MX_TIM2_Init();
//  MX_TIM3_Init();
//  MX_TIM4_Init();
  MX_USART2_UART_Init(115200);		//串口通信，用于和LTE通信
//  MX_IWDG_Init();
//	MX_RNG_Init();
//	platformFlagInit();
	
}


