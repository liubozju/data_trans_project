#ifndef __REGISTER_H__
#define __REGISTER_H__

#include "sys.h"
#include "usart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include <string.h>
#include "gprs.h"
#include "timers.h"

#define REGISTER_REP  "regrepok"
#define PBREADY "PBREADY"
#define PACKSTART "packstart"
#define PACKEND "packend"

#define CONFIG "config"
#define CAN_NUM "cannum"
#define RECID "RecID"
#define SENDID "SendID"
#define TCP_REV "<"
#define LEFT_JSON	"{"
#define RIGHT_JSON	"}"


int gDeviceRegister(void);






#endif 




