/*************************************************************************************************************
����:     �ϵ��ʼ����ʼ¼��,��ɺ�ʼ���ţ�ѭ������(¼��ʱ��Լ10s)
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
#include "e4.h"
#include "e1.h"
#include "s1.h"

volatile uint16_t delay_count = 0; // ��ʱ����
uint16_t time_count = 0;  // 1s��ʱ����

unsigned short playdata[PLAYCNT]; // �������ݻ���

uint16_t data_cnt = 0;
uint16_t sec_count = 0;
unsigned long playcnt = 0;	// ��Ƶ�ļ�����
unsigned char playflag = 0; // ������־

uint8_t sec_flag = 1;
uint8_t radio_state = 0;
uint8_t terminal_flag = 0;
uint8_t record_signal = 0;
uint8_t record_flag = 0;
i2c_addr_def e1_nixie_tube_addr; //???i2c_addr_def
i2c_addr_def e1_rgb_led_addr;	 // LED?i2c_addr_def
i2c_addr_def s1_key_addr;			 // S1?i2c_addr_def

uint8_t key_value;
uint16_t s = 10;
uint8_t minutes, seconds;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handle_state(void);
int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	gd_eval_com_init(EVAL_COM0);
	timer3_init();
	init_i2c();
	e4_init();
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);

	dis_led_init();
	
	e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
	s1_key_addr = s1_init(HT16K33_ADDRESS_S1);
	uint16_t cnt = 0;

	// record

	display_number(s);
	while (1)
	{
		if (s1_key_addr.flag)
		{
			key_value = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
			if (key_value == SW1)
			{
				// 录音
				if(radio_state == 0 || radio_state == 5){
					s = 10;
					display_number(s);
					radio_state = 1;
					memset(playdata, 0, sizeof(playdata));
				}
				
			}
			else if (key_value == SW2)
			{
				// 播放
				//  先停止录音
				if(radio_state == 2){
					terminal_flag = 1;
					radio_state = 3;
				}
				
			}
			else if (key_value == SW3)
			{
				// 终止重启
				radio_state = 5;
			}
		}
		handle_state();

		
		if (record_signal)
		{
			record_signal = 0;
			e4_playback_config();
			playflag = 1;
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 60);
			LED_OFF;
		}
		
		if (sec_flag && radio_state > 0 && radio_state < 3)
		{
			sec_flag = 0;
			if (e1_nixie_tube_addr.flag && s > 0)
			{
				display_number(--s);
			}
		}
	}
}

void handle_state()
{
	switch (radio_state)
	{
	case 1:
		// 启动录音
		radio_state = 2; // 录音中
		e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 0, 60); // 蓝色录音中
		record_signal = 0;
		playcnt = 0;
		e4_sound_recording_config();
		nvic_irq_enable(SPI1_IRQn, 0, 0);
		LED_ON;
		break;
	case 2:
		// 录音中
		if(record_flag){
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0); // 绿色录音完成
		}
		break;
	case 3:
		// 播放
		record_flag = 0;
		e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 60); // 白色播放中
		record_signal = 1;
		radio_state = 4; // 播放中
		break;
	case 4:
		// 播放中
		if(playcnt == 0){
			radio_state = 0; // 回到0
		}
		break;
	case 5: // 重启
		spi_i2s_interrupt_disable(I2S1_ADD, SPI_I2S_INT_RBNE);
		record_signal = 0;
		playflag = 0;
		playcnt = 0;
		s = 10;
		display_number(s);
		radio_state = 0;
		break;
	default:
		break;
	}
}

void display_number(uint16_t secs)
{
	minutes = secs / 60;
	seconds = secs % 60;
	// FIXME:add a point behind the minutes
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, minutes / 10, minutes % 10, seconds / 10, seconds % 10);
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
//��ʼ��led
*********************************************************************************************************/
void dis_led_init(void)
{
	/* enable the led clock */
	rcu_periph_clock_enable(RCU_GPIOF);
	/* configure led GPIO port */
	gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
	gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

	GPIO_BC(GPIOF) = GPIO_PIN_4;
}

/********************************************************************************************************
//1ms��ʱ�жϷ������
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
		}
		if (sec_count++ >= 1000)
		{
			sec_count = 0;
			sec_flag = 1;
		}
	}
}

/********************************************************************************************************
//spi2 i2s�ж�
*********************************************************************************************************/
void SPI1_IRQHandler(void)
{
	if (SET == spi_i2s_interrupt_flag_get(SPI1, SPI_I2S_INT_TBE))
	{
		if (playflag)
		{
			spi_i2s_data_transmit(SPI1, playdata[playcnt++]); // ����
			if (playcnt >= data_cnt)
			{
				playflag = 0;
				playcnt = 0;
				radio_state = 0;
			}
		}
		else
		{
			spi_i2s_data_transmit(SPI1, 0x0000);
		}
	}

	if (SET == spi_i2s_interrupt_flag_get(I2S1_ADD, SPI_I2S_INT_RBNE))
	{
		playdata[playcnt++] = spi_i2s_data_receive(I2S1_ADD); // ¼��
		if (playcnt >= PLAYCNT || s <= 0 || terminal_flag)  // 缓冲区满 或 时间到 或 手动停止
		{
			spi_i2s_interrupt_disable(I2S1_ADD, SPI_I2S_INT_RBNE);
			data_cnt = playcnt;
			record_flag = 1;
			playcnt = 0;
		}
	}
}
