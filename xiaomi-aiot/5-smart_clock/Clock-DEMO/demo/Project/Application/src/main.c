/*************************************************************************************************************
����:     ÿ���ȡһ�����崫������ֵ,�ж��Ƿ���������Ӧ,��ͨ������������
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
#include "s7.h"
#include "e1.h"
#include "s6.h"
#include "e4.h"

unsigned short playdata[PLAYCNT];
unsigned long playcnt = 0; //????l?????
unsigned char playflag = 0;

volatile uint16_t delay_count = 0; // ��ʱ����

uint16_t time_count = 0; // 1s��ʱ���� increase to decrease the time delay
uint16_t sec_count = 0;

uint16_t alert_delay = 2; // alert_delay means the loop count

uint8_t sec_flag = 1;
uint8_t second_flag = 1; // ʱ���־
uint8_t human_flag = 0;

uint8_t start_flag = 0;
uint8_t once_record = 1;
uint8_t record_flag = 0;

uint8_t print_buffer[100];
i2c_addr_def s7_addr;			 // S7��ַ�ṹ��
i2c_addr_def e1_nixie_tube_addr; // Digit		i2c_addr_def
i2c_addr_def e1_rgb_led_addr;	 // LEDlight  i2c_addr_def
i2c_addr_def s6_addr;			 // S6???????
//??????l????
uint8_t s6_data[2];
uint16_t s6_distance;

uint8_t minutes, seconds;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);

	gd_eval_com_init(EVAL_COM0);
	timer3_init();
		
	init_i2c();
	dis_led_init();
	e4_init();
	s6_addr = s6_init(GD32F330_ADDRESS_S6);
	s7_addr = s7_init(PCA9557_ADDRESS_S7);
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);

	uint16_t cnt = 0;

	// record
	uint16_t s = 10;
	display_number(s);
	while (1)
	{
		if (second_flag) // Ĭ��ÿ300msִ��һ��
		{
			second_flag = 0;
			if(!start_flag && once_record) // ??
			{
				start_flag = 1;
				playcnt = 0;
				e4_sound_recording_config();
				nvic_irq_enable(SPI2_IRQn,0,0);                //?????,????�??
				LED_ON;						
			}
			if(!once_record)
			{
				LED_OFF;
			}
			if (s == 0 && !human_flag)
			{
				if (record_flag)
				{
					record_flag = 0;
					e4_playback_config();
					playflag = 1;
					human_flag = 1;
				}
				else if(playflag == 0){
					record_flag = 1;
				}
			}
			if (s6_addr.flag)
			{
				if (s6_read_distance(s6_addr.periph, s6_addr.addr, s6_data))
				{
					s6_distance = (s6_data[0] << 8) + s6_data[1];
					// FIXME: so it is cm or mm
					sprintf((char *)print_buffer, (const char *)"s6 distance = %d\r\n", s6_distance);
					debug_printf(EVAL_COM0, (char *)print_buffer);
					if (s6_distance < 0x100 && once_record == 0)
					{
						human_flag = 1;
					}
				
				}
			}
		}
		if (sec_flag)
		{
			sec_flag = 0;
			if (e1_nixie_tube_addr.flag && s > 0)
			{
				display_number(--s);
			}
		}
	}
}

void display_number(uint16_t secs)
{
	minutes = secs / 60;
	seconds = secs % 60;
	// FIXME:add a point behind the minutes
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, minutes / 10, minutes % 10, seconds / 10, seconds % 10);
}

void delay_ms(uint16_t mstime)
{
	delay_count = mstime;
	while (delay_count)
	{
	}
}

void dis_led_init(void)
{
	/* enable the led clock */
	rcu_periph_clock_enable(RCU_GPIOF);
	/* configure led GPIO port */
	gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
	gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

	GPIO_BC(GPIOF) = GPIO_PIN_4;
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

/*********************************************************************************************************
������:     uart_print
��ڲ���:   usart_periph ���ں�USART0��USART2, *data Ҫ���͵�����,len Ҫ�������ݵĳ���
���ڲ���:   len �������ݳ���
����ֵ:     �ɹ��������ݳ��� ����������󷵻�-1
����:       yhh
����:       2023/7/10
��������:   ���ڷ�������,�ɹ����ط������ݳ���,����������󷵻�-1
**********************************************************************************************************/
int uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	uint8_t i;
	// ���USART�����Ƿ���Ч
	if (usart_periph != USART0 && usart_periph != USART2)
	{
		return -1;
	}

	// �������ָ���Ƿ�Ϊ��
	if (data == NULL)
	{
		return -1;
	}

	// ������ݳ����Ƿ�Ϊ0
	if (len <= 0)
	{
		return -1;
	}

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

		if (time_count++ >= 300)
		{
			time_count = 0;
			second_flag = 1;
		}
		if (sec_count++ >= 1000)
		{
			sec_count = 0;
			sec_flag = 1;
		}
	}
}

void SPI2_IRQHandler(void)
{
	if (SET == spi_i2s_interrupt_flag_get(SPI2, SPI_I2S_INT_TBE))
	{
		if (playflag)
		{
			spi_i2s_data_transmit(SPI2, playdata[playcnt++]); //????
			if (playcnt >= PLAYCNT)
			{
				playflag = 0;
				playcnt = 0;
			}
		}
		else
		{
			spi_i2s_data_transmit(SPI2, 0x0000);
		}
	}

	if (SET == spi_i2s_interrupt_flag_get(I2S2_ADD, SPI_I2S_INT_RBNE))
	{
		playdata[playcnt++] = spi_i2s_data_receive(I2S2_ADD); // �??
		if (playcnt >= PLAYCNT)
		{
			spi_i2s_interrupt_disable(I2S2_ADD, SPI_I2S_INT_RBNE);
			record_flag = 1;
			once_record = 0;
			playcnt = 0;
		}
	}
}
