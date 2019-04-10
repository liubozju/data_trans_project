/**
  ******************************************************************************
  * File Name          : CAN.c
  * Description        : This file provides code for the configuration
  *                      of the CAN instances.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include "log.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

CAN_ID can_id;

/* USER CODE BEGIN 0 */
CAN_TxHeaderTypeDef   TxMessage1;     
CAN_TxHeaderTypeDef   TxMessage2;    

CAN_RxHeaderTypeDef  RxMessage1;    
CAN_RxHeaderTypeDef  RxMessage2;     

CAN_RecvMsg can_recvmsg1;
CAN_RecvMsg can_recvmsg2;  
/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void MX_CAN1_Init(void)
{
	CAN_FilterTypeDef  sFilterConfig;

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 7;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_5TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_6TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
		printf("init failed 0");
    _Error_Handler(__FILE__, __LINE__);
  }

	 sFilterConfig.FilterIdHigh         = 0x0000;  
   sFilterConfig.FilterIdLow          = 0x1314;  
   sFilterConfig.FilterMaskIdHigh     = 0x0000;			
   sFilterConfig.FilterMaskIdLow      = 0x0000;			
   sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;        
   sFilterConfig.FilterBank=0;
   sFilterConfig.FilterScale=CAN_FILTERSCALE_32BIT;
   sFilterConfig.FilterMode=CAN_FILTERMODE_IDMASK;
   sFilterConfig.FilterActivation = ENABLE;         

   if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig)!=HAL_OK)
   {
		 printf("init failed 2");
     _Error_Handler(__FILE__, __LINE__); 
	 } 
	 HAL_CAN_Start(&hcan1);
	 HAL_CAN_ActivateNotification(&hcan1,CAN_IT_RX_FIFO0_MSG_PENDING);
	
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration    
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 3);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();
  
    /**CAN1 GPIO Configuration    
    PG0     ------> CAN1_RX
    PG1     ------> CAN1_TX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  if(hcan->Instance==CAN1)
  {
     HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxMessage1,can_recvmsg1.Data);
			for(int i=0;i<8;i++)
			{
				printf("RxMessage1:%x\r\n",can_recvmsg1.Data[i]);
			
			}
  }
	else if(hcan->Instance==CAN2)
	{
	 HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxMessage2,can_recvmsg2.Data);
	}
 
  
}

/*CAN总线发送数据的接口***
*	ID：发送的CANID
id_type:标准帧或者扩展帧
data_type:数据类型  数据帧或者远程帧
data：待发送的数据
*/
int gCAN_SendData(uint32_t ID,uint8_t id_type,uint8_t data_type,uint8_t * data)
{
	if(id_type == 0)		//标准帧
	{
		TxMessage1.StdId = ID;
		TxMessage1.IDE = CAN_ID_STD;
	}
	else
	{
		TxMessage1.ExtId = ID;
		TxMessage1.IDE = CAN_ID_EXT;		
	}
	if(data_type == 0 )	//发送的是数据帧
		TxMessage1.RTR = CAN_RTR_DATA;
	else
		TxMessage1.RTR = CAN_RTR_REMOTE;
	uint16_t sDataLen = strlen((const char *)data);	//获取数据总长度
	uint16_t sDataPage = sDataLen / 8 ;							//按照8字节进行划分
	uint16_t sDataLeft = sDataLen % 8 ;							//剩余的字节数
	uint16_t sCount = 0;
	uint8_t  sTR_Buf[9]={0};
	memset(sTR_Buf,0,9);
	for(;sDataPage>0 ;sDataPage--)									//数据有8个一组
	{
		TxMessage1.DLC = 8;
		memset(sTR_Buf,0,8);
		memcpy(sTR_Buf,(const void *)&data[sCount],8);//读取8个数据
		if(HAL_CAN_AddTxMessage(&hcan1,&TxMessage1,sTR_Buf,(uint32_t*)CAN_TX_MAILBOX0)!=HAL_OK)
		{
			LOG(LOG_ERROR,"sending wrong\r\n");
		  return 0;
		}
		sCount += 8;
	}
	vTaskDelay(20);
	if(sDataLeft > 0)							//不是8对齐的，有剩余数据
	{
		TxMessage1.DLC = sDataLeft;
		memset(sTR_Buf,0,8);
		memcpy(sTR_Buf,(const void *)&data[sCount],sDataLeft);//读取8个数据
//		LOG(LOG_INFO,"sTR_Buf : %s    sDataLeft: %d\r\n",sTR_Buf,sDataLeft);
		if(HAL_CAN_AddTxMessage(&hcan1,&TxMessage1,sTR_Buf,(uint32_t*)CAN_TX_MAILBOX0)!=HAL_OK)
		{
			LOG(LOG_ERROR,"sending wrong\r\n");
		 return 0;
		}		
	}
	return 1;
}

/* USER CODE END 1 */
void CAN_TRANSMIT1(void)
{						 

  
  TxMessage1.DLC=8;
  TxMessage1.StdId=0x314;
  TxMessage1.ExtId=0x00001314;
  TxMessage1.IDE=CAN_ID_EXT;
  TxMessage1.RTR=CAN_RTR_DATA;
 
	uint8_t TR_BUF[8]="88888888";

   if(HAL_CAN_AddTxMessage(&hcan1,&TxMessage1,TR_BUF,(uint32_t*)CAN_TX_MAILBOX0)!=HAL_OK)
  {
		printf("sending wrong\r\n");
  }
}
/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
