#include  "myflag.h"
#include  "string.h"
unsigned char firstTimeFlag[3]={0};			//全局变量，用于测试是否是初次上电
unsigned char updateFlag[20]={0};			//全局变量，用于测试是否是初次上电


void platformFlagInit(void )
{
	memset(firstTimeFlag,0,3);
	STMFLASH_Read(FIRST_TIME_FLAG,(uint8_t *)firstTimeFlag,3);
	if(firstTimeFlag[0] == 0x55 && firstTimeFlag[1] == 0xAA)		
	{
		printf("system start.\r\n");
		return;				//相等，说明不是初次上电，直接返回
	}
	else
	{
		printf("system start first time.\r\n");
		firstTimeFlag[0] = 0x55;
		firstTimeFlag[1] = 0xAA;
		firstTimeFlag[2] = '\0';
		STMFLASH_Write(FIRST_TIME_FLAG,(uint8_t *)firstTimeFlag,3);
		memcpy(updateFlag,"goAPP\0",strlen("goAPP\0"));
		STMFLASH_Write(UPDATE_FLAG,(uint8_t *)updateFlag,20);
	}
}









