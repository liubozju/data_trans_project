#include "include.h"

/*ƽ̨��س�ʼ��*/
void platformInit(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();									//��AB�˿�ʱ��
  sInteractive_Init();						//�����˿�IO��ʼ��
	LogInit(115200);								//DEBUG���ڳ�ʼ��
	Log(LOG_INFO,"SYSTEM START\r\n");
//	 MX_USART2_UART_Init(115200);		//����ͨ�ţ����ں�LTEͨ��
	 MX_USART2_UART_Init(460800);		//����ͨ�ţ����ں�LTEͨ��
//  MX_IWDG_Init();
//	platformFlagInit();
	
}


