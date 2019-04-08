#ifndef __DELAY_H__
#define __DELAY_H__
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "sys.h"

void hardDelayUs(uint16_t us);
void testDelayMs(uint16_t ms);
void delayInit(void);
void delayMs(uint16_t nms);
void delayUs(uint16_t nus);
void delayXms(uint16_t nms);
void delay_xms(u32 nms);
void delay_ms(u32 nms);
void delay_us(u32 nus);

#endif



