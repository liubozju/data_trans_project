#include "delay.h"
#include "FreeRTOS.h"	
#include "task.h"

static uint8_t  fac_us=0;//us��ʱ������
static uint16_t fac_ms=0;//ms��ʱ������

void delayInit(void) 
{
	u32 reload;
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK_DIV8); 
	fac_us=SystemCoreClock/1000000;				 
	reload=100;			 
	reload*=1000000/configTICK_RATE_HZ;			 
											 
	fac_ms=1000/configTICK_RATE_HZ;			    

	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;    
	SysTick->LOAD=reload; 					 
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk;   	
}


void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				    	 
	ticks=nus*fac_us; 					 
	told=SysTick->VAL;        			 
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow; 
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;		 
		}  
	};										    
}  

//ʱ�����OS�ṩ����Сʱ��ʱ�������������
void delay_ms(u32 nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED) 
	{		
		if(nms>=fac_ms)						 
		{ 
   			vTaskDelay(nms/fac_ms);	 	 
		}
		nms%=fac_ms;						 
	}
	delay_us((u32)(nms*1000));			 
}

//���������������
void delay_xms(u32 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

/*SysTickʵ���ӳ�n_ms*/
void delayXms(uint16_t nms)
{	 		  	  
	uint32_t temp;		   
	SysTick->LOAD=(uint32_t)nms*fac_ms;//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL =0x00;           //��ռ�����
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //��ʼ����  
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//�ȴ�ʱ�䵽��   /*countflagΪ0 enableΪ1*/
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	  	    
} 

//��ʱnus
//nusΪҪ��ʱ��us��.		    								   
void delayUs(uint16_t nus)
{		
	uint32_t temp;	    	 
	SysTick->LOAD=(uint32_t)nus*fac_us; //ʱ�����	  		 
	SysTick->VAL=0x00;        //��ռ�����
	SysTick->CTRL=0x01 ;      //��ʼ���� 	 
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL=0x00;       //�رռ�����
	SysTick->VAL =0X00;       //��ռ�����	 
}


void delayMs(uint16_t nms)
{	 	 
	uint8_t repeat=nms/540;	//������540,�ǿ��ǵ�ĳЩ�ͻ����ܳ�Ƶʹ��,
						//���糬Ƶ��248M��ʱ��,delay_xms���ֻ����ʱ541ms������
	uint16_t remain=nms%540;
	while(repeat)
	{
		delayXms(540);
		repeat--;
	}
	if(remain)delayXms(remain);
	
} 


/**************��һ��ʵ����ʱ�ķ���/ʹ��Ӳ����ʱ���ķ�ʽ����*******/
//void hardDelayUs(uint16_t us)
//{
//    uint16_t differ=0xffff-us-5;
//    HAL_TIM_Base_Start(&htim4);
//    __HAL_TIM_SetCounter(&htim4,differ);
//    while(differ<0xffff-5)
//    {
//        differ=__HAL_TIM_GetCounter(&htim4);
//    }
//    HAL_TIM_Base_Stop(&htim4);
//}

//void testDelayMs(uint16_t ms)
//{
//	for(;ms>0;ms--)
//	{
//		hardDelayUs(1000);
//	}
//}






