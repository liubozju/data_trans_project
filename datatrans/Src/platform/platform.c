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
	LogInit(115200);								//DEBUG���ڳ�ʼ��
	Log(LOG_INFO,"SYSTEM START\r\n");
  sInteractive_Init();						//�����˿�IO��ʼ��
	MX_DMA_Init();									//SD��dma��ʼ��
	//MX_SDIO_SD_Init();
	MX_TIM4_Init();
  MX_USART2_UART_Init(115200);		//����ͨ�ţ����ں�LTEͨ��
  MX_IWDG_Init();
	platformFlagInit();
}


