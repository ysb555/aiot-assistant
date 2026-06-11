#ifndef S6_H
#define S6_H

#include "i2c.h"

/* ==================== S6 超声波测距模块 (GD32F330) ==================== */
// I2C地址: 7位0x2C, 左移0x58〜0x5E
#define GD32F330_ADDRESS_S6         0x58
#define GD32F330_READ_CMD           0xAA    // 读取距离值命令

/* 函数声明 */
uint8_t s6_read_distance(uint32_t i2c_periph, uint8_t i2c_addr, uint16_t *distance_mm);

#endif // S6_H