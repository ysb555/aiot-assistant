#include "e3.h"

/* ==================== E3 窗帘电机驱动 (HR8833) ==================== */
/* 参考: xiaomi-aiot/3-auto_curtain_machine/Task3-1/E3-DEMO/demo */

/**
 * 初始化 E3 窗帘电机
 * address: 扫描起始地址 0x38
 */
i2c_addr_def e3_init(uint8_t address) {
    i2c_addr_def e_address;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        e_address = get_board_address(address + i * 2);
        if (e_address.flag) {
            // 初始设置位置为0 (完全打开)
            i2c_delay_byte_write(e_address.periph, e_address.addr, E3_REG_POSITION, 0);
            break;
        }
    }
    return e_address;
}

/**
 * 设置窗帘位置
 * position: 0(完全打开) ~ 100(完全关闭)
 */
void e3_set_position(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t position) {
    if (position > 100) position = 100;
    i2c_delay_byte_write(i2c_periph, i2c_addr, E3_REG_POSITION, position);
}

/**
 * 设置窗帘电机速度
 * speed: 0~100
 */
void e3_set_speed(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t speed) {
    if (speed > 100) speed = 100;
    i2c_delay_byte_write(i2c_periph, i2c_addr, E3_REG_SPEED, speed);
}

/**
 * 完全打开窗帘
 */
void e3_open(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, E3_REG_DIRECTION, 0);
    e3_set_position(i2c_periph, i2c_addr, 0);
}

/**
 * 完全关闭窗帘
 */
void e3_close(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, E3_REG_DIRECTION, 1);
    e3_set_position(i2c_periph, i2c_addr, 100);
}