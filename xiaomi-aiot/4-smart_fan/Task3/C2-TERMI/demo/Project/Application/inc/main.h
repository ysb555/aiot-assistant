/*!
    \file  main.h
    \brief the header file of main 
		\version 2021-08, V0.1
*/


#ifndef MAIN_H
#define MAIN_H

#include "gd32f4xx.h"
#include "stdint.h"

#define  LED_ON    (GPIO_BOP(GPIOF) = GPIO_PIN_4)
#define  LED_OFF   (GPIO_BC(GPIOF) = GPIO_PIN_4)
void card_handler();
void card_read();
void card_write();
void display_temperature(void);
void display_humidity(void);
void get_dp(float num);
void get_first_four_digits(float number);
//void handler_rec_data(void);
void dis_led_init(void);
#endif /* MAIN_H */
