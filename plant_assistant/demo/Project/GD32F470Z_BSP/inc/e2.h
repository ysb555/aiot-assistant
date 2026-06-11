#ifndef E2_H
#define E2_H

#include "i2c.h"

// PCA9685(风扇) I2C地址: 7位0x64, 左移0xC8〜0xCE
#define PCA9685_ADDRESS_E2          0xC8

// PCA9685寄存器 (同E1)
#define PCA9685_MODE1               0x00
#define PCA9685_MODE2               0x01
#define PCA9685_PRESCALE            0xFE
#define PCA9685_LED0_ON_L           0x06

void e2_fan_init(uint32_t i2c_periph, uint8_t i2c_addr);
void e2_fan_off(uint32_t i2c_periph, uint8_t i2c_addr);
void e2_speed_control(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed_percent);

#endif // E2_H