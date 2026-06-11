#include "../inc/plant_logic.h"
#include "../inc/main.h"
#include "e1.h"
#include "e2.h"
#include "c3.h"
#include <string.h>
#include <stdio.h>

/* ========================== 植物档案数据库 ========================== */
static const plant_profile_t profiles[PLANT_COUNT] = {
    // 绿萝: 喜阴湿, 温度18-28℃, 湿度50-70%, 光照200-500lux
    {
        .name                     = "绿萝",
        .plant_id                 = 1,
        .temp_min                 = 18.0f,
        .temp_max                 = 28.0f,
        .humi_min                 = 50.0f,
        .humi_max                 = 70.0f,
        .light_min                = 200.0f,
        .light_max                = 500.0f,
        .curtain_close_threshold  = 800.0f,   // 光照>800lux 关窗帘
        .curtain_open_threshold   = 400.0f,   // 光照<400lux 开窗帘
        .fan_on_threshold         = 75.0f,    // 湿度>75% 开风扇
        .fan_off_threshold        = 60.0f,    // 湿度<60% 关风扇
        .water_evap_threshold     = 500.0f
    },
    // 多肉: 喜光耐旱, 温度15-25℃, 湿度30-50%, 光照500-1000lux
    {
        .name                     = "多肉",
        .plant_id                 = 2,
        .temp_min                 = 15.0f,
        .temp_max                 = 25.0f,
        .humi_min                 = 30.0f,
        .humi_max                 = 50.0f,
        .light_min                = 500.0f,
        .light_max                = 1000.0f,
        .curtain_close_threshold  = 1200.0f,
        .curtain_open_threshold   = 600.0f,
        .fan_on_threshold         = 55.0f,
        .fan_off_threshold        = 40.0f,
        .water_evap_threshold     = 400.0f
    },
    // 发财树: 温暖湿润, 温度20-30℃, 湿度40-60%, 光照300-600lux
    {
        .name                     = "发财树",
        .plant_id                 = 3,
        .temp_min                 = 20.0f,
        .temp_max                 = 30.0f,
        .humi_min                 = 40.0f,
        .humi_max                 = 60.0f,
        .light_min                = 300.0f,
        .light_max                = 600.0f,
        .curtain_close_threshold  = 800.0f,
        .curtain_open_threshold   = 400.0f,
        .fan_on_threshold         = 68.0f,
        .fan_off_threshold        = 50.0f,
        .water_evap_threshold     = 550.0f
    }
};

/* ========================== 全局变量定义 ========================== */
plant_profile_t  current_profile;
env_data_t       current_env;
plant_type_t     current_plant_type = PLANT_GREEN_ROLO;
actuator_state_t actuators;
sys_ctrl_t       sys_ctrl;

/* ========================== 外部设备地址(由main.c初始化) ========================== */
extern i2c_addr_def s2_th_addr;      // SHT35 温湿度
extern i2c_addr_def s2_ss_addr;      // BH1750 光照
extern i2c_addr_def e1_rgb_addr;     // RGB灯
extern i2c_addr_def e1_nixie_addr;   // 数码管
extern i2c_addr_def e2_fan_addr;     // 风扇
extern i2c_addr_def s1_key_addr;     // 按键

/* ========================== 初始化 ========================== */

void plant_logic_init(void) {
    current_profile = profiles[current_plant_type];
    memset(&current_env, 0, sizeof(env_data_t));
    memset(&actuators, 0, sizeof(actuator_state_t));

    // 默认控制标志
    sys_ctrl.auto_curtain_enabled     = 1;
    sys_ctrl.auto_fan_enabled         = 1;
    sys_ctrl.auto_water_alert_enabled = 1;
    sys_ctrl.u2p_vision_enabled       = 1;
    sys_ctrl.display_mode             = 0;  // 默认显示评分

    actuators.fan_speed_low  = 50;
    actuators.fan_speed_high = 90;
}

/* ========================== 植物档案查询 ========================== */

const plant_profile_t* get_profile_by_type(plant_type_t type) {
    if (type >= PLANT_COUNT) return &profiles[0];
    return &profiles[type];
}

const plant_profile_t* get_profile_by_name(const char* name) {
    for (int i = 0; i < PLANT_COUNT; i++) {
        if (strcmp(profiles[i].name, name) == 0) {
            return &profiles[i];
        }
    }
    return NULL;
}

/* ========================== 植物切换 ========================== */

void switch_plant(void) {
    current_plant_type = (current_plant_type + 1) % PLANT_COUNT;
    current_profile = profiles[current_plant_type];

    // 切换后重置执行器状态（让回差逻辑重新判断）
    actuators.curtain_state = 0;
    actuators.fan_state = 0;

    // 显示植物编号
    display_plant_id();
    delay_ms(500);

    // RGB闪烁2次
    rgb_blink_twice();
}

/* ========================== 综合评分计算 ========================== */

void calc_score(void) {
    float temp_score  = 100.0f;
    float humi_score  = 100.0f;
    float light_score = 100.0f;

    // 温度评分: 适宜区间内100分, 超出每单位扣10分
    if (current_env.temperature < current_profile.temp_min) {
        temp_score -= (current_profile.temp_min - current_env.temperature) * 10.0f;
    } else if (current_env.temperature > current_profile.temp_max) {
        temp_score -= (current_env.temperature - current_profile.temp_max) * 10.0f;
    }

    // 湿度评分: 适宜区间内100分, 超出每单位扣2分
    if (current_env.humidity < current_profile.humi_min) {
        humi_score -= (current_profile.humi_min - current_env.humidity) * 2.0f;
    } else if (current_env.humidity > current_profile.humi_max) {
        humi_score -= (current_env.humidity - current_profile.humi_max) * 2.0f;
    }

    // 光照评分: 适宜区间内100分, 超出每2单位扣1分
    if (current_env.light < current_profile.light_min) {
        light_score -= (current_profile.light_min - current_env.light) * 0.5f;
    } else if (current_env.light > current_profile.light_max) {
        light_score -= (current_env.light - current_profile.light_max) * 0.5f;
    }

    // 限制最低分0
    if (temp_score  < 0) temp_score  = 0;
    if (humi_score  < 0) humi_score  = 0;
    if (light_score < 0) light_score = 0;

    current_env.temp_score  = (int)temp_score;
    current_env.humi_score  = (int)humi_score;
    current_env.light_score = (int)light_score;

    // 综合评分 = 温度×0.3 + 湿度×0.4 + 光照×0.3
    current_env.score = (int)(temp_score * 0.3f + humi_score * 0.4f + light_score * 0.3f);
    if (current_env.score > 100) current_env.score = 100;

    // 评分等级
    if (current_env.score >= 90)      current_env.level = SCORE_GREEN;
    else if (current_env.score >= 70) current_env.level = SCORE_YELLOW;
    else if (current_env.score >= 50) current_env.level = SCORE_ORANGE;
    else                              current_env.level = SCORE_RED;
}

/* ========================== 蒸发指数与浇水提醒 ========================== */

void update_evaporation(void) {
    // 蒸发增量 = 温度×0.3 + 光照×0.5 - 湿度×0.2
    current_env.evaporation_index = current_env.temperature * 0.3f
                                  + current_env.light * 0.5f
                                  - current_env.humidity * 0.2f;

    if (current_env.evaporation_index < 0) {
        current_env.evaporation_index = 0;
    }

    // 冷却期内不累加
    if (current_env.water_alert_cooldown == 0) {
        current_env.evaporation_sum += current_env.evaporation_index;
    } else {
        current_env.water_alert_cooldown--;
    }
}

void check_water_alert(void) {
    if (!sys_ctrl.auto_water_alert_enabled) {
        current_env.water_alert = 0;
        return;
    }

    if (current_env.evaporation_sum >= current_profile.water_evap_threshold) {
        current_env.water_alert = 1;
        current_env.evaporation_sum = 0;
        current_env.water_alert_cooldown = 3600;  // 1小时冷却

        // 通过WiFi发送浇水提醒
        c3_send_water_alert(current_profile.name);
    } else {
        current_env.water_alert = 0;
    }
}

/* ========================== 自动遮阳 (回差控制) ========================== */

void auto_curtain(void) {
    if (!sys_ctrl.auto_curtain_enabled) return;

    if (current_env.light > current_profile.curtain_close_threshold) {
        // 光照超过高阈值 → 关闭窗帘
        actuators.curtain_state = 1;
        // TODO: 实际硬件驱动 set_curtain(100)
    } else if (current_env.light < current_profile.curtain_open_threshold) {
        // 光照低于低阈值 → 打开窗帘
        actuators.curtain_state = 0;
        // TODO: 实际硬件驱动 set_curtain(0)
    }
    // 介于高低阈值之间 → 保持当前位置 (回差)
}

/* ========================== 自动通风 (回差控制) ========================== */

void auto_fan(void) {
    if (!sys_ctrl.auto_fan_enabled) return;

    if (current_env.humidity > current_profile.fan_on_threshold) {
        // 湿度超过高阈值 → 开风扇高速
        actuators.fan_state = 2;
        if (e2_fan_addr.flag) {
            e2_speed_control(e2_fan_addr.periph, e2_fan_addr.addr, actuators.fan_speed_high);
        }
    } else if (current_env.humidity < current_profile.fan_off_threshold) {
        // 湿度低于低阈值 → 关风扇
        actuators.fan_state = 0;
        if (e2_fan_addr.flag) {
            e2_fan_off(e2_fan_addr.periph, e2_fan_addr.addr);
        }
    }
    // 介于中间 → 保持当前状态 (回差)
}

/* ========================== 视觉融合决策 ========================== */

void u1p_fusion_decision(char* name, float score) {
    if (score < 0.5f) return;  // 置信度过低, 不动作

    const plant_profile_t* recognized = get_profile_by_name(name);
    if (recognized == NULL) return;  // 不在预置植物库中

    // 如果识别结果与当前植物档案不同
    if (recognized != &current_profile) {
        // 高置信度匹配时, 可考虑自动切换(需要用户确认, 这里仅记录)
        if (score > 0.8f) {
            printf("U2P: 检测到 %s (置信度: %.2f), 建议切换植物档案\r\n", name, score);
            // 自动切换需谨慎, 此处暂不自动切换
        }
    }

    // 基于视觉结果的微调
    int is_shade_loving = (strcmp(name, "绿萝") == 0 || strcmp(name, "发财树") == 0);
    int is_light_loving = (strcmp(name, "多肉") == 0);

    if (score > 0.7f) {
        if (is_shade_loving && current_env.light > 800.0f) {
            actuators.curtain_state = 1;
            // TODO: set_curtain(100)
        } else if (is_light_loving && current_env.light < 300.0f) {
            actuators.curtain_state = 0;
            // TODO: set_curtain(0)
        }
    }
}

/* ========================== RGB 颜色控制 ========================== */

// 评分->RGB颜色映射
static void score_to_rgb(score_level_t level, uint8_t* r, uint8_t* g, uint8_t* b) {
    switch (level) {
        case SCORE_GREEN:  *r = 0;   *g = 255; *b = 0;   break;  // 绿色
        case SCORE_YELLOW: *r = 255; *g = 255; *b = 0;   break;  // 黄色
        case SCORE_ORANGE: *r = 255; *g = 165; *b = 0;   break;  // 橙色
        case SCORE_RED:    *r = 255; *g = 0;   *b = 0;   break;  // 红色
        default:           *r = 0;   *g = 0;   *b = 0;   break;
    }
}

void set_rgb_by_score(int score) {
    score_level_t level;
    if (score >= 90)      level = SCORE_GREEN;
    else if (score >= 70) level = SCORE_YELLOW;
    else if (score >= 50) level = SCORE_ORANGE;
    else                  level = SCORE_RED;

    uint8_t r, g, b;
    score_to_rgb(level, &r, &g, &b);
    set_rgb_color(r, g, b);
}

void set_rgb_color(uint8_t r, uint8_t g, uint8_t b) {
    if (e1_rgb_addr.flag) {
        e1_rgb_set(e1_rgb_addr.periph, e1_rgb_addr.addr, r, g, b);
    }
}

void rgb_blink_twice(void) {
    for (int i = 0; i < 2; i++) {
        set_rgb_color(0, 0, 0);
        delay_ms(200);
        set_rgb_by_score(current_env.score);
        delay_ms(200);
    }
}

/* ========================== 数码管显示 ========================== */

void display_score(void) {
    if (!e1_nixie_addr.flag) return;
    uint8_t dp[4] = {0, 0, 0, 0};
    int s = current_env.score;
    uint8_t d1 = (s >= 100) ? 1 : ((s / 100) % 10);
    uint8_t d2 = (s / 10) % 10;
    uint8_t d3 = s % 10;
    uint8_t d4 = NODIS;
    if (s < 100) d1 = NODIS;
    if (s < 10)  d2 = NODIS;
    e1_nixie_display(e1_nixie_addr.periph, e1_nixie_addr.addr, d1, d2, d3, d4, dp);
}

void display_temperature(void) {
    if (!e1_nixie_addr.flag) return;
    uint8_t dp[4] = {0, 1, 0, 0};
    uint8_t dig[4];
    float t = current_env.temperature;

    int ip = (int)t;
    int fp = (int)((t - ip) * 10 + 0.5f);

    dig[0] = (ip >= 100) ? (ip / 100) % 10 : NODIS;
    dig[1] = (ip >= 10)  ? (ip / 10) % 10  : ((ip > 0) ? ip : NODIS);
    dig[2] = fp % 10;
    dig[3] = 12;  // 'C' for Celsius (0x0C = C)
    if (t < 10) {
        dig[0] = NODIS;
        dig[1] = ip % 10;
    }
    e1_nixie_display(e1_nixie_addr.periph, e1_nixie_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

void display_humidity(void) {
    if (!e1_nixie_addr.flag) return;
    uint8_t dp[4] = {0, 0, 0, 0};
    uint8_t dig[4];
    int h = (int)(current_env.humidity + 0.5f);

    dig[0] = (h >= 100) ? (h / 100) % 10 : NODIS;
    dig[1] = (h / 10) % 10;
    dig[2] = h % 10;
    dig[3] = 17;  // 'H' (用空位代替h)
    // 用 0x10=NODIS 替代 h, 简化
    if (h < 100) dig[0] = NODIS;
    e1_nixie_display(e1_nixie_addr.periph, e1_nixie_addr.addr, dig[0], dig[1], dig[2], NODIS, dp);
}

void display_light(void) {
    if (!e1_nixie_addr.flag) return;
    uint8_t dp[4] = {0, 0, 0, 0};
    uint8_t dig[4];
    int l = (int)(current_env.light + 0.5f);

    dig[0] = (l >= 1000) ? (l / 1000) % 10 : NODIS;
    dig[1] = (l / 100) % 10;
    dig[2] = (l / 10) % 10;
    dig[3] = l % 10;
    if (l < 1000 && l >= 100) { dig[0] = NODIS; }
    if (l < 100)  { dig[0] = NODIS; dig[1] = dig[2]; dig[2] = dig[3]; dig[3] = NODIS; }
    e1_nixie_display(e1_nixie_addr.periph, e1_nixie_addr.addr, dig[0], dig[1], dig[2], dig[3], dp);
}

void display_plant_id(void) {
    if (!e1_nixie_addr.flag) return;
    uint8_t dp[4] = {0, 0, 0, 0};
    uint8_t pid = current_profile.plant_id;
    e1_nixie_display(e1_nixie_addr.periph, e1_nixie_addr.addr, pid, NODIS, NODIS, NODIS, dp);
}

/* ========================== 显示辅助函数 ========================== */

void display_temperature_ui(void) {
    display_temperature();
    set_rgb_color(0, 60, 60);  // 青色表示温度
}

void display_humidity_ui(void) {
    display_humidity();
    set_rgb_color(40, 0, 60);  // 紫色表示湿度
}

void get_dp(float num, uint8_t* dp) {
    memset(dp, 0, 4);
    if (num > 0 && num < 10.0f)      dp[0] = 1;
    else if (num >= 10.0f && num < 100.0f)  dp[1] = 1;
    else if (num >= 100.0f && num < 1000.0f) dp[2] = 1;
    else                              dp[3] = 1;
}

void get_first_four_digits(float number, uint8_t* dig) {
    for (int k = 0; k < 4; k++) dig[k] = 0;

    int ip = (int)number;
    float fp = number - ip;

    int temp = ip, i = 0;
    while (temp > 0) { temp /= 10; i++; }
    if (i == 0) i = 1;

    int j = i;
    while (j > 0) { dig[j - 1] = ip % 10; ip /= 10; j--; }

    while (i < 4) {
        fp *= 10;
        dig[i] = (int)fp;
        fp -= (int)fp;
        i++;
    }
    if (fp >= 0.5f) {
        if (dig[3] < 9) dig[3]++;
    }
}