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
#include "s5.h"
#include "e4.h"
#include "e1.h"

volatile uint16_t delay_count = 0;               //延时变量
volatile uint16_t time_count = 0;                //1s定时变量
volatile uint8_t  second_flag = 0;				//时间标志
i2c_addr_def  e3_curtain_addr;			//E3 i2c_addr_def结构体
	
uint8_t   print_buffer[100];			   //打印数据缓存
i2c_addr_def s1_key_addr;				   //s1地址结构体
uint8_t key_value;						   //获得的按键值

i2c_addr_def s5_addr;					   //S5地址结构体i2c_addr_def

uint8_t wd_buffer[20];					   //NFC卡数据缓存
uint8_t DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};		//默认密码
uint8_t ucArray_ID[6];

//识别标志
uint8_t back_stat = 0;
uint8_t stat = 0;

unsigned short playdata[PLAYCNT];		   //语音数据缓存
unsigned long playcnt = 0;				   //音频文件索引
unsigned char playflag = 0;				   //放音标志

i2c_addr_def e1_nixie_tube_addr;		//数码管i2c_addr_def
i2c_addr_def e1_rgb_led_addr;			//LED灯i2c_addr_def

uint16_t keywords[4] = {1,2,3,4};
uint16_t input_keywords[4]={0};
uint16_t last_key = SWN;
uint8_t input_len = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	gd_eval_com_init(EVAL_COM0); 

	timer3_init(); 
	init_i2c();
	
	e4_init();
	s1_key_addr = s1_init(HT16K33_ADDRESS_S1);
	e3_curtain_addr = e3_init(CURTAIN_ADDRESS_E3);
	s5_addr = s5_init(MS523_ADDRESS_S5);
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
	
	

	delay_ms(0);
	while(1)
	{		
		if(second_flag) 	
		{
			second_flag = 0;
			
			//if(e1_nixie_tube_addr.flag)
			//{
			//	e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,0,0,0,0);
			//	e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);	
			//}
			// s1 //
			if(s1_key_addr.flag) //button
			{	 
				key_value = s1_key_scan(s1_key_addr.periph,s1_key_addr.addr);
				
				if(e3_curtain_addr.flag && key_value == SWC)
				{	
					e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,100);
					e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,1,1,1,1);
					e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);	
					e4_playback_config();
					
					delay_ms(7000); 
					e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,0);			
					e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,0,0,0,0);
			    e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
				}

				//keyword
				if(e3_curtain_addr.flag && ((key_value >= SW1 && key_value <= SW9) || key_value == SW0)&& input_len <= 4) //input
				{
					if(key_value != last_key && input_len < 4)
					{
						input_keywords[input_len]=key_value;
						input_keywords[0]=input_keywords[0]==0?NODIS:input_keywords[0];
						input_keywords[1]=input_keywords[1]==0?NODIS:input_keywords[1];
						input_keywords[2]=input_keywords[2]==0?NODIS:input_keywords[2];
						input_keywords[3]=input_keywords[3]==0?NODIS:input_keywords[3];
						e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,input_keywords[0],input_keywords[1],input_keywords[2],input_keywords[3]);
						
						input_len++;
						
						if(input_len==4)
						{
							if(memcmp(input_keywords, keywords, 4) == 0)
							{
								e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,100);
								e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,2,2,2,2);
								e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);	
								e4_playback_config();
					
								delay_ms(7000); 
								e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,0);			
								e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,0,0,0,0);
								e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
							}
							else //error
							{
								e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,8,8,8,8);
								e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,60,60);	
							
								delay_ms(3000);	
								e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,0,0,0,0);
								e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
							}
							memset(input_keywords, 0, sizeof(input_keywords));
							input_len=0;
						} 
					}
					last_key = key_value;
				}
				else if(key_value == SWN) 
				{
					last_key = SWN;
				}
				
				
				if(e3_curtain_addr.flag && key_value == SWA) //reset
				{
					if(key_value != last_key && input_len < 4)
					{
						input_keywords[input_len]=key_value;
						keywords[0]=input_keywords[0]==0?NODIS:input_keywords[0];
						keywords[1]=input_keywords[1]==0?NODIS:input_keywords[1];
						keywords[2]=input_keywords[2]==0?NODIS:input_keywords[2];
						keywords[3]=input_keywords[3]==0?NODIS:input_keywords[3];
						e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,keywords[0],keywords[1],keywords[2],keywords[3]);
						
						input_len++;
						
						if(input_len==4)
						{
							memset(input_keywords, 0, sizeof(input_keywords));
							input_len=0;
						}
					}
					last_key = key_value;
				}
				
				if(key_value != SWN)
				{	 
					sprintf((char *)print_buffer,(const char*)"key value = %d\r\n",key_value); 
					debug_printf(EVAL_COM0,(char*)print_buffer);
				}	
			}
			
			if(s5_addr.flag) //NFC
			{	 
				if(s5_detect(s5_addr.periph,s5_addr.addr,ucArray_ID))
				{
					if(s5_verify(s5_addr.periph,s5_addr.addr,PICC_AUTHENT1A,5,DefaultKey,ucArray_ID) == 0)
					{
						stat = 1;
						if(stat != back_stat)
						{	
							if(e3_curtain_addr.flag) {
								e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,100);
								e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,3,3,3,3);
								e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);	
								e4_playback_config();
							}
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
						}
						delay_ms(7000); 
						e3_set_position(e3_curtain_addr.periph,e3_curtain_addr.addr,0);
						e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,0,0,0,0);
			      e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
					}    									
				} 
				else
				{
					stat = 0;
				}	
				back_stat = stat;	
				s5_sleep(s5_addr.periph,s5_addr.addr);		
			}
			else
			{
				sprintf((char *)print_buffer,(const char*)"找不到S5子板\r\n"); 
				debug_printf(EVAL_COM0,(char*)print_buffer);
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







