#include <string.h>
#include "uart.h"

/* ========================== 串口接收缓冲区 ========================== */
static uint8_t   Uart0_RecvBuffer[512];
static uint16_t  Uart0_RecvLenth = 0;
static UartRecvState_t Uart0_RecvFlag = STATE_RX_IDLE;
static uint16_t  Uart0_RecvCnt = 0;

static uint8_t   Uart2_RecvBuffer[512];
static uint16_t  Uart2_RecvLenth = 0;
static UartRecvState_t Uart2_RecvFlag = STATE_RX_IDLE;
static uint16_t  Uart2_RecvCnt = 0;

volatile uint16_t delay_count = 0;  // 延时计数器

/* ========================== 串口初始化 ========================== */

/**
 * 初始化指定串口 (USART0 或 USART2)
 */
void uart_init(uint32_t usart_periph) {
    if (usart_periph == USART0) {
        nvic_irq_enable(USART0_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_GPIOA);
        rcu_periph_clock_enable(RCU_USART0);

        gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);   // TX
        gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_10);  // RX
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

        usart_deinit(USART0);
        usart_baudrate_set(USART0, 115200U);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_enable(USART0);
        usart_interrupt_enable(USART0, USART_INT_RBNE);
    } else if (usart_periph == USART2) {
        nvic_irq_enable(USART2_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_GPIOC);
        rcu_periph_clock_enable(RCU_USART2);

        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_10);  // TX
        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_11);  // RX
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_11);
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);

        usart_deinit(USART2);
        usart_baudrate_set(USART2, 115200U);
        usart_receive_config(USART2, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
        usart_enable(USART2);
        usart_interrupt_enable(USART2, USART_INT_RBNE);
    }
}

/* ========================== Timer3 1ms定时器 ========================== */

void timer3_init(void) {
    timer_parameter_struct timer_init_struct;

    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);

    timer_init_struct.prescaler         = 4199;
    timer_init_struct.period            = 20;
    timer_init_struct.alignedmode       = TIMER_COUNTER_EDGE;
    timer_init_struct.counterdirection  = TIMER_COUNTER_UP;
    timer_init_struct.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init_struct.repetitioncounter = 0;
    timer_init(TIMER3, &timer_init_struct);

    nvic_irq_enable(TIMER3_IRQn, 1, 1);
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    timer_enable(TIMER3);
}

/* ========================== 数据收发 ========================== */

uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, data[i]);
    }
    while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
    return len;
}

uint16_t uart_rece_bytes(uint32_t usart_periph, uint8_t* data, uint16_t len, uint16_t timeout) {
    UartRecvState_t* recvflag;
    uint8_t* recvbuf;
    uint16_t* recvlen;
    uint16_t ret_len = 0;

    if (usart_periph == USART0) {
        recvflag = &Uart0_RecvFlag;
        recvbuf  = Uart0_RecvBuffer;
        recvlen  = &Uart0_RecvLenth;
    } else {
        recvflag = &Uart2_RecvFlag;
        recvbuf  = Uart2_RecvBuffer;
        recvlen  = &Uart2_RecvLenth;
    }

    while (timeout--) {
        if (*recvflag == STATE_RX_RECEIVED) {
            ret_len = (*recvlen > len) ? len : *recvlen;
            memcpy(data, recvbuf, ret_len);
            memset(recvbuf, 0, *recvlen);
            *recvlen = 0;
            *recvflag = STATE_RX_IDLE;
            return ret_len;
        }
        delay_ms(1);
    }
    return ret_len;
}

/* ========================== 延时函数 ========================== */

void delay_ms(uint16_t mstime) {
    delay_count = mstime;
    while (delay_count);
}

/* ========================== 中断服务函数 ========================== */

void TIMER3_IRQHandler(void) {
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

        if (delay_count > 0) delay_count--;

        // USART0 接收超时检测
        if (Uart0_RecvFlag == STATE_RX_RECEIVING) {
            Uart0_RecvCnt++;
            if (Uart0_RecvCnt >= 20) {
                Uart0_RecvCnt = 0;
                Uart0_RecvFlag = STATE_RX_RECEIVED;
            }
        }

        // USART2 接收超时检测
        if (Uart2_RecvFlag == STATE_RX_RECEIVING) {
            Uart2_RecvCnt++;
            if (Uart2_RecvCnt >= 20) {
                Uart2_RecvCnt = 0;
                Uart2_RecvFlag = STATE_RX_RECEIVED;
            }
        }
    }
}

void USART0_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_RBNE)) {
        uint8_t ch = (uint8_t)usart_data_receive(USART0);
        if (Uart0_RecvLenth < sizeof(Uart0_RecvBuffer) - 1) {
            Uart0_RecvBuffer[Uart0_RecvLenth++] = ch;
            Uart0_RecvFlag = STATE_RX_RECEIVING;
            Uart0_RecvCnt = 0;
        }
    }
}

void USART2_IRQHandler(void) {
    if (RESET != usart_interrupt_flag_get(USART2, USART_INT_RBNE)) {
        uint8_t ch = (uint8_t)usart_data_receive(USART2);
        if (Uart2_RecvLenth < sizeof(Uart2_RecvBuffer) - 1) {
            Uart2_RecvBuffer[Uart2_RecvLenth++] = ch;
            Uart2_RecvFlag = STATE_RX_RECEIVING;
            Uart2_RecvCnt = 0;
        }
    }
}