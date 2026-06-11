#include "i2c.h"
#include "s2.h"

/**
 * 初始化 BH1750 光照传感器 (S2)
 * address: 起始扫描地址 0x46
 */
i2c_addr_def s2_init(uint8_t address) {
    i2c_addr_def e_address;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        e_address = get_board_address(address + i * 2);
        if (e_address.flag) {
            // 上电并设置连续高分辨率模式
            i2c_delay_byte_write(e_address.periph, e_address.addr, BH1750_POWER_ON, 0);
            i2c_delay_byte_write(e_address.periph, e_address.addr, BH1750_CONT_H_MODE, 0);
            break;
        }
    }
    return e_address;
}

/**
 * 扫描所有 S2 子板 (最多4个)
 */
void s2_all_init(s2_addr_def *s2_address, uint8_t s2_addr) {
    uint8_t i;
    for (i = 0; i < 4; i++) {
        s2_address->light_addr[i] = get_board_address(s2_addr + i * 2);
        if (s2_address->light_addr[i].flag) {
            i2c_delay_byte_write(s2_address->light_addr[i].periph,
                                 s2_address->light_addr[i].addr,
                                 BH1750_POWER_ON, 0);
            i2c_delay_byte_write(s2_address->light_addr[i].periph,
                                 s2_address->light_addr[i].addr,
                                 BH1750_CONT_H_MODE, 0);
        }
    }
}

/**
 * 读取光照强度 (lux)
 * 返回: 光照值 (float)
 */
float s2_get_light(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t buf[2] = {0};
    uint16_t raw;

    if (i2c_delay_read(i2c_periph, i2c_addr, 0x00, buf, 2) != 0)
        return 0.0f;

    raw = ((uint16_t)buf[0] << 8) | buf[1];
    return raw / 1.2f;  // BH1750 标准转换公式
}