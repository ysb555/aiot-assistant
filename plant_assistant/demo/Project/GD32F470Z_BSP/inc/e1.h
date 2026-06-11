#ifndef E1_H
#define E1_H

#include "i2c.h"

// PCA9685(PWM灯)  I2C地址: 7位0x60, 左移0xC0〜0xC6
// 拨码可调: 0xC0, 0xC2, 0xC4, 0xC6
#define PCA9685_ADDRESS_E1          0xC0

// HT16K33(数码管) I2C地址: 7位0x70, 左移0xE0〜0xE6
// 拨码可调: 0xE0, 0xE2, 0xE4, 0xE6
#define HT16K33_ADDRESS_E1          0xE0

// PCA9685 寄存器
#define PCA9685_MODE1               0x00
#define PCA9685_MODE2               0x01
#define PCA9685_PRESCALE            0xFE
#define PCA9685_LED0_ON_L           0x06
#define PCA9685_ALL_LED_ON_L        0xFA

// HT16K33 寄存器
#define HT16K33_SYSTEM_SETUP        0x20
#define HT16K33_KEY_DATA            0x40
#define HT16K33_DISPLAY_DATA        0x00
#define HT16K33_DISPLAY_ON          0x81
#define HT16K33_DISPLAY_OFF         0x80
#define HT16K33_INT_FLAG            0x89

// 数码管段码表 (共阴)
#define NODIS                       0x10    // 不显示

// 数码管显示结构
typedef struct {
    i2c_addr_def rgb_addr[4];
    i2c_addr_def nixie_addr[4];
} e1_addr_def;

// RGB灯
void     pca9685_init(uint32_t i2c_periph, uint8_t i2c_addr);
void     e1_rgb_init(uint32_t i2c_periph, uint8_t i2c_addr);
void     e1_rgb_set(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t red, uint8_t green, uint8_t blue);
void     e1_rgb_all_init(e1_addr_def *e1_address, uint8_t rgb_addr);

// 数码管
void     ht16k33_init(uint32_t i2c_periph, uint8_t i2c_addr);
void     e1_nixie_init(uint32_t i2c_periph, uint8_t i2c_addr);
void     e1_nixie_display(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t *dp);
void     e1_nixie_all_init(e1_addr_def *e1_address, uint8_t nixie_addr);

#endif // E1_H