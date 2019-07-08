#include "include.h"
#include "dma.h"
#include "sdio.h"
#include "stdio.h"


/*ƽ̨��س�ʼ��*/
void platformInit(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();									//��AB�˿�ʱ��
  sInteractive_Init();						//�����˿�IO��ʼ��
	MX_DMA_Init();									//SD��dma��ʼ��
	MX_SDIO_SD_Init();
	LogInit(115200);								//DEBUG���ڳ�ʼ��
	MX_TIM4_Init();
	Log(LOG_INFO,"SYSTEM START\r\n");
  MX_USART2_UART_Init(115200);		//����ͨ�ţ����ں�LTEͨ��
//  MX_IWDG_Init();
	platformFlagInit();
	
}


