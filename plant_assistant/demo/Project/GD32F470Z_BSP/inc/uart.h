#ifndef UART_H
#define UART_H

#include "gd32f4xx.h"

#define out

/* ========================== 串口接收状态 ========================== */
typedef enum {
    STATE_RX_IDLE,        // 空闲
    STATE_RX_RECEIVING,   // 接收中
    STATE_RX_RECEIVED,    // 接收完成
    STATE_RX_ERROR        // 接收错误
} UartRecvState_t;

/* ========================== 函数声明 ========================== */
void uart_init(uint32_t usart_periph);
void timer3_init(void);
void delay_ms(uint16_t mstime);
uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t* data, uint16_t len);
uint16_t uart_rece_bytes(uint32_t usart_periph, uint8_t* data, uint16_t len, uint16_t timeout);

/* ========================== 前端命令提取 ========================== */
uint16_t uart_get_line(char *line_buf, uint16_t max_len);

/* ========================== Timer3 中断 (1ms) ========================== */
void TIMER3_IRQHandler(void);

/* ========================== UART0 中断 ========================== */
void USART0_IRQHandler(void);

#endif // UART_H