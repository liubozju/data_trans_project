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
	for(i=0;i<1024;i++)															//����10K�����ݣ�ÿһҳʹ��1K�㹻�洢�����ˡ�
	{
		STMFLASH_BUF[i] = STMFLASH_ReadByte(ReadAddr);
		ReadAddr += 1;
	}
}

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(32λ)��
void STMFLASH_Read(uint32_t ReadAddr,uint8_t *pBuffer,uint32_t NumToRead) 	
{
	uint32_t i;	
	//�����ֽڶ�ȡ�Ӻ���ǰ��(ReadAddr+3 -> ReadAddr+2 -> ReadAddr+1 -> ReadAddr)
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadByte(ReadAddr);
		ReadAddr+=1;
	}
}


//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(32λ)��
void STMFLASH_Read_HalfWord(uint32_t ReadAddr,uint16_t *pBuffer,uint16_t NumToRead) 	
{
	uint16_t i;	
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr+i*2);
	}
}

//д��flash�������Ӧ��ַ�����ݣ������ҳ����.
/*�����Ӧ��ַû�����ݣ���ô��ֱ��д�뼴�ɡ� �����ֽڽ���д��*/
void STMFLASH_Write(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)	
{
	taskENTER_CRITICAL();     /*�����ٽ���*/
	
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t addrx = WriteAddr;				//д���ַ
	uint32_t endaddr = 0;
	int SectorStart;uint32_t SectorStartAddr = 0;
	int SectorEnd;
	
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//�Ƿ���ַ
    
	HAL_FLASH_Unlock();             //����	
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	
	SectorStartAddr = GetSectorAddr(WriteAddr);					//���������ַ  ��ȡ������ʼ��ַ
	endaddr = WriteAddr+NumToWrite;	 //д��Ľ�����ַ
	if(addrx < 0X1FFF0000)						//д���ַ�����洢��
	{
		while(addrx < endaddr){
			if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF){		//д���ַ��Ϊ�գ������ݣ���Ҫ����
				SectorStart = GetSector(addrx);									//���������ַ ��ȡ�������		����õ���ҳ����ҳͬʱ����
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ�ҳ���� 
				FlashEraseInit.Sector=SectorStart;         //Ҫ��������������
				FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
				FlashEraseInit.NbSectors=1;                //һ��ֻ����һ��ҳ
				if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
				{
					printf("erase data failed .\r\n");
					break;																	//flash��������������	
				}				
			}else{
				addrx+=4;
			}
		}
	}
	//������û�н��������Ĳ�������ֱ��д������
	while(WriteAddr<endaddr){
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,WriteAddr,*pBuffer)!=HAL_OK)//д������ҳ������
			{
				printf("write data failed .\r\n");
				break;	//д���쳣
			}
			pBuffer++;
			WriteAddr += 1;  
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

//�����ݼ���д��flash
void STMFLASH_Write_NoCheck(uint32_t WriteAddr, uint8_t *pBuffer, uint16_t NumToWrite)   
{ 			 		 
	uint16_t i;
	for (i = 0; i < NumToWrite; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,WriteAddr,*pBuffer)!=HAL_OK)//д������ҳ������
		{
			printf("write failed\r\n");
			break;	//д���쳣
		}
		pBuffer++;
		WriteAddr += 1; 
	}  
} 

//д��flash�������Ӧ��ַ�����ݣ������ҳ����
void STMFLASH_Write_WithBuf(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)	
{
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t PageError=0;
	uint32_t addrx = WriteAddr;				//д���ַ
	uint32_t offaddr = 0;
	uint32_t secremain = 0;
	uint32_t secoff=0;
  uint32_t i =0;	
	int SectorStart;uint32_t SectorStartAddr = 0;
	int SectorEnd;
	if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return;	//�Ƿ���ַ
    
	HAL_FLASH_Unlock();             //����	
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                          FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

	SectorStartAddr = GetSectorAddr(WriteAddr);			//������ʼ��ַ����ͷ1K�ռ�����д������
	offaddr = WriteAddr - SectorStartAddr;					//д�������������е�ƫ��
	secoff = offaddr ;														//ƫ��ת��Ϊ��
	secremain = STM_PAGE_SIZE- secoff;							//1Kʣ����ֿռ�
	if(NumToWrite < secremain)
	{
		secremain = NumToWrite;
	}
	while (1) 
	{	
		STMFLASH_Pagebuf(SectorStartAddr); //������Ҫд��ҳ������
		for (i = 0; i < secremain; i++) 
		{
			if (STMFLASH_BUF[secoff + i] != 0XFF)break;   //д������������
		}	
		if(i < secremain)  
		{
			SectorStart = GetSector(addrx);	
			FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ�ҳ���� 
			FlashEraseInit.Sector=SectorStart;         //Ҫ������ҳ����ַ
			FlashEraseInit.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
			FlashEraseInit.NbSectors=1;                             //һ��ֻ����һ��ҳ
			if(HAL_FLASHEx_Erase(&FlashEraseInit,&PageError)!=HAL_OK) 
			{
				printf("erase failed!\r\n");
				break;//flash��������������	
			}
			for (i = 0; i < secremain; i++) //��д������ݸ��ǵ���Ӧλ�ã�������ҳ����λ�õ�������Ϣ
			{
				STMFLASH_BUF[i + secoff] = pBuffer[i];	  
			}
			//�Ѹ�ҳ��������д��flash
			STMFLASH_Write_NoCheck(SectorStartAddr, STMFLASH_BUF, STM_PAGE_SIZE/4);
			memset(STMFLASH_BUF,0,STM_PAGE_SIZE); //�������������
		}
		else 
		{
			STMFLASH_Write_NoCheck(WriteAddr, pBuffer, secremain);//д��λ��û������ʱ��ֱ��д��
		}
		if (NumToWrite == secremain)break; 
	}
	HAL_FLASH_Lock();           //����
} 




//���ֽ�д��flash
void STMFLASH_WriteByte(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite)  
{
	uint32_t cnt,i;
	uint32_t data_remain = 0,Writetimes = 0;
	uint32_t writebuf[150]= {0};
	
	
	Writetimes  = NumToWrite /4;    //д�����
	data_remain = NumToWrite %4;    //����4byte����
	for(cnt = 0;cnt < Writetimes;cnt++)//4��8bit������ϻ����һ��32bit
	{
		writebuf[cnt] = (uint32_t)(pBuffer[4*cnt+3]<<24)+(pBuffer[4*cnt+2]<<16)
		                      +(pBuffer[4*cnt+1]<<8)+(pBuffer[4*cnt]);
	}
	if(data_remain > 0)
	{
		Writetimes++;       //�в���4byte���ݣ�д�����+1
		for(i = 0;i < data_remain;i++) //����4byte�����ݰ�����д��32λ������
		{
			writebuf[cnt] += (uint32_t)(pBuffer[4*cnt+3-i]<<(24-8*i));
		}
	}
	//STMFLASH_Write_WithBuf(WriteAddr,writebuf,Writetimes); //д��flash����С��λ4byte
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

