#include "i2c.h"
#include "s8.h"

/**
 * 计算 SHT35 CRC-8 校验
 */
static uint8_t sht35_crc8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else            crc <<= 1;
        }
    }
    return crc;
}

/**
 * 初始化 SHT35 温湿度传感器 (S8)
 * address: 起始扫描地址 0x88
 */
i2c_addr_def s8_init(uint8_t address) {
    i2c_addr_def e_address;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        e_address = get_board_address(address + i * 2);
        if (e_address.flag) {
            // 软复位
            uint8_t cmd[2] = {(SHT35_CMD_SOFT_RESET >> 8) & 0xFF, SHT35_CMD_SOFT_RESET & 0xFF};
            i2c_write(e_address.periph, e_address.addr, cmd[0], &cmd[1], 1);
            // 等待复位完成
            for (volatile uint32_t d = 0; d < 100000; d++);
            break;
        }
    }
    return e_address;
}

/**
 * 扫描所有 S8 子板
 */
void s8_all_init(s8_addr_def *s8_address, uint8_t s8_addr) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        s8_address->th_addr[i] = get_board_address(s8_addr + i * 2);
        if (s8_address->th_addr[i].flag) {
            uint8_t cmd[2] = {(SHT35_CMD_SOFT_RESET >> 8) & 0xFF, SHT35_CMD_SOFT_RESET & 0xFF};
            i2c_write(s8_address->th_addr[i].periph, s8_address->th_addr[i].addr, cmd[0], &cmd[1], 1);
            for (volatile uint32_t d = 0; d < 100000; d++);
        }
    }
}

/**
 * 读取温湿度
 * 使用高重复性测量模式
 */
void s8_get_temp_humi(uint32_t i2c_periph, uint8_t i2c_addr, float *temp, float *humi) {
    uint8_t cmd[2] = {(SHT35_CMD_MEAS_HIGH_REP >> 8) & 0xFF, SHT35_CMD_MEAS_HIGH_REP & 0xFF};
    uint8_t buf[6] = {0};

    // 发送测量命令
    i2c_write(i2c_periph, i2c_addr, cmd[0], &cmd[1], 1);

    // 等待测量完成 (高重复性约15ms)
    for (volatile uint32_t d = 0; d < 500000; d++);

    // 读取6字节数据
    if (i2c_direct_read(i2c_periph, i2c_addr, buf, 6) != 0) {
        *temp = 0.0f;
        *humi = 0.0f;
        return;
    }

    // 解析温度
    uint16_t raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    *temp = -45.0f + 175.0f * raw_temp / 65535.0f;

    // 解析湿度
    uint16_t raw_humi = ((uint16_t)buf[3] << 8) | buf[4];
    *humi = 100.0f * raw_humi / 65535.0f;
    if (*humi > 100.0f) *humi = 100.0f;
    if (*humi < 0.0f)   *humi = 0.0f;
}