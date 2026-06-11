#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "stdio.h"
#include "string.h"
#include "uart.h"
#include "i2c.h"
#include "main.h"
#include "c2.h"
#include "e1.h"
#include "e2.h"
#include "s2.h"
#include "s5.h"

// 定义Zigbee模式
#define TERMINAL 0
#define COORDINATOR 1

// 全局变量
uint8_t zigbee_mode = TERMINAL;
volatile uint8_t c2_result;
uint8_t rec_data[100];
uint8_t len;
uint8_t rec_result;

// 地址
i2c_addr_def s2_th_addr;
i2c_addr_def s2_ss_addr;
i2c_addr_def e1_nixie_tube_addr;
i2c_addr_def e1_rgb_led_addr;
i2c_addr_def e2_fan_addr;
i2c_addr_def s5_addr;

// 值
s2_para s2_value;
float ss_value;

// NFC相关
uint8_t s5_mode;
uint8_t wd_buffer[20];
uint8_t DefaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ucArray_ID[6];

// 温度阈值
uint8_t threshold1 = 20;
uint8_t threshold2 = 38;

// 风扇速度1
uint8_t fan_speeds[2] = {40, 90};

// 显示相关
uint8_t dp[4] = {0, 0, 0, 0};
uint8_t dig[4] = {0, 0, 0, 0};
uint8_t cmd_buffer[17];

// 状态标志
uint8_t back_stat = 0;
uint8_t stat = 0;
uint8_t speed_state = 0;
uint8_t run_flag = 0;
uint8_t dis_flag = 0;
uint8_t writing = 0b01;

// 显示阈值标志
uint8_t show_threshold_flag = 0;

// 命令处理函数声明
uint8_t cmd_turn(char *args);
uint8_t cmd_set(char *args);
uint8_t cmd_display(char *args);
uint8_t cmd_nfc(char *args);
uint8_t cmd_check(char *args);
uint8_t cmd_test(char *args);

// 命令表
static struct
{
	char *name;
	char *description;
	uint8_t (*handler)(char *);
} cmd_table[] = {
	{"turn", "Turn the fan on or off", cmd_turn},
	{"set", "Set the temperature threshold", cmd_set},
	{"dis", "display the temperature or humidity", cmd_display},
	{"check", "Check the temperature thresholds", cmd_check},
	{"test", "Test the System", cmd_test},
	{"nfc", "NFC control", cmd_nfc}};

// 解析命令
uint8_t handler_cmd(char *args)
{
	char *str_end = args + strlen(args);
	char *cmd = strtok(args, " ");

	if (cmd != NULL)
	{
		args = cmd + strlen(cmd) + 1;
		if (args > str_end)
		{
			return 1;
		}
		for (int i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++)
		{
			if (strcmp(cmd, cmd_table[i].name) == 0)
			{
				return cmd_table[i].handler(args);
			}
		}
	}
	return 1;
}

// 处理"turn"命令
uint8_t cmd_turn(char *args)
{
	args = strtok(NULL, " ");
	if (args == NULL)
	{
		return 1;
	}
	if (strcmp(args, "on") == 0)
	{
		run_flag = 1;
		dis_flag = 0b11;
		return 0;
	}
	else if (strcmp(args, "off") == 0)
	{
		run_flag = 0;
		dis_flag = 0;
		return 0;
	}
	else
	{
		return 1;
	}
}

// 处理"set"命令
uint8_t cmd_set(char *args)
{
	args = strtok(NULL, " ");
	if (args == NULL)
	{
		return 1;
	}
	if (strcmp(args, "t1") == 0)
	{
		args = strtok(NULL, " ");
		if (args == NULL)
		{
			return 1;
		}
		uint8_t value = atoi(args);
		if (value <= 0)
		{
			return 1;
		}
		threshold1 = value;
		return 0;
	}
	else if (strcmp(args, "t2") == 0)
	{
		args = strtok(NULL, " ");
		if (args == NULL)
		{
			return 1;
		}
		uint8_t value = atoi(args);
		if (value <= 0)
		{
			return 1;
		}
		threshold2 = value;
		return 0;
	}
	else if (strcmp(args, "developer") == 0)
	{
		s5_mode = 2;
		return 0;
	}
	else if (strcmp(args, "normal") == 0)
	{
		s5_mode = 0;
		return 0;
	}
	else if (strcmp(args, "reader") == 0)
	{
		s5_mode = 1;
		return 0;
	}
	return 1;
}

// 处理"dis"命令
uint8_t cmd_display(char *args)
{
	args = strtok(NULL, " ");
	if (args == NULL)
	{
		return 1;
	}
	if (strcmp(args, "t") == 0)
	{
		dis_flag |= 0b10;
		return 0;
	}
	else if (strcmp(args, "h") == 0)
	{
		dis_flag |= 0b100;
		return 0;
	}
	else if (strcmp(args, "off") == 0)
	{
		dis_flag = 0;
		return 0;
	}
	return 1;
}
int data[6];
// 处理"nfc"命令
uint8_t cmd_nfc(char *args)
{
	if (strncmp(args, "write", 5)==0)
	{
		sscanf(args, "write %d %d %d %d", &data[0], &data[1], &data[2], &data[3]);
		data[5] = 0x11;
	}
	else if (strncmp(args, "priv", 4)==0)
	{
		sscanf(args, "priv %d", &data[4]);
		if(data[4]>=0 && data[4] < 8) data[5] = 0x10;
	}
	if(data[5]){
		writing |= 0b10;
		return 0;
	}
	return 1;
}

// 处理"check"命令
uint8_t cmd_check(char *args)
{
	args = strtok(NULL, " ");
	if (args == NULL)
	{
		return 1;
	}
	show_threshold_flag = 1;
	if (strcmp(args, "t1") == 0)
	{
		memset(dp, 0, 4 * sizeof(uint8_t));
		e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, threshold1 / 10, threshold1 % 10, NODIS, NODIS, dp);
		return 0;
	}
	else if (strcmp(args, "t2") == 0)
	{
		memset(dp, 0, 4 * sizeof(uint8_t));
		e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, threshold2 / 10, threshold2 % 10, NODIS, NODIS, dp);
		return 0;
	}
	else if (strcmp(args, "s1") == 0)
	{
		memset(dp, 0, 4 * sizeof(uint8_t));
		e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, fan_speeds[0] / 10, fan_speeds[0] % 10, NODIS, NODIS, dp);
		return 0;
	}
	else if (strcmp(args, "s2") == 0)
	{
		memset(dp, 0, 4 * sizeof(uint8_t));
		e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, fan_speeds[1] / 10, fan_speeds[1] % 10, NODIS, NODIS, dp);
		return 0;
	}
	return 1;
}

// 处理"test"命令
uint8_t cmd_test(char *args)
{
	args = strtok(NULL, " ");
	if (args != NULL)
	{
		return 1;
	}
	LED_OFF;
	delay_ms(500);
	LED_ON;
	delay_ms(500);
	LED_OFF;
	delay_ms(500);
	LED_ON;
	delay_ms(500);
	return 0;
}

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
	gd_eval_com_init(EVAL_COM0);
	timer3_init();
	uart_init(USART2);
	init_i2c();

	c2_result = c2_init(zigbee_mode);
	s2_th_addr = s2_init(TH_ADDRESS_S2);
	s2_ss_addr = s2_init(S1_ADDRESS_S2);
	s5_addr = s5_init(MS523_ADDRESS_S5);
	e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
	e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
	e2_fan_addr = e2_init(PCA9685_ADDRESS_E2);
	e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);

	while (1)
	{
		if (zigbee_mode == TERMINAL)
		{
			memset(rec_data, 0, 100);
			len = c2_rec_data(rec_data, 50, 500);
			if (len)
			{
				uint8_t result = handler_cmd((char *)rec_data);
				if (result == 0)
				{
					c2_broadcast_data("OK", 0x03);
				}
				else if (result == 1)
				{
					c2_broadcast_data("ERROR", 0x03);
				}
			}
		}
		else
		{
			memset(rec_data, 0, 100);
			len = c2_rec_data(rec_data, 20, 500);
			if (len > 0)
			{
				if (strstr((char *)rec_data, "test") != NULL)
					rec_result = 1;
				else
					rec_result = 0;
			}
		}

		if (show_threshold_flag)
		{
			show_threshold_flag = 0;
			e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 30);
			delay_ms(2000);
		}

		delay_ms(100);
		if (s2_th_addr.flag && s2_ss_addr.flag)
		{
			s2_value = s2_read_sht3x(s2_th_addr.periph, s2_th_addr.addr);
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

		if (e1_nixie_tube_addr.flag)
		{

			if (dis_flag & 0b101)
			{
				display_temperature();
				e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 60);
			}
			else if (dis_flag & 0b101)
			{
				display_humidity();
				e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 40, 0, 60);
			}
			else
			{
				get_dp(0);
				get_first_four_digits(0);
				e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
			}
		}

		if (e2_fan_addr.flag)
		{
			switch (speed_state)
			{
			case 0:
				e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, 0);
				break;
			case 1:
				e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, fan_speeds[0] * run_flag);
				break;
			case 2:
				e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, fan_speeds[1] * run_flag);
				break;
			}
		}

		if (s5_addr.flag)
		{
			if (s5_detect(s5_addr.periph, s5_addr.addr, ucArray_ID))
			{
				if (s5_verify(s5_addr.periph, s5_addr.addr, PICC_AUTHENT1A, 5, DefaultKey, ucArray_ID) == 0)
				{
					stat = 1;
					if (stat != back_stat)
					{
						if (s5_mode == 0)
						{
							card_handler();
						}
						else if (s5_mode == 1)
						{
							
							delay_ms(10);
							memset(wd_buffer, 0, 20);
							memset(rec_data, 0, sizeof(rec_data));
							if (s5_read_data(s5_addr.periph, s5_addr.addr, 6, wd_buffer) == 0)
							{
								// 传输格式化信息
								sprintf((char*)rec_data, "NFC_DATA: priv: 0x%X, speeds: %d %d,thresholds: %d, %d", wd_buffer[0], wd_buffer[1], wd_buffer[2], wd_buffer[3], wd_buffer[4]);
								c2_broadcast_data((char *)rec_data, 0x01);
								delay_ms(2000);
							}
							else
							{
								c2_broadcast_data("ReadError", 0x01);
								delay_ms(200);
							}
						}
						else if (s5_mode == 2 && writing == 0b11)
						{
							writing = 0;
							card_write();
						}
					}
				}
			}
			else
			{
			stat = 0;
			}
		}
		
		back_stat = stat;
	}
}

// 其他函数保持不变
// ...

// normal
// priv|speed1|speed2|th1|th2|
// priv: ...|th|speed|onoff
void card_handler()
{
	memset(wd_buffer, 0, 20);
	if (s5_read_data(s5_addr.periph, s5_addr.addr, 6, wd_buffer) == 0)
	{
		// priv
		if (wd_buffer[0] & 0x1)
		{
			run_flag ^= 1;
		}
		if (wd_buffer[0] & 0x2)
		{
			if (wd_buffer[1] <= fan_speeds[1] && wd_buffer[1] > 0)
			{
				fan_speeds[0] = wd_buffer[1];
			}
			if (wd_buffer[2] <= 100 && wd_buffer[2] > fan_speeds[0])
			{
				fan_speeds[1] = wd_buffer[2];
			}
		}
		if (wd_buffer[0] & 0x4)
		{
			if (wd_buffer[3] <= threshold2 && wd_buffer[3] > 0)
			{
				threshold1 = wd_buffer[3];
			}
			if (wd_buffer[4] <= 100 && wd_buffer[4] > threshold1)
			{
				threshold2 = wd_buffer[4];
			}
		}
	}
}


// developer
// FIXME: use cmd_buffer
void card_write()
{
	memset(wd_buffer, 0, sizeof(wd_buffer));
	if (s5_read_data(s5_addr.periph, s5_addr.addr, 6, wd_buffer) == 0)
	{
		memset(rec_data, 0, sizeof(rec_data));
		sprintf((char*)rec_data, "NFC_DATA: priv: 0x%X, speeds: %d %d,thresholds: %d, %d", wd_buffer[0], wd_buffer[1], wd_buffer[2], wd_buffer[3], wd_buffer[4]);
		c2_broadcast_data((char *)rec_data, 0x01);
		
		delay_ms(200);
	}
	if (data[5] == 0x11)
	{
		for(int i = 0; i < 4; ++i){
			wd_buffer[i+1] = (uint8_t)data[i];
		}
	}
	else if (data[5] == 0x10)
	{
		wd_buffer[0] = (uint8_t)data[4];
	}
	else
	{
		c2_broadcast_data("WriteError", 0x01);
		return;
	}
	data[5] = 0;
	if (s5_write_data(s5_addr.periph, s5_addr.addr, 6, wd_buffer) == 0)
	{
		sprintf((char*)rec_data, "NFC_DATA': priv: 0x%X, speeds: %d %d,thresholds: %d, %d", wd_buffer[0], wd_buffer[1], wd_buffer[2], wd_buffer[3], wd_buffer[4]);
		c2_broadcast_data((char *)rec_data, 0x01);
		memset(rec_data, 0, sizeof(rec_data));
		
	}
	writing |= 0b01;
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

void display_temperature(void)
{
	float value = s2_value.temperature;
	get_dp(value);
	get_first_four_digits(value);
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

void display_humidity(void)
{
	float value = s2_value.humidity;
	get_dp(value);
	get_first_four_digits(value);
	e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}