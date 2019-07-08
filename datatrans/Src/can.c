/* Includes ------------------------------------------------------------------*/
#include "can.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include "log.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "Interactive.h"
#include "upgrade.h"
#include "gprs.h"
#include "message.h"


/*请求链接帧*/
const uint8_t Connect_P[8]={0x02,0x10,0xfa,0xff,0xff,0xff,0xff,0xff};
const uint8_t Connect_P_OK[8]={0x02,0x50,0xfa,0xff,0xff,0xff,0xff,0xff};

/*请求安全访问帧*/
const uint8_t SafeConnect_P[8]={0x02,0x27,0xfb,0xff,0xff,0xff,0xff,0xff};
uint8_t SafeConnect_P_OK[8]={0x04,0x67,0xfb,0x01,0x02,0xff,0xff,0xff};   //3  4 

/*解密帧*/
/*解密算法： **
**  收到同意访问帧：SafeConnect_P_OK[8]={0x04,0x67,0xfb,0xH1,0xL1,0xff,0xff,0xff}; 
**	计算得到0XH2 0XL2    Decrypt_P[8]={0x04,0x27,0xfc,0xH2,0xL1,0xff,0xff,0xff};
**  计算方法是：  (((0xH1*256+0XL1) XOR 0X4521) + ((0XH1*256+0XL1) AND 0X3421))  AND 0XFFFF
*/
uint8_t Decrypt_P[8]={0x04,0x27,0xfc,0x02,0x01,0xff,0xff,0xff};					//3   4
const uint8_t Decrypt_P_OK[8]={0x03,0x67,0xfc,0x34,0xff,0xff,0xff,0xff};
const uint8_t Decrypt_P_FAIL[8]={0x03,0x67,0xfc,0x35,0xff,0xff,0xff,0xff};

/*固件更新指令帧*/
const uint8_t Update_P[8]={0x10,0x01,0x92,0xff,0xff,0xff,0xff,0xff};
const uint8_t Update_P_OK[8]={0x02,0x7B,0x92,0x00,0x00,0x00,0x00,0x00};

/*行更新指令*/
const uint8_t LineEnd_P[8] = {0x10,0x01,0xFE,0x00,0x00,0x00,0x00,0x00};
const uint8_t LineEnd_P_OK[8] = {0x02,0x7B,0x03,0x00,0x00,0x00,0x00,0x00};
const uint8_t LineEnd_P_FAIL[8] = {0x02,0x7B,0x00,0x00,0x00,0x00,0x00,0x00};

/*固件更新完成*/
const uint8_t UpdateFinish_P[8]={0x10,0x01,0xfc,0x00,0x00,0x00,0x00,0x00};
const uint8_t UpdateFinish_P_OK[8]={0x02,0x7B,0x01,0x00,0x00,0x00,0x00,0x00};

/*返回的数据，用于校验CAN返回包是否正确*/
uint8_t Can_rev_data[9]={0};

SemaphoreHandle_t CANSendStart;					/*发送起始信号量*/
SemaphoreHandle_t CanRevStatus;					/*等待CAN返回帧*/
SemaphoreHandle_t Can2RevStatus;					/*等待CAN返回帧*/


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
CAN_HandleTypeDef hcan2;

/* CAN1 init function */
void MX_CAN1_Init(uint32_t RecID)
{
	CAN_FilterTypeDef  sFilterConfig;

  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 7;
  hcan1.Init.Mode = CAN_MODE_NORMAL;					//正常工作模式
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;			//500K的通信速率
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

	 sFilterConfig.FilterIdHigh         = (((uint32_t)RecID<<21)&0xffff0000)>>16;  
	 sFilterConfig.FilterIdLow          = (((uint32_t)RecID<<21)|CAN_ID_STD|CAN_RTR_DATA)& 0xffff;  
	 sFilterConfig.FilterMaskIdHigh     = 0xFFFF;			
	 sFilterConfig.FilterMaskIdLow      = 0xFFFF;			
	 sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;        
	 sFilterConfig.FilterBank=0;
	 sFilterConfig.FilterScale=CAN_FILTERSCALE_32BIT;
	 sFilterConfig.FilterMode=CAN_FILTERMODE_IDMASK;
	 sFilterConfig.FilterActivation = ENABLE;
	 sFilterConfig.SlaveStartFilterBank = 14;

   if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig)!=HAL_OK)
   {
		 printf("init failed 2");
     _Error_Handler(__FILE__, __LINE__); 
	 }
	 HAL_CAN_Start(&hcan1);
	 HAL_CAN_ActivateNotification(&hcan1,CAN_IT_RX_FIFO0_MSG_PENDING);
	
}

/* CAN1 init function */
void MX_CAN2_Init(uint32_t RecID)
{
	CAN_FilterTypeDef  sFilterConfig;

  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 7;
  hcan2.Init.Mode = CAN_MODE_NORMAL;					//正常工作模式
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;			//500K的通信速率
  hcan2.Init.TimeSeg1 = CAN_BS1_5TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_6TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
		printf("init failed 0");
    _Error_Handler(__FILE__, __LINE__);
  }

	 sFilterConfig.FilterIdHigh         = (((uint32_t)RecID<<21)&0xffff0000)>>16;  
	 sFilterConfig.FilterIdLow          = (((uint32_t)RecID<<21)|CAN_ID_STD|CAN_RTR_DATA)& 0xffff;  
	 sFilterConfig.FilterMaskIdHigh     = 0xFFFF;			
	 sFilterConfig.FilterMaskIdLow      = 0xFFFF;			
	 sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;        
	 sFilterConfig.FilterBank=14;
	 sFilterConfig.FilterScale=CAN_FILTERSCALE_32BIT;
	 sFilterConfig.FilterMode=CAN_FILTERMODE_IDMASK;
	 sFilterConfig.FilterActivation = ENABLE;
	 sFilterConfig.SlaveStartFilterBank = 14;	

   if(HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig)!=HAL_OK)
   {
		 printf("init failed 2");
     _Error_Handler(__FILE__, __LINE__); 
	 } 
	 HAL_CAN_Start(&hcan2);
	 HAL_CAN_ActivateNotification(&hcan2,CAN_IT_RX_FIFO0_MSG_PENDING);
	
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
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }else if(canHandle->Instance==CAN2)
  {
		__HAL_RCC_CAN2_CLK_ENABLE();
		__HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration    
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
		
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
//			for(int i=0;i<8;i++)
//			{
//				printf("RxMessage1:%x\r\n",can_recvmsg1.Data[i]);
//			
//			}
			xSemaphoreGiveFromISR(CanRevStatus,0);
  }
	else if(hcan->Instance==CAN2)
	{
	 HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxMessage2,can_recvmsg2.Data);
		xSemaphoreGiveFromISR(Can2RevStatus,0);
	} 
}

/*CAN总线发送数据的接口***
*	ID：发送的CANID
id_type:标准帧或者扩展帧
data_type:数据类型  数据帧或者远程帧
data：待发送的数据
*/

int gCAN_SendData(uint32_t ID,uint8_t id_type,uint8_t data_type,const unsigned char * data,const uint16_t datalen)
{
	uint16_t i=0;
	uint32_t TX_MailBOX=CAN_TX_MAILBOX0;
	if(id_type == CAN_ID_STD)		//标准帧
	{
		TxMessage1.StdId = ID;
		TxMessage1.IDE = CAN_ID_STD;
	}
	else
	{
		TxMessage1.ExtId = ID;
		TxMessage1.IDE = CAN_ID_EXT;		
	}
	if(data_type == CAN_RTR_DATA )	//发送的是数据帧
		TxMessage1.RTR = CAN_RTR_DATA;
	else
		TxMessage1.RTR = CAN_RTR_REMOTE;
	uint16_t sDataLen = datalen;	//获取数据总长度
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
		while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)==0 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		i=0;
		while(HAL_CAN_IsTxMessagePending(&hcan1,CAN_TX_MAILBOX0)==1 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		if(HAL_CAN_AddTxMessage(&hcan1,&TxMessage1,sTR_Buf,&TX_MailBOX)!=HAL_OK)
		{
			/*此处如果发送失败需要上报发送失败的消息*/
			LOG(LOG_ERROR,"sending wrong \r\n");
			//gUploadErrorCode(CAN_SEND_ERR);
		  return 0;
		}
		sCount += 8;
	}
	if(sDataLeft > 0)							//不是8对齐的，有剩余数据
	{
		TxMessage1.DLC = sDataLeft;
		memset(sTR_Buf,0,8);
		memcpy(sTR_Buf,(const void *)&data[sCount],sDataLeft);//读取8个数据
		while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)==0 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		i=0;
		while(HAL_CAN_IsTxMessagePending(&hcan1,CAN_TX_MAILBOX0)==1 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}	
		if(HAL_CAN_AddTxMessage(&hcan1,&TxMessage1,sTR_Buf,&TX_MailBOX)!=HAL_OK)
		{
			/*此处如果发送失败需要上报发送失败的消息*/
			LOG(LOG_ERROR,"sending wrong\r\n");
			//gUploadErrorCode(CAN_SEND_ERR);
		 return 0;
		}		
	}
	return 1;
}

/*CAN总线发送数据的接口***
*	ID：发送的CANID
id_type:标准帧或者扩展帧
data_type:数据类型  数据帧或者远程帧
data：待发送的数据
*/

int gCAN2_SendData(uint32_t ID,uint8_t id_type,uint8_t data_type,const unsigned char * data,const uint16_t datalen)
{
	uint16_t i=0;
	uint32_t TX_MailBOX=CAN_TX_MAILBOX0;
	if(id_type == CAN_ID_STD)		//标准帧
	{
		TxMessage2.StdId = ID;
		TxMessage2.IDE = CAN_ID_STD;
	}
	else
	{
		TxMessage2.ExtId = ID;
		TxMessage2.IDE = CAN_ID_EXT;		
	}
	if(data_type == CAN_RTR_DATA )	//发送的是数据帧
		TxMessage2.RTR = CAN_RTR_DATA;
	else
		TxMessage2.RTR = CAN_RTR_REMOTE;
	uint16_t sDataLen = datalen;	//获取数据总长度
	uint16_t sDataPage = sDataLen / 8 ;							//按照8字节进行划分
	uint16_t sDataLeft = sDataLen % 8 ;							//剩余的字节数
	uint16_t sCount = 0;
	uint8_t  sTR_Buf[9]={0};
	memset(sTR_Buf,0,9);
	for(;sDataPage>0 ;sDataPage--)									//数据有8个一组
	{
		TxMessage2.DLC = 8;
		memset(sTR_Buf,0,8);
		memcpy(sTR_Buf,(const void *)&data[sCount],8);//读取8个数据
		while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan2)==0 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		i=0;
		while(HAL_CAN_IsTxMessagePending(&hcan2,CAN_TX_MAILBOX0)==1 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		if(HAL_CAN_AddTxMessage(&hcan2,&TxMessage2,sTR_Buf,&TX_MailBOX)!=HAL_OK)
		{
			/*此处如果发送失败需要上报发送失败的消息*/
			LOG(LOG_ERROR,"sending wrong \r\n");
			//gUploadErrorCode(CAN_SEND_ERR);
		  return 0;
		}
		sCount += 8;
	}
	if(sDataLeft > 0)							//不是8对齐的，有剩余数据
	{
		TxMessage2.DLC = sDataLeft;
		memset(sTR_Buf,0,8);
		memcpy(sTR_Buf,(const void *)&data[sCount],sDataLeft);//读取8个数据
		while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan2)==0 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		i=0;
		while(HAL_CAN_IsTxMessagePending(&hcan2,CAN_TX_MAILBOX0)==1 && i<0xfff) i++;
		if(i>=0xfff){
			return 0;
		}
		if(HAL_CAN_AddTxMessage(&hcan2,&TxMessage2,sTR_Buf,&TX_MailBOX)!=HAL_OK)
		{
			/*此处如果发送失败需要上报发送失败的消息*/
			LOG(LOG_ERROR,"sending wrong\r\n");
			//gUploadErrorCode(CAN_SEND_ERR);
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

/*具体的发送函数*/
static uint8_t CANSend(const unsigned char * send_data,const unsigned char * rec_data,const uint16_t datalen)
{
	int rc = -1;
	BaseType_t err;
	
	memset(can_recvmsg1.Data,0,sizeof(can_recvmsg1.Data));
	gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,send_data,datalen);
	
	/*发送完数据后，等待数据返回，时间1S*/
	err = xSemaphoreTake(CanRevStatus,20000);
	/*等到信号量*/
	if(err == pdTRUE){
			/*判断CAN接收到的数据是否和期望值不相同*/
			if(memcmp(rec_data,can_recvmsg1.Data,8) == 0){
				rc = Can_Success;
			}else if(send_data[0] == 0x04){		/*CAN发送解密帧有两种返回*/
				if(memcmp(Decrypt_P_FAIL,can_recvmsg1.Data,8)==0){	/*收到解密失败帧*/
					//gUploadErrorCode(CAN_DECRYPT_ERR);
					LOG(LOG_ERROR,"Decrypt_P_FAIL\r\n");
					rc = Can_Connect_Failed;
				}else{		/*既不是解密成功帧  也不是解密失败帧  是错误帧*/
					//gUploadErrorCode(CAN_WRONG_REV_ERR);
					LOG(LOG_ERROR,"Decrypt_P wrong FAIL\r\n");
					rc = Can_Rev_Wrong;
				}
			}else if(send_data[2] == 0xFE){		/*行更新指令同样具有两种返回*/
				if(memcmp(LineEnd_P_FAIL,can_recvmsg1.Data,8) == 0){			
					LOG(LOG_ERROR,"LineEnd_P_FAIL\r\n");
					//gUploadErrorCode(CAN_FILE_ERR);
					rc = Can_File_Wrong;
				}else{	/*收到其他无意义信息*/
					//gUploadErrorCode(CAN_WRONG_REV_ERR);
					LOG(LOG_ERROR,"LineEnd wrong FAIL\r\n");
					rc = Can_Rev_Wrong;
				}
		}else{/*收到其他无意义信息*/
			//gUploadErrorCode(CAN_WRONG_REV_ERR);
			LOG(LOG_ERROR,"CAN wrong FAIL\r\n");
			rc = Can_Rev_Wrong;
		}
	}else{	/*没有接收到数据，等待超时，需要上报错误信息*/
		//gUploadErrorCode(CAN_NO_REV_ERR);
		LOG(LOG_ERROR,"can is not responding.\r\n");
		rc = Can_Rev_Wrong;
	}
	return rc;
}


/*具体的发送函数*/
static uint8_t CAN2Send(const unsigned char * send_data,const unsigned char * rec_data,const uint16_t datalen)
{
	int rc = -1;
	BaseType_t err;
	
	memset(can_recvmsg2.Data,0,sizeof(can_recvmsg2.Data));
	gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,send_data,datalen);
	
	/*发送完数据后，等待数据返回，时间1S*/
	err = xSemaphoreTake(Can2RevStatus,20000);
	/*等到信号量*/
	if(err == pdTRUE){
			/*判断CAN接收到的数据是否和期望值不相同*/
			if(memcmp(rec_data,can_recvmsg2.Data,8) == 0){
				rc = Can_Success;
			}else if(send_data[0] == 0x04){		/*CAN发送解密帧有两种返回*/
				if(memcmp(Decrypt_P_FAIL,can_recvmsg2.Data,8)==0){	/*收到解密失败帧*/
					//gUploadErrorCode(CAN_DECRYPT_ERR);
					LOG(LOG_ERROR,"Decrypt_P_FAIL\r\n");
					rc = Can_Connect_Failed;
				}else{		/*既不是解密成功帧  也不是解密失败帧  是错误帧*/
					//gUploadErrorCode(CAN_WRONG_REV_ERR);
					LOG(LOG_ERROR,"Decrypt_P wrong FAIL\r\n");
					rc = Can_Rev_Wrong;
				}
			}else if(send_data[2] == 0xFE){		/*行更新指令同样具有两种返回*/
				if(memcmp(LineEnd_P_FAIL,can_recvmsg2.Data,8) == 0){			
					LOG(LOG_ERROR,"LineEnd_P_FAIL\r\n");
					//gUploadErrorCode(CAN_FILE_ERR);
					rc = Can_File_Wrong;
				}else{	/*收到其他无意义信息*/
					//gUploadErrorCode(CAN_WRONG_REV_ERR);
					LOG(LOG_ERROR,"LineEnd wrong FAIL\r\n");
					rc = Can_Rev_Wrong;
				}
		}else{/*收到其他无意义信息*/
			//gUploadErrorCode(CAN_WRONG_REV_ERR);
			LOG(LOG_ERROR,"CAN wrong FAIL\r\n");
			rc = Can_Rev_Wrong;
		}
	}else{	/*没有接收到数据，等待超时，需要上报错误信息*/
		//gUploadErrorCode(CAN_NO_REV_ERR);
		LOG(LOG_ERROR,"can is not responding.\r\n");
		rc = Can_Rev_Wrong;
	}
	return rc;
}

///*CAN数据发送之前的准备函数*/
//uint8_t CanSendLinePack(uint8_t * sLinepack)
//{
//	int rc = -1;
//	BaseType_t err;	
//	/*如果是一号端口*/
//	if(can_id.Can_num == Can_num1){	

//		/*发送一行固件包数据*/
//		gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,sLinepack);
//		/*发送行结束帧*/
//		rc = CANSend(LineEnd_P,LineEnd_P_OK);
//		if(rc != Can_Success){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
//			return rc;
//		}
//		rc = CAN_PRE_OK;
//	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
//		/*发送一行固件包数据*/
//		gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,sLinepack);
//		/*发送行结束帧*/
//		rc = CANSend(LineEnd_P,LineEnd_P_OK);
//		if(rc != Can_Success){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
//			return rc;
//		}
//		rc = CAN_PRE_OK;	
//	}
//	return rc;
//}

///*CAN数据发送之前的准备函数*/
//uint8_t CanSendEndPack(void)
//{
//	int rc = -1;
//	BaseType_t err;	
//	/*如果是一号端口*/
//	if(can_id.Can_num == Can_num1){	

//		/*发送固件结束帧*/
//		rc = CANSend(UpdateFinish_P,UpdateFinish_P_OK);
//		if(rc != Can_Success){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
//			return rc;
//		}
//		rc = CAN_PRE_OK;
//	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
//		/*发送固件结束帧*/
//		rc = CANSend(UpdateFinish_P,UpdateFinish_P_OK);
//		if(rc != Can_Success){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
//			return rc;
//		}
//		rc = CAN_PRE_OK;	
//	}
//	return rc;
//}


/*CAN数据发送之前的准备函数*/
uint8_t CanPre(void)
{
	int rc = -1;
	char H1=0;char L1=0;
	char H2=0;char L2=0;
	BaseType_t err;
	
	/*初始化对应CAN接口*/
	/*如果是一号端口*/
	if(can_id.Can_num == Can_num1){
		//MX_CAN1_Init(can_id.RecID);	/*指定接收ID初始化对应端口*/
		
		/*发送请求链接帧*/
		LOG(LOG_INFO,"start sending Connect_P\r\n");
		
		rc = CANSend(Connect_P,Connect_P_OK,sizeof(Connect_P));
		if(rc != Can_Success){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN SEND is wrong.! Please check!\r\n");
			return rc;
		}
		/*发送请求安全访问帧*/
		memset(can_recvmsg1.Data,0,sizeof(can_recvmsg1.Data));
		gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,SafeConnect_P,sizeof(SafeConnect_P));
		/*发送完数据后，等待数据返回，时间1S*/
		err = xSemaphoreTake(CanRevStatus,20000);
		/*等到信号量*/
		if(err == pdTRUE){
			/*判断安全访问帧返回是否正确*/
			if(can_recvmsg1.Data[0] != 0x04 || can_recvmsg1.Data[1] != 0x67 ||can_recvmsg1.Data[2] != 0xfb){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN SafeConnect_P is wrong.! Please check!\r\n");
				return rc;			
			}
			H1=can_recvmsg1.Data[3];
			L1=can_recvmsg1.Data[4];
			H2=((((H1*256+L1)^0x4521) + ((H1*256+L1)&0x3421))>>8)&0xff;
			L2=((((H1*256+L1)^0x4521) + ((H1*256+L1)&0x3421)))&0xff;
			Decrypt_P[3]=H2;Decrypt_P[4]=L2;
		}
		/*发送解密帧*/
		rc = CANSend(Decrypt_P,Decrypt_P_OK,sizeof(Decrypt_P));
		if(rc != Can_Success){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
				return rc;	
		}
		/*发送固件更新帧*/
		rc = CANSend(Update_P,Update_P_OK,sizeof(Update_P));
		if(rc != Can_Success){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
				return rc;	
		}
		rc = CAN_PRE_OK;
	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
		//MX_CAN2_Init(can_id.RecID);	/*指定接收ID初始化对应端口*/
		
		/*发送请求链接帧*/
		LOG(LOG_INFO,"start sending Connect_P\r\n");
		
		rc = CAN2Send(Connect_P,Connect_P_OK,sizeof(Connect_P));
		if(rc != Can_Success){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN SEND is wrong.! Please check!\r\n");
			return rc;
		}
		/*发送请求安全访问帧*/
		memset(can_recvmsg2.Data,0,sizeof(can_recvmsg2.Data));
		gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,SafeConnect_P,sizeof(SafeConnect_P));
		/*发送完数据后，等待数据返回，时间1S*/
		err = xSemaphoreTake(Can2RevStatus,20000);
		/*等到信号量*/
		if(err == pdTRUE){
			/*判断安全访问帧返回是否正确*/
			if(can_recvmsg2.Data[0] != 0x04 || can_recvmsg2.Data[1] != 0x67 ||can_recvmsg2.Data[2] != 0xfb){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN SafeConnect_P is wrong.! Please check!\r\n");
				return rc;			
			}
			H1=can_recvmsg2.Data[3];
			L1=can_recvmsg2.Data[4];
			H2=((((H1*256+L1)^0x4521) + ((H1*256+L1)&0x3421))>>8)&0xff;
			L2=((((H1*256+L1)^0x4521) + ((H1*256+L1)&0x3421)))&0xff;
			Decrypt_P[3]=H2;Decrypt_P[4]=L2;
		}
		/*发送解密帧*/
		rc = CAN2Send(Decrypt_P,Decrypt_P_OK,sizeof(Decrypt_P));
		if(rc != Can_Success){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
				return rc;	
		}
		/*发送固件更新帧*/
		rc = CAN2Send(Update_P,Update_P_OK,sizeof(Update_P));
		if(rc != Can_Success){
				rc = CAN_PRE_FAIL;
				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
				return rc;	
		}
		rc = CAN_PRE_OK;		
	}
	return rc;
}

///*CAN数据发送之前的准备函数*/
//uint8_t CanPre(void)
//{
//	int rc = -1;
//	char H1=0;char L1=0;
//	char H2=0;char L2=0;
//	BaseType_t err;
//	/*初始化对应CAN接口*/
//	/*如果是一号端口*/
//	if(can_id.Can_num == Can_num1){
//		MX_CAN1_Init(can_id.RecID);	/*指定接收ID初始化对应端口*/
//		
//		/*发送请求链接帧*/
//		rc = gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Connect_P,sizeof(Connect_P));
//		if(rc != 1){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN SEND is wrong.! Please check!\r\n");
//			return rc;
//		}
//		/*发送请求安全访问帧*/
//		memset(can_recvmsg1.Data,0,sizeof(can_recvmsg1.Data));
//		gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,SafeConnect_P,sizeof(SafeConnect_P));
//		/*发送完数据后，等待数据返回，时间1S*/
//		/*发送解密帧*/
//		rc = gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Decrypt_P,sizeof(Decrypt_P));
//		if(rc != 1){
//				rc = CAN_PRE_FAIL;
//				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
//				return rc;	
//		}
//		/*发送固件更新帧*/
//		rc = gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Update_P,sizeof(Update_P));
//		if(rc != 1){
//				rc = CAN_PRE_FAIL;
//				LOG(LOG_ERROR,"CAN Update_P is wrong.! Please check!\r\n");
//				return rc;	
//		}
//		rc = CAN_PRE_OK;
//	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
//		MX_CAN2_Init(can_id.RecID);	/*指定接收ID初始化对应端口*/
//		
//		/*发送请求链接帧*/
//		rc = gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Connect_P,sizeof(Connect_P));
//		if(rc != 1){
//			rc = CAN_PRE_FAIL;
//			LOG(LOG_ERROR,"CAN SEND is wrong.! Please check!\r\n");
//			return rc;
//		}
//		/*发送请求安全访问帧*/
//		memset(can_recvmsg1.Data,0,sizeof(can_recvmsg1.Data));
//		gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,SafeConnect_P,sizeof(SafeConnect_P));
//		/*发送完数据后，等待数据返回，时间1S*/
//		/*发送解密帧*/
//		rc = gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Decrypt_P,sizeof(Decrypt_P));
//		if(rc != 1){
//				rc = CAN_PRE_FAIL;
//				LOG(LOG_ERROR,"CAN Decrypt_P is wrong.! Please check!\r\n");
//				return rc;	
//		}
//		/*发送固件更新帧*/
//		rc = gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,Update_P,sizeof(Update_P));
//		if(rc != 1){
//				rc = CAN_PRE_FAIL;
//				LOG(LOG_ERROR,"CAN Update_P is wrong.! Please check!\r\n");
//				return rc;	
//		}	
//	}
//	return rc;
//}


/*CAN数据发送之前的准备函数*/
uint8_t CanSendLinePack(uint8_t * sLinepack)
{
	int rc = -1;
	BaseType_t err;	
	/*如果是一号端口*/
	if(can_id.Can_num == Can_num1){	

		/*发送一行固件包数据*/
		gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,sLinepack,strlen((const char *)sLinepack));
		/*发送行结束帧*/
		rc = gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,LineEnd_P,sizeof(LineEnd_P));
		if(rc != 1){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
			return rc;
		}
		rc = CAN_PRE_OK;
	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
		/*发送一行固件包数据*/
		gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,sLinepack,strlen((const char *)sLinepack));
		/*发送行结束帧*/
		rc = gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,LineEnd_P,sizeof(LineEnd_P));
		if(rc != 1){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
			return rc;
		}
		rc = CAN_PRE_OK;
	}
	return rc;
}

/*CAN数据发送之前的准备函数*/
uint8_t CanSendEndPack(void)
{
	int rc = -1;
	BaseType_t err;	
	/*如果是一号端口*/
	if(can_id.Can_num == Can_num1){	

		/*发送固件结束帧*/
		rc = gCAN_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,UpdateFinish_P,sizeof(UpdateFinish_P));
		if(rc != 1){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
			return rc;
		}
		rc = CAN_PRE_OK;
	}else if(can_id.Can_num == Can_num2){	/*2号CAN端口*/
		/*发送固件结束帧*/
		rc = gCAN2_SendData(can_id.SendID,CAN_ID_STD,CAN_RTR_DATA,UpdateFinish_P,sizeof(UpdateFinish_P));
		if(rc != 1){
			rc = CAN_PRE_FAIL;
			LOG(LOG_ERROR,"CAN send line end  is wrong.! Please check!\r\n");
			return rc;
		}
		rc = CAN_PRE_OK;	
	}
	return rc;
}




/*CAN数据发送任务*/
void CANSendTask(void *pArg)   
{
	BaseType_t err;
	/*等待开始发送信号量 --一直等待*/
	err = xSemaphoreTake(CANSendStart,portMAX_DELAY);
	/*成功获取到信号量*/
	if(err == pdTRUE)
	{
		
	}
	/*获取信号量失败*/
	else
	{
		LOG(LOG_ERROR,"cannot get the start signal");
	}
}

#define UPLOAD_ERRORCODE_TO_INTERNET 	"{\"type\":\"updatefail\",\"imei\":\"%s\",\"code\":%d}"
/*从网络获取固件包--上报获取包*/
void gUploadErrorCode(uint8_t errorType)
{
	/*非联网状态下不上报*/
	char * str = pvPortMalloc(200);
	memset(str,0,200);
	sprintf(str,UPLOAD_ERRORCODE_TO_INTERNET,gGprs.gimei,errorType);
	LOG(LOG_INFO,"str: %s\r\n",str);
	MessageSend(str,1);
	vPortFree(str);
}




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
