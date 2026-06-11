#ifndef S2_H
#define S2_H

#include "i2c.h"

#define out

/* ==================== S2 三合一传感器板 ==================== */
// S2 子板集成了三个传感器芯片:
//   1. SHT35   (温湿度)    - I2C地址 0x88 / 0x8A
//   2. BH1750  (光照度)    - I2C地址 0x46 / 0xB8
//   3. ICM20608 (6轴陀螺仪) - I2C地址 0xD0 / 0xD2 (可选)

/* -------- SHT35 温湿度 ------ */
#define TH_ADDRESS_S2               0x88       // SHT35 起始地址 (8位: 0x88, 0x8A)

// SHT35 命令
#define SHT35_CMD_MEAS_HIGH_REP     0x2400  // 高重复性测量
#define SHT35_CMD_MEAS_MED_REP      0x240B  // 中重复性测量
#define SHT35_CMD_MEAS_LOW_REP      0x2416  // 低重复性测量
#define SHT35_CMD_SOFT_RESET        0x30A2  // 软复位
#define SHT35_CMD_HEATER_ON         0x306D  // 加热器开
#define SHT35_CMD_HEATER_OFF        0x3066  // 加热器关
#define SHT35_CMD_READ_STATUS       0xF32D  // 读状态寄存器

/* -------- BH1750 光照 ------ */
#define S1_ADDRESS_S2               0x46       // BH1750 起始地址 (8位: 0x46, 0xB8)
#define S2_ADDRESS_S2               0xB8

// BH1750 命令
#define BH1750_POWER_ON             0x01
#define BH1750_POWER_OFF            0x00
#define BH1750_RESET                0x07
#define BH1750_CONT_H_MODE          0x10    // 连续高分辨率模式
#define BH1750_CONT_H_MODE2         0x11    // 连续高分辨率模式2
#define BH1750_CONT_L_MODE          0x13    // 连续低分辨率模式
#define BH1750_ONE_H_MODE           0x20    // 单次高分辨率模式

/* -------- ICM20608 6轴 ------ */
#define AX_ADDRESS_S2               0xD0       // ICM20608 起始地址 (8位: 0xD0, 0xD2)

/* ==================== 数据结构 ==================== */

// S2 板三合一地址结构
typedef struct {
    i2c_addr_def th_addr;   // SHT35 温湿度
    i2c_addr_def ss_addr;   // BH1750 光照
    i2c_addr_def ax_addr;   // ICM20608 6轴 (可选)
} s2_addr_def;

// 温湿度返回结构
typedef struct {
    float temperature;
    float humidity;
} s2_para;

// 6轴传感器返回结构
typedef struct {
    float gyro_x_act;
    float gyro_y_act;
    float gyro_z_act;
    float accel_x_act;
    float accel_y_act;
    float accel_z_act;
    float temp_act;
} ax_para;

/* ==================== 函数声明 ==================== */

// 单个传感器初始化
i2c_addr_def s2_init(uint8_t address);

// 三合一初始化 (自动扫描三个传感器地址)
void s2_all_init(s2_addr_def *s2_address, uint8_t s2_th_addr, uint8_t s2_ss_addr, uint8_t s2_ax_addr);

// 读取温湿度 (SHT35)
s2_para s2_read_sht3x(uint32_t i2c_periph, uint8_t i2c_addr);

// 便捷函数: 同时读取温湿度到两个float指针
void s2_get_temp_humi(uint32_t i2c_periph, uint8_t i2c_addr, float *temp, float *humi);

// 读取光照度 (BH1750)
float s2_read_bh1750_value(uint32_t i2c_periph, uint8_t i2c_addr);

// 便捷函数: 读取光照
float s2_get_light(uint32_t i2c_periph, uint8_t i2c_addr);

// 读取6轴数据 (ICM20608, 可选)
ax_para s2_read_icm20608_value(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // S2_H