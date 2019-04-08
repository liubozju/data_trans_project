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
#define TCP_REV "<"


int gDeviceRegister(void);






#endif 




