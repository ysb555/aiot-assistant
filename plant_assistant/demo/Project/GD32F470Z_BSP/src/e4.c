#include "e4.h"
#include "systick.h"

/* ==================== E4 扬声器驱动 (WM8978) ==================== */
/* 参考: xiaomi-aiot/5-smart_clock/Clock-DEMO/demo */

/* 写 WM8978 寄存器 (9位寄存器地址 + 7位数据) */
static void wm8978_write_reg(uint8_t reg, uint16_t val) {
    uint8_t buf[2];
    buf[0] = ((reg << 1) & 0xFE) | ((val >> 8) & 0x01);
    buf[1] = val & 0xFF;
    // 通过 I2C0 发送
    i2c_write(I2C0, WM8978_ADDRESS_E4, buf[0], &buf[1], 1);
}

/**
 * 初始化 WM8978 音频芯片
 */
uint8_t wm8978_init(uint8_t i2c_addr) {
    // 检查设备
    if (!i2c_addr_poll(I2C0, i2c_addr)) return 1;

    // 软复位
    wm8978_write_reg(WM8978_RESET, 0x0000);
    delay_1ms(50);

    // 电源管理: 开启所有模块
    wm8978_write_reg(WM8978_POWER_MANAGEMENT_1, 0x0000);
    wm8978_write_reg(WM8978_POWER_MANAGEMENT_2, 0x0000);
    wm8978_write_reg(WM8978_POWER_MANAGEMENT_3, 0x0000);

    // 音频接口配置
    wm8978_write_reg(WM8978_AUDIO_INTERFACE, 0x000A);  // I2S 16bit
    wm8978_write_reg(WM8978_COMPANDING_CTRL, 0x0000);
    wm8978_write_reg(WM8978_CLOCK_GEN_CTRL, 0x000C);
    wm8978_write_reg(WM8978_ADDITIONAL_CTRL, 0x0000);

    // 音量设置
    wm8978_write_reg(WM8978_LOUT1_VOLUME, 0x0079);
    wm8978_write_reg(WM8978_ROUT1_VOLUME, 0x0079);
    wm8978_write_reg(WM8978_LOUT2_VOLUME, 0x0079);
    wm8978_write_reg(WM8978_ROUT2_VOLUME, 0x0079);

    // 扬声器音量
    wm8978_write_reg(WM8978_SPK_VOLUME_LEFT, 0x0079);
    wm8978_write_reg(WM8978_SPK_VOLUME_RIGHT, 0x0079);

    return 0;
}

/**
 * 设置耳机音量
 * voll, volr: 0~63 (0=静音, 63=最大)
 */
void wm8978_hp_vol_set(uint8_t voll, uint8_t volr) {
    if (voll > 63) voll = 63;
    if (volr > 63) volr = 63;
    wm8978_write_reg(WM8978_LOUT1_VOLUME, voll | 0x0100);
    wm8978_write_reg(WM8978_ROUT1_VOLUME, volr | 0x0100);
}

/**
 * 设置扬声器音量
 * volx: 0~63
 */
void wm8978_spk_vol_set(uint8_t volx) {
    if (volx > 63) volx = 63;
    wm8978_write_reg(WM8978_SPK_VOLUME_LEFT,  volx | 0x0100);
    wm8978_write_reg(WM8978_SPK_VOLUME_RIGHT, volx | 0x0100);
}

/**
 * 蜂鸣提示
 * duration_ms: 持续时间(毫秒)
 */
void wm8978_beep(uint16_t duration_ms) {
    // 设置较高音量产生蜂鸣
    wm8978_spk_vol_set(50);
    delay_1ms(duration_ms);
    wm8978_spk_vol_set(0);
}