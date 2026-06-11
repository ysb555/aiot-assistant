#ifndef E2_H
#define E2_H

#include "gd32f4xx.h"
#include "plant_logic.h"

/* ========================== PCA9685 寄存器 (风扇) ========================== */
#define E2_PCA9685_MODE1          0x00
#define E2_PCA9685_PRESCALE       0xFE

#define CN0_ON_L                  0x06
#define CN0_ON_H                  0x07
#define CN0_OFF_L                 0x08
#define CN0_OFF_H                 0x09

#define ALLCN_ON_L                0xFA
#define ALLCN_ON_H                0xFB
#define ALLCN_OFF_L               0xFC
#define ALLCN_OFF_H               0xFD

#define PCA9685_ADDRESS_E2        0xC8

/* ========================== 函数声明 ========================== */
i2c_addr_def e2_init(uint8_t address);
void pca9685_e2_init(uint32_t i2c_periph, uint8_t i2c_addr);
void e2_set_pwm(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t channel, uint16_t on, uint16_t off);
void e2_speed_control(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed_percent);
void e2_fan_off(uint32_t i2c_periph, uint8_t i2c_addr);

#endif // E2_H