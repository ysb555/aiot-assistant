#ifndef E3_H
#define E3_H

#include "i2c.h"

/* ==================== E3 窗帘电机驱动 (HR8833) ==================== */
// I2C地址: 7位0x1C, 左移0x38〜0x3E
#define CURTAIN_ADDRESS_E3          0x38

// 窗帘电机寄存器
#define E3_REG_POSITION             0x03    // 位置寄存器 (0-100%)
#define E3_REG_SPEED                0x04    // 速度寄存器
#define E3_REG_DIRECTION            0x05    // 方向寄存器 (0=打开, 1=关闭)

/* 函数声明 */
i2c_addr_def e3_init(uint8_t address);
void e3_set_position(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t position);
void e3_set_speed(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed);
void e3_open(uint32_t i2c_periph, uint8_t i2c_addr);
void e3_close(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // E3_H