#ifndef __MY_FLASH_H__
#define __MY_FLASH_H__

#include "sys.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

//FLASH��ʼ��ַ
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH����ʼ��ַ
#define STM_PAGE_SIZE  1024*10		        //10K
#define STM_SECTOR_SIZE 1024*128			//128K


/*��־λ��ַ*/

/*���ô洢����ַ*/
#define WRITE_ADDR_START 0X08020000							//����5  ���ڱ������ݣ�128K��С
#define FIRST_TIME_FLAG	 (WRITE_ADDR_START+10)		//��ʼ�������������򣬽�������һ��
#define UPDATE_FLAG			(FIRST_TIME_FLAG+10)		//������ʹ��2K��Ե�����ݵ�Ԫ  �洢���±��  

/**/
#define testflag1		(UPDATE_FLAG+20)
#define testflag2		(testflag1+20)
#define testflag3		(testflag2+20)

/*500K�ռ�Ĵ洢��ַ   ÿҳ128K���洢����Ϣ��ÿҳֻ��128KB*/
#define PACK_ADDR_START 0X08040000							//����6  ���ڱ������ݣ�128K��С










#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */




#define FLAG_ADDR_BASE       0x0803A040  //��־λ��ŵ�


void STMFLASH_Read(uint32_t ReadAddr,uint8_t *pBuffer,uint32_t NumToRead);	
void STMFLASH_Write(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite);//�����ֽ�д������
void STMFLASH_WriteByte(uint32_t WriteAddr,uint8_t *pBuffer,uint32_t NumToWrite);//���ֽ�д��flash
void STMFLASH_Erase(uint32_t EraseAddr);

uint32_t STMFLASH_ReadWord(uint32_t faddr);
uint8_t CheckIfInFlash(uint32_t DataAdd,uint8_t * databuf,uint8_t datalen);

void HxCheckFlash(void);

static uint32_t GetSector(uint32_t Address);								//���������ַ������sector����
static uint32_t GetSectorAddr(uint32_t Address);


#endif



