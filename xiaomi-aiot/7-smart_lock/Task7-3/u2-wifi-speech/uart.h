/**@file  uart.h
* @brief    uart.c的头文件
* @details  定义了三个外部变量
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
#ifndef __UART_H__
#define __UART_H__


#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <sys/select.h>
#include <fcntl.h> //文件控制定义 
#include <termios.h>//终端控制定义 
#include <errno.h> 
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

 
#define RET_ERR  -1    
#define RET_OK   0
 
#define LEN_BUF_RECV_UNPACK 16
#define LEN_BUF_RECV 64
extern int serial_fd;
extern unsigned int total_send;
extern unsigned int total_length; 
int init_serial(const char* device);
int uart_send(int fd, uint8_t *data, int datalen);
int uart_recv(int fd, uint8_t *data, int datalen);
void delay_ms(uint16_t mstime);

#define S_TIMEOUT 1
#endif


