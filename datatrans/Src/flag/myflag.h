#ifndef __MY_FLAG_H__
#define  __MY_FLAG_H__

#include "sys.h"
#include "myflash.h"
#include "usart.h"

/*�ⲿ��������*/ 





void platformFlagInit(void );
extern void gWriteDiffStartFlag(uint8_t sFlag);
extern void gWriteDiffEndFlag(uint8_t sFlag);
extern void gClearDiffFlag(void);

#endif 



