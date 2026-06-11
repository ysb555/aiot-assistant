#ifndef C2_H
#define C2_H

#include "gd32f4xx.h"

/* ==================== C2 Zigbee 模块 (E18-MS1-PCB) ==================== */
// 通信方式: UART2 (PC10 TX, PC11 RX)
// 工作模式: 终端(TERMINAL) 或 协调器(COORDINATOR)

#define C2_TERMINAL                 0       // 终端模式
#define C2_COORDINATOR              1       // 协调器模式

// C2 依赖 UART2, 波特率 115200
#define C2_UART                     USART2
#define C2_UART_RCC                 RCU_USART2
#define C2_GPIO_RCC                 RCU_GPIOC
#define C2_GPIO_PORT                GPIOC
#define C2_TX_PIN                   GPIO_PIN_10
#define C2_RX_PIN                   GPIO_PIN_11
#define C2_AF                       GPIO_AF_7

// Zigbee 组网参数
#define C2_PAN_ID                   0x2244
#define C2_GROUP_ID                 0x0001
#define C2_KEY                      0x12345678

/* 函数声明 */
uint8_t c2_init(uint8_t zigbee_mode);
void c2_broadcast_data(char *send_data, uint8_t mode);
uint8_t c2_rec_data(uint8_t *rec_data, uint16_t len, uint16_t timeout);
void c2_send_string(char *str);
void c2_uart_init(void);

#endif // C2_H