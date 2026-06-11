#ifndef S2_H
#define S2_H

#include "i2c.h"

#define out
#define BH1750_ADDRESS_S2           0x46    // 7位地址0x23, 左移后0x46 (拨码开关可调: 0x46,0x48,0x4A,0x4C)

// BH1750 命令
#define BH1750_POWER_ON             0x01
#define BH1750_POWER_OFF            0x00
#define BH1750_RESET                0x07
#define BH1750_CONT_H_MODE          0x10    // 连续高分辨率模式 1lux精度
#define BH1750_CONT_H_MODE2         0x11    // 连续高分辨率模式2 0.5lux精度
#define BH1750_CONT_L_MODE          0x13    // 连续低分辨率模式 4lux精度
#define BH1750_ONE_H_MODE           0x20    // 单次高分辨率模式
#define BH1750_ONE_H_MODE2          0x21    // 单次高分辨率模式2
#define BH1750_ONE_L_MODE           0x22    // 单次低分辨率模式

typedef struct {
    i2c_addr_def light_addr[4];
} s2_addr_def;

i2c_addr_def s2_init(uint8_t address);
void s2_all_init(s2_addr_def *s2_address, uint8_t s2_addr);
float s2_get_light(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // S2_H