#include "c2.h"
#include "systick.h"
#include <string.h>
#include <stdio.h>

/* ==================== C2 Zigbee 驱动 ==================== */
/* 使用 UART2 (PC10/PC11) 与 E18-MS1-PCB Zigbee 透传模块通信 */
/* 参考: xiaomi-aiot/4-smart_fan/Task3/C2-TERMI/demo */

/* --- UART2 底层初始化 --- */
void c2_uart_init(void) {
    rcu_periph_clock_enable(C2_GPIO_RCC);
    rcu_periph_clock_enable(C2_UART_RCC);

    gpio_af_set(C2_GPIO_PORT, C2_AF, C2_TX_PIN | C2_RX_PIN);
    gpio_mode_set(C2_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, C2_TX_PIN | C2_RX_PIN);
    gpio_output_options_set(C2_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, C2_TX_PIN | C2_RX_PIN);

    usart_deinit(C2_UART);
    usart_baudrate_set(C2_UART, 115200U);
    usart_word_length_set(C2_UART, USART_WL_8BIT);
    usart_stop_bit_set(C2_UART, USART_STB_1BIT);
    usart_parity_config(C2_UART, USART_PM_NONE);
    usart_hardware_flow_rts_config(C2_UART, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(C2_UART, USART_CTS_DISABLE);
    usart_receive_config(C2_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(C2_UART, USART_TRANSMIT_ENABLE);
    usart_enable(C2_UART);
}

/* --- 发送单个字节 --- */
static void c2_send_byte(uint8_t data) {
    while (usart_flag_get(C2_UART, USART_FLAG_TBE) == RESET);
    usart_data_transmit(C2_UART, data);
}

/* --- 发送字符串 --- */
void c2_send_string(char *str) {
    while (*str) {
        c2_send_byte((uint8_t)*str++);
    }
}

/* --- 接收单个字节 (阻塞) --- */
static uint8_t c2_rec_byte(uint16_t timeout_ms) {
    uint32_t start = get_tick();
    while (usart_flag_get(C2_UART, USART_FLAG_RBNE) == RESET) {
        if (get_tick() - start >= timeout_ms) return 0;
    }
    return (uint8_t)usart_data_receive(C2_UART);
}

/* --- 清空接收缓冲区 --- */
static void c2_flush_rx(void) {
    while (usart_flag_get(C2_UART, USART_FLAG_RBNE) != RESET) {
        usart_data_receive(C2_UART);
    }
}

/**
 * 初始化 C2 Zigbee 模块
 * zigbee_mode: C2_TERMINAL(终端) 或 C2_COORDINATOR(协调器)
 * 返回: 0成功, 非0失败
 */
uint8_t c2_init(uint8_t zigbee_mode) {
    uint8_t result = 0;
    uint8_t recvdata[50];
    uint8_t i;

    uint8_t read_device_data[4] = {0xFE, 0x01, 0xFE, 0xFF};           // 读取设备信息
    uint8_t terminal_cmd[5]     = {0xFD, 0x02, 0x01, 0x02, 0xFF};     // 设置为终端
    uint8_t coordinator_cmd[5]  = {0xFD, 0x02, 0x01, 0x00, 0xFF};     // 设置为协调器
    uint8_t set_pan_id[6]       = {0xFD, 0x03, 0x03,
                                    (C2_PAN_ID >> 8) & 0xFF,
                                    C2_PAN_ID & 0xFF, 0xFF};          // 设置PAN ID
    uint8_t set_group_id[6]     = {0xFD, 0x03, 0x04,
                                    (C2_GROUP_ID >> 8) & 0xFF,
                                    C2_GROUP_ID & 0xFF, 0xFF};        // 设置组ID
    uint8_t set_key[8]          = {0xFD, 0x05, 0x05,
                                    (C2_KEY >> 24) & 0xFF,
                                    (C2_KEY >> 16) & 0xFF,
                                    (C2_KEY >> 8) & 0xFF,
                                    C2_KEY & 0xFF, 0xFF};             // 设置密钥
    uint8_t save_config[3]      = {0xFD, 0x00, 0xFF};                  // 保存配置

    c2_uart_init();
    c2_flush_rx();

    // 1. 读取设备信息
    for (i = 0; i < 4; i++) c2_send_byte(read_device_data[i]);
    delay_1ms(100);
    for (i = 0; i < 50; i++) recvdata[i] = c2_rec_byte(500);
    if (recvdata[0] != 0xFE || recvdata[1] != 0x04) {
        result = 1;
    }

    // 2. 设置设备类型
    if (zigbee_mode == C2_COORDINATOR) {
        for (i = 0; i < 5; i++) c2_send_byte(coordinator_cmd[i]);
    } else {
        for (i = 0; i < 5; i++) c2_send_byte(terminal_cmd[i]);
    }
    delay_1ms(100);
    c2_flush_rx();

    // 3. 设置 PAN ID
    for (i = 0; i < 6; i++) c2_send_byte(set_pan_id[i]);
    delay_1ms(100);
    c2_flush_rx();

    // 4. 设置组 ID
    for (i = 0; i < 6; i++) c2_send_byte(set_group_id[i]);
    delay_1ms(100);
    c2_flush_rx();

    // 5. 设置密钥
    for (i = 0; i < 8; i++) c2_send_byte(set_key[i]);
    delay_1ms(100);
    c2_flush_rx();

    // 6. 保存配置
    for (i = 0; i < 3; i++) c2_send_byte(save_config[i]);
    delay_1ms(100);
    c2_flush_rx();

    return result;
}

/**
 * 广播数据
 * send_data: 要发送的字符串
 * mode: 0=透传模式, 1=广播模式
 */
void c2_broadcast_data(char *send_data, uint8_t mode) {
    if (mode == 0) {
        // 透传模式: 直接发送
        c2_send_string(send_data);
    } else {
        // 广播模式: 添加广播指令前缀
        uint8_t broadcast_cmd[4] = {0xFD, 0x01, 0x06, 0xFF};
        for (uint8_t i = 0; i < 4; i++) c2_send_byte(broadcast_cmd[i]);
        delay_1ms(10);
        c2_send_string(send_data);
    }
}

/**
 * 接收数据
 * rec_data: 接收缓冲区
 * len: 缓冲区长度
 * timeout: 超时(ms)
 * 返回: 实际接收字节数
 */
uint8_t c2_rec_data(uint8_t *rec_data, uint16_t len, uint16_t timeout) {
    uint8_t count = 0;
    uint32_t start = get_tick();

    while (count < len) {
        if (usart_flag_get(C2_UART, USART_FLAG_RBNE) != RESET) {
            rec_data[count++] = (uint8_t)usart_data_receive(C2_UART);
            start = get_tick();  // 重置超时
        }
        if (get_tick() - start >= timeout) break;
    }
    return count;
}