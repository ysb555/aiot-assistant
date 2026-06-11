#include "i2c.h"
#include "e2.h"

void e2_fan_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 复位
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE2, 0x04);

    // 设置PWM频率约200Hz
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x10);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_PRESCALE, 0x1E);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0x20);
    for (volatile uint32_t d = 0; d < 10000; d++);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_MODE1, 0xA1);

    e2_fan_off(i2c_periph, i2c_addr);
}

void e2_fan_off(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 通道0: OFF=0, 占空比0%
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 0, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 1, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 2, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 3, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 4, 0x00);
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9685_LED0_ON_L + 5, 0x0F);
}

void e2_speed_control(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed_percent) {
    if (speed_percent > 100) speed_percent = 100;

    uint16_t off_val = (uint16_t)speed_percent * 0x10;
    if (speed_percent == 0) {
        e2_fan_off(i2c_periph, i2c_addr);
        return;
    }

    uint8_t buf[4];
    buf[0] = PCA9685_LED0_ON_L + 0;
    buf[1] = 0x00;                        // ON_L
    buf[2] = 0x00;                        // ON_H
    buf[3] = (uint8_t)(off_val & 0xFF);   // OFF_L
    buf[4] = (uint8_t)((off_val >> 8) & 0x0F);  // OFF_H
    i2c_write(i2c_periph, i2c_addr, buf[0], &buf[1], 4);
}