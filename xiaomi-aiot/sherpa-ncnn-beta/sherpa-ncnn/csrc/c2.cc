/**@file  c2.c
 * @brief    负责通过uart通信发送指令控制zigbee模块的行为
 * @details  主要包含初始化zigbee模块、zigbee广播模块、zigbee接收模块
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

#include "c2.h"

#include "stdio.h"
#include "string.h"
#include "uart.h"

/*********************************************************************************************
函数名:    c2_init
功能:      初始化zigbee模块
入口参数:  zigbeemode:zigbee工作模式,0为终端，1为协调器
出口参数：  无
返回值：   成功返回1,失败返回0
作者：     wyh
日期:      2023/7/6
调用描述:  初始化zigbee模块工作模式,协调器或者终端
**********************************************************************************************/
uint8_t c2_init(uint8_t zigbee_mode) {
  uint8_t result = 0;
  uint8_t recvdata[50];
  uint8_t i;

  uint8_t terminal[5] = {0xFD, 0x02, 0x01, 0x02,
                         0xFF};  // 配置设备类型，配置为 02 终端
  uint8_t coordinator[5] = {0xFD, 0x02, 0x01, 0x00, 0xFF};  // 配置为协调器
  uint8_t pan_id[6] = {0xFD, 0x03, 0x03,
                       0x3F, 0x00, 0xFF};  // 配置PAN_ID 0x3f2c
  uint8_t group_id[5] = {
      0xFD, 0x02, 0x09, 0x01,
      0xFF};  // 配置网络组
              //  uint8_t test_state[4] = {0xFE,0x01,0x17,0xFF};
              //  //test,get working state,expect 0xFB,0X03
  uint8_t key[20] = {0xFD, 0x11, 0x04, 0x12, 0x13, 0x15, 0x17,
                     0x19, 0x1B, 0x1D, 0x1F, 0x10, 0x12, 0x14,
                     0x16, 0x18, 0x1A, 0x1C, 0x1D, 0xFF};
  result = 0;
  // 配置设备类型，通过uart发送指令，期望收到0xFA,0x01
  for (i = 0; i < 3; i++) {
    memset(recvdata, 0, 50);
    if (zigbee_mode == COORDINATOR)
      uart_send(serial_fd, coordinator, 5);
    else
      uart_send(serial_fd, terminal, 5);

    uart_recv(serial_fd, recvdata, 10);
    //  printf("配置设备类型 recvdata[0] ==%d,recvdata[1]
    //  ==%d\n",recvdata[0],recvdata[1]);
    if ((recvdata[0] == 0xFA) && (recvdata[1] == 0x01)) {
      result = 1;
      break;
    }
  }
  // 配置PAN_ID，通过uart发送指令，期望收到0xFA,0x03
  if (result) {
    result = 0;
    for (i = 0; i < 3; i++) {
      memset(recvdata, 0, 50);
      uart_send(serial_fd, pan_id, 6);
      uart_recv(serial_fd, recvdata, 10);
      //  printf("配置PAN_ID recvdata[0] ==%d,recvdata[1]
      //  ==%d\n",recvdata[0],recvdata[1]);
      if ((recvdata[0] == 0xFA) && (recvdata[1] == 0x03)) {
        result = 1;
        break;
      }
    }
  } else {
    printf("c2_init error:配置设备类型\n");
  }
  // 配置网络组，通过uart发送指令，期望收到0xFA,0x09
  if (result) {
    result = 0;
    for (i = 0; i < 3; i++) {
      memset(recvdata, 0, 50);
      uart_send(serial_fd, group_id, 5);
      uart_recv(serial_fd, recvdata, 10);
      //  printf("module4 recvdata[0] ==%d,recvdata[1]
      //  ==%d\n",recvdata[0],recvdata[1]);
      if ((recvdata[0] == 0xFA) && (recvdata[1] == 0x09)) {
        result = 1;
        break;
      }
    }

  } else {
    printf("c2_init error:配置PAN_ID\n");
  }
  if (result) {
    result = 0;
    for (i = 0; i < 3; i++) {
      memset(recvdata, 0, 50);
      uart_send(serial_fd, key, 20);
      uart_recv(serial_fd, recvdata, 10);
      //  printf("module4 recvdata[0] ==%d,recvdata[1]
      //  ==%d\n",recvdata[0],recvdata[1]);
      if ((recvdata[0] == 0xFA) && (recvdata[1] == 0x04)) {
        result = 1;
        break;
      }
    }
    if (!result) printf("c2_init error:配置秘钥\n");
  } else {
    printf("c2_init error:配置网络组\n");
  }
  return result;
}

/*********************************************************************************************
函数名:    c2_broadcast_data
功能:      c2发送广播数据
入口参数:  *send_data:待发送数据指针 mode:广播模式
                   01：广播模式 1 ——该消息广播到全网络中所有设备
                   02：广播模式 2
——该消息广播到只对打开了接收（除休眠模式）的设备 03：广播模式 3
——该消息广播到所有全功能设备（路由器和协调器） 出口参数： 无 返回值：   无
作者：     wyh
日期:      2023/7/6
调用描述:  发送广播数据
**********************************************************************************************/
void c2_broadcast_data(char *send_data, uint8_t mode) {
  char sendbuf[100];
  uint8_t len;

  sendbuf[0] = 0xFC;
  len = strlen(send_data);
  sendbuf[1] = len + 2;
  sendbuf[2] = 0x01;
  sendbuf[3] = mode;
  strncpy(&sendbuf[4], (const char *)send_data, len);
  if(uart_send(serial_fd, (uint8_t *)sendbuf, len + 4) < 0){
		perror("failed to send\n");
	}
}

/*********************************************************************************************
函数名:    c2_rec_data
功能:      接收zigbee发送数据
入口参数:  *rec_data:接收数据指针  len:接收数据长度
出口参数： *rec_data
返回值：   实际接收长度
作者：     wyh
日期:      2023/7/6
调用描述:  接收zigbee发送数据
**********************************************************************************************/
int c2_rec_data(uint8_t *rec_data, uint16_t len) {
  int recvlen;

  recvlen = uart_recv(serial_fd, rec_data, len);

  return recvlen;
}

// 基于poll的rec_data
int c2_rec_data_poll(uint8_t *rec_data, uint16_t len) {
  int recvlen;

  recvlen = uart_recv_poll(serial_fd, rec_data, len);

  return recvlen;
}

int c2_rec_data_nonblock(uint8_t *rec_data, uint16_t len) {
  int recvlen;

  recvlen = uart_recv_nonblock(serial_fd, rec_data, len);

  return recvlen;
}
