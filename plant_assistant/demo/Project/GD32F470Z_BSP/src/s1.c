#include "i2c.h"
#include "s1.h"

/* ========================== S1 矩阵按键驱动 ========================== */

/**
 * 初始化 S1 矩阵按键
 */
i2c_addr_def s1_init(uint8_t address) {
    i2c_addr_def dev;
    dev.flag = 0;
    dev.periph = I2C0;
    dev.addr = 0;

    if (i2c_addr_poll(I2C0, address)) {
        dev.periph = I2C0;
        dev.addr = address;
        dev.flag = 1;
    } else if (i2c_addr_poll(I2C1, address)) {
        dev.periph = I2C1;
        dev.addr = address;
        dev.flag = 1;
    }

    return dev;
}

/**
 * 扫描按键
 * 返回: S1_KEY_NONE(无按键), S1_KEY_SWITCH(切换键)
 */
uint8_t s1_scan_key(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t key_val = 0;

    // 通过 I2C 读取按键寄存器
    // 不同S1固件版本可能不同，这里读取通用按键状态寄存器
    if (i2c_delay_read(i2c_periph, i2c_addr, 0x00, &key_val, 1)) {
        return S1_KEY_NONE;
    }

    // 按键消抖: 如果按键值不为0，等待10ms再读一次
    if (key_val != 0) {
        for (volatile uint32_t d = 0; d < 500000; d++);
        uint8_t key_val2 = 0;
        i2c_delay_read(i2c_periph, i2c_addr, 0x00, &key_val2, 1);
        if (key_val == key_val2) {
            return S1_KEY_SWITCH;
        }
    }

    return S1_KEY_NONE;
}