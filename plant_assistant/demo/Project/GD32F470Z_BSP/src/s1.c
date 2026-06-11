#include "i2c.h"
#include "s1.h"
#include "systick.h"

/* ==================== S1 矩阵按键驱动 (HT16K33) ==================== */

void s1_key_init(uint32_t i2c_periph, uint8_t i2c_addr) {
    // 开启振荡器 + 启用按键扫描
    i2c_delay_byte_write(i2c_periph, i2c_addr, HT16K33_SYSTEM_SETUP | 0x01, 0);
    // 开启显示 (INT/ROW 引脚用于按键扫描)
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x81 | 8, 0);
    // 启用中断
    i2c_delay_byte_write(i2c_periph, i2c_addr, HT16K33_INT_FLAG, 0);
}

uint8_t s1_read_key(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t buf[3] = {0};
    uint8_t row, col;

    i2c_delay_read(i2c_periph, i2c_addr, HT16K33_KEY_DATA, buf, 3);

    for (row = 0; row < 3; row++) {
        if (buf[row] != 0) {
            for (col = 0; col < 8; col++) {
                if (buf[row] & (1 << col)) {
                    return (row * 8 + col + 1);  // 按键编号 1~16
                }
            }
        }
    }
    return KEY_NONE;
}

uint8_t s1_key_scan(uint32_t i2c_periph, uint8_t i2c_addr, key_state_t *state) {
    uint8_t raw = s1_read_key(i2c_periph, i2c_addr);

    if (raw == KEY_NONE) {
        // 无按键
        state->current_key = KEY_NONE;
        state->press_count = 0;
        state->long_press_flag = 0;
        return KEY_NONE;
    }

    if (state->current_key != raw) {
        state->current_key = raw;
        state->press_count = 0;
        state->press_time = get_tick();
        return KEY_NONE;  // 去抖中
    }

    state->press_count++;
    if (state->press_count < 3) return KEY_NONE;  // 去抖

    // 检测长按
    if (get_tick() - state->press_time > KEY_LONG_PRESS_MS) {
        if (!state->long_press_flag) {
            state->long_press_flag = 1;
            return raw | 0x80;  // 长按标记
        }
        return KEY_NONE;
    }

    // 短按: 仅在释放时返回
    if (state->press_count == 3) {
        return raw;
    }

    return KEY_NONE;
}