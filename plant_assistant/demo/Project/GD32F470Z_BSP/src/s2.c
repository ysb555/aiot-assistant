#include "i2c.h"
#include "s2.h"

/* ====================================================================
 *  S2 三合一传感器板驱动
 *  一块 S2 子板上集成了:
 *    - SHT35   温湿度传感器 (I2C: 0x88/0x8A)
 *    - BH1750  光照传感器   (I2C: 0x46/0xB8)
 *    - ICM20608 6轴陀螺仪   (I2C: 0xD0/0xD2, 可选)
 * ==================================================================== */

/* ========================== 内部延时 ========================== */
static void s2_delay_ms(uint16_t count) {
    volatile uint32_t decount;
    for (uint16_t i = 0; i < count; i++) {
        for (decount = 0; decount < 50000; decount++);
    }
}

/* ========================== SHT35 温湿度 ========================== */

static void s2_sht3x_softreset(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x30, 0xA2);
}

static uint8_t s2_sht3x_crc_cal(uint16_t DAT) {
    uint8_t i, t, temp;
    uint8_t CRC_BYTE = 0xFF;
    temp = (DAT >> 8) & 0xFF;
    for (t = 0; t < 2; t++) {
        CRC_BYTE ^= temp;
        for (i = 0; i < 8; i++) {
            if (CRC_BYTE & 0x80) {
                CRC_BYTE <<= 1;
                CRC_BYTE ^= 0x31;
            } else {
                CRC_BYTE <<= 1;
            }
        }
        if (t == 0) temp = DAT & 0xFF;
    }
    return CRC_BYTE;
}

s2_para s2_read_sht3x(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t  th_value[6];
    s2_para  sht_para;
    uint16_t tmp;

    s2_para zero_para = {0.0f, 0.0f};

    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x2C, 0x0D);
    s2_delay_ms(100);
    if (i2c_direct_read(i2c_periph, i2c_addr, th_value, 6) != 0) {
        return zero_para;
    }

    tmp = ((uint16_t)th_value[0] << 8) | th_value[1];
    if (s2_sht3x_crc_cal(tmp) == th_value[2]) {
        sht_para.temperature = (float)tmp * 175.0f / 65535.0f - 45.0f;
    } else {
        sht_para.temperature = 0.0f;
    }

    tmp = ((uint16_t)th_value[3] << 8) | th_value[4];
    if (s2_sht3x_crc_cal(tmp) == th_value[5]) {
        sht_para.humidity = (float)tmp * 100.0f / 65535.0f;
    } else {
        sht_para.humidity = 0.0f;
    }

    return sht_para;
}

// 便捷函数: 同时获取温度和湿度
void s2_get_temp_humi(uint32_t i2c_periph, uint8_t i2c_addr, float *temp, float *humi) {
    s2_para para = s2_read_sht3x(i2c_periph, i2c_addr);
    *temp = para.temperature;
    *humi = para.humidity;
}

/* ========================== BH1750 光照 ========================== */

static void s2_init_bh1750(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_cmd_write(i2c_periph, i2c_addr, 0x01);   // Power On
    i2c_cmd_write(i2c_periph, i2c_addr, 0x10);   // Continuous High Res Mode
}

float s2_read_bh1750_value(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint16_t tmp;
    uint8_t ss_value[2] = {0};
    float ss;

    if (i2c_direct_read(i2c_periph, i2c_addr, ss_value, 2) != 0) {
        return 0.0f;
    }

    tmp = ((uint16_t)ss_value[0] << 8) | ss_value[1];
    ss = (float)tmp * 10.0f / 12.0f;   // BH1750 标准转换
    return ss;
}

// 便捷函数: 读取光照 (兼容旧接口)
float s2_get_light(uint32_t i2c_periph, uint8_t i2c_addr) {
    return s2_read_bh1750_value(i2c_periph, i2c_addr);
}

/* ========================== ICM20608 6轴 (可选) ========================== */

static float s2_icm20608_gyro_scaleget(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t data;
    float gyroscale;
    i2c_delay_read(i2c_periph, i2c_addr, 0x1B, &data, 1);
    data = (data >> 3) & 0x03;
    switch (data) {
        case 0:  gyroscale = 131.0f;  break;
        case 1:  gyroscale = 65.5f;   break;
        case 2:  gyroscale = 32.8f;   break;
        case 3:  gyroscale = 16.4f;   break;
        default: gyroscale = 131.0f;  break;
    }
    return gyroscale;
}

static uint16_t s2_icm20608_accel_scaleget(uint32_t i2c_periph, uint8_t i2c_addr) {
    uint8_t data;
    uint16_t accelscale;
    i2c_delay_read(i2c_periph, i2c_addr, 0x1C, &data, 1);
    data = (data >> 3) & 0x03;
    switch (data) {
        case 0:  accelscale = 16384;  break;
        case 1:  accelscale = 8192;   break;
        case 2:  accelscale = 4096;   break;
        case 3:  accelscale = 2048;   break;
        default: accelscale = 16384;  break;
    }
    return accelscale;
}

static void s2_init_icm20608(uint32_t i2c_periph, uint8_t i2c_addr) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x6B, 0x80);  // 复位
    s2_delay_ms(100);
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x6B, 0x01);  // 唤醒
    s2_delay_ms(100);
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x19, 0x00);  // 采样率
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x1B, 0x18);  // 陀螺仪±2000dps
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x1C, 0x18);  // 加速度计±16g
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x1A, 0x04);  // 低通滤波
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x1D, 0x04);  // 加速度低通
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x6C, 0x00);  // 开启所有轴
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x1E, 0x00);  // 关闭低功耗
    i2c_delay_byte_write(i2c_periph, i2c_addr, 0x23, 0x00);  // 关闭FIFO
}

ax_para s2_read_icm20608_value(uint32_t i2c_periph, uint8_t i2c_addr) {
    ax_para icm_value;
    float gyroscale;
    uint16_t accescale;
    uint8_t data[14] = {0};
    int16_t gyro_x_adc, gyro_y_adc, gyro_z_adc;
    int16_t accel_x_adc, accel_y_adc, accel_z_adc;
    int16_t temp_adc;

    memset(&icm_value, 0, sizeof(icm_value));

    gyroscale = s2_icm20608_gyro_scaleget(i2c_periph, i2c_addr);
    accescale = s2_icm20608_accel_scaleget(i2c_periph, i2c_addr);

    if (i2c_delay_read(i2c_periph, i2c_addr, 0x3B, data, 14) != 0) {
        return icm_value;
    }

    accel_x_adc = ((int16_t)data[0] << 8) | data[1];
    accel_y_adc = ((int16_t)data[2] << 8) | data[3];
    accel_z_adc = ((int16_t)data[4] << 8) | data[5];
    temp_adc    = ((int16_t)data[6] << 8) | data[7];
    gyro_x_adc  = ((int16_t)data[8] << 8) | data[9];
    gyro_y_adc  = ((int16_t)data[10] << 8) | data[11];
    gyro_z_adc  = ((int16_t)data[12] << 8) | data[13];

    icm_value.gyro_x_act  = (float)gyro_x_adc / gyroscale;
    icm_value.gyro_y_act  = (float)gyro_y_adc / gyroscale;
    icm_value.gyro_z_act  = (float)gyro_z_adc / gyroscale;
    icm_value.accel_x_act = (float)accel_x_adc / accescale;
    icm_value.accel_y_act = (float)accel_y_adc / accescale;
    icm_value.accel_z_act = (float)accel_z_adc / accescale;
    icm_value.temp_act    = (float)((temp_adc - 25) * 10 / 3268) + 25.0f;

    return icm_value;
}

/* ========================== 初始化入口 ========================== */

i2c_addr_def s2_init(uint8_t address) {
    i2c_addr_def e_addess;
    uint8_t i;

    memset(&e_addess, 0, sizeof(e_addess));

    // SHT35 或 ICM20608 地址范围
    if ((address == TH_ADDRESS_S2) || (address == AX_ADDRESS_S2)) {
        for (i = 0; i < 2; i++) {
            if (i2c_addr_poll(I2C0, address + i * 2)) {
                e_addess.periph = I2C0;
                e_addess.addr = address + i * 2;
                e_addess.flag = 1;
                break;
            }
        }
        if (e_addess.flag != 1) {
            for (i = 0; i < 2; i++) {
                if (i2c_addr_poll(I2C1, address + i * 2)) {
                    e_addess.periph = I2C1;
                    e_addess.addr = address + i * 2;
                    e_addess.flag = 1;
                    break;
                }
            }
        }
        if (e_addess.flag) {
            if (e_addess.addr == TH_ADDRESS_S2 || e_addess.addr == (TH_ADDRESS_S2 + 2)) {
                s2_sht3x_softreset(e_addess.periph, e_addess.addr);
            } else if (e_addess.addr == AX_ADDRESS_S2 || e_addess.addr == (AX_ADDRESS_S2 + 2)) {
                s2_init_icm20608(e_addess.periph, e_addess.addr);
            }
        }
    }
    // BH1750 地址范围
    else if (address == S1_ADDRESS_S2) {
        e_addess = get_board_address(S1_ADDRESS_S2);
        if (e_addess.flag != 1) {
            e_addess = get_board_address(S2_ADDRESS_S2);
        }
        if (e_addess.flag) {
            s2_init_bh1750(e_addess.periph, e_addess.addr);
        }
    }

    return e_addess;
}

void s2_all_init(s2_addr_def *s2_address, uint8_t s2_th_addr, uint8_t s2_ss_addr, uint8_t s2_ax_addr) {
    s2_address->th_addr = s2_init(s2_th_addr);   // SHT35 温湿度
    s2_address->ss_addr = s2_init(s2_ss_addr);   // BH1750 光照
    s2_address->ax_addr = s2_init(s2_ax_addr);   // ICM20608 6轴
}