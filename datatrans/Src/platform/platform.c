#include "include.h"

/*ƽ̨��س�ʼ��*/
void platformInit(void)
{
  HAL_Init();
  SystemClock_Config();
//	delayInit();
  MX_GPIO_Init();									//��AB�˿�ʱ��
  sInteractive_Init();						//�����˿�IO��ʼ��
	LogInit(115200);								//DEBUG���ڳ�ʼ��
	LOG(LOG_INFO,"SYSTEM START\r\n");
	MX_CAN1_Init();
	//  MX_TIM2_Init();
//  MX_TIM3_Init();
//  MX_TIM4_Init();
  MX_USART2_UART_Init(115200);		//����ͨ�ţ����ں�LTEͨ��
//  MX_IWDG_Init();
//	MX_RNG_Init();
//	platformFlagInit();
	
}


