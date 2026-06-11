#ifndef E1_H
#define E1_H

#include "gd32f4xx.h"
#include "plant_logic.h"

/* ========================== PCA9685 寄存器定义 ========================== */
#define E1_PCA9685_MODE1          0x00
#define E1_PCA9685_PRESCALE       0xFE

#define LED0_ON_L                 0x06
#define LED0_ON_H                 0x07
#define LED0_OFF_L                0x08
#define LED0_OFF_H                0x09

#define ALLLED_ON_L               0xFA
#define ALLLED_ON_H               0xFB
#define ALLLED_OFF_L              0xFC
#define ALLLED_OFF_H              0xFD

/* ========================== HT16K33 寄存器定义 ========================== */
#define HT16K33_SYSTEM_ON         0x21
#define HT16K33_DISPLAY_ON        0x81
#define HT16K33_DISPLAY_OFF       0x80
#define HT16K33_DIMMING_DEFAULT   0xEF

/* ========================== E1 设备地址 ========================== */
#define PCA9685_ADDRESS_E1        0xC0
#define HT16K33_ADDRESS_E1        0xE0

/* ========================== 函数声明 ========================== */
i2c_addr_def e1_init_rgb(uint8_t address);
i2c_addr_def e1_init_nixie(uint8_t address);
void e1_all_init(i2c_addr_def* rgb_addr, i2c_addr_def* nixie_addr);

// PCA9685 (RGB灯)
void pca9685_init(uint32_t i2c_periph, uint8_t i2c_addr);
void pca9685_set_pwm(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t channel, uint16_t on, uint16_t off);
void pca9685_set_frequency(uint32_t i2c_periph, uint8_t i2c_addr, uint16_t freq);
void e1_rgb_set(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t red, uint8_t green, uint8_t blue);
void e1_rgb_off(uint32_t i2c_periph, uint8_t i2c_addr);

// HT16K33 (数码管)
void ht16k33_init(uint32_t i2c_periph, uint8_t i2c_addr);
void ht16k33_display_off(uint32_t i2c_periph, uint8_t i2c_addr);
void ht16k33_display_digit(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t pos, uint8_t digit, uint8_t dp);
void e1_nixie_display(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t* dp);

#endif // E1_H