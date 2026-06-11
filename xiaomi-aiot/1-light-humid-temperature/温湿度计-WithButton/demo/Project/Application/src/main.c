/*************************************************************************************************************

*************************************************************************************************************/

#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "main.h"
#include "i2c.h"
#include "s2.h"
#include "e1.h"
#include "s1.h"
#include "stdio.h"
#include "string.h"

//
uint16_t re_count = 0;           //
uint8_t re_flag = 0;
uint16_t data_count = 0;        //     
uint8_t display_state = 4;      // 
uint8_t system_state = 0;          // 
uint8_t second_flag = 0;	

s2_para s2_value;               // 
float ss_value;                 // 
i2c_addr_def s2_th_addr;        // 
i2c_addr_def s2_ss_addr;        // 
i2c_addr_def e1_nixie_tube_addr;// 
i2c_addr_def e1_rgb_led_addr;			//
i2c_addr_def s1_key_addr;				   //init s1

uint8_t dis_number;
uint8_t key_value;						   //difine key_value(see s1.h)
uint8_t print_buffer[100];      // 
uint8_t dp[4] = {0, 0, 0, 0};
uint8_t dig[4] = {0, 0, 0, 0};
uint8_t tmp[4] = {0, 0, 0, 0};
// 
void display_temperature(void);
void display_humidity(void);
void display_sunshine(void);
void get_dp(float num);
void get_first_four_digits(float number);
void get_display_state(void);

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
    // 
    s2_th_addr = s2_init(TH_ADDRESS_S2);
    s2_ss_addr = s2_init(S1_ADDRESS_S2);
    // 
    e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
	  e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
    //
    s1_key_addr = s1_init(HT16K33_ADDRESS_S1);

    while(1)
	{		
			if(re_flag == 1)
					{
            if (s2_th_addr.flag && s2_ss_addr.flag)
            {
                // 
                s2_value = s2_read_sht3x(s2_th_addr.periph, s2_th_addr.addr);
                // 
                ss_value = s2_read_bh1750_value(s2_ss_addr.periph, s2_ss_addr.addr);
                // 
                sprintf((char *)print_buffer, "temperature = %f\r\n", s2_value.temperature);
                debug_printf(EVAL_COM0, (char *)print_buffer);
                sprintf((char *)print_buffer, "humidity = %f\r\n", s2_value.humidity);
                debug_printf(EVAL_COM0, (char *)print_buffer);
                sprintf((char *)print_buffer, "sunshine = %f\r\n", ss_value);
                debug_printf(EVAL_COM0, (char *)print_buffer);
            }
            else
            {
                sprintf((char *)print_buffer, "sensor not found\r\n");
                debug_printf(EVAL_COM0, (char *)print_buffer);
            }
            re_flag = 0;
        }
				
		if(second_flag)		
		{
			second_flag = 0;
			if(s1_key_addr.flag)
			{
				key_value = s1_key_scan(s1_key_addr.periph,s1_key_addr.addr);
				if(key_value != SWN)
				{
					sprintf((char *)print_buffer,(const char*)"key value = %d\r\n",key_value); 
					debug_printf(EVAL_COM0,(char*)print_buffer);
					get_display_state();
				}	
				
			}
			else
			{
				sprintf((char *)print_buffer,(const char*)"???S1??\r\n"); 
				debug_printf(EVAL_COM0,(char*)print_buffer); 
			}	
			
			if(system_state == 1)
            {
											
                switch (display_state)
                {
                    case 1:
                        display_temperature();
                        e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,0,0);
                        break;
                    case 2:
                        e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,60,0);
                        display_humidity();
                        break;
                    case 3:
                        e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,60);
                        display_sunshine();
                        break;
                    case 4:
                        e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,60,60,60);
                        e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, 0, 0, 0, 0, tmp);
                        
                }
            }
            else
            {
                e1_rgb_control(e1_rgb_led_addr.periph,e1_rgb_led_addr.addr,0,0,0);
                e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, 15, 15, 15, 15, tmp);
            }
		}						
	}
	
}

/**
 * @brief 
 */

void get_display_state(){

    if(key_value == SW1)
    { //temperature
        display_state = 1;
    }
    else if(key_value == SW2)
    { //humidity
        display_state = 2;
    }
    else if(key_value == SW3)
    { //sunshine
        display_state = 3;
    }
    else if(key_value == SW4)
    { //reset
        display_state = 4;
    }
    else if(key_value == SW8)
    {
        system_state = 0; // swtich off
				display_state = 4;
				
    }
    else if(key_value == SW9)
    {
        system_state = 1; // switch on
    }

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

/**
 * @brief 
 */
void TIMER3_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP))
    {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
        if(data_count++ >= 700){
            data_count = 0;
            second_flag = 1;
        }
        if(re_count++ >= 1500){
						re_flag = 1;
				}
    }
}

//utilizer
void get_dp(float num)
{
	memset(dp, 0, 4 * sizeof(uint8_t));
	if(num > 0 && num < 10.0) dp[0] = 1;
	else if(num >= 10.0 && num < 100.0) dp[1] = 1;
	else if(num >= 100.0 && num < 1000.0) dp[2] = 1;
	else dp[3] = 1;
}

void get_first_four_digits(float number) {
	
    for (int k = 0; k < 4; k++) {
        dig[k] = 0;
    }
		
		int ip = (int)number;
    float fp = number - ip;
		
		int temp = ip;
    int i = 0;
    while(temp > 0){
			temp /= 10;
			i++;
		}
		
		int j = i;
		while(j > 0){
			dig[j - 1] = ip % 10;
			ip /= 10;
			j--;
		}

		while(i < 4){
			fp *= 10;
			dig[i] = (int)fp;
			fp -= (int)fp;
			i++;
		}
        if(fp >= 0.5){
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
    timer_init_struct.prescaler         = 4199; // 
    timer_init_struct.period            = 20;   // 
    timer_init_struct.alignedmode       = TIMER_COUNTER_EDGE;
    timer_init_struct.counterdirection  = TIMER_COUNTER_UP;
    timer_init_struct.clockdivision     = TIMER_CKDIV_DIV1;
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
    if (usart_periph != USART0 && usart_periph != USART2) return -1;
    if (data == NULL) return -1;
    if (len <= 0) return -1;
    for (i = 0; i < len; i++)
    {
        while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, data[i]);
    }
    while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
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
