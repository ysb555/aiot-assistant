#ifndef S5_H
#define S5_H

#include "i2c.h"

/* ==================== S5 NFC 读卡模块 (MS523) ==================== */
// I2C地址: 7位0x28, 左移0x50〜0x56
#define MS523_ADDRESS_S5            0x50

// MS523 命令
#define PCD_IDLE                    0x00    // 取消当前命令
#define PCD_MEM                     0x01    // 存储
#define PCD_AUTHENT                 0x0E    // 验证密钥
#define PCD_RECEIVE                 0x08    // 接收数据
#define PCD_TRANSMIT                0x04    // 发送数据
#define PCD_TRANSCEIVE              0x0C    // 收发数据
#define PCD_RESETPHASE              0x0F    // 复位
#define PCD_CALCCRC                 0x03    // CRC计算

// 寻卡/防冲突
#define PICC_REQIDL                 0x26    // 寻天线内未进入休眠的卡
#define PICC_REQALL                 0x52    // 寻天线内所有卡
#define PICC_ANTICOLL               0x93    // 防冲突
#define PICC_SELECTTAG              0x93    // 选卡
#define PICC_AUTHENT1A              0x60    // 验证A密钥
#define PICC_AUTHENT1B              0x61    // 验证B密钥
#define PICC_READ                   0x30    // 读块
#define PICC_WRITE                  0xA0    // 写块

// 卡类型
#define CARD_TYPE_UNKNOWN           0x00
#define CARD_TYPE_MIFARE_1K         0x04
#define CARD_TYPE_MIFARE_4K         0x08
#define CARD_TYPE_MIFARE_UL         0x44

/* 函数声明 */
uint8_t s5_pcd_request(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t req_code, uint8_t *card_type);
uint8_t s5_pcd_anticoll(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id);
uint8_t s5_pcd_select(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id);
uint8_t s5_card_read(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id, uint8_t block, uint8_t *data);
uint8_t s5_card_write(uint32_t i2c_periph, uint8_t i2c_addr, uint8_t *card_id, uint8_t block, uint8_t *data);

#endif // S5_H