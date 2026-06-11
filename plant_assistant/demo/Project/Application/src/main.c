#include "gd32f4xx.h"
#include "systick.h"
#include "i2c.h"
#include "uart.h"
#include "main.h"
#include "plant_logic.h"
#include "s1.h"
#include "c2.h"
#include "c3.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ==================== UART 端口分配 ==================== */
// UART0  (PA9/PA10):  C3 WiFi 模块 + 调试串口(前端控制)  [由 uart.c 管理]
// USART1 (PA2/PA3):   u2p_vision 视觉识别通信            [由 main.c 管理]
// USART2 (PC10/PC11): C2 Zigbee 模块                    [由 c2.c 管理]

/* ==================== 全局变量 ==================== */
static key_state_t g_key_state;

/* ==================== USART1 初始化 (u2p_vision 视觉识别) ==================== */
static void usart1_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);

    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2 | GPIO_PIN_3);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, 115200U);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_hardware_flow_rts_config(USART1, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART1, USART_CTS_DISABLE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);

    nvic_irq_enable(USART1_IRQn, 2, 2);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
}

/* ==================== UART 发送辅助函数 ==================== */
void usart_send_string(uint32_t usart_periph, char *str) {
    while (*str) {
        while (usart_flag_get(usart_periph, USART_FLAG_TBE) == RESET);
        usart_data_transmit(usart_periph, (uint8_t)*str++);
    }
}

/* ==================== USART1 接收中断 (u2p_vision) ==================== */
static char g_vision_rx_buf[128];
static uint8_t g_vision_rx_idx;

void USART1_IRQHandler(void) {
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET) {
        uint8_t ch = usart_data_receive(USART1);
        if (ch == '\r' || ch == '\n') {
            if (g_vision_rx_idx > 0) {
                g_vision_rx_buf[g_vision_rx_idx] = '\0';
                // 解析 VISION:plant_id,plant_name
                if (strncmp(g_vision_rx_buf, "VISION:", 7) == 0) {
                    int id = atoi(&g_vision_rx_buf[7]);
                    if (id >= 1 && id <= MAX_PLANTS) {
                        g_sys.vision_plant_id = (uint8_t)id;
                        g_sys.vision_ready = 1;
                    }
                }
                g_vision_rx_idx = 0;
            }
        } else if (g_vision_rx_idx < sizeof(g_vision_rx_buf) - 1) {
            g_vision_rx_buf[g_vision_rx_idx++] = (char)ch;
        }
    }
}

/* ==================== 前端命令处理 ==================== */
static void frontend_process(void) {
    char line[128];
    uint16_t len;
    // 逐行提取并处理
    while ((len = uart_get_line(line, sizeof(line))) > 0) {
        if (len > 0) {
            uart_command_parse((uint8_t*)line);
        }
    }
}

/* ==================== 硬件初始化 ==================== */
void device_init(void) {
    // I2C 初始化
    init_i2c();

    // Timer3 1ms 定时器 (uart.c 需要)
    timer3_init();

    // UART0: C3 WiFi + 调试串口 (由 uart.c 管理中断)
    uart_init(USART0);

    // USART1: u2p_vision 视觉识别
    usart1_init();

    // 初始化 C3 WiFi 模块
    c3_init();

    // 初始化植物助手逻辑 (含所有传感器/执行器/C2 Zigbee)
    plant_logic_init();

    // 启动提示
    usart_send_string(USART0, "\r\n================================\r\n");
    usart_send_string(USART0, " Xiaomi AIoT Plant Assistant\r\n");
    usart_send_string(USART0, "================================\r\n");
    usart_send_string(USART0, "Type HELP for command list\r\n");
    usart_send_string(USART0, "================================\r\n\r\n");
}

/* ==================== 按键处理 ==================== */
static void key_handle(uint8_t key) {
    switch (key) {
        case KEY_1:
            plant_switch(0);
            break;
        case KEY_2:
            plant_switch(1);
            break;
        case KEY_3:
            plant_switch(2);
            break;
        case KEY_4:
            plant_switch_next();
            break;
        case KEY_5:
            g_sys.actuator.auto_mode = !g_sys.actuator.auto_mode;
            if (g_sys.actuator.auto_mode) {
                g_sys.actuator.fan_auto = 1;
                g_sys.actuator.curtain_auto = 1;
            }
            usart_send_string(USART0, g_sys.actuator.auto_mode ? "AUTO:ON\r\n" : "AUTO:OFF\r\n");
            break;
        case KEY_6:
            if (g_sys.actuator.curtain_pos == 0) {
                g_sys.actuator.curtain_auto = 0;
                g_sys.actuator.curtain_pos = 100;
                usart_send_string(USART0, "CURTAIN:CLOSE\r\n");
            } else {
                g_sys.actuator.curtain_auto = 0;
                g_sys.actuator.curtain_pos = 0;
                usart_send_string(USART0, "CURTAIN:OPEN\r\n");
            }
            break;
        case KEY_7:
            g_sys.actuator.fan_auto = 0;
            if (g_sys.actuator.fan_speed == 0) {
                g_sys.actuator.fan_speed = 100;
                usart_send_string(USART0, "FAN:ON\r\n");
            } else {
                g_sys.actuator.fan_speed = 0;
                usart_send_string(USART0, "FAN:OFF\r\n");
            }
            break;
        case KEY_8:
            zigbee_send_status();
            usart_send_string(USART0, "ZIGBEE:Status sent\r\n");
            break;
        default:
            break;
    }
}

/* ==================== WiFi 连接状态机 ==================== */
static int g_wifi_run_state = 1;
static int g_tcp_status = -1;

/* ==================== 主循环 ==================== */
int main(void) {
    uint32_t tick = 0;
    uint32_t last_zigbee_send = 0;
    uint32_t last_nfc_scan = 0;

    // 初始化系统
    systick_config();
    device_init();

    while (1) {
        tick++;

        /* --- 每 10ms 检查前端命令 --- */
        frontend_process();

        /* --- 每 50ms 执行 --- */
        if (tick % 5 == 0) {
            uint8_t key = s1_key_scan(I2C0, HT16K33_KEY_ADDRESS_S1, &g_key_state);
            if (key != KEY_NONE) {
                key_handle(key);
            }
        }

        /* --- 每秒执行 --- */
        if (tick % 100 == 0) {
            // WiFi 连接状态机
            c3_wifi_tcp_lead(&g_wifi_run_state, &g_tcp_status);

            // 读取传感器
            sensors_read();

            // 计算评分
            score_calculate();

            // 更新显示
            display_update();
            rgb_update();

            // 自动控制
            auto_shade_control();
            auto_vent_control();
            auto_light_control();

            // 蒸发指数
            evaporation_update();

            // 浇水提醒检查
            watering_check();

            // 视觉识别结果处理
            vision_uart_process();

            // Zigbee 接收处理
            zigbee_recv_process();

            // 每10秒发送一次 Zigbee 状态
            last_zigbee_send++;
            if (last_zigbee_send >= 10) {
                zigbee_send_status();
                last_zigbee_send = 0;
            }

            // 每2秒扫描一次 NFC
            last_nfc_scan++;
            if (last_nfc_scan >= 2) {
                nfc_card_process();
                last_nfc_scan = 0;
            }
        }

        delay_1ms(10);
    }
}