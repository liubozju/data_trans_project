#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "sys.h"
#include "usart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"



/*���ݶ���*/
typedef struct MESSAGE{
	const char *payload;		/*����ָ��*/
	uint16_t payloadLen;		/*���ݳ���*/
	
	SemaphoreHandle_t OKBinarySemaphore;	/*��ֵ�ź������������OK��ģ�鷵��*/
	void (*send)(u8 head);								/*���ͺ���*/
}Msg;

typedef enum{
	GETFIRM_STOP = 0,
	GETFIRM_START,
}GETFIRM;


extern GETFIRM getfirm;

extern int MessageReceiveFromISR(char *msg);
void MessageReceiveTask(void *pArg);
void MessageSendTask(void *pArg);
extern int  MessageSendFromISR(char *msg,u8 head);
extern void deviceSend(u8 head);
extern void MsgInfoConfig(void);
extern int  MessageSend(const char *msg,u8 head);
extern void deviceDirectSend(char * str);








#endif 


