/*************************************************************************************************************

*************************************************************************************************************/

#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "main.h"
#include "i2c.h"
#include "s1.h"
#include "s2.h"
#include "e1.h"
#include "e2.h"
#include "stdio.h"
#include "string.h"

// counter
uint16_t delay_count = 0; //
uint16_t half_count = 0;
uint16_t time_count = 0;

// flags
uint8_t half_second_flag = 0;
uint8_t run_flag = 0; //

// states
uint8_t speed_state = 0; //
enum
{
	NORMAL = 0,
	DEFI,
	THRESHOLD1,
	THRESHOLD2,
	CHECK,
} input_state;
uint8_t temp = NORMAL;

// addresses
i2c_addr_def s1_key_addr;		 // s1
i2c_addr_def s2_th_addr;		 //
i2c_addr_def s2_ss_addr;		 //
i2c_addr_def e1_nixie_tube_addr; //
i2c_addr_def e1_rgb_led_addr;	 //
i2c_addr_def e2_fan_addr;		 // e2

// values
s2_para s2_value;  // temperature and humidity
float ss_value;	   // sunshine
uint8_t key_value; // key value
uint8_t valid = 0;

uint8_t values = 0;
uint8_t threshold1 = 20;
uint8_t threshold2 = 38;

uint8_t print_buffer[100]; // debug
uint8_t dp[4] = {0, 0, 0, 0};
uint8_t dig[4] = {0, 0, 0, 0};

// declarations
void display_temperature(void);
void display_humidity(void);
void display_sunshine(void);
void get_dp(float num);
void get_first_four_digits(float number);
void task1(void);
void task1pro(void);
/**
 * @brief
 */
int main(void)
{
	//
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	//
	gd_eval_com_init(EVAL_COM0);
	//
	timer3_init();
	//
	init_i2c();

	// s1
	s1_key_addr = s1_init(HT16K33_ADDRESS_S1);
	//
	s2_th_addr = s2_init(TH_ADDRESS_S2);
	s2_ss_addr = s2_init(S1_ADDRESS_S2);
	//
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	// e2
	e2_fan_addr = e2_init(PCA9685_ADDRESS_E2);

	while (1)
	{
		//
		if (half_second_flag)
		{
			delay_ms(100);
			half_second_flag = 0;
			// get values
			task1pro();
		} // half_second
	} // while
}

/**
 * @brief
 */
void display_temperature(void)
{
	float value = s2_value.temperature;
	get_dp(value);
	get_first_four_digits(value);
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

/**
 * @brief
 */
void display_humidity(void)
{
	float value = s2_value.humidity;
	get_dp(value);
	get_first_four_digits(value);
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

/**
 * @brief
 */
void display_sunshine(void)
{
	float value = ss_value;
	get_dp(value);
	get_first_four_digits(value);
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

void delay_ms(uint16_t mstime)
{
	delay_count = mstime;

	while (delay_count)
	{
	}
}


/**
 * @brief
 */
void TIMER3_IRQHandler(void)
{
	if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
		if (delay_count > 0)
			delay_count--;

		if (half_count++ >= 500) // show frequency
		{
			half_count = 0;
			half_second_flag = 1;
		}
	}
}

// utilizer
void get_dp(float num)
{
	memset(dp, 0, 4 * sizeof(uint8_t));
	if (num > 0 && num < 10.0)
		dp[0] = 1;
	else if (num >= 10.0 && num < 100.0)
		dp[1] = 1;
	else if (num >= 100.0 && num < 1000.0)
		dp[2] = 1;
	else
		dp[3] = 1;
}

void get_first_four_digits(float number)
{

	for (int k = 0; k < 4; k++)
	{
		dig[k] = 0;
	}

	int ip = (int)number;
	float fp = number - ip;

	int temp = ip;
	int i = 0;
	while (temp > 0)
	{
		temp /= 10;
		i++;
	}

	int j = i;
	while (j > 0)
	{
		dig[j - 1] = ip % 10;
		ip /= 10;
		j--;
	}

	while (i < 4)
	{
		fp *= 10;
		dig[i] = (int)fp;
		fp -= (int)fp;
		i++;
	}
	if (fp >= 0.5)
	{
		dig[3]++;
	}
}

/**
 * @brief
 */
void timer3_init(void)
{
	timer_parameter_struct timer_init_struct;
	rcu_periph_clock_enable(RCU_TIMER3);
	timer_deinit(TIMER3);
	timer_init_struct.prescaler = 4199; //
	timer_init_struct.period = 20;		//
	timer_init_struct.alignedmode = TIMER_COUNTER_EDGE;
	timer_init_struct.counterdirection = TIMER_COUNTER_UP;
	timer_init_struct.clockdivision = TIMER_CKDIV_DIV1;
	timer_init_struct.repetitioncounter = 0;
	timer_init(TIMER3, &timer_init_struct);
	nvic_irq_enable(TIMER3_IRQn, 1, 1);
	timer_interrupt_enable(TIMER3, TIMER_INT_UP);
	timer_enable(TIMER3);
}

/**
 * @brief
 */
uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	uint8_t i;
	if (usart_periph != USART0 && usart_periph != USART2)
		return -1;
	if (data == NULL)
		return -1;
	if (len <= 0)
		return -1;
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

/**
 * @brief
 */
void debug_printf(uint32_t usart_periph, char *string)
{
	uint8_t buffer[100];
	uint16_t len = strlen(string);
	strncpy((char *)buffer, string, len);
	uart_print(usart_periph, buffer, len);
}

void task1(void)
{
	if (s2_th_addr.flag && s2_ss_addr.flag)
	{
		//
		s2_value = s2_read_sht3x(s2_th_addr.periph, s2_th_addr.addr);
		//
		ss_value = s2_read_bh1750_value(s2_ss_addr.periph, s2_ss_addr.addr);

		if (s2_value.temperature > 28.0)
		{
			speed_state = 2;
		}
		else if (s2_value.temperature > 20.0)
		{
			speed_state = 1;
		}
		else
		{
			speed_state = 0;
		}
	}
	else
	{
		sprintf((char *)print_buffer, "sensor not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	}
	if (s1_key_addr.flag)
	{
		key_value = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
		if (key_value == SW0)
		{
			run_flag = !run_flag;
		}
	}

	// change fan speed
	if (e2_fan_addr.flag)
	{
		switch (speed_state)
		{
		case 0:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 0);
			break;
		case 1:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 50 * run_flag);
			break;
		case 2:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 90 * run_flag);
			break;
		}
	}
	else
	{
		sprintf((char *)print_buffer, "E2 not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	} // e2

	// display
	if (e1_nixie_tube_addr.flag)
	{

		display_temperature();
		e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 0);
	}
	else
	{
		sprintf((char *)print_buffer, "E1 not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	} // e1
}

void task1pro(void)
{

	if (s2_th_addr.flag && s2_ss_addr.flag)
	{
		//
		s2_value = s2_read_sht3x(s2_th_addr.periph, s2_th_addr.addr);
		//
		ss_value = s2_read_bh1750_value(s2_ss_addr.periph, s2_ss_addr.addr);

		if (s2_value.temperature > threshold2)
		{
			speed_state = 2;
		}
		else if (s2_value.temperature > threshold1)
		{
			speed_state = 1;
		}
		else
		{
			speed_state = 0;
		}
	}
	else
	{
		sprintf((char *)print_buffer, "sensor not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	}
	if (s1_key_addr.flag)
	{
		
		key_value = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
		switch (input_state)
		{
		case NORMAL:
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 30, 40, 0);
			if (key_value == SW0)
				run_flag = !run_flag;
			else if (key_value == SWA)
			{
				input_state = DEFI;
				memset(dig, 0, 4 * sizeof(uint8_t));
				memset(dp, 0, 4 * sizeof(uint8_t));
			}
			else if (key_value == SWC)
			{
				input_state = CHECK;
				memset(dig, 0, 4 * sizeof(uint8_t));
				memset(dp, 0, 4 * sizeof(uint8_t));
			}
			break;
		case DEFI:
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 0);
			e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, temp-1, NODIS, NODIS, NODIS, dp);
			if (key_value == SW1)
				temp = THRESHOLD1;
			else if (key_value == SW2)
				temp = THRESHOLD2;
			else if (key_value == SWC) // confirm the state
			{
				input_state = temp;
				memset(dig, 0, 4 * sizeof(uint8_t));
				values = 0;
				valid = 0;
				temp = NORMAL;
			}
			else if (key_value == SWA)
			{
				temp = NORMAL;
				input_state = NORMAL;
			}
			break;
		case THRESHOLD1:
			if (key_value != SWN)
			{
				if (key_value == SWA)
				{
					if (valid)
					{
						valid -= 1;
					}
					values /= 10;
					dig[valid] = NODIS;
				}
				else if (key_value == SWC)
				{
					e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
					if (values <= 0)
						values = threshold1;
					else if(values >= threshold2)
						values = threshold2;
					threshold1 = values;
					temp = 0;
					input_state = NORMAL;
				}
				else
				{
					if (valid < 2)
					{
						dig[valid] = key_value;
						values = values * 10 + key_value;
						valid += 1;
					}
					e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 60);
				}
			}
			e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], NODIS, NODIS, dp);
			break;
			break;
		case THRESHOLD2:
			if (key_value != SWN)
			{
				if (key_value == SWA)
				{
					if (valid)
					{
						valid -= 1;
					}
					values /= 10;
					dig[valid] = NODIS;
				}
				else if (key_value == SWC)
				{
					e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
					if (values <= threshold1)
						values = threshold2;
					threshold2 = values;
					temp = 0;
					input_state = NORMAL;
				}
				else
				{
					if (valid < 2)
					{
						dig[valid] = key_value;
						values = values * 10 + key_value;
						valid += 1;
					}
					e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 60);
				}
			}
			e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], NODIS, NODIS, dp);
			break;
		case CHECK:
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 0, 60);

			if (key_value == SW1)
			{
				get_dp(threshold1);
				get_first_four_digits(threshold1);
			}
			else if (key_value == SW2)
			{
				get_dp(threshold2);
				get_first_four_digits(threshold2);
			}
			else if (key_value == SWC)
			{
				input_state = NORMAL;
			}
			e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
			break;

		default:
			break;
		}
	}

	// change fan speed
	if (e2_fan_addr.flag)
	{
		switch (speed_state)
		{
		case 0:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 0);
			break;
		case 1:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 50 * run_flag);
			break;
		case 2:
			e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 90 * run_flag);
			break;
		}
	}
	else
	{
		sprintf((char *)print_buffer, "E2 not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	} // e2

	// display
	if (e1_nixie_tube_addr.flag && input_state == NORMAL)
	{

		display_temperature();
		// e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 30, 30, 30);
	}
	else
	{
		sprintf((char *)print_buffer, "E1 not found\r\n");
		debug_printf(EVAL_COM0, (char *)print_buffer);
	} // e1
}