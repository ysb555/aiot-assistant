#ifndef PLANT_LOGIC_H
#define PLANT_LOGIC_H

#include "gd32f4xx.h"
#include <stdint.h>

/* ==================== 植物档案 ==================== */
#define MAX_PLANTS                  3

typedef struct {
    uint8_t     id;                 // 植物编号 1~3
    char        name[20];           // 植物名称
    float       temp_min;           // 温度下限
    float       temp_max;           // 温度上限
    float       humi_min;           // 湿度下限
    float       humi_max;           // 湿度上限
    float       light_min;          // 光照下限
    float       light_max;          // 光照上限
    float       shade_high;         // 遮阳高阈值 (光照超过此值关闭窗帘)
    float       shade_low;          // 遮阳低阈值 (光照低于此值打开窗帘)
    float       vent_high;          // 通风高阈值 (湿度超过此值开启风扇)
    float       vent_low;           // 通风低阈值 (湿度低于此值关闭风扇)
    float       water_threshold;    // 浇水蒸发指数阈值
} plant_profile_t;

/* ==================== 传感器数据 ==================== */
typedef struct {
    float       temperature;        // 温度 (℃)
    float       humidity;           // 湿度 (%)
    float       light;              // 光照 (lux)
    uint16_t    distance;           // 超声波距离 (mm)
    uint8_t     human_present;      // 人体红外 (1=有人)
} sensor_data_t;

/* ==================== 评分系统 ==================== */
typedef struct {
    uint8_t     temp_score;         // 温度分 (0-100)
    uint8_t     humi_score;         // 湿度分 (0-100)
    uint8_t     light_score;        // 光照分 (0-100)
    uint8_t     total_score;        // 综合分 (0-100)
} score_system_t;

/* ==================== 执行器状态 ==================== */
typedef struct {
    uint8_t     curtain_pos;        // 窗帘位置 (0-100%)
    uint8_t     fan_speed;          // 风扇转速 (0-100%)
    uint8_t     rgb_r;              // RGB 红
    uint8_t     rgb_g;              // RGB 绿
    uint8_t     rgb_b;              // RGB 蓝
    uint8_t     auto_mode;          // 自动模式 (1=自动, 0=手动)
    uint8_t     fan_auto;           // 风扇自动 (1=自动)
    uint8_t     curtain_auto;       // 窗帘自动 (1=自动)
} actuator_state_t;

/* ==================== 系统状态 ==================== */
typedef struct {
    uint8_t         current_plant;          // 当前植物编号 (0~MAX_PLANTS-1)
    float           evaporation_index;      // 蒸发指数累加值
    uint32_t        water_cooldown_start;   // 浇水冷却起始时间
    uint8_t         water_cooldown;         // 浇水冷却标志
    sensor_data_t   sensor;
    score_system_t  score;
    actuator_state_t actuator;
    plant_profile_t plants[MAX_PLANTS];
    uint8_t         nfc_plant_id;           // NFC刷卡读取的植物ID
    uint8_t         nfc_card_detected;      // NFC检测到卡片
    uint8_t         zigbee_rx_buf[128];     // Zigbee接收缓冲区
    uint16_t        zigbee_rx_len;          // Zigbee接收长度
    uint8_t         vision_plant_id;        // 视觉识别植物ID
    uint8_t         vision_ready;           // 视觉识别结果就绪
} system_state_t;

/* ==================== 全局变量声明 ==================== */
extern system_state_t g_sys;

/* ==================== 函数声明 ==================== */

// 初始化
void plant_logic_init(void);
void plant_profiles_init(void);

// 传感器
void sensors_read(void);

// 评分
void score_calculate(void);
void score_to_rgb(uint8_t score, uint8_t *r, uint8_t *g, uint8_t *b);

// 自动控制
void auto_shade_control(void);
void auto_vent_control(void);
void auto_light_control(void);      // 根据人体红外自动灯光

// 蒸发与浇水
void evaporation_update(void);
void watering_check(void);

// 植物切换
void plant_switch(uint8_t plant_id);
void plant_switch_next(void);
void plant_switch_by_nfc(void);

// 显示更新
void display_update(void);
void rgb_update(void);

// 通信
void wifi_send_watering_alert(void);
void zigbee_send_status(void);
void zigbee_recv_process(void);
void vision_uart_process(void);

// 前端串口命令
void uart_command_parse(uint8_t *cmd);

// NFC 刷卡处理
void nfc_card_process(void);

#endif // PLANT_LOGIC_H