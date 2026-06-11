#include "i2c.h"
#include "e2.h"

/* ========================== E2 风扇驱动 (PCA9685) ========================== */

/**
 * 初始化 PCA9685 (E2风扇)
 */
void pca9685_e2_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, E2_PCA9685_MODE1, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLCN_ON_L,  0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLCN_ON_H,  0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLCN_OFF_L, 0x00);
    i2c_byte_write(i2c_periph, i2c_addr, ALLCN_OFF_H, 0x10);
}

/**
 * 设置 E2 风扇 PWM 通道
 */
void e2_set_pwm(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t channel, uint16_t on, uint16_t off) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, CN0_ON_L  + 4 * channel, on);
    i2c_delay_byte_write(i2c_periph, i2c_addr, CN0_ON_H  + 4 * channel, on >> 8);
    i2c_delay_byte_write(i2c_periph, i2c_addr, CN0_OFF_L + 4 * channel, off);
    i2c_delay_byte_write(i2c_periph, i2c_addr, CN0_OFF_H + 4 * channel, off >> 8);
}

/**
 * 风扇转速控制
 * speed_percent: 0-100
 */
void e2_speed_control(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed_percent) {
    uint16_t on, off;

    if (speed_percent > 100) speed_percent = 100;

    on = 0x0000;
    off = on + (uint16_t)speed_percent * 0xFFF / 100;
    e2_set_pwm(i2c_periph, i2c_addr, 0, on, off);
}

/**
 * 关闭风扇
 */
void e2_fan_off(uint32_t i2c_periph, uint8_t i2c_addr) {
    e2_set_pwm(i2c_periph, i2c_addr, 0, 0, 0);
}

/* ========================== E2 设备初始化 ========================== */

/**
 * 初始化 E2 风扇子板
 */
i2c_addr_def e2_init(uint8_t address) {
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
        pca9685_e2_init(dev.periph, dev.addr);
    }
    return dev;
}