#ifndef __CONVERSION_H
#define __CONVERSION_H

#include "sys.h"
#include "usart.h"
#include <string.h>


typedef enum{
	PackOK = 1,
	PackFail,
}pack_status;


void StrToHex(char* str,uint8_t * hex,int strlen);
void HexToStr(uint8_t* hex,char * str,int hexlen);
void DecToStr(uint32_t dec,char * str);
void FloatToStr(float f,char * str);
char* FindStr(char* buf,uint16_t buflen,char* aim);
uint16_t StrToDec(char * str,uint8_t len);
uint16_t StrToDec(char * str,uint8_t len);

uint8_t gPackCheck(char * str,uint16_t strlen);

#endif
