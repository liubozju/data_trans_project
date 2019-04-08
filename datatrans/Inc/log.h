#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "sys.h"
#include "usart.h"


#define LOG_DEBUG 	"DEBUG"				//调试
#define LOG_TRACE 	"TRACE"				//跟踪
#define LOG_ERROR 	"ERROR"				//错误
#define LOG_INFO 		"INFOR"				//信息
#define LOG_CRIT 		"CRTCL"				//重要

#define LOG(level, format, ...) 	do{	printf("[%s | %s @%s,%d] " format ,level,__func__,__FILE__,__LINE__,##__VA_ARGS__);	}while(0)








#endif




