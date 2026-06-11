/*************************************************************************************************************
功能:     每3秒改变一次电机转速,改变3次后停止
版本:     2023-5-16 V1.0
修改记录: 无
作者:     ZZZ
编码格式：GB2312
*************************************************************************************************************/

#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "stdio.h"
#include "string.h"
#include "main.h"
#include "i2c.h"
#include "e2.h"
#include "s5.h"

//时间计数
uint16_t delay_count = 0;
uint16_t time_count = 0;

uint8_t  second_flag = 0;		//时间标志

i2c_addr_def e2_fan_addr;		//E2风扇i2c_addr_def
i2c_addr_def s5_addr;	

uint8_t   print_buffer[100];

uint8_t wd_buffer[20];					   //NFC卡数据缓存
uint8_t DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};		//默认密码
uint8_t ucArray_ID[6];	

uint8_t back_stat = 0;
uint8_t stat = 0;

uint8_t fan_flag = 0;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	gd_eval_com_init(EVAL_COM0); 

	timer3_init(); 
	init_i2c();

	e2_fan_addr = e2_init(PCA9685_ADDRESS_E2);
	s5_addr = s5_init(MS523_ADDRESS_S5);

	while(1)
	{		
		if(second_flag)
		{
			second_flag = 0; 
			if(s5_addr.flag)
			{	 
				if(s5_detect(s5_addr.periph,s5_addr.addr,ucArray_ID))
				{
					if(s5_verify(s5_addr.periph,s5_addr.addr,PICC_AUTHENT1A,5,DefaultKey,ucArray_ID) == 0)
					{
						stat = 1;
						if(stat != back_stat)
						{	
							memset(wd_buffer,0,20);
							strncpy((char*)wd_buffer,"123456789ABCDE\r\n",16);
							if(s5_write_data(s5_addr.periph,s5_addr.addr,6,wd_buffer) == 0)
							{	
								memset(wd_buffer,0,20);
								if(s5_read_data(s5_addr.periph,s5_addr.addr,6,wd_buffer) == 0)
								{
									debug_printf(EVAL_COM0,(char *)wd_buffer);
								}
							}		

							fan_flag ^= 1;
						}									
					}    									
				} 
				else
				{
					stat = 0;
				}	
				back_stat = stat;	
				s5_sleep(s5_addr.periph,s5_addr.addr);		
			}


			/*****************/ 
			if(e2_fan_addr.flag)
			{
				if(fan_flag)
				{
					e2_speed_control(e2_fan_addr.periph,e2_fan_addr.addr,50);
				}
				else
				{
					e2_speed_control(e2_fan_addr.periph,e2_fan_addr.addr,0);
				}
			}
			else
			{
				sprintf((char *)print_buffer,(const char*)"找不到E2子板\r\n"); 
				debug_printf(EVAL_COM0,(char*)print_buffer); 
			}					
			
			
		}					 
	}

}


	


/***********************************************************************************************************
//timer3 init 1ms定时
************************************************************************************************************/
void timer3_init(void)
{
		timer_parameter_struct timer_init_struct;
		
		rcu_periph_clock_enable(RCU_TIMER3);
		
		timer_deinit(TIMER3);
		timer_init_struct.prescaler			= 4199;	
		timer_init_struct.period			= 20;
		timer_init_struct.alignedmode		= TIMER_COUNTER_EDGE;
		timer_init_struct.counterdirection	= TIMER_COUNTER_UP;		
		timer_init_struct.clockdivision		= TIMER_CKDIV_DIV1;		
		timer_init_struct.repetitioncounter = 0;				
		timer_init(TIMER3, &timer_init_struct);
		
		nvic_irq_enable(TIMER3_IRQn, 1, 1); 
		timer_interrupt_enable(TIMER3, TIMER_INT_UP);
		timer_enable(TIMER3);
}




/********************************************************************************************************
延时ms函数
*********************************************************************************************************/
void delay_ms(uint16_t mstime)
{
     delay_count = mstime;
     while(delay_count)
		 {
		 }	  
}


/********************************************************************************************************
串口发送数据
*********************************************************************************************************/
uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	   uint8_t i;
		 for(i = 0; i < len; i++) 
	   {
			   while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);      
			   usart_data_transmit(usart_periph, data[i]);
		 }
		 while(usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
		 return len;
}

/********************************************************************************************************
调试串口发送数据
*********************************************************************************************************/
void debug_printf(uint32_t usart_periph,char *string)
{
     uint8_t  buffer[100];
	   uint16_t len;
	   
	   len = strlen(string);
	   strncpy((char*)buffer,string,len);
	   uart_print(usart_periph,buffer,len);
}

/********************************************************************************************************
1ms定时中断服务程序
*********************************************************************************************************/
void TIMER3_IRQHandler(void)
{
	if(timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

		if(delay_count > 0)
			delay_count--;	 

		if(time_count++ >= 300)
		{
			time_count = 0;
			second_flag = 1;
		}					 
	}
}







