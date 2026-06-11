/*************************************************************************************************************
����:     ѭ����ⰴ���Ƿ���,����⵽��������,��ͨ�����ڰѼ�ֵ���ͳ�ȥ
�汾:     2023-5-16 V1.0
�޸ļ�¼: ��
����:     ZZZ
�����ʽ��GB2312
*************************************************************************************************************/

#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "main.h"
#include "i2c.h"
#include "stdio.h"
#include "string.h"
#include "s1.h"
#include "e1.h"

uint16_t delay_count = 0; // ��ʱ����
uint16_t time_count = 0;  // 1s��ʱ����

uint8_t second_flag = 0;   // ʱ���־
uint8_t print_buffer[100]; // ��ӡ���ݻ���
i2c_addr_def s1_key_addr;  // s1��ַ�ṹ��
uint8_t key_value;		   // ��õİ���ֵ

uint8_t dis_number = 0;

i2c_addr_def e1_nixie_tube_addr; //???i2c_addr_def
i2c_addr_def e1_rgb_led_addr;	 // LED?i2c_addr_def
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	gd_eval_com_init(EVAL_COM0);

	timer3_init();
	init_i2c();
	// s1
	s1_key_addr = s1_init(HT16K33_ADDRESS_S1);
	// e1
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);

	uint8_t numbers[4] = {NODIS, NODIS, NODIS, NODIS};
	uint8_t valid = 0;

	uint8_t i;
	while (1)
	{
		if (second_flag)
		{
			second_flag = 0;
			if (s1_key_addr.flag)
			{
				key_value = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
				if (key_value != SWN)
				{
					sprintf((char *)print_buffer, (const char *)"key value = %d\r\n", key_value);
					debug_printf(EVAL_COM0, (char *)print_buffer);
					dis_number = key_value;
					if (dis_number == SWA)
					{
						if (valid)
						{
							valid -= 1;
						}

						e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 60);
						numbers[valid] = NODIS;
					}
					else if (dis_number == SWC)
					{

						e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
					}
					else
					{
						if (valid < 4)
						{
							numbers[valid] = key_value;
							valid += 1;
						}
						e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 0);
					}
					e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, numbers[0], numbers[1], numbers[2], numbers[3]);
					// for(i=0; i<4; ++i){
					// e1_digital_display(e1_nixie_tube_addr.periph,e1_nixie_tube_addr.addr,numbers[0],numbers[1],numbers[2],numbers[3]);
					// e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);
					//}
				}
			}
			else
			{
				sprintf((char *)print_buffer, (const char *)"�Ҳ���S1�Ӱ�\r\n");
				debug_printf(EVAL_COM0, (char *)print_buffer);
			}
		}
	}
}

/***********************************************************************************************************
//timer3 init 1ms��ʱ
************************************************************************************************************/
void timer3_init(void)
{
	timer_parameter_struct timer_init_struct;

	rcu_periph_clock_enable(RCU_TIMER3);

	timer_deinit(TIMER3);
	timer_init_struct.prescaler = 4199;
	timer_init_struct.period = 20;
	timer_init_struct.alignedmode = TIMER_COUNTER_EDGE;
	timer_init_struct.counterdirection = TIMER_COUNTER_UP;
	timer_init_struct.clockdivision = TIMER_CKDIV_DIV1;
	timer_init_struct.repetitioncounter = 0;
	timer_init(TIMER3, &timer_init_struct);

	nvic_irq_enable(TIMER3_IRQn, 1, 1);
	timer_interrupt_enable(TIMER3, TIMER_INT_UP);
	timer_enable(TIMER3);
}

/********************************************************************************************************
��ʱms����
*********************************************************************************************************/
void delay_ms(uint16_t mstime)
{
	delay_count = mstime;
	while (delay_count)
	{
	}
}

/********************************************************************************************************
���ڷ�������
*********************************************************************************************************/
uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	uint8_t i;
	for (i = 0; i < len; i++)
	{
		while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET)
			;
		usart_data_transmit(usart_periph, data[i]);
	}
	while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET)
		;
	return len;
}

/********************************************************************************************************
���Դ��ڷ�������
*********************************************************************************************************/
void debug_printf(uint32_t usart_periph, char *string)
{
	uint8_t buffer[100];
	uint16_t len;

	len = strlen(string);
	strncpy((char *)buffer, string, len);
	uart_print(usart_periph, buffer, len);
}
/********************************************************************************************************
1ms��ʱ�жϷ������
*********************************************************************************************************/
void TIMER3_IRQHandler(void)
{
	if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

		if (delay_count > 0)
			delay_count--;

		if (time_count++ >= 700)
		{
			time_count = 0;
			second_flag = 1;
		}
	}
}
