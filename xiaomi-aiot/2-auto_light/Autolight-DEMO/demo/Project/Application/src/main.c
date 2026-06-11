/*************************************************************************************************************
功能:     每秒读取一次人体传感器的值,判断是否存在人体感应,并通过串口输出结果
版本:     2023-5-16 V1.0
修改记录: 无
作者:     ZZZ
编码格式：GB2312
*************************************************************************************************************/

#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "main.h"
#include "i2c.h"
#include "stdio.h"
#include "string.h"
#include "s7.h"
#include "e1.h"


uint16_t  delay_count = 0;                 //延时变量

uint16_t  time_count = 0;                  //1s定时变量 increase to decrease the time delay
uint16_t	half_time_count = 0;

uint16_t  alert_delay = 2;						// alert_delay means the loop count

uint8_t 	half_second_flag = 1;
uint8_t   second_flag = 1;				   //时间标志
uint8_t		human_flag = 0;

uint8_t   print_buffer[100];
i2c_addr_def s7_addr;					   //S7地址结构体
i2c_addr_def e1_nixie_tube_addr;		//Digit		i2c_addr_def
i2c_addr_def e1_rgb_led_addr;			//LEDlight  i2c_addr_def
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	gd_eval_com_init(EVAL_COM0); 

	timer3_init(); 
	init_i2c();

	s7_addr = s7_init(PCA9557_ADDRESS_S7);
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
	
	uint16_t cnt = 0;

	while(1)
	{		
		if(second_flag)//默认每300ms执行一次		
		{
			second_flag = 0;
			if(s7_addr.flag)
			{	 

				if(s7_get_status(s7_addr.periph,s7_addr.addr))
				{
					sprintf((char *)print_buffer,(const char*)"human presence detected\r\n");
					human_flag = 1;
					cnt = alert_delay;
					debug_printf(EVAL_COM0,(char*)print_buffer);
					
				}
				else
				{
					sprintf((char *)print_buffer,(const char*)"no body here\r\n");
					human_flag = 0;
					debug_printf(EVAL_COM0,(char*)print_buffer); 
				}	
			}	
			else
			{
				sprintf((char *)print_buffer,(const char*)"找不到S7子板\r\n");
				debug_printf(EVAL_COM0,(char*)print_buffer); 
			}								 
		}
		if(half_second_flag){
			if(e1_rgb_led_addr.flag)
			{	
					
					if(human_flag){
						e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,0,0);
					}	
					else{
						e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,60,60);
					}
					
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




/*********************************************************************************************************
函数名:     uart_print
入口参数:   usart_periph 串口号USART0或USART2, *data 要发送的数据,len 要发送数据的长度
出口参数:   len 发送数据长度
返回值:     成功返回数据长度 输入参数错误返回-1
作者:       yhh
日期:       2023/7/10
调用描述:   串口发送数据,成功返回发送数据长度,输入参数错误返回-1
**********************************************************************************************************/
int uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	uint8_t i;
	// 检查USART外设是否有效
	if (usart_periph != USART0 && usart_periph != USART2)
	{
		return -1;
	}

	// 检查数据指针是否为空
	if (data == NULL)
	{
		return -1;
	}

	// 检查数据长度是否为0
	if (len <= 0)
	{
		return -1;
	}

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
		if(half_time_count++ >= 300){
			half_time_count=0;
			half_second_flag = 1;
		}
	}
}







