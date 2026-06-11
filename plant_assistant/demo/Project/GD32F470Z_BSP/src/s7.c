#include "s7.h"

/* ==================== S7 人体红外驱动 (PCA9557) ==================== */
/* 参考: xiaomi-aiot/2-auto_light/Autolight-DEMO/demo */

/**
 * 获取人体红外状态
 * i2c_periph: I2C外设
 * i2c_addr: S7 I2C地址
 * 返回: bit0=红外传感器状态 (1=有人, 0=无人)
 */
uint8_t s7_get_status(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t data[1] = {0};

    // 配置所有端口为输入
    i2c_delay_byte_write(i2c_periph, i2c_addr, PCA9557_CONFIG_REG, 0xF8);

    // 读取输入端口
    i2c_delay_read(i2c_periph, i2c_addr, PCA9557_INPUT_PORT_REG, data, 1);

    return data[0] & 0x01;  // bit0 = 红外状态
}