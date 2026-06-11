/**@file  main.h
* @brief       main.c的头文件
* @details  定义了串口的路径
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

#ifndef APP_H
#define APP_H

#include "stdint.h"
#define BASE_DEVICE "/dev/ttyS"
#define DEVICE "/dev/ttyS4" 

// void delay_ms(uint16_t mstime);
// uint16_t uart_print(uint32_t usart_periph, uint8_t *data, uint16_t len);
// void debug_printf(uint32_t usart_periph,char *string);


#endif /* MAIN_H */
