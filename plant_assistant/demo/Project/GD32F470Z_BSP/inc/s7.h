#ifndef S7_H
#define S7_H

#include "i2c.h"

/* ==================== S7 人体红外检测模块 (PCA9557) ==================== */
// I2C地址: 7位0x18, 左移0x30〜0x36
#define PCA9557_ADDRESS_S7          0x30

// PCA9557 寄存器
#define PCA9557_INPUT_PORT_REG      0x00    // 输入端口寄存器
#define PCA9557_OUTPUT_PORT_REG     0x01    // 输出端口寄存器
#define PCA9557_POLARITY_INV_REG    0x02    // 极性反转寄存器
#define PCA9557_CONFIG_REG          0x03    // 配置寄存器 (0=输出, 1=输入)

/* 函数声明 */
uint8_t s7_get_status(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // S7_H