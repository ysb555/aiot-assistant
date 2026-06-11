/**@file  c2.h
 * @brief    c2.c的头文件
 * @details  定义了zigbee的两种工作状态
 * @author      wangyuhang  any question please send mail to 1195341023@qq.com
 * @date        2023-7-5
 * @version     V1.0
 **********************************************************************************
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2023/07/05  <td>1.0      <td>wangyuhang  <td>创建初始版本
 * </table>
 *
 **********************************************************************************
 */
#ifndef C2_H
#define C2_H

#include <stdint.h>
#define TERMINAL 0
#define COORDINATOR 1
#define C2 "/dev/ttyS4"
// #define USART2 "/dev/i2c-3"
uint8_t c2_init(uint8_t zigbee_mode);
void c2_broadcast_data(char *send_data, uint8_t mode);
int c2_rec_data(uint8_t *rec_data, uint16_t len);
int c2_rec_data_poll(uint8_t *rec_data, uint16_t len);
int c2_rec_data_nonblock(uint8_t *rec_data, uint16_t len);
#endif
