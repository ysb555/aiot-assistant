/*!
    \file  main.h
    \brief the header file of main 
		\version 2021-08, V0.1
*/


#ifndef MAIN_H
#define MAIN_H

#include "gd32f4xx.h"
#include "stdint.h"



void timer3_init(void);
void delay_ms(uint16_t mstime);
uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len);
void debug_printf(uint32_t usart_periph,char *string);


#endif /* MAIN_H */
