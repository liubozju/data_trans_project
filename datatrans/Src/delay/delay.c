#include "delay.h"
#include "FreeRTOS.h"	
#include "task.h"

static uint8_t  fac_us=0;//us延时倍乘数
static uint16_t fac_ms=0;//ms延时倍乘数

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

//时间大于OS提供的最小时间时会引起任务调度
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

//不会引起任务调度
void delay_xms(u32 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

/*SysTick实现延迟n_ms*/
void delayXms(uint16_t nms)
{	 		  	  
	uint32_t temp;		   
	SysTick->LOAD=(uint32_t)nms*fac_ms;//时间加载(SysTick->LOAD为24bit)
	SysTick->VAL =0x00;           //清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;          //开始倒数  
	do
	{
		temp=SysTick->CTRL;
	}
	while((temp&0x01)&&!(temp&(1<<16)));//等待时间到达   /*countflag为0 enable为1*/
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
	SysTick->VAL =0X00;       //清空计数器	  	    
} 

//延时nus
//nus为要延时的us数.		    								   
void delayUs(uint16_t nus)
{		
	uint32_t temp;	    	 
	SysTick->LOAD=(uint32_t)nus*fac_us; //时间加载	  		 
	SysTick->VAL=0x00;        //清空计数器
	SysTick->CTRL=0x01 ;      //开始倒数 	 
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));//等待时间到达   
	SysTick->CTRL=0x00;       //关闭计数器
	SysTick->VAL =0X00;       //清空计数器	 
}


void delayMs(uint16_t nms)
{	 	 
	uint8_t repeat=nms/540;	//这里用540,是考虑到某些客户可能超频使用,
						//比如超频到248M的时候,delay_xms最大只能延时541ms左右了
	uint16_t remain=nms%540;
	while(repeat)
	{
		delayXms(540);
		repeat--;
	}
	if(remain)delayXms(remain);
	
} 


/**************另一种实现延时的方法/使用硬件定时器的方式产生*******/
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






