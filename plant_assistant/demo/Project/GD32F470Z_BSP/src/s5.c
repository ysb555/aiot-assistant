#include "s5.h"
#include "systick.h"
#include <string.h>

/* ==================== S5 NFC 驱动 (MS523) ==================== */
/* 参考: xiaomi-aiot/4-smart_fan/Task2-刷卡风扇/demo */

/* 写 MS523 寄存器 */
static void s5_write_reg(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t reg, uint8_t val) {
    i2c_delay_byte_write(i2c_periph, i2c_addr, reg, val);
}

/* 读 MS523 寄存器 */
static uint8_t s5_read_reg(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t reg) {
    uint8_t buf[1] = {0};
    i2c_delay_read(i2c_periph, i2c_addr, reg, buf, 1);
    return buf[0];
}

/* 通过FIFO发送命令 */
static uint8_t s5_pcd_command(uint32_t i2c_periph, uint8_t i2c_addr,
                               uint8_t cmd, uint8_t *send_data, uint8_t send_len,
                               uint8_t *recv_data, uint8_t *recv_len) {
    uint8_t status;
    uint8_t irq_en = 0x00;
    uint8_t wait_for;
    uint8_t n;
    uint8_t i;

    // 设置命令
    s5_write_reg(i2c_periph, i2c_addr, 0x01, 0x0F);  // 清除中断标志

    switch (cmd) {
        case PCD_AUTHENT:
            irq_en = 0x12;
            wait_for = 0x10;
            break;
        case PCD_TRANSCEIVE:
            irq_en = 0x77;
            wait_for = 0x30;
            break;
        default:
            break;
    }

    s5_write_reg(i2c_periph, i2c_addr, 0x02, irq_en | 0x80);
    s5_write_reg(i2c_periph, i2c_addr, 0x04, 0x80);  // 清除中断标志
    s5_write_reg(i2c_periph, i2c_addr, 0x0A, 0x80);  // 清零FIFO
    s5_write_reg(i2c_periph, i2c_addr, 0x01, 0x00);  // 停止

    // 写入FIFO
    for (i = 0; i < send_len; i++) {
        s5_write_reg(i2c_periph, i2c_addr, 0x09, send_data[i]);
    }

    // 执行命令
    s5_write_reg(i2c_periph, i2c_addr, 0x01, cmd);

    if (cmd == PCD_TRANSCEIVE) {
        s5_write_reg(i2c_periph, i2c_addr, 0x04, 0x80);  // 起始发送
    }

    // 等待完成
    n = 0;
    for (i = 0; i < 200; i++) {
        delay_1ms(1);
        n = s5_read_reg(i2c_periph, i2c_addr, 0x04);
        if (n & (wait_for | 0x01)) break;
    }

    s5_write_reg(i2c_periph, i2c_addr, 0x04, 0x80);  // 清除中断

    if (n & 0x01) return 0;  // 错误

    status = 0;
    if (cmd == PCD_TRANSCEIVE) {
        n = s5_read_reg(i2c_periph, i2c_addr, 0x0A);
        if (n > 0 && n <= 64) {
            *recv_len = n;
            for (i = 0; i < n; i++) {
                recv_data[i] = s5_read_reg(i2c_periph, i2c_addr, 0x09);
            }
        }
    }

    if (cmd == PCD_AUTHENT) {
        status = s5_read_reg(i2c_periph, i2c_addr, 0x08);
        if (!(status & 0x08)) return 0;
    }

    return 1;
}

/* 寻卡 */
uint8_t s5_pcd_request(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t req_code, uint8_t *card_type) {
    uint8_t status;
    uint8_t recv_len;
    uint8_t recv_buf[16];

    s5_write_reg(i2c_periph, i2c_addr, 0x14, 0x03);  // 清除位模式

    uint8_t tag_type[1] = {req_code};
    if (!s5_pcd_command(i2c_periph, i2c_addr, PCD_TRANSCEIVE, tag_type, 1, recv_buf, &recv_len))
        return 0;

    if (recv_len == 2) {
        *card_type = recv_buf[0] << 8 | recv_buf[1];
        return 1;
    }
    return 0;
}

/* 防冲突，获取卡号 */
uint8_t s5_pcd_anticoll(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id) {
    uint8_t status;
    uint8_t recv_len;
    uint8_t recv_buf[16];
    uint8_t send_buf[2] = {PICC_ANTICOLL, 0x20};

    s5_write_reg(i2c_periph, i2c_addr, 0x14, 0x00);

    if (!s5_pcd_command(i2c_periph, i2c_addr, PCD_TRANSCEIVE, send_buf, 2, recv_buf, &recv_len))
        return 0;

    if (recv_len == 5) {
        memcpy(card_id, recv_buf, 4);
        return 1;
    }
    return 0;
}

/* 选卡 */
uint8_t s5_pcd_select(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id) {
    uint8_t recv_len;
    uint8_t recv_buf[16];
    uint8_t send_buf[7] = {PICC_SELECTTAG, 0x70, 0, 0, 0, 0, 0};

    memcpy(&send_buf[2], card_id, 4);

    if (!s5_pcd_command(i2c_periph, i2c_addr, PCD_TRANSCEIVE, send_buf, 7, recv_buf, &recv_len))
        return 0;

    return 1;
}

/* 读卡块数据 */
uint8_t s5_card_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id,
                     uint8_t block, uint8_t *data) {
    uint8_t recv_len;
    uint8_t recv_buf[18];
    uint8_t send_buf[4] = {PICC_READ, block, 0, 0};

    if (!s5_card_auth(i2c_periph, i2c_addr, PICC_AUTHENT1A, block, card_id))
        return 0;

    if (!s5_pcd_command(i2c_periph, i2c_addr, PCD_TRANSCEIVE, send_buf, 2, recv_buf, &recv_len))
        return 0;

    if (recv_len == 16) {
        memcpy(data, recv_buf, 16);
        return 1;
    }
    return 0;
}

/* 写卡块数据 */
uint8_t s5_card_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id,
                      uint8_t block, uint8_t *data) {
    uint8_t recv_len;
    uint8_t recv_buf[4];
    uint8_t send_buf[18];

    if (!s5_card_auth(i2c_periph, i2c_addr, PICC_AUTHENT1B, block, card_id))
        return 0;

    send_buf[0] = PICC_WRITE;
    send_buf[1] = block;
    memcpy(&send_buf[2], data, 16);

    if (!s5_pcd_command(i2c_periph, i2c_addr, PCD_TRANSCEIVE, send_buf, 18, recv_buf, &recv_len))
        return 0;

    return 1;
}

/* 验证密钥 (内部函数) */
static uint8_t s5_card_auth(uint32_t i2c_periph, uint8_t i2c_addr,
                             uint8_t auth_mode, uint8_t block, uint8_t *card_id) {
    uint8_t recv_len;
    uint8_t recv_buf[4];
    uint8_t send_buf[12];

    send_buf[0] = auth_mode;
    send_buf[1] = block;
    // 默认密钥 FFFFFFFFFFFF
    for (int i = 2; i < 8; i++) send_buf[i] = 0xFF;
    memcpy(&send_buf[8], card_id, 4);

    return s5_pcd_command(i2c_periph, i2c_addr, PCD_AUTHENT, send_buf, 12, recv_buf, &recv_len);
}