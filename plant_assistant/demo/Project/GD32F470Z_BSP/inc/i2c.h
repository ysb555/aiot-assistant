#ifndef I2C_H
#define I2C_H

#include "gd32f4xx.h"

#define I2C0_SPEED              100000
#define I2C0_SLAVE_ADDRESS7     0xA0
#define I2C1_SPEED              100000
#define I2C1_SLAVE_ADDRESS7     0xA0

typedef struct {
    uint8_t  flag;
    uint32_t periph;
    uint8_t  addr;
} i2c_addr_def;

/* ========================== I2C 函数声明 ========================== */
void init_i2c(void);

// 地址扫描
uint8_t i2c_addr_poll(uint32_t i2c_periph, uint8_t poll_addr);

// 基础读写
void i2c_cmd_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t cmd);
void i2c_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer);
void i2c_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte);
uint8_t i2c_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte);
void i2c_direct_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t* p_buffer, uint16_t number_of_byte);

// 带延时的读写 (用于 E1/E2/S2 等外设)
void i2c_delay_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer);
uint8_t i2c_delay_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte);

// 通用地址获取
i2c_addr_def get_board_address(uint8_t address);

#endif // I2C_H