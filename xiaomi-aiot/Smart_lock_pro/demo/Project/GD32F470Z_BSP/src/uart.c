#include <string.h>
#include "uart.h"

// USART0 接收相关变量
uint8_t         Uart0_RecvBuffer[512];  // 接收缓冲区
uint16_t        Uart0_RecvLenth = 0;    // 接收数据长度
UartRecvState_t Uart0_RecvFlag;         // 接收状态标志
uint16_t        Uart0_RecvCnt   = 0;    // 接收计数器

// USART2 接收相关变量
uint8_t         Uart2_RecvBuffer[512];
uint16_t        Uart2_RecvLenth = 0;
UartRecvState_t Uart2_RecvFlag;
uint16_t        Uart2_RecvCnt   = 0;

// USART5 接收相关变量
uint8_t         Uart5_RecvBuffer[512];
uint16_t        Uart5_RecvLenth = 0;
UartRecvState_t Uart5_RecvFlag;
uint16_t        Uart5_RecvCnt   = 0;

// 延时计数器
volatile uint16_t delay_count = 0;

// 函数声明
void     uart_init(uint32_t usart_periph);
void     timer3_init(void);
uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len);
uint16_t uart_receive_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len, uint16_t timeout);
void     delay_ms(uint16_t mstime);
void     TIMER3_IRQHandler(void);
void     USART0_IRQHandler(void);
void     USART2_IRQHandler(void);
void     USART5_IRQHandler(void);

// 外部变量引用（来自 main.c）
extern uint16_t time_count;
extern uint8_t  botton_flag;           // 按键标志
extern uint8_t  human_detect_flag;     // 人体检测标志
extern uint8_t  system_state;          // 系统状态
extern uint16_t unlock_failure_count;
extern uint16_t auto_lock_time;
extern uint8_t  active_flag;           // 活动标志
extern uint8_t  nfc_enable_flag;       // NFC 使能标志
extern uint8_t  temporary_password_flag;
extern uint32_t temporary_password_time;
extern uint16_t wifi_time;
extern uint8_t  wifi_flag;
extern uint16_t wifi_active_time;
extern uint8_t  wifi_active_flag;
extern uint8_t  voice_authorized_flag;
extern uint16_t voice_authorized_time;
extern uint8_t  nfc_authorized_flag;
extern uint16_t nfc_authorized_time;
extern uint32_t time_limit;
extern uint8_t  s7_human_detected;
extern uint16_t human_detect_count;
extern uint8_t  human_long_stay_flag;

/*********************************************************************************************
 * 函数名:    uart_init
 * 功能:      初始化指定的 USART 外设
 * 输入参数:  usart_periph   USART0/USART2
 * 输出参数:  无
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void uart_init(uint32_t usart_periph) {
    if (usart_periph == USART0) {
        nvic_irq_enable(USART0_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_GPIOB);                                 // 使能 GPIO 时钟
        rcu_periph_clock_enable(RCU_USART0);                                // 使能 USART 时钟
        gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_6);                          // 配置 USART Tx 引脚
        gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_7);                          // 配置 USART Rx 引脚
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);   // 设置 Tx 为复用功能
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
        gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7);   // 设置 Rx 为复用功能
        gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
        // USART 配置
        usart_deinit(USART0);
        usart_baudrate_set(USART0, 115200U);
        usart_receive_config(USART0, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
        usart_enable(USART0);
        usart_interrupt_enable(USART0, USART_INT_RBNE);
    } else if (usart_periph == USART2) {
        nvic_irq_enable(USART2_IRQn, 0, 0);
        rcu_periph_clock_enable(RCU_GPIOC);                                 // 使能 GPIO 时钟
        rcu_periph_clock_enable(RCU_USART2);                                // 使能 USART 时钟
        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_10);                         // 配置 USART Tx 引脚
        gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_11);                         // 配置 USART Rx 引脚
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);  // 设置 Tx 为复用功能
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
        gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_11);  // 设置 Rx 为复用功能
        gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
        // USART 配置
        usart_deinit(USART2);
        usart_baudrate_set(USART2, 115200U);
        usart_receive_config(USART2, USART_RECEIVE_ENABLE);
        usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
        usart_enable(USART2);
        usart_interrupt_enable(USART2, USART_INT_RBNE);
    }
}

/*********************************************************************************************
 * 函数名:    timer3_init
 * 功能:      初始化 TIMER3，配置为 1ms 定时中断
 * 输入参数:  无
 * 输出参数:  无
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void timer3_init(void) {
    timer_parameter_struct timer_init_struct;
    rcu_periph_clock_enable(RCU_TIMER3);
    timer_deinit(TIMER3);
    timer_init_struct.prescaler         = 4199;    // 预分频器
    timer_init_struct.period            = 20;      // 定时周期
    timer_init_struct.alignedmode       = TIMER_COUNTER_EDGE;
    timer_init_struct.counterdirection  = TIMER_COUNTER_UP;
    timer_init_struct.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init_struct.repetitioncounter = 0;
    timer_init(TIMER3, &timer_init_struct);
    nvic_irq_enable(TIMER3_IRQn, 1, 1);
    timer_interrupt_enable(TIMER3, TIMER_INT_UP);
    timer_enable(TIMER3);
}

/*********************************************************************************************
 * 函数名:    uart_send_bytes
 * 功能:      通过指定的 USART 发送字节数据
 * 输入参数:  usart_periph   USART 外设
 *            data           要发送的数据
 *            len            数据长度
 * 输出参数:  无
 * 返回值:    发送的字节数
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len) {
    uint8_t i;
    for (i = 0; i < len; i++) {
        while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
        usart_data_transmit(usart_periph, data[i]);
    }
    while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET);
    return len;
}

/*********************************************************************************************
 * 函数名:    uart_receive_bytes
 * 功能:      接收字节数据
 * 输入参数:  usart_periph   USART 外设
 *            data           接收数据缓冲区
 *            len            期望接收的长度
 *            timeout        超时时间
 * 输出参数:  data           接收到的数据
 * 返回值:    实际接收的字节数
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
uint16_t uart_receive_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len, uint16_t timeout) {
    UartRecvState_t *recvflag;
    uint8_t         *recvbuf;
    uint16_t        *recvlen;
    uint16_t        ret_len = 0;

    if (usart_periph == USART0) {
        recvflag = &Uart0_RecvFlag;
        recvbuf  = Uart0_RecvBuffer;
        recvlen  = &Uart0_RecvLenth;
    } else if (usart_periph == USART2) {
        recvflag = &Uart2_RecvFlag;
        recvbuf  = Uart2_RecvBuffer;
        recvlen  = &Uart2_RecvLenth;
    } else {
        return 0;  // 未支持的 USART
    }

    while (timeout--) {
        if (*recvflag == STATE_RX_RECEIVED) {
            if (*recvlen > len) {
                ret_len = len;
            } else {
                ret_len = *recvlen;
            }
            memcpy(data, recvbuf, ret_len);
            memset(recvbuf, 0, *recvlen);
            *recvlen  = 0;
            *recvflag = STATE_RX_IDLE;
            break;
        }
        delay_ms(1);
    }
    return ret_len;
}

/*********************************************************************************************
 * 函数名:    delay_ms
 * 功能:      毫秒级延时
 * 输入参数:  mstime         延时毫秒数
 * 输出参数:  无
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void delay_ms(uint16_t mstime) {
    delay_count = mstime;
    while (delay_count) {
        // 等待 delay_count 在中断中减到 0
    }
}

/*********************************************************************************************
 * 函数名:    TIMER3_IRQHandler
 * 功能:      TIMER3 中断处理函数，处理定时器中断和相关标志位
 * 输入参数:  无
 * 输出参数:  无
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void TIMER3_IRQHandler(void) {
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

        if (delay_count > 0) {
            delay_count--;
        }

        if (time_count++ >= 500) {
            time_count = 0;
            botton_flag = 1;
            human_detect_flag = 1;
            nfc_enable_flag = 1;
        }

        if (s7_human_detected) {
            human_detect_count++;
            if (human_detect_count >= 10000) {
                human_detect_count = 0;
                human_long_stay_flag = 1;
            }
        } else {
            human_long_stay_flag = 0;
        }

        if (voice_authorized_flag) {
            if (voice_authorized_time++ >= 8000) {
                voice_authorized_time = 0;
                voice_authorized_flag = 0;
                system_state = 0;
            }
        }

        if (nfc_authorized_flag) {
            if (nfc_authorized_time++ >= 8000) {
                nfc_authorized_time = 0;
                nfc_authorized_flag = 0;
                system_state = 0;
            }
        }

        if (system_state == 3) {
            if (unlock_failure_count++ >= 3700) {
                unlock_failure_count = 0;
                system_state = 0;
            }
        }

        if (wifi_time++ >= 300) {
            wifi_flag = 1;
            wifi_time = 0;
        }

        if (temporary_password_flag) {
            if (temporary_password_time++ >= time_limit) {
                temporary_password_time = 0;
                temporary_password_flag = 0;
            }
        }

        if (!active_flag) {
            auto_lock_time++;
        } else {
            auto_lock_time = 0;
        }

        if (Uart0_RecvFlag == STATE_RX_RECEIVING) {
            Uart0_RecvCnt++;
            if (Uart0_RecvCnt >= 20) {
                Uart0_RecvCnt = 0;
                Uart0_RecvFlag = STATE_RX_RECEIVED;
            }
        }

        if (Uart2_RecvFlag == STATE_RX_RECEIVING) {
            Uart2_RecvCnt++;
            if (Uart2_RecvCnt >= 20) {
                Uart2_RecvCnt = 0;
                Uart2_RecvFlag = STATE_RX_RECEIVED;
            }
        }
    }
}

/*********************************************************************************************
 * 函数名:    USART0_IRQHandler
 * 功能:      USART0 中断处理函数，处理接收数据
 * 输入参数:  无
 * 输出参数:  Uart0_RecvBuffer 接收到的数据
 *            Uart0_RecvLenth  接收数据长度
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void USART0_IRQHandler(void) {
    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);
        Uart0_RecvBuffer[Uart0_RecvLenth] = usart_data_receive(USART0);
        Uart0_RecvLenth++;
        if (Uart0_RecvFlag != STATE_RX_RECEIVING) {
            Uart0_RecvFlag = STATE_RX_RECEIVING;
        }
        Uart0_RecvCnt = 0;
    }
}

/*********************************************************************************************
 * 函数名:    USART2_IRQHandler
 * 功能:      USART2 中断处理函数，处理接收数据
 * 输入参数:  无
 * 输出参数:  Uart2_RecvBuffer 接收到的数据
 *            Uart2_RecvLenth  接收数据长度
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void USART2_IRQHandler(void) {
    if (usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART2, USART_INT_FLAG_RBNE);
        Uart2_RecvBuffer[Uart2_RecvLenth] = usart_data_receive(USART2);
        Uart2_RecvLenth++;
        if (Uart2_RecvFlag != STATE_RX_RECEIVING) {
            Uart2_RecvFlag = STATE_RX_RECEIVING;
        }
        Uart2_RecvCnt = 0;
    }
}

/*********************************************************************************************
 * 函数名:    USART5_IRQHandler
 * 功能:      USART5 中断处理函数，处理接收数据
 * 输入参数:  无
 * 输出参数:  Uart5_RecvBuffer 接收到的数据
 *            Uart5_RecvLenth  接收数据长度
 * 返回值:    无
 * 作者:      ZZZ
 * 日期:      2023/4/6
 *********************************************************************************************/
void USART5_IRQHandler(void) {
    if (usart_interrupt_flag_get(USART5, USART_INT_FLAG_RBNE) != RESET) {
        usart_interrupt_flag_clear(USART5, USART_INT_FLAG_RBNE);
        Uart5_RecvBuffer[Uart5_RecvLenth] = usart_data_receive(USART5);
        Uart5_RecvLenth++;
        if (Uart5_RecvFlag != STATE_RX_RECEIVING) {
            Uart5_RecvFlag = STATE_RX_RECEIVING;
        }
        Uart5_RecvCnt = 0;
    }
}
