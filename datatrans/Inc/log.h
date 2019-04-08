#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "sys.h"
#include "usart.h"


#define LOG_DEBUG 	"DEBUG"				//����
#define LOG_TRACE 	"TRACE"				//����
#define LOG_ERROR 	"ERROR"				//����
#define LOG_INFO 		"INFOR"				//��Ϣ
#define LOG_CRIT 		"CRTCL"				//��Ҫ

#define LOG(level, format, ...) 	do{	printf("[%s | %s @%s,%d] " format ,level,__func__,__FILE__,__LINE__,##__VA_ARGS__);	}while(0)








#endif




