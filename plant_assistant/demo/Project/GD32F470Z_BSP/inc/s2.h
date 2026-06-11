#ifndef S2_H
#define S2_H

#include "gd32f4xx.h"
#include "plant_logic.h"

/* ========================== S2 传感器地址 ========================== */
#define TH_ADDRESS_S2             0x88   // SHT35 温湿度
#define S1_ADDRESS_S2             0x46   // BH1750 光照
#define S2_ADDRESS_S2             0xB8   // 备选BH1750地址

/* ========================== 传感器数据结构体 ========================== */
typedef struct {
    float temperature;
    float humidity;
} s2_th_para;

typedef struct {
    i2c_addr_def th_addr;    // 温湿度传感器地址
    i2c_addr_def ss_addr;    // 光照传感器地址
} s2_addr_def;

/* ========================== 函数声明 ========================== */
i2c_addr_def s2_init_sht35(uint8_t address);
i2c_addr_def s2_init_bh1750_dev(uint8_t address);
void s2_all_init(s2_addr_def* s2_addr, uint8_t th_addr, uint8_t ss_addr);

// SHT35 温湿度传感器
void s2_sht35_soft_reset(uint32_t i2c_periph, uint8_t i2c_addr);
s2_th_para s2_read_sht35(uint32_t i2c_periph, uint8_t i2c_addr);

// BH1750 光照传感器
void s2_bh1750_init(uint32_t i2c_periph, uint8_t i2c_addr);
float s2_read_bh1750(uint32_t i2c_periph, uint8_t i2c_addr);

// CRC校验
uint8_t s2_sht35_crc_cal(uint16_t dat);

#endif // S2_H