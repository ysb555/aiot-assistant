#include "plant_logic.h"
#include "s2.h"
#include "s5.h"
#include "s6.h"
#include "s7.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "e4.h"
#include "c2.h"
#include "c3.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>

/* ==================== 全局系统状态 ==================== */
system_state_t g_sys;

/* ==================== I2C 地址 (全局唯一实例) ==================== */
// S2 三合一传感器板: th_addr(SHT35温湿度) + ss_addr(BH1750光照) + ax_addr(ICM20608)
static s2_addr_def g_s2_addr;
static i2c_addr_def g_s5_addr;      // MS523 NFC (S5)
static i2c_addr_def g_s6_addr;      // 超声波 (S6)
static i2c_addr_def g_s7_addr;      // 人体红外 (S7)
static i2c_addr_def g_e1_rgb_addr;  // E1 RGB灯
static i2c_addr_def g_e1_nix_addr;  // E1 数码管
static i2c_addr_def g_e2_addr;      // E2 风扇
static i2c_addr_def g_e3_addr;      // E3 窗帘电机
static i2c_addr_def g_s1_key_addr;  // S1 矩阵按键

/* ==================== 初始化 ==================== */

void plant_profiles_init(void) {
    // 植物1: 绿萝
    g_sys.plants[0].id = 1;
    strcpy(g_sys.plants[0].name, "绿萝");
    g_sys.plants[0].temp_min = 18;
    g_sys.plants[0].temp_max = 28;
    g_sys.plants[0].humi_min = 50;
    g_sys.plants[0].humi_max = 80;
    g_sys.plants[0].light_min = 500;
    g_sys.plants[0].light_max = 15000;
    g_sys.plants[0].shade_high = 20000;
    g_sys.plants[0].shade_low = 8000;
    g_sys.plants[0].vent_high = 80;
    g_sys.plants[0].vent_low = 60;
    g_sys.plants[0].water_threshold = 500;

    // 植物2: 多肉
    g_sys.plants[1].id = 2;
    strcpy(g_sys.plants[1].name, "多肉");
    g_sys.plants[1].temp_min = 15;
    g_sys.plants[1].temp_max = 30;
    g_sys.plants[1].humi_min = 30;
    g_sys.plants[1].humi_max = 60;
    g_sys.plants[1].light_min = 2000;
    g_sys.plants[1].light_max = 30000;
    g_sys.plants[1].shade_high = 35000;
    g_sys.plants[1].shade_low = 15000;
    g_sys.plants[1].vent_high = 65;
    g_sys.plants[1].vent_low = 45;
    g_sys.plants[1].water_threshold = 800;

    // 植物3: 发财树
    g_sys.plants[2].id = 3;
    strcpy(g_sys.plants[2].name, "发财树");
    g_sys.plants[2].temp_min = 20;
    g_sys.plants[2].temp_max = 32;
    g_sys.plants[2].humi_min = 40;
    g_sys.plants[2].humi_max = 75;
    g_sys.plants[2].light_min = 1000;
    g_sys.plants[2].light_max = 25000;
    g_sys.plants[2].shade_high = 28000;
    g_sys.plants[2].shade_low = 12000;
    g_sys.plants[2].vent_high = 75;
    g_sys.plants[2].vent_low = 55;
    g_sys.plants[2].water_threshold = 600;
}

void plant_logic_init(void) {
    // 初始化植物档案
    plant_profiles_init();

    // 系统默认状态
    g_sys.current_plant = 0;            // 默认绿萝
    g_sys.evaporation_index = 0;
    g_sys.water_cooldown = 0;
    g_sys.actuator.auto_mode = 1;       // 默认自动模式
    g_sys.actuator.fan_auto = 1;
    g_sys.actuator.curtain_auto = 1;
    g_sys.actuator.curtain_pos = 0;
    g_sys.actuator.fan_speed = 0;
    g_sys.actuator.rgb_r = 0;
    g_sys.actuator.rgb_g = 255;
    g_sys.actuator.rgb_b = 0;
    g_sys.nfc_card_detected = 0;
    g_sys.vision_ready = 0;

    // 初始化所有硬件
    // S2: 三合一传感器板 (SHT35温湿度 + BH1750光照 + ICM20608 6轴)
    s2_all_init(&g_s2_addr, TH_ADDRESS_S2, S1_ADDRESS_S2, AX_ADDRESS_S2);

    // S5: NFC (MS523)
    g_s5_addr = get_board_address(MS523_ADDRESS_S5);

    // S6: 超声波
    g_s6_addr = get_board_address(GD32F330_ADDRESS_S6);

    // S7: 人体红外 (PCA9557)
    g_s7_addr = get_board_address(PCA9557_ADDRESS_S7);

    // E1: RGB灯 + 数码管
    g_e1_rgb_addr = get_board_address(PCA9685_ADDRESS_E1);
    g_e1_nix_addr = get_board_address(HT16K33_ADDRESS_E1);

    // E2: 风扇
    g_e2_addr = get_board_address(PCA9685_ADDRESS_E2);

    // E3: 窗帘电机
    g_e3_addr = e3_init(CURTAIN_ADDRESS_E3);

    // E4: 扬声器 (可选)
    wm8978_init(WM8978_ADDRESS_E4);

    // S1: 矩阵按键
    g_s1_key_addr = get_board_address(HT16K33_KEY_ADDRESS_S1);

    // 初始化 RGB
    if (g_e1_rgb_addr.flag) {
        e1_rgb_init(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr);
    }

    // 初始化数码管
    if (g_e1_nix_addr.flag) {
        e1_nixie_init(g_e1_nix_addr.periph, g_e1_nix_addr.addr);
    }

    // 初始化风扇
    if (g_e2_addr.flag) {
        e2_fan_init(g_e2_addr.periph, g_e2_addr.addr);
    }

    // 初始化按键
    if (g_s1_key_addr.flag) {
        s1_key_init(g_s1_key_addr.periph, g_s1_key_addr.addr);
    }

    // 初始化 C2 Zigbee
    c2_init(C2_TERMINAL);

    // 初始化 C3 WiFi (在 main.c 中调用)
    // c3_init() 已在 device_init() 中调用

    // 播放初始化提示音
    wm8978_beep(200);
}

/* ==================== 传感器读取 ==================== */

void sensors_read(void) {
    // 读取温湿度 (S2板上的SHT35)
    if (g_s2_addr.th_addr.flag) {
        s2_get_temp_humi(g_s2_addr.th_addr.periph, g_s2_addr.th_addr.addr,
                         &g_sys.sensor.temperature,
                         &g_sys.sensor.humidity);
    }

    // 读取光照 (S2板上的BH1750)
    if (g_s2_addr.ss_addr.flag) {
        g_sys.sensor.light = s2_read_bh1750_value(g_s2_addr.ss_addr.periph,
                                                   g_s2_addr.ss_addr.addr);
    }

    // 读取超声波距离 (S6)
    if (g_s6_addr.flag) {
        s6_read_distance(g_s6_addr.periph, g_s6_addr.addr,
                         &g_sys.sensor.distance);
    }

    // 读取人体红外 (S7)
    if (g_s7_addr.flag) {
        g_sys.sensor.human_present = s7_get_status(g_s7_addr.periph,
                                                    g_s7_addr.addr);
    }
}

/* ==================== 评分系统 ==================== */

void score_calculate(void) {
    plant_profile_t *p = &g_sys.plants[g_sys.current_plant];

    // 温度分
    if (g_sys.sensor.temperature >= p->temp_min && g_sys.sensor.temperature <= p->temp_max) {
        g_sys.score.temp_score = 100;
    } else if (g_sys.sensor.temperature < p->temp_min) {
        int diff = (int)(p->temp_min - g_sys.sensor.temperature);
        int s = 100 - diff * 10;
        g_sys.score.temp_score = (s > 0) ? (uint8_t)s : 0;
    } else {
        int diff = (int)(g_sys.sensor.temperature - p->temp_max);
        int s = 100 - diff * 10;
        g_sys.score.temp_score = (s > 0) ? (uint8_t)s : 0;
    }

    // 湿度分
    if (g_sys.sensor.humidity >= p->humi_min && g_sys.sensor.humidity <= p->humi_max) {
        g_sys.score.humi_score = 100;
    } else if (g_sys.sensor.humidity < p->humi_min) {
        int diff = (int)(p->humi_min - g_sys.sensor.humidity);
        int s = 100 - diff * 2;
        g_sys.score.humi_score = (s > 0) ? (uint8_t)s : 0;
    } else {
        int diff = (int)(g_sys.sensor.humidity - p->humi_max);
        int s = 100 - diff * 2;
        g_sys.score.humi_score = (s > 0) ? (uint8_t)s : 0;
    }

    // 光照分
    if (g_sys.sensor.light >= p->light_min && g_sys.sensor.light <= p->light_max) {
        g_sys.score.light_score = 100;
    } else if (g_sys.sensor.light < p->light_min) {
        int diff = (int)((p->light_min - g_sys.sensor.light) / 2);
        int s = 100 - diff;
        g_sys.score.light_score = (s > 0) ? (uint8_t)s : 0;
    } else {
        int diff = (int)((g_sys.sensor.light - p->light_max) / 2);
        int s = 100 - diff;
        g_sys.score.light_score = (s > 0) ? (uint8_t)s : 0;
    }

    // 综合分
    g_sys.score.total_score = (uint8_t)(
        g_sys.score.temp_score * 0.3f +
        g_sys.score.humi_score * 0.4f +
        g_sys.score.light_score * 0.3f
    );
}

void score_to_rgb(uint8_t score, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (score >= 90)      { *r = 0;   *g = 255; *b = 0;   }  // 绿色
    else if (score >= 70) { *r = 255; *g = 255; *b = 0;   }  // 黄色
    else if (score >= 50) { *r = 255; *g = 165; *b = 0;   }  // 橙色
    else                  { *r = 255; *g = 0;   *b = 0;   }  // 红色
}

/* ==================== 自动控制 ==================== */

void auto_shade_control(void) {
    if (!g_sys.actuator.curtain_auto) return;
    if (!g_e3_addr.flag) return;

    plant_profile_t *p = &g_sys.plants[g_sys.current_plant];
    float light = g_sys.sensor.light;

    if (light >= p->shade_high) {
        // 光照过高，关闭窗帘
        e3_set_position(g_e3_addr.periph, g_e3_addr.addr, 100);
        g_sys.actuator.curtain_pos = 100;
    } else if (light <= p->shade_low) {
        // 光照不足，打开窗帘
        e3_set_position(g_e3_addr.periph, g_e3_addr.addr, 0);
        g_sys.actuator.curtain_pos = 0;
    }
    // 中间区域保持当前状态 (回差)
}

void auto_vent_control(void) {
    if (!g_sys.actuator.fan_auto) return;
    if (!g_e2_addr.flag) return;

    plant_profile_t *p = &g_sys.plants[g_sys.current_plant];
    float humi = g_sys.sensor.humidity;

    if (humi >= p->vent_high) {
        // 湿度过高，开启风扇
        e2_speed_control(g_e2_addr.periph, g_e2_addr.addr, 100);
        g_sys.actuator.fan_speed = 100;
    } else if (humi <= p->vent_low) {
        // 湿度降低，关闭风扇
        e2_speed_control(g_e2_addr.periph, g_e2_addr.addr, 0);
        g_sys.actuator.fan_speed = 0;
    }
    // 中间区域保持当前状态 (回差)
}

void auto_light_control(void) {
    // 根据人体红外自动调节灯光
    if (!g_e1_rgb_addr.flag) return;

    if (g_sys.sensor.human_present) {
        // 有人时加大亮度
        score_to_rgb(g_sys.score.total_score,
                     &g_sys.actuator.rgb_r,
                     &g_sys.actuator.rgb_g,
                     &g_sys.actuator.rgb_b);
        e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr,
                   g_sys.actuator.rgb_r,
                   g_sys.actuator.rgb_g,
                   g_sys.actuator.rgb_b);
    }
}

/* ==================== 蒸发指数与浇水提醒 ==================== */

void evaporation_update(void) {
    // 蒸发增量 = 温度×0.3 + 光照×0.5 - 湿度×0.2
    float inc = g_sys.sensor.temperature * 0.3f +
                g_sys.sensor.light * 0.5f -
                g_sys.sensor.humidity * 0.2f;
    if (inc < 0) inc = 0;
    g_sys.evaporation_index += inc;
}

void watering_check(void) {
    if (g_sys.water_cooldown) {
        // 冷却期检查 (1小时)
        if (get_tick() - g_sys.water_cooldown_start >= 3600000) {
            g_sys.water_cooldown = 0;
        }
        return;
    }

    plant_profile_t *p = &g_sys.plants[g_sys.current_plant];

    if (g_sys.evaporation_index >= p->water_threshold) {
        // 触发浇水提醒
        wifi_send_watering_alert();

        // 蜂鸣提示
        wm8978_beep(500);

        // 通过 Zigbee 广播浇水提醒
        char zigbee_msg[64];
        sprintf(zigbee_msg, "WATER:Evap=%.0f,Plant=%s",
                g_sys.evaporation_index, p->name);
        c2_broadcast_data(zigbee_msg, 1);

        // 蒸发指数清零
        g_sys.evaporation_index = 0;

        // 进入冷却期
        g_sys.water_cooldown = 1;
        g_sys.water_cooldown_start = get_tick();
    }
}

/* ==================== 植物档案切换 ==================== */

void plant_switch(uint8_t plant_id) {
    if (plant_id >= MAX_PLANTS) return;

    g_sys.current_plant = plant_id;
    g_sys.evaporation_index = 0;
    g_sys.water_cooldown = 0;

    // 闪烁提示
    if (g_e1_rgb_addr.flag) {
        e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr, 0, 0, 0);
        delay_1ms(200);
        e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr, 0, 255, 0);
        delay_1ms(200);
        e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr, 0, 0, 0);
        delay_1ms(200);
        e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr, 0, 255, 0);
    }

    // 蜂鸣确认
    wm8978_beep(150);

    // Zigbee 广播切换信息
    char msg[64];
    sprintf(msg, "PLANT:%d,%s", plant_id + 1, g_sys.plants[plant_id].name);
    c2_broadcast_data(msg, 1);
}

void plant_switch_next(void) {
    uint8_t next = (g_sys.current_plant + 1) % MAX_PLANTS;
    plant_switch(next);
}

void plant_switch_by_nfc(void) {
    if (!g_sys.nfc_card_detected) return;
    if (g_sys.nfc_plant_id >= 1 && g_sys.nfc_plant_id <= MAX_PLANTS) {
        plant_switch(g_sys.nfc_plant_id - 1);
    }
    g_sys.nfc_card_detected = 0;
}

/* ==================== 显示更新 ==================== */

void display_update(void) {
    if (!g_e1_nix_addr.flag) return;

    uint8_t score = g_sys.score.total_score;
    uint8_t d1, d2, d3, d4;
    uint8_t dp[4] = {0, 0, 0, 0};

    // 显示格式: P1 85 (植物编号 + 综合分)
    // 第1位: 植物编号
    d1 = g_sys.plants[g_sys.current_plant].id;
    // 第2位: 空
    d2 = 16;  // 全灭
    // 第3位: 十位
    d3 = score / 10;
    // 第4位: 个位
    d4 = score % 10;

    e1_nixie_display(g_e1_nix_addr.periph, g_e1_nix_addr.addr, d1, d2, d3, d4, dp);
}

void rgb_update(void) {
    if (!g_e1_rgb_addr.flag) return;

    score_to_rgb(g_sys.score.total_score,
                 &g_sys.actuator.rgb_r,
                 &g_sys.actuator.rgb_g,
                 &g_sys.actuator.rgb_b);

    e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr,
               g_sys.actuator.rgb_r,
               g_sys.actuator.rgb_g,
               g_sys.actuator.rgb_b);
}

/* ==================== 通信 ==================== */

void wifi_send_watering_alert(void) {
    // 通过 C3 WiFi 发送浇水提醒 HTTP GET 请求
    char query[200];
    sprintf(query,
        "plant=%s&temp=%.1f&humi=%.1f&light=%.0f&evap=%.0f",
        g_sys.plants[g_sys.current_plant].name,
        g_sys.sensor.temperature,
        g_sys.sensor.humidity,
        g_sys.sensor.light,
        g_sys.evaporation_index
    );

    c3_send_http_get(WATER_ALERT_PATH, query);
}

void zigbee_send_status(void) {
    char msg[128];
    sprintf(msg,
        "{\"plant\":\"%s\",\"temp\":%.1f,\"humi\":%.1f,\"light\":%.0f,"
        "\"score\":%d,\"distance\":%d,\"human\":%d,"
        "\"curtain\":%d,\"fan\":%d}",
        g_sys.plants[g_sys.current_plant].name,
        g_sys.sensor.temperature,
        g_sys.sensor.humidity,
        g_sys.sensor.light,
        g_sys.score.total_score,
        g_sys.sensor.distance,
        g_sys.sensor.human_present,
        g_sys.actuator.curtain_pos,
        g_sys.actuator.fan_speed
    );
    c2_broadcast_data(msg, 0);  // 透传模式
}

void zigbee_recv_process(void) {
    uint8_t len = c2_rec_data(g_sys.zigbee_rx_buf, 128, 10);
    if (len > 0) {
        g_sys.zigbee_rx_len = len;
        // 处理接收到的 Zigbee 命令
        uart_command_parse(g_sys.zigbee_rx_buf);
    }
}

void vision_uart_process(void) {
    // 通过 USART1 接收 u2p_vision 的识别结果
    // 格式: "VISION:plant_id,plant_name"
    // 这里由 main.c 中的 UART 接收中断处理，此处做协议解析
    if (g_sys.vision_ready) {
        if (g_sys.vision_plant_id >= 1 && g_sys.vision_plant_id <= MAX_PLANTS) {
            plant_switch(g_sys.vision_plant_id - 1);
        }
        g_sys.vision_ready = 0;
    }
}

/* ==================== NFC 刷卡处理 ==================== */

void nfc_card_process(void) {
    if (!g_s5_addr.flag) return;

    uint8_t card_type = 0;
    uint8_t card_id[4] = {0};

    // 寻卡
    if (s5_pcd_request(g_s5_addr.periph, g_s5_addr.addr, PICC_REQALL, &card_type)) {
        // 防冲突
        if (s5_pcd_anticoll(g_s5_addr.periph, g_s5_addr.addr, card_id)) {
            // 选卡
            if (s5_pcd_select(g_s5_addr.periph, g_s5_addr.addr, card_id)) {
                // 读取块0数据 (植物ID存储在块0)
                uint8_t data[16] = {0};
                if (s5_card_read(g_s5_addr.periph, g_s5_addr.addr, card_id, 0, data)) {
                    g_sys.nfc_plant_id = data[0];  // 第一个字节为植物ID
                    g_sys.nfc_card_detected = 1;
                    plant_switch_by_nfc();
                }
            }
        }
    }
}

/* ==================== 串口命令解析 (前端控制) ==================== */
/**
 * 命令格式: "CMD:arg1,arg2,..."
 * 支持的命令:
 *   TEMP?          - 查询温度
 *   HUMI?          - 查询湿度
 *   LIGHT?         - 查询光照
 *   SCORE?         - 查询评分
 *   STATUS?        - 查询全部状态
 *   PLANT?         - 查询当前植物
 *   PLANT:N        - 切换到植物N (N=1,2,3)
 *   PLANT:NEXT     - 切换到下一个植物
 *   CURTAIN:OPEN   - 手动打开窗帘
 *   CURTAIN:CLOSE  - 手动关闭窗帘
 *   CURTAIN:POS,N  - 设置窗帘位置N%
 *   CURTAIN:AUTO   - 恢复窗帘自动模式
 *   FAN:ON         - 手动开启风扇
 *   FAN:OFF        - 手动关闭风扇
 *   FAN:SPEED,N    - 设置风扇转速N%
 *   FAN:AUTO       - 恢复风扇自动模式
 *   AUTO:ON        - 开启全自动模式
 *   AUTO:OFF       - 关闭全自动模式(手动)
 *   RGB:R,G,B      - 手动设置RGB颜色
 *   BEEP           - 蜂鸣测试
 *   DISTANCE?      - 查询超声波距离
 *   HUMAN?         - 查询人体红外状态
 *   ZIGBEE:MSG     - 通过Zigbee发送消息
 *   HELP           - 显示帮助
 */
void uart_command_parse(uint8_t *cmd) {
    char cmd_str[128];
    uint8_t len = strlen((char*)cmd);
    if (len > 127) len = 127;
    memcpy(cmd_str, cmd, len);
    cmd_str[len] = '\0';

    char response[256];

    if (strncmp(cmd_str, "TEMP?", 5) == 0) {
        sprintf(response, "TEMP:%.1f C\r\n", g_sys.sensor.temperature);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "HUMI?", 5) == 0) {
        sprintf(response, "HUMI:%.1f %%\r\n", g_sys.sensor.humidity);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "LIGHT?", 6) == 0) {
        sprintf(response, "LIGHT:%.0f lux\r\n", g_sys.sensor.light);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "SCORE?", 6) == 0) {
        sprintf(response, "SCORE:T=%d H=%d L=%d Total=%d\r\n",
                g_sys.score.temp_score, g_sys.score.humi_score,
                g_sys.score.light_score, g_sys.score.total_score);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "STATUS?", 7) == 0) {
        plant_profile_t *p = &g_sys.plants[g_sys.current_plant];
        sprintf(response,
            "=== PLANT ASSISTANT STATUS ===\r\n"
            "Plant: %s (ID=%d)\r\n"
            "Temp: %.1f C [%d-%d] Score:%d\r\n"
            "Humi: %.1f %% [%d-%d] Score:%d\r\n"
            "Light: %.0f lux [%d-%d] Score:%d\r\n"
            "Total Score: %d/100\r\n"
            "Evaporation: %.1f / %.0f\r\n"
            "Curtain: %d%% (%s)\r\n"
            "Fan: %d%% (%s)\r\n"
            "Distance: %d mm\r\n"
            "Human: %s\r\n"
            "Auto Mode: %s\r\n"
            "Cooldown: %s\r\n"
            "===============================\r\n",
            p->name, p->id,
            g_sys.sensor.temperature, (int)p->temp_min, (int)p->temp_max, g_sys.score.temp_score,
            g_sys.sensor.humidity, (int)p->humi_min, (int)p->humi_max, g_sys.score.humi_score,
            g_sys.sensor.light, (int)p->light_min, (int)p->light_max, g_sys.score.light_score,
            g_sys.score.total_score,
            g_sys.evaporation_index, p->water_threshold,
            g_sys.actuator.curtain_pos, g_sys.actuator.curtain_auto ? "AUTO" : "MANUAL",
            g_sys.actuator.fan_speed, g_sys.actuator.fan_auto ? "AUTO" : "MANUAL",
            g_sys.sensor.distance,
            g_sys.sensor.human_present ? "YES" : "NO",
            g_sys.actuator.auto_mode ? "ON" : "OFF",
            g_sys.water_cooldown ? "YES" : "NO"
        );
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "PLANT:NEXT", 10) == 0) {
        plant_switch_next();
        sprintf(response, "PLANT:Switched to %s\r\n",
                g_sys.plants[g_sys.current_plant].name);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "PLANT:", 6) == 0) {
        uint8_t id = cmd_str[6] - '0';
        if (id >= 1 && id <= MAX_PLANTS) {
            plant_switch(id - 1);
            sprintf(response, "PLANT:Switched to %s\r\n",
                    g_sys.plants[g_sys.current_plant].name);
        } else {
            sprintf(response, "ERROR:Invalid plant ID\r\n");
        }
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "PLANT?", 6) == 0) {
        sprintf(response, "PLANT:%s (ID=%d)\r\n",
                g_sys.plants[g_sys.current_plant].name,
                g_sys.plants[g_sys.current_plant].id);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "CURTAIN:OPEN", 12) == 0) {
        g_sys.actuator.curtain_auto = 0;
        if (g_e3_addr.flag) e3_open(g_e3_addr.periph, g_e3_addr.addr);
        g_sys.actuator.curtain_pos = 0;
        usart_send_string(USART0, "CURTAIN:OPEN\r\n");
    }
    else if (strncmp(cmd_str, "CURTAIN:CLOSE", 13) == 0) {
        g_sys.actuator.curtain_auto = 0;
        if (g_e3_addr.flag) e3_close(g_e3_addr.periph, g_e3_addr.addr);
        g_sys.actuator.curtain_pos = 100;
        usart_send_string(USART0, "CURTAIN:CLOSE\r\n");
    }
    else if (strncmp(cmd_str, "CURTAIN:POS,", 12) == 0) {
        g_sys.actuator.curtain_auto = 0;
        uint8_t pos = (uint8_t)atoi(&cmd_str[12]);
        if (g_e3_addr.flag) e3_set_position(g_e3_addr.periph, g_e3_addr.addr, pos);
        g_sys.actuator.curtain_pos = pos;
        sprintf(response, "CURTAIN:POS=%d%%\r\n", pos);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "CURTAIN:AUTO", 12) == 0) {
        g_sys.actuator.curtain_auto = 1;
        usart_send_string(USART0, "CURTAIN:AUTO mode restored\r\n");
    }
    else if (strncmp(cmd_str, "FAN:ON", 6) == 0) {
        g_sys.actuator.fan_auto = 0;
        if (g_e2_addr.flag) e2_speed_control(g_e2_addr.periph, g_e2_addr.addr, 100);
        g_sys.actuator.fan_speed = 100;
        usart_send_string(USART0, "FAN:ON\r\n");
    }
    else if (strncmp(cmd_str, "FAN:OFF", 7) == 0) {
        g_sys.actuator.fan_auto = 0;
        if (g_e2_addr.flag) e2_fan_off(g_e2_addr.periph, g_e2_addr.addr);
        g_sys.actuator.fan_speed = 0;
        usart_send_string(USART0, "FAN:OFF\r\n");
    }
    else if (strncmp(cmd_str, "FAN:SPEED,", 10) == 0) {
        g_sys.actuator.fan_auto = 0;
        uint8_t speed = (uint8_t)atoi(&cmd_str[10]);
        if (g_e2_addr.flag) e2_speed_control(g_e2_addr.periph, g_e2_addr.addr, speed);
        g_sys.actuator.fan_speed = speed;
        sprintf(response, "FAN:SPEED=%d%%\r\n", speed);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "FAN:AUTO", 8) == 0) {
        g_sys.actuator.fan_auto = 1;
        usart_send_string(USART0, "FAN:AUTO mode restored\r\n");
    }
    else if (strncmp(cmd_str, "AUTO:ON", 7) == 0) {
        g_sys.actuator.auto_mode = 1;
        g_sys.actuator.fan_auto = 1;
        g_sys.actuator.curtain_auto = 1;
        usart_send_string(USART0, "AUTO:ON - Full auto mode\r\n");
    }
    else if (strncmp(cmd_str, "AUTO:OFF", 8) == 0) {
        g_sys.actuator.auto_mode = 0;
        usart_send_string(USART0, "AUTO:OFF - Manual mode\r\n");
    }
    else if (strncmp(cmd_str, "RGB:", 4) == 0) {
        uint8_t r, g, b;
        if (sscanf(&cmd_str[4], "%d,%d,%d", (int*)&r, (int*)&g, (int*)&b) == 3) {
            if (g_e1_rgb_addr.flag) e1_rgb_set(g_e1_rgb_addr.periph, g_e1_rgb_addr.addr, r, g, b);
            g_sys.actuator.rgb_r = r;
            g_sys.actuator.rgb_g = g;
            g_sys.actuator.rgb_b = b;
            sprintf(response, "RGB:Set to %d,%d,%d\r\n", r, g, b);
        } else {
            sprintf(response, "ERROR:Usage RGB:r,g,b\r\n");
        }
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "BEEP", 4) == 0) {
        wm8978_beep(300);
        usart_send_string(USART0, "BEEP:OK\r\n");
    }
    else if (strncmp(cmd_str, "DISTANCE?", 9) == 0) {
        sprintf(response, "DISTANCE:%d mm\r\n", g_sys.sensor.distance);
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "HUMAN?", 6) == 0) {
        sprintf(response, "HUMAN:%s\r\n", g_sys.sensor.human_present ? "YES" : "NO");
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "ZIGBEE:", 7) == 0) {
        c2_broadcast_data(&cmd_str[7], 1);
        sprintf(response, "ZIGBEE:Sent via broadcast\r\n");
        usart_send_string(USART0, response);
    }
    else if (strncmp(cmd_str, "HELP", 4) == 0) {
        const char *help_text =
            "\r\n=== PLANT ASSISTANT COMMANDS ===\r\n"
            "TEMP?          Query temperature\r\n"
            "HUMI?          Query humidity\r\n"
            "LIGHT?         Query light intensity\r\n"
            "SCORE?         Query score details\r\n"
            "STATUS?        Query full system status\r\n"
            "PLANT?         Query current plant\r\n"
            "PLANT:N        Switch to plant N (1-3)\r\n"
            "PLANT:NEXT     Switch to next plant\r\n"
            "CURTAIN:OPEN   Open curtain manually\r\n"
            "CURTAIN:CLOSE  Close curtain manually\r\n"
            "CURTAIN:POS,N  Set curtain position N%\r\n"
            "CURTAIN:AUTO   Resume curtain auto\r\n"
            "FAN:ON         Turn fan on\r\n"
            "FAN:OFF        Turn fan off\r\n"
            "FAN:SPEED,N    Set fan speed N%\r\n"
            "FAN:AUTO       Resume fan auto\r\n"
            "AUTO:ON        Enable full auto mode\r\n"
            "AUTO:OFF       Disable auto mode\r\n"
            "RGB:R,G,B      Set RGB color\r\n"
            "BEEP           Test beep\r\n"
            "DISTANCE?      Query ultrasonic distance\r\n"
            "HUMAN?         Query human presence\r\n"
            "ZIGBEE:MSG     Send message via Zigbee\r\n"
            "HELP           Show this help\r\n"
            "================================\r\n";
        usart_send_string(USART0, (char*)help_text);
    }
    else {
        sprintf(response, "ERROR:Unknown command '%s'. Type HELP for list.\r\n", cmd_str);
        usart_send_string(USART0, response);
    }
}