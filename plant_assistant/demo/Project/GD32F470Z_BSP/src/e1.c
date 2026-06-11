#include "i2c.h"
#include "e1.h"
#include "main.h"

/* ========================== HT16K33 段码数据 ========================== */
// 0-F 段码: {byte1, byte2}  0x10=不显示
static const uint8_t seg_data[17][2] = {
    {0xF8, 0x01}, // 0
    {0x30, 0x00}, // 1
    {0xD8, 0x02}, // 2
    {0x78, 0x02}, // 3
    {0x30, 0x03}, // 4
    {0x68, 0x03}, // 5
    {0xE8, 0x03}, // 6
    {0x38, 0x00}, // 7
    {0xF8, 0x03}, // 8
    {0x78, 0x03}, // 9
    {0xB8, 0x03}, // A
    {0xE0, 0x03}, // B
    {0xC8, 0x01}, // C
    {0xF0, 0x02}, // D
    {0xC8, 0x03}, // E
    {0x88, 0x03}, // F
    {0x00, 0x00}, // 0x10 = 空
};

/* ========================== PCA9685 RGB 灯驱动 ========================== */

/**
 * 初始化 PCA9685
 */
void pca9685_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, E1_PCA9685_MODE1, 0x00);
    // 关闭所有PWM输出
    i2c_byte_write(i2c_periph, i2c_addr, ALLLED_ON_L,  0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLLED_ON_H,  0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLLED_OFF_L, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLLED_OFF_H, 0x10);
}

/**
 * 设置 PCA9685 单个通道 PWM
 * duty = (off - on) / 4096
 */
void pca9685_set_pwm(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t channel, uint16_t on, uint16_t off) {
    i2c_byte_write(i2c_periph, i2c_addr, LED0_ON_L  + 4 * channel, on);
    i2c_byte_write(i2c_periph, i2c_addr, LED0_ON_H  + 4 * channel, on >> 8);
    i2c_byte_write(i2c_periph, i2c_addr, LED0_OFF_L + 4 * channel, off);
    i2c_byte_write(i2c_periph, i2c_addr, LED0_OFF_H + 4 * channel, off >> 8);
}

/**
 * 设置 PCA9685 PWM 频率
 */
void pca9685_set_frequency(uint32_t i2c_periph, uint8_t i2c_addr, uint16_t freq) {
    uint8_t prescale;
    uint8_t old_mode, new_mode;

    if (freq > 1526) freq = 1526;
    if (freq < 24)   freq = 24;

    i2c_read(i2c_periph, i2c_addr, E1_PCA9685_MODE1, &old_mode, 1);
    new_mode = (old_mode & 0x7F) | 0x10;
    i2c_byte_write(i2c_periph, i2c_addr, E1_PCA9685_MODE1, new_mode);

    prescale = (uint8_t)(25000000UL / 4096 / freq - 1);
    i2c_byte_write(i2c_periph, i2c_addr, E1_PCA9685_PRESCALE, prescale);
    i2c_byte_write(i2c_periph, i2c_addr, E1_PCA9685_MODE1, old_mode);

    delay_ms(5);
    i2c_byte_write(i2c_periph, i2c_addr, E1_PCA9685_MODE1, old_mode | 0xA1);
}

/**
 * RGB 灯颜色控制
 * red/green/blue: 0-255
 * 通道: R=1, G=0, B=2
 */
void e1_rgb_set(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t red, uint8_t green, uint8_t blue) {
    uint16_t on, off;

    // 绿色 - 通道0
    on = 0x0F;
    off = on + (uint16_t)green * 0x10;
    pca9685_set_pwm(i2c_periph, i2c_addr, 0, on, off);

    // 红色 - 通道1
    on = 0x0F;
    off = on + (uint16_t)red * 0x10;
    pca9685_set_pwm(i2c_periph, i2c_addr, 1, on, off);

    // 蓝色 - 通道2
    on = 0x0F;
    off = on + (uint16_t)blue * 0x10;
    pca9685_set_pwm(i2c_periph, i2c_addr, 2, on, off);
}

/**
 * 关闭 RGB 灯
 */
void e1_rgb_off(uint32_t i2c_periph, uint8_t i2c_addr) {
    e1_rgb_set(i2c_periph, i2c_addr, 0, 0, 0);
}

/* ========================== HT16K33 数码管驱动 ========================== */

/**
 * 初始化 HT16K33 数码管芯片
 */
void ht16k33_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_cmd_write(i2c_periph, i2c_addr, HT16K33_SYSTEM_ON);
    ht16k33_display_off(i2c_periph, i2c_addr);
}

/**
 * 关闭所有数码管显示
 */
void ht16k33_display_off(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_byte_write(i2c_periph, i2c_addr, 0x02, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x03, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x04, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x05, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x06, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x07, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x08, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, 0x09, 0x00);
    i2c_cmd_write(i2c_periph, i2c_addr, HT16K33_DISPLAY_ON);
}

/**
 * 显示单个数码管位
 * pos: 1-4 (数码管位置)
 * digit: 0-F 或 NODIS(0x10)不显示
 * dp: 小数点 0=无 1=有
 */
void ht16k33_display_digit(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t pos, uint8_t digit, uint8_t dp) {
    uint8_t reg1, reg2;
    uint8_t byte1, byte2;

    switch (pos) {
        case 1: reg1 = 0x02; reg2 = 0x03; break;
        case 2: reg1 = 0x04; reg2 = 0x05; break;
        case 3: reg1 = 0x06; reg2 = 0x07; break;
        case 4: reg1 = 0x08; reg2 = 0x09; break;
        default: return;
    }

    if (digit > 16) digit = 16;  // 防止越界
    byte1 = seg_data[digit][0];
    byte2 = seg_data[digit][1];

    if (dp == 1) {
        byte1 |= 0x07;
        byte2 |= 0xFC;
    }

    i2c_byte_write(i2c_periph, i2c_addr, reg1, byte1);
    i2c_byte_write(i2c_periph, i2c_addr, reg2, byte2);
    i2c_cmd_write(i2c_periph, i2c_addr, HT16K33_DISPLAY_ON);
}

/**
 * 显示4位数码管
 */
void e1_nixie_display(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, uint8_t* dp) {
    ht16k33_display_digit(i2c_periph, i2c_addr, 1, d1, dp[0]);
    ht16k33_display_digit(i2c_periph, i2c_addr, 2, d2, dp[1]);
    ht16k33_display_digit(i2c_periph, i2c_addr, 3, d3, dp[2]);
    ht16k33_display_digit(i2c_periph, i2c_addr, 4, d4, dp[3]);
}

/* ========================== E1 初始化 ========================== */

/**
 * 初始化 E1 RGB 子板
 */
i2c_addr_def e1_init_rgb(uint8_t address) {
    i2c_addr_def dev;
    dev.flag = 0;
    dev.periph = I2C0;
    dev.addr = 0;

    uint8_t addrs[] = {address, address + 2, address + 4, address + 6};
    for (int i = 0; i < 4; i++) {
        if (i2c_addr_poll(I2C0, addrs[i])) {
            dev.periph = I2C0;
            dev.addr = addrs[i];
            dev.flag = 1;
            break;
        }
    }
    if (!dev.flag) {
        for (int i = 0; i < 4; i++) {
            if (i2c_addr_poll(I2C1, addrs[i])) {
                dev.periph = I2C1;
                dev.addr = addrs[i];
                dev.flag = 1;
                break;
            }
        }
    }
    if (dev.flag) {
        pca9685_init(dev.periph, dev.addr);
    }
    return dev;
}

/**
 * 初始化 E1 数码管子板
 */
i2c_addr_def e1_init_nixie(uint8_t address) {
    i2c_addr_def dev;
    dev.flag = 0;
    dev.periph = I2C0;
    dev.addr = 0;

    uint8_t addrs[] = {address, address + 2, address + 4, address + 6};
    for (int i = 0; i < 4; i++) {
        if (i2c_addr_poll(I2C0, addrs[i])) {
            dev.periph = I2C0;
            dev.addr = addrs[i];
            dev.flag = 1;
            break;
        }
    }
    if (!dev.flag) {
        for (int i = 0; i < 4; i++) {
            if (i2c_addr_poll(I2C1, addrs[i])) {
                dev.periph = I2C1;
                dev.addr = addrs[i];
                dev.flag = 1;
                break;
            }
        }
    }
    if (dev.flag) {
        ht16k33_init(dev.periph, dev.addr);
    }
    return dev;
}

/**
 * 初始化所有 E1 子板
 */
void e1_all_init(i2c_addr_def* rgb_addr, i2c_addr_def* nixie_addr) {
    *rgb_addr   = e1_init_rgb(PCA9685_ADDRESS_E1);
    *nixie_addr = e1_init_nixie(HT16K33_ADDRESS_E1);
}