#ifndef PLANT_LOGIC_H
#define PLANT_LOGIC_H

#include <stdint.h>

/* ========================== 植物档案枚举 ========================== */
typedef enum {
    PLANT_GREEN_ROLO   = 0,  // 绿萝
    PLANT_SUCCULENT    = 1,  // 多肉
    PLANT_MONEY_TREE   = 2,  // 发财树
    PLANT_COUNT
} plant_type_t;

/* ========================== 评分等级枚举 ========================== */
typedef enum {
    SCORE_GREEN  = 0,  // 90-100: 优秀
    SCORE_YELLOW = 1,  // 70-89:  良好
    SCORE_ORANGE = 2,  // 50-69:  一般
    SCORE_RED    = 3   // 0-49:   差
} score_level_t;

/* ========================== 植物配置结构体 ========================== */
typedef struct {
    char*  name;                       // 植物名称
    uint8_t plant_id;                  // 植物编号 1-3
    float  temp_min;                   // 温度适宜下限 (℃)
    float  temp_max;                   // 温度适宜上限 (℃)
    float  humi_min;                   // 湿度适宜下限 (%)
    float  humi_max;                   // 湿度适宜上限 (%)
    float  light_min;                  // 光照适宜下限 (lux)
    float  light_max;                  // 光照适宜上限 (lux)
    float  curtain_close_threshold;    // 遮阳关闭高阈值 (lux)
    float  curtain_open_threshold;     // 遮阳开启低阈值 (lux)
    float  fan_on_threshold;           // 通风开启高阈值 (%)
    float  fan_off_threshold;          // 通风关闭低阈值 (%)
    float  water_evap_threshold;       // 浇水提醒蒸发阈值
} plant_profile_t;

/* ========================== 环境数据结构体 ========================== */
typedef struct {
    float  temperature;                // 当前温度 (℃)
    float  humidity;                   // 当前湿度 (%)
    float  light;                      // 当前光照 (lux)
    int    score;                      // 综合评分 (0-100)
    int    temp_score;                 // 温度单项分
    int    humi_score;                 // 湿度单项分
    int    light_score;                // 光照单项分
    score_level_t level;              // 评分等级
    float  evaporation_index;          // 当前秒蒸发增量
    float  evaporation_sum;            // 蒸发累加值
    uint8_t water_alert;               // 浇水提醒标志
    uint32_t water_alert_cooldown;     // 冷却倒计时(秒)
} env_data_t;

/* ========================== 设备地址结构体 ========================== */
typedef struct {
    uint8_t  flag;                     // 设备是否存在
    uint32_t periph;                   // I2C外设 (I2C0/I2C1)
    uint8_t  addr;                     // I2C设备地址
} i2c_addr_def;

/* ========================== 执行器状态 ========================== */
typedef struct {
    uint8_t curtain_state;             // 窗帘状态: 0=打开, 1=关闭
    uint8_t fan_state;                 // 风扇状态: 0=关闭, 1=低速, 2=高速
    uint8_t fan_speed_low;             // 低速百分比 (默认50)
    uint8_t fan_speed_high;            // 高速百分比 (默认90)
} actuator_state_t;

/* ========================== 系统控制标志 ========================== */
typedef struct {
    uint8_t auto_curtain_enabled;      // 自动遮阳使能
    uint8_t auto_fan_enabled;          // 自动通风使能
    uint8_t auto_water_alert_enabled;  // 自动浇水提醒使能
    uint8_t u2p_vision_enabled;        // 视觉识别使能
    uint8_t display_mode;              // 显示模式: 0=评分, 1=温度, 2=湿度, 3=光照
} sys_ctrl_t;

/* ========================== 全局变量声明 ========================== */
extern plant_profile_t current_profile;
extern env_data_t       current_env;
extern plant_type_t     current_plant_type;
extern actuator_state_t actuators;
extern sys_ctrl_t       sys_ctrl;

/* ========================== I2C设备地址定义 ========================== */
#define ADDR_S2_LIGHT          0x46   // BH1750 光照传感器
#define ADDR_S2_TEMP_HUMI      0x88   // SHT35 温湿度传感器
#define ADDR_S1_KEY            0xE8   // 矩阵按键
#define ADDR_E1_RGB            0xC0   // PCA9685 RGB灯
#define ADDR_E1_NIXIE          0xE0   // HT16K33 数码管
#define ADDR_E2_FAN            0xC8   // PCA9685 风扇
#define ADDR_E3_CURTAIN        0x38   // 窗帘电机

#define I2C0_SPEED             100000
#define I2C0_SLAVE_ADDRESS7    0x72

/* ========================== 数码管特殊值 ========================== */
#define NODIS                  0x10   // 不显示(空)

/* ========================== 函数声明 ========================== */
void plant_logic_init(void);
void switch_plant(void);
void calc_score(void);
void update_evaporation(void);
void check_water_alert(void);
void auto_curtain(void);
void auto_fan(void);
void u1p_fusion_decision(char* name, float score);

// 植物档案查询
const plant_profile_t* get_profile_by_type(plant_type_t type);
const plant_profile_t* get_profile_by_name(const char* name);

// RGB颜色控制
void set_rgb_by_score(int score);
void set_rgb_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_blink_twice(void);

// 数码管显示
void display_score(void);
void display_temperature(void);
void display_humidity(void);
void display_light(void);
void display_plant_id(void);

#endif // PLANT_LOGIC_H