
/**@file  main.c
 * @brief       项目主函数文件
 * @details  主要包含协议应用栈程序框架，main函数入口
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
#include "stdio.h"
#include "string.h"
#include "uart.h"
#include "app.h"
#include "c2.h"
#include "c3.h"
#define DEBUG 0
uint8_t zigbee_mode = COORDINATOR;

int tcp_init_state = -1;
int run_wifi_state = 1;

char SSID[128];
char PWD[128];
char ip[15];
char port[5];

uint16_t rec_len;
char rec_buffer[200];
char output[200];
void parse_wifi_data(const char *raw_data, char *parsed_data)
{
	const char *data_start = strstr(raw_data, "+IPD,");
	if (data_start)
	{
		// 跳过"+IPD,"部分
		data_start += strlen("+IPD,");
		// 寻找":"
		const char *colon_pos = strchr(data_start, ':');
		if (colon_pos)
		{
			// 跳过冒号，获取实际数据
			strcpy(parsed_data, colon_pos + 1);
			// 去掉末尾的\r\n
			char *end = strstr(parsed_data, "\r\n");
			if (end)
			{
				*end = '\0';
			}
		}
	}
}
int *app(void)
{
	// 初始化wifi，tcp模块
	sprintf(SSID, "Xiaomi_9539");
	sprintf(PWD, "1895@xiaomi");
	sprintf(ip, "192.168.31.228");
	sprintf(port, "8080");
	int fd;
	if (c3_init(&fd) == 0)
	{
		printf("C3 init error.\n");
		return 1;
	}
	printf("C3 Ready.\n");
	// 初始化zigbee模块
	while (1)
	{
		printf("C2 init\n");
		char device[10] = BASE_DEVICE;
		char dev_num[1] = "0";
		int init_success = 0;
		// 轮询每个串口
		for (int i = 0; i < 6; i++)
		{
			dev_num[0] = (char)('0' + (char)i);
			device[9] = '\0';
			strcat(device, dev_num);
			// printf("dev_num = %c,device[9]=%c\n",dev_num[0],device[9]);
			// 打开对应串口
			if (init_serial(device) == -1)
			{
				// printf("not this device:%c\n",dev_num[0]);
				continue;
			}
			// 尝试在对应串口初始化zigbee
			if (c2_init(zigbee_mode) == 0)
			{
				// printf("not this device:%c\n",dev_num[0]);
				close(serial_fd);
				continue;
			}
			init_success = 1;
			break;
		}
		if (init_success == 0)
		{
			// 初始化失败，不是这个串口，或者设备不存在
			printf("C2 init error\n");
			// return 0;
			continue;
		}
		else
		{
			// 初始化成功
			printf("C2 uart_init success:serial_fd=%d,dev_num=%c\n", serial_fd, dev_num[0]);
			break;
		}
	}
	// 开始工作
	while (1)
	{
		c3_wifi_tcp_lead(fd, &run_wifi_state, &tcp_init_state);
		if (tcp_init_state == 0)
		{
			rec_len = c3_wifi_tcp_receive(fd, rec_buffer, sizeof(rec_buffer));
			if (rec_len > 0)
			{
				parse_wifi_data(rec_buffer, output);
				c2_broadcast_data(output, 0x01); // 广播到所有终端
				memset(rec_buffer, 0, sizeof(rec_buffer));
				memset(output, 0, sizeof(output));
#ifdef DEBUG
// debug		
				c3_wifi_tcp_send(fd, output);
#endif
			}
			delay_ms(200);
			// 从ZigBee接收数据并转发到WiFi
			rec_len = c2_rec_data(rec_buffer, sizeof(rec_buffer));
			if (rec_len > 0)
			{
				c3_wifi_tcp_send(fd, rec_buffer); // 通过WiFi发送
				memset(rec_buffer, 0, sizeof(rec_buffer));
			}
		}
		sleep(1);
	}
}
