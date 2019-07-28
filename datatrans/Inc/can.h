/**
  ******************************************************************************
  * File Name          : CAN.h
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __can_H
#define __can_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"

/* USER CODE BEGIN Includes */
typedef struct
{
	uint8_t Data[8];
}CAN_RecvMsg;

typedef struct{
	uint16_t RecID;
	uint16_t SendID;
	uint8_t Can_num;
}CAN_ID;

typedef enum{
	CAN_PRE_OK = 0,
	CAN_PRE_FAIL,
}CAN_PRE;

enum{
	Can_Success = 0x00,
	Can_SendFail,
	Can_Not_Respond,
	Can_Rev_Wrong,
	Can_Connect_Failed,
	Can_File_Wrong,
};

typedef enum{
	CAN_SEND_ERR = 100,
	CAN_NO_REV_ERR,
	CAN_WRONG_REV_ERR,
	CAN_DECRYPT_ERR,
	CAN_FILE_ERR,
}CanErrorCode;

typedef enum{
	Can_num1 = 1,
	Can_num2,
}CAN_NUM_IO;

extern CAN_ID can_id;

extern CAN_RecvMsg can_recvmsg;	
/* USER CODE END Includes */
/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

extern void _Error_Handler(char *, int);

void MX_CAN1_Init(uint32_t RecID);
void MX_CAN2_Init(uint32_t RecID);

/* USER CODE BEGIN Prototypes */
void CAN_TRANSMIT1(void);
int gCAN_SendData(uint32_t ID,uint8_t id_type,uint8_t data_type,const unsigned char * data,const uint16_t datalen);
int gCAN2_SendData(uint32_t ID,uint8_t id_type,uint8_t data_type,const unsigned char * data,const uint16_t datalen);

/* USER CODE END Prototypes */


extern uint8_t CanSendEndPack(void);
extern void gUploadErrorCode(uint8_t errorType);
extern uint8_t CanPre(void);
extern uint8_t CanSendLinePack(uint8_t * sLinepack,uint8_t len);

#ifdef __cplusplus
}
#endif
#endif /*__ can_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
