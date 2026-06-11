#ifndef S1_H
#define S1_H

#include "gd32f4xx.h"
#include "plant_logic.h"

/* ========================== S1 矩阵按键 ========================== */
#define S1_ADDRESS                0xE8
#define S1_KEY_NONE               0x00
#define S1_KEY_SWITCH             0x01   // 切换植物按键

/* ========================== 函数声明 ========================== */
i2c_addr_def s1_init(uint8_t address);
uint8_t s1_scan_key(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // S1_H