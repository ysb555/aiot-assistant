#include <string.h>
#include "uart.h"

/* ========================== 串口接收缓冲区 ========================== */
static uint8_t   Uart0_RecvBuffer[512];
static uint16_t  Uart0_RecvLenth = 0;
static UartRecvState_t Uart0_RecvFlag = STATE_RX_IDLE;
static uint16_t  Uart0_RecvCnt = 0;

volatile uint16_t delay_count = 0;  // 延时计数器

/* ========================== 串口初始化 (仅 UART0) ========================== */

/**
 * 初始化 UART0 (PA9 TX, PA10 RX)
 * 用于 C3 WiFi 模块 + 调试串口(前端控制)
 * 注意: USART1 由 main.c 独立初始化
 *        USART2 由 c2.c 独立初始化(轮询模式)
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
        usart_word_length_set(USART0, USART_WL_8BIT);
        usart_stop_bit_set(USART0, USART_STB_1BIT);
        usart_parity_config(USART0, USART_PM_NONE);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_enable(USART0);
        usart_interrupt_enable(USART0, USART_INT_RBNE);
    }
    // USART2 由 c2.c 独立管理，此处不处理
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
    UartRecvState_t* recvflag = &Uart0_RecvFlag;
    uint8_t* recvbuf  = Uart0_RecvBuffer;
    uint16_t* recvlen  = &Uart0_RecvLenth;
    uint16_t ret_len = 0;

    // USART2 由 c2.c 轮询管理，这里只支持 UART0
    if (usart_periph != USART0) return 0;

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

        // UART0 接收超时检测 (20ms无新数据 → 接收完成)
        if (Uart0_RecvFlag == STATE_RX_RECEIVING) {
            Uart0_RecvCnt++;
            if (Uart0_RecvCnt >= 20) {
                Uart0_RecvCnt = 0;
                Uart0_RecvFlag = STATE_RX_RECEIVED;
            }
        }
    }
}

/**
 * UART0 接收中断
 * 将所有接收到的字节缓冲到 Uart0_RecvBuffer
 * 由 TIMER3_IRQHandler 检测一帧结束 (20ms超时)
 */
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

/**
 * 从 UART0 缓冲区中提取一行以 \r\n 结尾的前端命令
 * 返回: 提取到的字节数 (0=无完整命令)
 */
uint16_t uart_get_line(char *line_buf, uint16_t max_len) {
    uint16_t i, j;
    if (Uart0_RecvFlag != STATE_RX_RECEIVED && Uart0_RecvLenth == 0) return 0;

    for (i = 0; i < Uart0_RecvLenth - 1; i++) {
        if (Uart0_RecvBuffer[i] == '\r' && Uart0_RecvBuffer[i+1] == '\n') {
            // 找到一行
            uint16_t line_len = (i < max_len - 1) ? i : max_len - 1;
            memcpy(line_buf, Uart0_RecvBuffer, line_len);
            line_buf[line_len] = '\0';

            // 从缓冲区移除已处理的行
            uint16_t remaining = Uart0_RecvLenth - (i + 2);
            if (remaining > 0) {
                for (j = 0; j < remaining; j++) {
                    Uart0_RecvBuffer[j] = Uart0_RecvBuffer[i + 2 + j];
                }
            }
            Uart0_RecvLenth = remaining;

            // 检查缓冲区中是否还有数据
            if (Uart0_RecvLenth == 0) {
                Uart0_RecvFlag = STATE_RX_IDLE;
            }

            return line_len;
        }
    }

    // 如果是接收完成状态但没有找到 \r\n，也尝试提取（纯 \n 结尾）
    if (Uart0_RecvFlag == STATE_RX_RECEIVED && Uart0_RecvLenth > 0) {
        for (i = 0; i < Uart0_RecvLenth; i++) {
            if (Uart0_RecvBuffer[i] == '\n') {
                uint16_t line_len = (i < max_len - 1) ? i : max_len - 1;
                memcpy(line_buf, Uart0_RecvBuffer, line_len);
                line_buf[line_len] = '\0';
                Uart0_RecvLenth = 0;
                Uart0_RecvFlag = STATE_RX_IDLE;
                return line_len;
            }
        }
    }

    return 0;
}