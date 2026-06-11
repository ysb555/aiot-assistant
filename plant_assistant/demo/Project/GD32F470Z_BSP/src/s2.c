#include "i2c.h"
#include "s2.h"

/* ========================== SHT35 温湿度传感器 ========================== */

/**
 * SHT35 软复位
 */
void s2_sht35_soft_reset(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x30, 0xA2);
}

/**
 * SHT35 CRC校验
 */
uint8_t s2_sht35_crc_cal(uint16_t dat) {
    uint8_t i, t, temp;
    uint8_t crc_byte = 0xFF;
    temp = (uint8_t)(dat >> 8);

    for (t = 0; t < 2; t++) {
        crc_byte ^= temp;
        for (i = 0; i < 8; i++) {
            if (crc_byte & 0x80) {
                crc_byte <<= 1;
                crc_byte ^= 0x31;
            } else {
                crc_byte <<= 1;
            }
        }
        if (t == 0) {
            temp = (uint8_t)(dat & 0xFF);
        }
    }
    return crc_byte;
}

/**
 * 读取 SHT35 温湿度
 */
s2_th_para s2_read_sht35(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t  buf[6];
    s2_th_para result;
    uint16_t tmp;

    result.temperature = 0.0f;
    result.humidity = 0.0f;

    // 发送测量命令 (高精度模式)
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x2C, 0x0D);
    // 等待测量完成
    for (volatile uint32_t d = 0; d < 50000; d++);
    // 读取6字节数据
    i2c_direct_read(i2c_periph, i2c_addr, buf, 6);

    // 温度
    tmp = ((uint16_t)buf[0] << 8) | buf[1];
    if (s2_sht35_crc_cal(tmp) == buf[2]) {
        result.temperature = (float)tmp * 175.0f / 65535.0f - 45.0f;
    }

    // 湿度
    tmp = ((uint16_t)buf[3] << 8) | buf[4];
    if (s2_sht35_crc_cal(tmp) == buf[5]) {
        result.humidity = (float)tmp * 100.0f / 65535.0f;
    }

    return result;
}

/* ========================== BH1750 光照传感器 ========================== */

/**
 * 初始化 BH1750
 */
void s2_bh1750_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 上电命令
    i2c_cmd_write(i2c_periph, i2c_addr, 0x01);
}

/**
 * 读取 BH1750 光照值 (lux)
 */
float s2_read_bh1750(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t buf[2];
    uint16_t raw;

    // 连续高分辨率模式
    i2c_cmd_write(i2c_periph, i2c_addr, 0x10);
    // 等待测量完成
    for (volatile uint32_t d = 0; d < 50000; d++);
    // 读取2字节
    i2c_direct_read(i2c_periph, i2c_addr, buf, 2);

    raw = ((uint16_t)buf[0] << 8) | buf[1];
    return (float)raw / 1.2f;
}

/* ========================== S2 设备初始化 ========================== */

/**
 * 初始化 SHT35
 */
i2c_addr_def s2_init_sht35(uint8_t address) {
    i2c_addr_def dev;
    dev.flag = 0;
    dev.periph = I2C0;
    dev.addr = 0;

    uint8_t addrs[] = {address, address + 2};
    for (int i = 0; i < 2; i++) {
        if (i2c_addr_poll(I2C0, addrs[i])) {
            dev.periph = I2C0;
            dev.addr = addrs[i];
            dev.flag = 1;
            break;
        }
    }
    if (!dev.flag) {
        for (int i = 0; i < 2; i++) {
            if (i2c_addr_poll(I2C1, addrs[i])) {
                dev.periph = I2C1;
                dev.addr = addrs[i];
                dev.flag = 1;
                break;
            }
        }
    }
    if (dev.flag) {
        s2_sht35_soft_reset(dev.periph, dev.addr);
    }
    return dev;
}

/**
 * 初始化 BH1750
 */
i2c_addr_def s2_init_bh1750_dev(uint8_t address) {
    i2c_addr_def dev;
    dev.flag = 0;
    dev.periph = I2C0;
    dev.addr = 0;

    if (i2c_addr_poll(I2C0, address)) {
        dev.periph = I2C0;
        dev.addr = address;
        dev.flag = 1;
    } else if (i2c_addr_poll(I2C0, S2_ADDRESS_S2)) {
        dev.periph = I2C0;
        dev.addr = S2_ADDRESS_S2;
        dev.flag = 1;
    } else if (i2c_addr_poll(I2C1, address)) {
        dev.periph = I2C1;
        dev.addr = address;
        dev.flag = 1;
    }

    if (dev.flag) {
        s2_bh1750_init(dev.periph, dev.addr);
    }
    return dev;
}

/**
 * 初始化所有 S2 传感器
 */
void s2_all_init(s2_addr_def* s2_addr, uint8_t th_addr, uint8_t ss_addr) {
    s2_addr->th_addr = s2_init_sht35(th_addr);
    s2_addr->ss_addr = s2_init_bh1750_dev(ss_addr);
}