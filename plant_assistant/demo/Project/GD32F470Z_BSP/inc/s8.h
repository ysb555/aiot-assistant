#ifndef S8_H
#define S8_H

#include "i2c.h"

#define out
#define SHT35_ADDRESS_S8            0x88    // 7位地址0x44, 左移后0x88 (拨码开关可调: 0x88,0x8A)

// SHT35 命令
#define SHT35_CMD_MEAS_HIGH_REP     0x2400  // 高重复性测量
#define SHT35_CMD_MEAS_MED_REP      0x240B  // 中重复性测量
#define SHT35_CMD_MEAS_LOW_REP      0x2416  // 低重复性测量
#define SHT35_CMD_SOFT_RESET        0x30A2  // 软复位
#define SHT35_CMD_HEATER_ON         0x306D  // 加热器开
#define SHT35_CMD_HEATER_OFF        0x3066  // 加热器关
#define SHT35_CMD_READ_STATUS       0xF32D  // 读状态寄存器

typedef struct {
    i2c_addr_def th_addr[4];
} s8_addr_def;

i2c_addr_def s8_init(uint8_t address);
void s8_all_init(s8_addr_def *s8_address, uint8_t s8_addr);
void s8_get_temp_humi(uint32_t i2c_periph, uint8_t i2c_addr, float *temp, float *humi);

#endif // S8_H