#ifndef E4_H
#define E4_H

#include "i2c.h"

/* ==================== E4 扬声器驱动 (WM8978) ==================== */
// I2C地址: 7位0x1A, 左移0x34
#define WM8978_ADDRESS_E4           0x34

// WM8978 寄存器
#define WM8978_RESET                0x00
#define WM8978_POWER_MANAGEMENT_1   0x01
#define WM8978_POWER_MANAGEMENT_2   0x02
#define WM8978_POWER_MANAGEMENT_3   0x03
#define WM8978_AUDIO_INTERFACE      0x04
#define WM8978_COMPANDING_CTRL      0x05
#define WM8978_CLOCK_GEN_CTRL       0x06
#define WM8978_ADDITIONAL_CTRL      0x07
#define WM8978_GPIO_STUFF           0x08
#define WM8978_LEFT_INP_PGA_GAIN    0x2F
#define WM8978_RIGHT_INP_PGA_GAIN   0x30
#define WM8978_LOUT1_VOLUME         0x34
#define WM8978_ROUT1_VOLUME         0x35
#define WM8978_LOUT2_VOLUME         0x36
#define WM8978_ROUT2_VOLUME         0x37
#define WM8978_OUT3_MIXER_CTRL      0x3B
#define WM8978_OUT4_MIXER_CTRL      0x3C
#define WM8978_SPK_VOLUME_LEFT      0x1A
#define WM8978_SPK_VOLUME_RIGHT     0x1B

/* 函数声明 */
uint8_t wm8978_init(uint8_t i2c_addr);
void wm8978_hp_vol_set(uint8_t voll, uint8_t volr);
void wm8978_spk_vol_set(uint8_t volx);
void wm8978_beep(uint16_t duration_ms);

#endif // E4_H