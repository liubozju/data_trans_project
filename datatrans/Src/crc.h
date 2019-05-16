#ifndef __CRC_H
#define __CRC_H

//-------------include---------------//
#include "sys.h"

//!CRC����������������
#define CRC_TYPE                  u16
//!CRC16ת��Ϊ�ַ����� ,���65535,5λ��
#define FLASH_CRC_BYTE_LENGTH     5

/*
Example.
		u8 temp[10] = {0x10,0x11,0x28,0x30,0x33,0x89,0xA4,0xA3};
		u32 crc16result1 = CalCrc(0,temp, 8,0x8005);
		temp[9] = (u8)crc16result1;
		temp[8] = (u8)(crc16result1>>8);
		u16 crc16result2 = CalCrc(0,temp, 10,0x8005);
		
		printf("Result1 = %d\r\n",crc16result1);
		printf("Result2 = %d\r\n\r\n",crc16result2);
*/

CRC_TYPE CalCrc(u16 CRCIn, c8 * buf, u16 len,u16 polyHex);
#endif

