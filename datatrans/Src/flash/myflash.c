#include "myflash.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include <stm32_hal_legacy.h>
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

uint32_t STMFLASH_BUF[STM_PAGE_SIZE/4]={0};

//��ȡָ����ַ����(32λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
	return *(uint32_t*)faddr; //��ȡ4byte
}

//��ȡָ����ַ���ֽ�(8λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
uint8_t STMFLASH_ReadByte(uint32_t faddr)
{
	return *(uint8_t*)faddr; //��ȡ1byte
}

//��ȡָ����ַ���ֽ�(16λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
uint8_t STMFLASH_ReadHalfWord(uint32_t faddr)
{
	return *(uint16_t*)faddr; //��ȡ2byte
}



//flashҳ���ݻ���
void STMFLASH_Pagebuf(uint32_t ReadAddr)
{
	uint16_t i;
	for(i=0;i<256*10;i++)															//����10K�����ݣ�ÿһҳʹ��10K�㹻�洢�����ˡ�
	{
		STMFLASH_BUF[i] = STMFLASH_ReadWord(ReadAddr);
		ReadAddr += 4;
	}
}


/*************************һ�����߶�������ڴ洢�������ݣ�����洢����Ƶ����ȡ������ʱʹ��************/
/*���FLASH��־λ*/
static uint16_t MEM_If_Init_FS(void)
{
 
    HAL_FLASH_Unlock(); 
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
		return 0;
}


/*FLASH�Ĳ�������*/
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


//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(32λ)��
//���Է���flashд��4���ֽ��ǵ���  ����д��0x12 0x34 0x56 0x78  ���ֽڣ���ַ�ӵ͵��ߣ�����0x78 0x56 0x34 0x21
void STMFLASH_Read(uint32_t ReadAddr,uint8_t *pBuffer,uint8_t NumToRead) 	
{
	uint8_t i;
	uint32_t DataAddr;
	
	//�����ֽڶ�ȡ�Ӻ���ǰ��(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		if(i%4 == 0)
		{
			DataAddr = ReadAddr+i+4;//ÿ4���ֽڰѶ�ȡ��ַ����ַ����i����ȡ��ַ�ȼ�4
		}
		pBuffer[i]=STMFLASH_ReadByte(--DataAddr);
	}
}


//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(32λ)��
//���Է���flashд��4���ֽ��ǵ���  ����д��0x12 0x34 0x56 0x78  ���ֽڣ���ַ�ӵ͵��ߣ�����0x78 0x56 0x34 0x21
void STMFLASH_Read_HalfWord(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead) 	
{
	uint16_t i;	
	//�����ֽڶ�ȡ�Ӻ���ǰ��(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr+i*2);
	}
}

//�����ݼ���д��flash
void STMFLASH_Write_NoCheck(uint32_t WriteAddr, uint32_t *pBuffer, uint16_t NumToWrite)   
{ 			 		 
	uint16_t i;
	for (i = 0; i < NumToWrite; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//д������ҳ������
		{ 
			break;	//д���쳣
		}
		pBuffer++;
		WriteAddr += 4; 
	}  
} 
 
//���ֽ�д��flash    ���ֽ�д������  ���ֵ����д��500���ֽڡ�����500���ֽڵĿ���ֱ��ʹ��  STMFLASH_Write
void STMFLASH_WriteByte(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)  
{
	uint32_t cnt,i;
	uint32_t data_remain = 0,Writetimes = 0;
	uint32_t writebuf[150]= {0};
	
	Writetimes  = NumToWrite /4;    //д�����
	data_remain = NumToWrite %4;    //����4byte����
	for(cnt = 0;cnt < Writetimes;cnt++)//4��8bit������ϻ����һ��32bit
	{
		writebuf[cnt] = (uint32_t)(pBuffer[4*cnt]<<24)+(pBuffer[4*cnt+1]<<16)
		                      +(pBuffer[4*cnt+2]<<8)+(pBuffer[4*cnt+3]);
	}
	if(data_remain > 0)
	{
		Writetimes++;       //�в���4byte���ݣ�д�����+1
		for(i = 0;i < data_remain;i++) //����4byte�����ݰ�����д��32λ������
		{
			writebuf[cnt] += (uint32_t)(pBuffer[4*cnt+i]<<(24-8*i));
		}
	}
	__disable_irq();
	STMFLASH_Write(WriteAddr,writebuf,Writetimes); //д��flash����С��λ4byte
	__enable_irq();
}

//д��flash�������Ӧ��ַ�����ݣ������ҳ����
void STMFLASH_Write(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t NumToWrite)	
{
	taskENTER_CRITICAL();     /*�����ٽ���*/
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t secoff=0;	   
	uint16_t secremain=0;  	   
	uint16_t i=0;    
	uint32_t offaddr=0; 
	int SectorStart;uint32_t SectorStartAddr = 0;
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//�Ƿ���ַ
    
	HAL_FLASH_Unlock();             //����	
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	SectorStart = GetSector(WriteAddr);									//���������ַ ��ȡ�������
	SectorStartAddr = GetSectorAddr(WriteAddr);					//���������ַ  ��ȡ������ʼ��ַ
	
	offaddr = WriteAddr - SectorStartAddr;		 //��������ʼ��ַ�ľ��� 
	secoff = (offaddr ) / 4;		 								//��ҳ��д������ݳ���
	secremain = 1024*10 - secoff;								//��10KΪ��С��ʣ��Ŀռ��С
	if (NumToWrite < secremain)									//��ȡ��Ҫд������ݳ���
	{
		secremain = NumToWrite; 									//��10KΪ��С��д����������10K
	}
	STMFLASH_Pagebuf(SectorStartAddr); 				//������Ҫд��ҳ������  10K�Ĵ�С
	while (1) 
	{	
		for (i = 0; i < secremain; i++) 
		{
			if (STMFLASH_BUF[secoff + i] != 0XFFFFFFFF)break;   //д������������ʱ����Ҫ����
		}
		if(i < secremain)																			//��������Ҫ����
		{
			FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ�ҳ���� 
			FlashEraseInit.Sector=SectorStart;         //Ҫ��������������
			FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
			FlashEraseInit.NbSectors=1;                //һ��ֻ����һ��ҳ
			if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
			{
				printf("erase data failed .\r\n");
				break;																	//flash��������������	
			}
			for (i = 0; i < secremain; i++) 					//��д������ݸ��ǵ���Ӧλ�ã�������ҳ����λ�õ�������Ϣ
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			//�Ѹ�ҳ��������д��flash
			STMFLASH_Write_NoCheck(SectorStartAddr, STMFLASH_BUF, STM_PAGE_SIZE/4);
			memset(STMFLASH_BUF,0,STM_PAGE_SIZE/4); //�������������			
		}
		else 																				//û�����ݣ�ֻ��Ҫֱ��д�뼴��
		{
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);
		}
		break;
	}
	HAL_FLASH_Lock();           //����
	taskEXIT_CRITICAL();     /*�˳��ٽ���*/
} 

void STMFLASH_Erase(uint32_t EraseAddr)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	HAL_FLASH_Unlock();             //����	
	FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ�ҳ���� 
	FlashEraseInit.Sector=EraseAddr;         //Ҫ������ҳ����ַ
	FlashEraseInit.NbSectors=1;                             //һ��ֻ����һ��ҳ
	if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
	{
		printf("erase page error\r\n");//flash��������������	
	}
	HAL_FLASH_Lock();           //����
}

//���flash�Ƿ�������
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
		return 1;//��ȡflash���ݳɹ�
	}
	return 0;//flashδ�洢������
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


/*  ����ʹ�ô���
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

