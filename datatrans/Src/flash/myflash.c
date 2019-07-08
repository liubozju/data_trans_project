#include "myflash.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include <stm32_hal_legacy.h>
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "stdio.h"

uint8_t STMFLASH_BUF[STM_PAGE_SIZE]={0};

//读取指定地址的字(32位数据) 
//faddr:读地址 
//返回值:对应数据.
uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
	return *(uint32_t*)faddr; //读取4byte
}

//读取指定地址的字节(8位数据) 
//faddr:读地址 
//返回值:对应数据.
uint8_t STMFLASH_ReadByte(uint32_t faddr)
{
	return *(uint8_t*)faddr; //读取1byte
}



//读取指定地址的字节(16位数据) 
//faddr:读地址 
//返回值:对应数据.
uint8_t STMFLASH_ReadHalfWord(uint32_t faddr)
{
	return *(uint16_t*)faddr; //读取2byte
}



//flash页数据缓存
void STMFLASH_Pagebuf(uint32_t ReadAddr)
{
	uint16_t i;
	for(i=0;i<1024;i++)															//缓存10K的数据，每一页使用1K足够存储数据了。
	{
		STMFLASH_BUF[i] = STMFLASH_ReadByte(ReadAddr);
		ReadAddr += 1;
	}
}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(32位)数
void STMFLASH_Read(uint32_t ReadAddr,uint8_t *pBuffer,uint32_t NumToRead) 	
{
	uint32_t i;	
	//单个字节读取从后往前读(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadByte(ReadAddr);
		ReadAddr+=1;
	}
}


//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(32位)数
void STMFLASH_Read_HalfWord(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead) 	
{
	uint16_t i;	
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr+i*2);
	}
}

//写入flash，如果对应地址有数据，则进行页擦除.
/*如果对应地址没有数据，那么就直接写入即可。 按照字节进行写入*/
void STMFLASH_Write(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)	
{
	taskENTER_CRITICAL();     /*进入临界区*/
	
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t addrx = WriteAddr;				//写入地址
	uint32_t endaddr = 0;
	int SectorStart;uint32_t SectorStartAddr = 0;
	int SectorEnd;
	
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//非法地址
    
	HAL_FLASH_Unlock();             //解锁	
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	
	SectorStartAddr = GetSectorAddr(WriteAddr);					//根据输入地址  获取扇区起始地址
	endaddr = WriteAddr+NumToWrite;	 //写入的结束地址
	if(addrx < 0X1FFF0000)						//写入地址在主存储区
	{
		while(addrx < endaddr){
			if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF){		//写入地址不为空，有数据，需要擦除
				SectorStart = GetSector(addrx);									//根据输入地址 获取扇区编号		如果用到多页，多页同时擦除
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，页擦除 
				FlashEraseInit.Sector=SectorStart;         //要擦除的扇区号码
				FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
				FlashEraseInit.NbSectors=1;                //一次只擦除一个页
				if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
				{
					printf("erase data failed .\r\n");
					break;																	//flash擦除发生错误了	
				}				
			}else{
				addrx+=4;
			}
		}
	}
	//不管有没有进行扇区的擦除，都直接写入数据
	while(WriteAddr<endaddr){
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,WriteAddr,*pBuffer)!=HAL_OK)//写入整个页的数据
			{
				printf("write data failed .\r\n");
				break;	//写入异常
			}
			pBuffer++;
			WriteAddr += 1;  
	}
	HAL_FLASH_Lock();           //上锁
	taskEXIT_CRITICAL();     /*退出临界区*/
} 

void STMFLASH_Erase(uint32_t EraseAddr)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	HAL_FLASH_Unlock();             //解锁	
	FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，页擦除 
	FlashEraseInit.Sector=EraseAddr;         //要擦除的页基地址
	FlashEraseInit.NbSectors=1;                             //一次只擦除一个页
	if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
	{
		printf("erase page error\r\n");//flash擦除发生错误了	
	}
	HAL_FLASH_Lock();           //上锁
}

//检查flash是否有数据
uint8_t CheckIfInFlash(uint32_t DataAdd,uint8_t * databuf,uint8_t datalen)
{
	uint8_t i;
	STMFLASH_Read(DataAdd,databuf,datalen);
	for(i = 0;i < datalen;i++)
	{
		if(*databuf != 0xFF)
		{
			break;
		}
		databuf++;
	}
	if(i < datalen)
	{
		return 1;//读取flash数据成功
	}
	return 0;//flash未存储该数据
}

/**
  * @brief  Gets the sector of a given address
  * @param  Address: Flash address
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else
  {
    sector = FLASH_SECTOR_11;  
  }
 
  return sector;
}

static uint32_t GetSectorAddr(uint32_t Address)
{
  uint32_t sectoraddr = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sectoraddr = ADDR_FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sectoraddr = ADDR_FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sectoraddr = ADDR_FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sectoraddr = ADDR_FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sectoraddr = ADDR_FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sectoraddr = ADDR_FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sectoraddr = ADDR_FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sectoraddr = ADDR_FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sectoraddr = ADDR_FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sectoraddr = ADDR_FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sectoraddr = ADDR_FLASH_SECTOR_10;  
  }
  else
  {
    sectoraddr = ADDR_FLASH_SECTOR_11;  
  }
 
  return sectoraddr;
}

//无数据检测的写入flash
void STMFLASH_Write_NoCheck(uint32_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)   
{ 			 		 
	uint16_t i;
	for (i = 0; i < NumToWrite; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,WriteAddr,*pBuffer)!=HAL_OK)//写入整个页的数据
		{
			printf("write failed\r\n");
			break;	//写入异常
		}
		pBuffer++;
		WriteAddr += 1; 
	}  
} 

//写入flash，如果对应地址有数据，则进行页擦除
void STMFLASH_Write_WithBuf(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)	
{
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t addrx = WriteAddr;				//写入地址
	uint32_t offaddr = 0;
	uint32_t secremain = 0;
	uint32_t secoff=0;
  uint32_t i =0;	
	int SectorStart;uint32_t SectorStartAddr = 0;
	int SectorEnd;
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//非法地址
    
	HAL_FLASH_Unlock();             //解锁	
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	SectorStartAddr = GetSectorAddr(WriteAddr);			//扇区起始地址，，头1K空间用于写入数据
	offaddr = WriteAddr - SectorStartAddr;					//写入数据在扇区中的偏移
	secoff = offaddr ;														//偏移转换为字
	secremain = STM_PAGE_SIZE- secoff;							//1K剩余的字空间
	if(NumToWrite < secremain)
	{
		secremain = NumToWrite;
	}
	while (1) 
	{	
		STMFLASH_Pagebuf(SectorStartAddr); //缓存需要写入页的数据
		for (i = 0; i < secremain; i++) 
		{
			if (STMFLASH_BUF[secoff + i] != 0XFF)break;   //写入区域有数据
		}	
		if(i < secremain)  
		{
			SectorStart = GetSector(addrx);	
			FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，页擦除 
			FlashEraseInit.Sector=SectorStart;         //要擦除的页基地址
			FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
			FlashEraseInit.NbSectors=1;                             //一次只擦除一个页
			if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
			{
				printf("erase failed!\r\n");
				break;//flash擦除发生错误了	
			}
			for (i = 0; i < secremain; i++) //把写入的数据覆盖到对应位置，保留该页其他位置的数据信息
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			//把该页缓存数据写入flash
			STMFLASH_Write_NoCheck(SectorStartAddr, STMFLASH_BUF, STM_PAGE_SIZE/4);
			memset(STMFLASH_BUF,0,STM_PAGE_SIZE); //清除缓存区数据
		}
		else 
		{
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);//写入位置没有数据时，直接写入
		}
		if (NumToWrite == secremain)break; 
	}
	HAL_FLASH_Lock();           //上锁
} 




//按字节写入flash
void STMFLASH_WriteByte(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)  
{
	uint32_t cnt,i;
	uint32_t data_remain = 0,Writetimes = 0;
	uint32_t writebuf[150]= {0};
	
	
	Writetimes  = NumToWrite /4;    //写入次数
	data_remain = NumToWrite %4;    //不足4byte数据
	for(cnt = 0;cnt < Writetimes;cnt++)//4个8bit数据组合缓存进一个32bit
	{
		writebuf[cnt] = (uint32_t)(pBuffer[4*cnt+3]<<24)+(pBuffer[4*cnt+2]<<16)
		                      +(pBuffer[4*cnt+1]<<8)+(pBuffer[4*cnt]);
	}
	if(data_remain > 0)
	{
		Writetimes++;       //有不足4byte数据，写入次数+1
		for(i = 0;i < data_remain;i++) //不足4byte的数据按倒序写入32位缓存中
		{
			writebuf[cnt] += (uint32_t)(pBuffer[4*cnt+3-i]<<(24-8*i));
		}
	}
	//STMFLASH_Write_WithBuf(WriteAddr,writebuf,Writetimes); //写入flash，最小单位4byte
}



/*  测试使用代码
	static char test1[20]="start\0";
	static char test2[20] = "end\0";
	static char test3[20] = "reset\0";
	printf("**************this is online flash test*********** \r\n");
	printf("wrinting test1 : %s\r\n",test1);
	STMFLASH_WriteByte(testflag1,(uint8_t *)test1,20);
	printf("wrinting test2 : %s\r\n",test2);
	STMFLASH_WriteByte(testflag2,(uint8_t *)test2,20);	
	printf("wrinting test3 : %s\r\n",test3);
	STMFLASH_WriteByte(testflag3,(uint8_t *)test3,20);	
	printf("clear data *******\r\n");
	memset(test1,0,20);memset(test2,0,20);memset(test3,0,20);
	STMFLASH_Read(testflag1,(uint8_t *)test1,20);
	printf("readding test1 : %s\r\n",test1);
	STMFLASH_Read(testflag2,(uint8_t *)test2,20);
	printf("readding test2 : %s\r\n",test2);
	STMFLASH_Read(testflag3,(uint8_t *)test3,20);
	printf("readding test3 : %s\r\n",test3);
	printf("clear test2 and copy.************\r\n");
	memset(test2,0,20);memcpy(test2,"test2\0",5);
	STMFLASH_WriteByte(testflag2,(uint8_t *)test2,20);
	STMFLASH_Read(testflag2,(uint8_t *)test2,20);
	printf("readding test2 : %s\r\n",test2);

*/

