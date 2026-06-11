#include "gd32f4xx.h"
#include "i2c.h"
#include "systick.h"
#include "uart.h"
#include "e1.h"
#include "e2.h"
#include "s2.h"
#include "s1.h"
#include "c3.h"
#include "../Application/inc/plant_logic.h"
#include "../Application/inc/main.h"
#include <stdio.h>
#include <string.h>

/* ========================== 全局设备地址 ========================== */
i2c_addr_def s2_th_addr;      // SHT35 温湿度传感器 (I2C:0x44)
i2c_addr_def s2_ss_addr;      // BH1750 光照传感器 (I2C:0x23)
i2c_addr_def e1_rgb_addr;     // PCA9685 RGB (I2C:0x40)
i2c_addr_def e1_nixie_addr;   // HT16K33 数码管 (I2C:0x70)
i2c_addr_def e2_fan_addr;     // PCA9685 风扇 (I2C:0x60)
i2c_addr_def s1_key_addr;     // S1按键 (I2C:0x24)
i2c_addr_def u2p_vision_addr; // u2p_vision (I2C:0x30)

/* ========================== 系统状态标志 ========================== */
volatile uint32_t sys_tick_1s = 0;     // 1秒计数器 (每50ms+1, 到20即为1秒)
volatile uint32_t sys_tick_50ms = 0;   // 50ms tick 触发
volatile uint32_t uart_recv_tick = 0;  // UART 接收标记

/* u2p_vision 通讯变量 */
char vision_name[32] = {0};
float vision_score = 0.0f;
uint8_t vision_data_ready = 0;

/* WiFi 连接状态 */
int wifi_run_state = 1;
int tcp_status = -1;

/* 按键防抖变量 */
uint16_t key_debounce_cnt = 0;
uint8_t  key_last_state = 1;

/* ========================== 硬件初始化 ========================== */

void system_init(void) {
    // 基础时钟配置
    rcu_config();

    // Systick (提供给 delay_ms)
    systick_config();

    // I2C (PB8/9=I2C0, PF0/1=I2C1)
    init_i2c();

    // UART0 (PA9/PA10) = C3 WiFi, UART2 (PC10/PC11) = u2p_vision
    uart_init(USART0);
    uart_init(USART2);

    // Timer3 1ms基准
    timer3_init();

    // 延时确保外设稳定
    delay_ms(100);
}

void device_init(void) {
    // 扫描所有I2C设备
    s2_th_addr      = get_board_address(0x44);   // SHT35
    s2_ss_addr      = get_board_address(0x23);   // BH1750
    e1_rgb_addr     = get_board_address(0x40);   // PCA9685 RGB
    e1_nixie_addr   = get_board_address(0x70);   // HT16K33
    e2_fan_addr     = get_board_address(0x60);   // PCA9685 Fan
    s1_key_addr     = get_board_address(0x24);   // S1 Button
    u2p_vision_addr = get_board_address(0x30);   // u2p_vision

    // 初始化传感器
    if (s2_th_addr.flag) s2_th_init(s2_th_addr.periph, s2_th_addr.addr);
    if (s2_ss_addr.flag) s2_ss_init(s2_ss_addr.periph, s2_ss_addr.addr);

    // 初始化执行器
    if (e1_rgb_addr.flag) {
        e1_rgb_init(e1_rgb_addr.periph, e1_rgb_addr.addr);
        e1_rgb_set(e1_rgb_addr.periph, e1_rgb_addr.addr, 0, 0, 0);
    }
    if (e1_nixie_addr.flag) {
        e1_nixie_init(e1_nixie_addr.periph, e1_nixie_addr.addr);
    }
    if (e2_fan_addr.flag) {
        e2_fan_init(e2_fan_addr.periph, e2_fan_addr.addr);
    }

    // 初始化按键
    if (s1_key_addr.flag) s1_key_init(s1_key_addr.periph, s1_key_addr.addr);

    // 初始化WiFi
    c3_init();

    // 初始化植物逻辑
    plant_logic_init();
}

/* ========================== 1哨(1秒) 任务循环 ========================== */

void task_1s_loop(void) {
    // 1. 读取传感器数据
    if (s2_th_addr.flag) {
        s2_get_temp_humi(s2_th_addr.periph, s2_th_addr.addr,
                         &current_env.temperature, &current_env.humidity);
    }
    if (s2_ss_addr.flag) {
        current_env.light = s2_get_light(s2_ss_addr.periph, s2_ss_addr.addr);
    }

    // 2. 计算评分
    calc_score();

    // 3. 更新蒸发指数 & 浇水提醒
    update_evaporation();
    check_water_alert();

    // 4. 自动遮阳
    auto_curtain();

    // 5. 自动通风
    auto_fan();

    // 6. WiFi 连接状态机
    c3_wifi_tcp_lead(&wifi_run_state, &tcp_status);

    // 7. 视觉融合
    if (vision_data_ready) {
        vision_data_ready = 0;
        u1p_fusion_decision(vision_name, vision_score);
    }

    // 8. 更新显示与RGB
    update_display();
}

/* ========================== 显示更新 ========================== */

void update_display(void) {
    static uint8_t disp_prev_mode = 0xFF;
    static uint32_t disp_slice_tick = 0;

    disp_slice_tick++;

    // 模式切换时强制刷新
    if (disp_prev_mode != sys_ctrl.display_mode) {
        disp_prev_mode = sys_ctrl.display_mode;
        disp_slice_tick = 0;
    }

    switch (sys_ctrl.display_mode) {
        case 0:  // 综合评分
            display_score();
            set_rgb_by_score(current_env.score);
            break;
        case 1:  // 温度
            display_temperature_ui();
            break;
        case 2:  // 湿度
            display_humidity_ui();
            break;
        case 3:  // 光照（RGB=自然白色）
            display_light();
            set_rgb_color(40, 40, 40);
            break;
        default:
            break;
    }

    // 根据评分切换RGB
    set_rgb_by_score(current_env.score);
}

/* ========================== 按键处理 ========================== */

void process_key(void) {
    if (!s1_key_addr.flag) return;

    uint8_t key_code = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
    key_code &= 0x0F;  // 低4位

    if (key_code == 0) {
        key_last_state = 0;
        key_debounce_cnt = 0;
        return;
    }

    // 消抖处理
    if (key_code == key_last_state) {
        key_debounce_cnt++;
        if (key_debounce_cnt < 5) return;  // 50ms消抖
    } else {
        key_last_state = key_code;
        key_debounce_cnt = 0;
        return;
    }

    // 按键处理
    switch (key_code) {
        case 0x01:  // 按键1: 切换显示模式
            sys_ctrl.display_mode = (sys_ctrl.display_mode + 1) % 4;
            key_debounce_cnt = 0;
            key_last_state = 0;
            break;

        case 0x02:  // 按键2: 切换植物
            switch_plant();
            key_debounce_cnt = 0;
            key_last_state = 0;
            break;

        case 0x04:  // 按键3: 手动开/关自动遮阳
            sys_ctrl.auto_curtain_enabled = !sys_ctrl.auto_curtain_enabled;
            key_debounce_cnt = 0;
            key_last_state = 0;
            break;

        case 0x08:  // 按键4: 手动开/关自动通风
            sys_ctrl.auto_fan_enabled = !sys_ctrl.auto_fan_enabled;
            if (!sys_ctrl.auto_fan_enabled && e2_fan_addr.flag) {
                e2_fan_off(e2_fan_addr.periph, e2_fan_addr.addr);
                actuators.fan_state = 0;
            }
            key_debounce_cnt = 0;
            key_last_state = 0;
            break;

        default:
            key_last_state = 0;
            key_debounce_cnt = 0;
            break;
    }
}

/* ========================== UART 命令解析 (u2p_vision & 调试) ========================== */

void parse_uart_commands(void) {
    static uint8_t uart2_buf[128];
    static uint16_t uart2_len = 0;

    // 从USART2接收视觉数据
    int16_t recv = uart_rece_bytes(USART2, uart2_buf, sizeof(uart2_buf), 5);
    if (recv > 0) {
        uart2_buf[recv] = 0;

        // u2p_vision: "NAME:xxx SCORE:0.x"
        char* name_start = strstr((char*)uart2_buf, "NAME:");
        char* score_start = strstr((char*)uart2_buf, "SCORE:");
        if (name_start && score_start) {
            name_start += 5;
            score_start += 6;
            sscanf(name_start, "%31s", vision_name);
            vision_score = (float)atof(score_start);
            vision_data_ready = 1;
        }
    }

    // 从USART0接收调试命令
    static uint8_t uart0_buf[64];
    int16_t recv0 = uart_rece_bytes(USART0, uart0_buf, sizeof(uart0_buf), 5);
    if (recv0 > 0) {
        uart0_buf[recv0] = 0;

        // 调试命令: "TEMP?" "HUMI?" "LIGHT?" "SCORE?"
        if (strstr((char*)uart0_buf, "TEMP?")) {
            char resp[32];
            snprintf(resp, sizeof(resp), "TEMP:%.1f\r\n", current_env.temperature);
            uart_send_bytes(USART0, (uint8_t*)resp, strlen(resp));
        } else if (strstr((char*)uart0_buf, "HUMI?")) {
            char resp[32];
            snprintf(resp, sizeof(resp), "HUMI:%.1f\r\n", current_env.humidity);
            uart_send_bytes(USART0, (uint8_t*)resp, strlen(resp));
        } else if (strstr((char*)uart0_buf, "LIGHT?")) {
            char resp[32];
            snprintf(resp, sizeof(resp), "LIGHT:%.1f\r\n", current_env.light);
            uart_send_bytes(USART0, (uint8_t*)resp, strlen(resp));
        } else if (strstr((char*)uart0_buf, "SCORE?")) {
            char resp[64];
            snprintf(resp, sizeof(resp), "SCORE:%d T:%d H:%d L:%d\r\n",
                     current_env.score, current_env.temp_score,
                     current_env.humi_score, current_env.light_score);
            uart_send_bytes(USART0, (uint8_t*)resp, strlen(resp));
        }
    }
}

/* ========================== 主循环 ========================== */

int main(void) {
    // 硬件初始化
    system_init();
    device_init();

    // 主循环标志
    while (1) {
        // 50ms Tick 由 SysTick/Timer3 中断触发
        if (sys_tick_50ms) {
            sys_tick_50ms = 0;
            sys_tick_1s++;

            // 按键扫描 (50ms周期)
            process_key();

            // UART 命令解析
            parse_uart_commands();

            // 1秒到 → 执行1哨任务
            if (sys_tick_1s >= 20) {
                sys_tick_1s = 0;
                task_1s_loop();
            }
        }
    }
}