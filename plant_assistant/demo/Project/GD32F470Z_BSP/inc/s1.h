#ifndef S1_H
#define S1_H

#include "i2c.h"

// HT16K33(矩阵按键) I2C地址: 7位0x74, 左移0xE8〜0xEE
#define HT16K33_KEY_ADDRESS_S1      0xE8

// HT16K33 寄存器
#define HT16K33_SYSTEM_SETUP        0x20
#define HT16K33_KEY_DATA            0x40
#define HT16K33_INT_FLAG            0x89

// 按键值定义
#define KEY_NONE                    0x00
#define KEY_1                       0x01
#define KEY_2                       0x02
#define KEY_3                       0x03
#define KEY_4                       0x04
#define KEY_5                       0x05
#define KEY_6                       0x06
#define KEY_7                       0x07
#define KEY_8                       0x08
#define KEY_9                       0x09
#define KEY_10                      0x0A
#define KEY_11                      0x0B
#define KEY_12                      0x0C
#define KEY_13                      0x0D
#define KEY_14                      0x0E
#define KEY_15                      0x0F
#define KEY_16                      0x10

// 按键去抖参数
#define KEY_DEBOUNCE_MS             50
#define KEY_LONG_PRESS_MS           1000

typedef struct {
    uint8_t     current_key;
    uint8_t     last_key;
    uint8_t     press_count;
    uint32_t    press_time;
    uint8_t     long_press_flag;
} key_state_t;

void s1_key_init(uint32_t i2c_periph, uint8_t i2c_addr);
uint8_t s1_read_key(uint32_t i2c_periph, uint8_t i2c_addr);
uint8_t s1_key_scan(uint32_t i2c_periph, uint8_t i2c_addr, key_state_t *state);

#endif // S1_H