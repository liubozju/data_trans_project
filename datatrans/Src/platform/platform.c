#include "include.h"
#include "dma.h"
#include "sdio.h"
#include "stdio.h"


/*平台相关初始化*/
void platformInit(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();									//打开AB端口时钟
	LogInit(115200);								//DEBUG串口初始化
	Log(LOG_INFO,"SYSTEM START\r\n");
  sInteractive_Init();						//交互端口IO初始化
	MX_DMA_Init();									//SD卡dma初始化
	//MX_SDIO_SD_Init();
	MX_TIM4_Init();
  MX_USART2_UART_Init(115200);		//串口通信，用于和LTE通信
  MX_IWDG_Init();
	platformFlagInit();
}


