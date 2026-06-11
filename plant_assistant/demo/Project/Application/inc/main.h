#ifndef MAIN_H
#define MAIN_H

#include "plant_logic.h"
#include "i2c.h"
#include "s1.h"

/* ==================== UART 公共函数 ==================== */
void usart_send_string(uint32_t usart_periph, char *str);

/* ==================== 初始化 ==================== */
void device_init(void);
void systick_config(void);

/* ==================== 延时 ==================== */
void delay_1ms(uint32_t ms);

#endif // MAIN_H