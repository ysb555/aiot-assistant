/*!
    \file  main.h
    \brief the header file of main 
		\version 2021-08, V0.1
*/


#ifndef MAIN_H
#define MAIN_H

#include "stdint.h"

#define  LED_ON    (GPIO_BOP(GPIOF) = GPIO_PIN_4)
#define  LED_OFF   (GPIO_BC(GPIOF) = GPIO_PIN_4)

void display_number(uint16_t secs);
void delay_ms(uint16_t mstime);
void dis_led_init(void);
void timer3_init(void);
int uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len);
void debug_printf(uint32_t usart_periph,char *string);


#endif /* MAIN_H */
