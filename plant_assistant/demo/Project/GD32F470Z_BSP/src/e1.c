#include "i2c.h"
#include "e1.h"

/* ==================== 段码表 (0~9, A~F, 特殊字符) ==================== */
static const uint8_t SEG_TABLE[] = {
    0x3F,  // 0
    0x06,  // 1
    0x5B,  // 2
    0x4F,  // 3
    0x66,  // 4
    0x6D,  // 5
    0x7D,  // 6
    0x07,  // 7
    0x7F,  // 8
    0x6F,  // 9
    0x77,  // A (10)
    0x7C,  // B (11)
    0x39,  // C (12)
    0x5E,  // D (13)
    0x79,  // E (14)
    0x71,  // F (15)
    0x00,  // 全灭 (16)
    0x40,  // '-' (17)
};

/* ==================== PCA9685 (RGB灯) ==================== */

void pca9685_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 复位
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE2, 0x04);  // OUTDRV=1 totem pole

    // 设置PWM频率约200Hz (prescale取决于晶振)
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x10);  // sleep
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_PRESCALE, 0x1E);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x20);  // auto-increment
    // 等待振荡器稳定
    for (volatile uint32_t d = 0; d < 10000; d++);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0xA1);  // restart + auto-increment
}

void e1_rgb_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    pca9685_init(i2c_periph, i2c_addr);
    // 初始关闭所有通道
    e1_rgb_set(i2c_periph, i2c_addr, 0, 0, 0);
}

void e1_rgb_set(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t buf[8];

    // 通道0: 绿(GREEN)
    buf[0] = PCA9685_LED0_ON_L + 0 * 4;
    buf[1] = 0x00; buf[2] = 0x00;                    // ON = 0
    buf[3] = (uint8_t)(green * 16) & 0xFF;           // OFF = green*16
    buf[4] = (uint8_t)((green * 16) >> 8) & 0x0F;

    // 通道1: 红(RED)
    buf[5] = (uint8_t)(red * 16) & 0xFF;
    buf[6] = (uint8_t)((red * 16) >> 8) & 0x0F;

    // 通道2: 蓝(BLUE)
    buf[7] = (uint8_t)(blue * 16) & 0xFF;

    i2c_write(i2c_periph, i2c_addr, buf[0], &buf[1], 7);
}

void e1_rgb_all_init(e1_addr_def *e1_address, uint8_t rgb_addr) {
    for (uint8_t i = 0; i < 4; i++) {
        e1_address->rgb_addr[i] = get_board_address(rgb_addr + i * 2);
        if (e1_address->rgb_addr[i].flag) {
            pca9685_init(e1_address->rgb_addr[i].periph, e1_address->rgb_addr[i].addr);
            e1_rgb_set(e1_address->rgb_addr[i].periph, e1_address->rgb_addr[i].addr, 0, 0, 0);
        }
    }
}

/* ==================== HT16K33 (数码管) ==================== */

void ht16k33_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 开启振荡器
    i2c_delay_byte_write(i2c_periph, i2c_addr, HT16K33_SYSTEM_SETUP | 0x01, 0);
    // 设置亮度 (0~15)
    i2c_delay_byte_write(i2c_periph, i2c_addr, HT16K33_DISPLAY_ON | 8, 0);
}

void e1_nixie_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    ht16k33_init(i2c_periph, i2c_addr);
}

void e1_nixie_display(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t *dp) {
    uint8_t buf[9];
    buf[0] = HT16K33_DISPLAY_DATA;

    for (int i = 0; i < 4; i++) {
        uint8_t dig[4] = {d1, d2, d3, d4};
        uint8_t seg = (dig[i] < 18) ? SEG_TABLE[dig[i]] : 0x00;
        if (dp && dp[i]) seg |= 0x80;  // 小数点
        buf[i * 2 + 1] = seg;
        buf[i * 2 + 2] = 0x00;
    }
    i2c_write(i2c_periph, i2c_addr, buf[0], &buf[1], 8);
}

void e1_nixie_all_init(e1_addr_def *e1_address, uint8_t nixie_addr) {
    for (uint8_t i = 0; i < 4; i++) {
        e1_address->nixie_addr[i] = get_board_address(nixie_addr + i * 2);
        if (e1_address->nixie_addr[i].flag) {
            ht16k33_init(e1_address->nixie_addr[i].periph, e1_address->nixie_addr[i].addr);
        }
    }
}