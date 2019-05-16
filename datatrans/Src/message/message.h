#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "sys.h"
#include "usart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"



/*数据定义*/
typedef struct MESSAGE{
	const char *payload;		/*内容指针*/
	uint16_t payloadLen;		/*数据长度*/
	
	SemaphoreHandle_t OKBinarySemaphore;	/*二值信号量，用于针对OK的模组返回*/
	void (*send)(u8 head);								/*发送函数*/
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


