#include "myflash.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include <stm32_hal_legacy.h>
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

uint32_t STMFLASH_BUF[STM_PAGE_SIZE/4]={0};

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
	for(i=0;i<256*10;i++)															//缓存10K的数据，每一页使用10K足够存储数据了。
	{
		STMFLASH_BUF[i] = STMFLASH_ReadWord(ReadAddr);
		ReadAddr += 4;
	}
}


/*************************一个或者多个扇区内存储大量数据，不会存储少量频繁读取的数据时使用************/
/*清除FLASH标志位*/
static uint16_t MEM_If_Init_FS(void)
{
 
    HAL_FLASH_Unlock(); 
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
		return 0;
}


/*FLASH的擦除操作*/
uint16_t MEM_If_Erase_FS(uint32_t start_Add,uint32_t end_Add)
{
    /* USER CODE BEGIN 3 */
    uint32_t UserStartSector;
    uint32_t SectorError;
    FLASH_EraseInitTypeDef pEraseInit;
 
    /* Unlock the Flash to enable the flash control register access *************/
    MEM_If_Init_FS();
 
    /* Get the sector where start the user flash area */
    UserStartSector = GetSector(start_Add);
 
    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = UserStartSector;
    pEraseInit.NbSectors = GetSector(end_Add)-UserStartSector+1 ;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;
 
    if (HAL_FLASHEx_Erase(&pEraseInit, &SectorError) != HAL_OK)
    {
        /* Error occurred while page erase */
        return (1);
    }
 
    return (0);
    /* USER CODE END 3 */
}


uint16_t MEM_If_Write_FS(uint8_t *src, uint8_t *dest, uint32_t Len)
{
    /* USER CODE BEGIN 3 */
    uint32_t i = 0;
 
    for(i = 0; i < Len; i+=4)
    {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
           be done by byte */
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(dest+i), *(uint32_t*)(src+i)) == HAL_OK)
        {
            /* Check the written value */
            if(*(uint32_t *)(src + i) != *(uint32_t*)(dest+i))
            {
                /* Flash content doesn't match SRAM content */
                return 2;
            }
        }
        else
        {
            /* Error occurred while writing data in Flash memory */
            return 1;
        }
    }
    return (HAL_OK);
    /* USER CODE END 3 */
}

uint8_t MEM_If_Read_FS (uint8_t *src, uint8_t *dest, uint32_t Len)
{
    /* Return a valid address to avoid HardFault */
    /* USER CODE BEGIN 4 */
	    uint32_t i = 0;
    uint8_t *psrc = src;
 
    for(i = 0; i < Len; i++)
    {
        dest[i] = *psrc++;
    }
		
    return HAL_OK;
    /* USER CODE END 4 */
}


//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(32位)数
//调试发现flash写入4个字节是倒序  例：写入0x12 0x34 0x56 0x78  按字节（地址从低到高）读出0x78 0x56 0x34 0x21
void STMFLASH_Read(uint32_t ReadAddr,uint8_t *pBuffer,uint8_t NumToRead) 	
{
	uint8_t i;
	uint32_t DataAddr;
	
	//单个字节读取从后往前读(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		if(i%4 == 0)
		{
			DataAddr = ReadAddr+i+4;//每4个字节把读取地址基地址增加i，读取地址先加4
		}
		pBuffer[i]=STMFLASH_ReadByte(--DataAddr);
	}
}


//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToRead:字(32位)数
//调试发现flash写入4个字节是倒序  例：写入0x12 0x34 0x56 0x78  按字节（地址从低到高）读出0x78 0x56 0x34 0x21
void STMFLASH_Read_HalfWord(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead) 	
{
	uint16_t i;	
	//单个字节读取从后往前读(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr+i*2);
	}
}

//无数据检测的写入flash
void STMFLASH_Write_NoCheck(uint32_t WriteAddr, uint32_t *pBuffer, uint16_t NumToWrite)   
{ 			 		 
	uint16_t i;
	for (i = 0; i < NumToWrite; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//写入整个页的数据
		{ 
			break;	//写入异常
		}
		pBuffer++;
		WriteAddr += 4; 
	}  
} 
 
//按字节写入flash    按字节写入数据  最大值可以写入500个字节。大于500个字节的可以直接使用  STMFLASH_Write
void STMFLASH_WriteByte(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)  
{
	uint32_t cnt,i;
	uint32_t data_remain = 0,Writetimes = 0;
	uint32_t writebuf[150]= {0};
	
	Writetimes  = NumToWrite /4;    //写入次数
	data_remain = NumToWrite %4;    //不足4byte数据
	for(cnt = 0;cnt < Writetimes;cnt++)//4个8bit数据组合缓存进一个32bit
	{
		writebuf[cnt] = (uint32_t)(pBuffer[4*cnt]<<24)+(pBuffer[4*cnt+1]<<16)
		                      +(pBuffer[4*cnt+2]<<8)+(pBuffer[4*cnt+3]);
	}
	if(data_remain > 0)
	{
		Writetimes++;       //有不足4byte数据，写入次数+1
		for(i = 0;i < data_remain;i++) //不足4byte的数据按倒序写入32位缓存中
		{
			writebuf[cnt] += (uint32_t)(pBuffer[4*cnt+i]<<(24-8*i));
		}
	}
	__disable_irq();
	STMFLASH_Write(WriteAddr,writebuf,Writetimes); //写入flash，最小单位4byte
	__enable_irq();
}

//写入flash，如果对应地址有数据，则进行页擦除
void STMFLASH_Write(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t NumToWrite)	
{
	taskENTER_CRITICAL();     /*进入临界区*/
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t secoff=0;	   
	uint16_t secremain=0;  	   
	uint16_t i=0;    
	uint32_t offaddr=0; 
	int SectorStart;uint32_t SectorStartAddr = 0;
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//非法地址
    
	HAL_FLASH_Unlock();             //解锁	
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	SectorStart = GetSector(WriteAddr);									//根据输入地址 获取扇区编号
	SectorStartAddr = GetSectorAddr(WriteAddr);					//根据输入地址  获取扇区起始地址
	
	offaddr = WriteAddr - SectorStartAddr;		 //与扇区起始地址的距离 
	secoff = (offaddr ) / 4;		 								//该页已写入的数据长度
	secremain = 1024*10 - secoff;								//以10K为大小，剩余的空间大小
	if (NumToWrite < secremain)									//获取将要写入的数据长度
	{
		secremain = NumToWrite; 									//以10K为大小，写入数据少于10K
	}
	STMFLASH_Pagebuf(SectorStartAddr); 				//缓存需要写入页的数据  10K的大小
	while (1) 
	{	
		for (i = 0; i < secremain; i++) 
		{
			if (STMFLASH_BUF[secoff + i] != 0XFFFFFFFF)break;   //写入区域有数据时，需要擦除
		}
		if(i < secremain)																			//有数据需要擦除
		{
			FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，页擦除 
			FlashEraseInit.Sector=SectorStart;         //要擦除的扇区号码
			FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
			FlashEraseInit.NbSectors=1;                //一次只擦除一个页
			if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
			{
				printf("erase data failed .\r\n");
				break;																	//flash擦除发生错误了	
			}
			for (i = 0; i < secremain; i++) 					//把写入的数据覆盖到对应位置，保留该页其他位置的数据信息
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			//把该页缓存数据写入flash
			STMFLASH_Write_NoCheck(SectorStartAddr, STMFLASH_BUF, STM_PAGE_SIZE/4);
			memset(STMFLASH_BUF,0,STM_PAGE_SIZE/4); //清除缓存区数据			
		}
		else 																				//没有数据，只需要直接写入即可
		{
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);
		}
		break;
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

