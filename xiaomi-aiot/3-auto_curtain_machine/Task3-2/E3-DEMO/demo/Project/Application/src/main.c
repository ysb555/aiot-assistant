/*************************************************************************************************************
功能:     上电后将窗帘放在初始位置
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
#include "e3.h"
#include "s1.h"
#include "s2.h"

volatile uint16_t delay_count = 0;               //延时变量
volatile uint16_t time_count = 0;                //1s定时变量
volatile uint8_t  second_flag = 0;				//时间标志
i2c_addr_def  e3_curtain_addr;			//E3 i2c_addr_def结构体



uint8_t   print_buffer[100];
//i2c_addr_def结构体变量
i2c_addr_def s2_th_addr;		//温湿度
i2c_addr_def s2_ss_addr;		//光照
i2c_addr_def s2_ax_addr;		//六轴
//获取传感器的值
s2_para s2_value;				//温湿度
float ss_value;					//光照
ax_para ax_value;				//六轴




uint8_t   print_buffer[100];			   //打印数据缓存
i2c_addr_def s1_key_addr;				   //s1地址结构体
uint8_t key_value;						   //获得的按键值
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	gd_eval_com_init(EVAL_COM0); 

	timer3_init(); 
	init_i2c();
	s1_key_addr = s1_init(HT16K33_ADDRESS_S1);

	e3_curtain_addr = e3_init(CURTAIN_ADDRESS_E3);
	
	
	
	s2_th_addr = s2_init(TH_ADDRESS_S2);
	s2_ss_addr = s2_init(S1_ADDRESS_S2);
	s2_ax_addr = s2_init(AX_ADDRESS_S2);
	//if(e3_curtain_addr.flag)
	//{	
		//delay_ms(5000);
		//e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,60);

		//delay_ms(3000); 
		//e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,0);
	//}
	


	delay_ms(0);
	while(1)
	{		
		if(second_flag) 	
		{
			second_flag = 0;
			
			if(s1_key_addr.flag)
			{	 
				key_value = s1_key_scan(s1_key_addr.periph,s1_key_addr.addr);
				if(e3_curtain_addr.flag && key_value == SW1)
				{	
					//delay_ms(5000);
					e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,0);
				}
				else if (e3_curtain_addr.flag && key_value == SW2)
				{
					e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,100);
				}
				if(key_value != SWN)
				{	 
					sprintf((char *)print_buffer,(const char*)"key value = %d\r\n",key_value); 
					debug_printf(EVAL_COM0,(char*)print_buffer);
				}	
			}
			
			
			
			
			
			  if(s2_ss_addr.flag)  // ???????????
{	 
    // ?????????
    float light_value = s2_read_bh1750_value(s2_ss_addr.periph, s2_ss_addr.addr);
    
    // ?????(???)
    sprintf((char *)print_buffer, (const char*)"light = %.2f lux\r\n", light_value); 
    debug_printf(EVAL_COM0, (char*)print_buffer);

    // ????????????
    if(e3_curtain_addr.flag)  // ??????????
    {
        if(light_value > 500.0)  // ?????(??????)
        {
            e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, 0);  // 0%??
            debug_printf(EVAL_COM0, "????,??????\r\n");
        }
        else if(light_value < 100.0)  // ?????(??????)
        {
            e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, 100);  // 100%??
            debug_printf(EVAL_COM0, "????,??????\r\n");
        }
        else  // ????(????)
        {
            // ????????????(100-500lux???100-0%)
            uint8_t position = (uint8_t)(100 - (light_value - 100) * 100 / 400);
            e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, position);
            sprintf((char *)print_buffer, "????????:%d%%\r\n", position);
            debug_printf(EVAL_COM0, (char*)print_buffer);
        }
    }
}
			
			
			
			
			
			
			
			
			if(e3_curtain_addr.flag == 0)
			{	 
				sprintf((char *)print_buffer,(const char*)"找不到E3子板\r\n"); 
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
			
			   if(time_count++ >= 1000)
         {
					    time_count = 0;
					    second_flag = 1;
         }					 
		}
}







