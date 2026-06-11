#include "i2c.h"

static uint16_t i2c_over_time = 0;

static uint8_t judge_i2c_over_time(void) {
    i2c_over_time++;
    return (i2c_over_time > 60000) ? 1 : 0;
}

/* ========================== 初始化 ========================== */

void init_i2c(void) {
    // I2C0: PB8(SCL), PB9(SDA)
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_8);
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_9);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    i2c_clock_config(I2C0, I2C0_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C0_SLAVE_ADDRESS7);
    i2c_enable(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    // I2C1: PF1(SCL), PF0(SDA)
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_I2C1);

    gpio_af_set(GPIOF, GPIO_AF_4, GPIO_PIN_1);
    gpio_af_set(GPIOF, GPIO_AF_4, GPIO_PIN_0);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_0);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_1);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_1);

    i2c_clock_config(I2C1, I2C1_SPEED, I2C_DTCY_2);
    i2c_mode_addr_config(I2C1, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C1_SLAVE_ADDRESS7);
    i2c_enable(I2C1);
    i2c_ack_config(I2C1, I2C_ACK_ENABLE);
}

/* ========================== 地址扫描 ========================== */

uint8_t i2c_addr_poll(uint32_t i2c_periph, uint8_t poll_addr) {
    uint16_t i = 0;

    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (++i > 60000) return 0;
    }

    i2c_start_on_bus(i2c_periph);
    i = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (++i > 60000) return 0;
    }

    i2c_master_addressing(i2c_periph, poll_addr, I2C_TRANSMITTER);
    i = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
        if (++i > 60000) {
            i2c_stop_on_bus(i2c_periph);
            return 0;
        }
    }

    i2c_stop_on_bus(i2c_periph);
    i = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (++i > 60000) return 0;
    }
    return 1;
}

/* ========================== 基础 I2C 操作 ========================== */

void i2c_cmd_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t cmd) {
    i2c_over_time = 0;
    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (judge_i2c_over_time()) return;
    }

    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    i2c_over_time = 0;
    while (SET != i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_data_transmit(i2c_periph, cmd);

    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_stop_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (judge_i2c_over_time()) return;
    }
}

void i2c_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer) {
    i2c_over_time = 0;
    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (judge_i2c_over_time()) return;
    }

    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    i2c_over_time = 0;
    while (SET != i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_data_transmit(i2c_periph, write_address);

    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_data_transmit(i2c_periph, buffer);

    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_stop_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (judge_i2c_over_time()) return;
    }
}

void i2c_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t* p_buffer, uint8_t number_of_byte) {
    i2c_over_time = 0;
    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (judge_i2c_over_time()) return;
    }

    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    i2c_over_time = 0;
    while (SET != i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }
    i2c_data_transmit(i2c_periph, write_address);

    for (uint8_t n = 0; n < number_of_byte; n++) {
        i2c_over_time = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
            if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
        }
        i2c_data_transmit(i2c_periph, p_buffer[n]);
    }

    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_stop_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (judge_i2c_over_time()) return;
    }
}

uint8_t i2c_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte) {
    i2c_over_time = 0;
    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (judge_i2c_over_time()) return 1;
    }

    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return 1; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_TRANSMITTER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return 1; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    i2c_over_time = 0;
    while (SET != i2c_flag_get(i2c_periph, I2C_FLAG_TBE)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return 1; }
    }
    i2c_data_transmit(i2c_periph, read_address);

    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_BTC)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return 1; }
    }

    // 重新启动，切换为接收模式
    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return 1; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_RECEIVER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return 1; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    for (uint16_t n = 0; n < number_of_byte; n++) {
        if (n == number_of_byte - 1) {
            i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        }
        i2c_over_time = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_RBNE)) {
            if (judge_i2c_over_time()) {
                i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
                i2c_stop_on_bus(i2c_periph);
                return 1;
            }
        }
        p_buffer[n] = i2c_data_receive(i2c_periph);
    }

    i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    i2c_stop_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (judge_i2c_over_time()) return 0;
    }
    return 0;
}

void i2c_direct_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t* p_buffer, uint16_t number_of_byte) {
    i2c_over_time = 0;
    while (i2c_flag_get(i2c_periph, I2C_FLAG_I2CBSY)) {
        if (judge_i2c_over_time()) return;
    }

    i2c_start_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (!i2c_flag_get(i2c_periph, I2C_FLAG_SBSEND)) {
        if (judge_i2c_over_time()) { i2c_stop_on_bus(i2c_periph); return; }
    }

    i2c_master_addressing(i2c_periph, i2c_addr, I2C_RECEIVER);
    {
        uint16_t _i = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_ADDSEND)) {
            if (++_i > 60000) { i2c_stop_on_bus(i2c_periph); return; }
        }
    }
    i2c_flag_clear(i2c_periph, I2C_FLAG_ADDSEND);

    for (uint16_t n = 0; n < number_of_byte; n++) {
        if (n == number_of_byte - 1) {
            i2c_ack_config(i2c_periph, I2C_ACK_DISABLE);
        }
        i2c_over_time = 0;
        while (!i2c_flag_get(i2c_periph, I2C_FLAG_RBNE)) {
            if (judge_i2c_over_time()) {
                i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
                i2c_stop_on_bus(i2c_periph);
                return;
            }
        }
        p_buffer[n] = i2c_data_receive(i2c_periph);
    }

    i2c_ack_config(i2c_periph, I2C_ACK_ENABLE);
    i2c_stop_on_bus(i2c_periph);
    i2c_over_time = 0;
    while (I2C_CTL0(i2c_periph) & 0x0200) {
        if (judge_i2c_over_time()) return;
    }
}

/* ========================== 带延时的 I2C 操作 ========================== */

void i2c_delay_byte_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t write_address, uint8_t buffer) {
    i2c_byte_write(i2c_periph, i2c_addr, write_address, buffer);
    // 微小延时确保时序
    for (volatile uint32_t d = 0; d < 1000; d++);
}

uint8_t i2c_delay_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte) {
    uint8_t ret = i2c_read(i2c_periph, i2c_addr, read_address, p_buffer, number_of_byte);
    for (volatile uint32_t d = 0; d < 1000; d++);
    return ret;
}

/* ========================== 通用地址获取 ========================== */

i2c_addr_def get_board_address(uint8_t address) {
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