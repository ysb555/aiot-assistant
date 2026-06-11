#include "s6.h"
#include "systick.h"

/* ==================== S6 超声波驱动 ==================== */
/* 参考: xiaomi-aiot/5-smart_clock/Clock-DEMO/demo */

/**
 * 读取超声波距离 (mm)
 * i2c_periph: I2C外设
 * i2c_addr: S6 I2C地址
 * distance_mm: 输出距离值
 * 返回: 0成功, 非0失败
 */
uint8_t s6_read_distance(uint32_t i2c_periph, uint8_t i2c_addr, uint16_t *distance_mm) {
    uint8_t recvdata[2] = {0};

    // 发送读取命令
    i2c_delay_byte_write(i2c_periph, i2c_addr, GD32F330_READ_CMD, 0);

    // 等待测量完成
    delay_1ms(60);

    // 读取2字节数据
    if (i2c_delay_read(i2c_periph, i2c_addr, 0x00, recvdata, 2) != 0) {
        *distance_mm = 0;
        return 1;
    }

    *distance_mm = ((uint16_t)recvdata[0] << 8) | recvdata[1];
    return 0;
}