#ifndef MAIN_H
#define MAIN_H

#include "../inc/plant_logic.h"
#include "i2c.h"

// 全局设备地址 (在 main.c 中定义, 由其他模块 extern)
extern i2c_addr_def s2_th_addr;
extern i2c_addr_def s2_ss_addr;
extern i2c_addr_def e1_rgb_addr;
extern i2c_addr_def e1_nixie_addr;
extern i2c_addr_def e2_fan_addr;
extern i2c_addr_def s1_key_addr;
extern i2c_addr_def u2p_vision_addr;

// 系统tick标志
extern volatile uint32_t sys_tick_1s;
extern volatile uint32_t sys_tick_50ms;

// 视觉融合变量
extern char vision_name[32];
extern float vision_score;
extern uint8_t vision_data_ready;

// WiFi状态
extern int wifi_run_state;
extern int tcp_status;

// 初始化
void system_init(void);
void device_init(void);
void rcu_config(void);

// 任务调度
void task_1s_loop(void);

// 显示更新
void update_display(void);

// 按键处理
void process_key(void);

// UART命令解析
void parse_uart_commands(void);

#endif // MAIN_H