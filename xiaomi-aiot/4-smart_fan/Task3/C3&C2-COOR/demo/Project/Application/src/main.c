/*************************************************************************************************************
����:     ��ʼ��wifiģ���,ÿ3�������õķ��������Ͳ�������
�汾:     2023-5-16 V1.0
�޸ļ�¼: ��
����:     ZZZ
�����ʽ��GB2312
*************************************************************************************************************/

#include "gd32f4xx.h"
#include "stdio.h"
#include "string.h"
#include "uart.h"
#include "c3.h"
#include "c2.h"
#include "gd32f470z_eval.h"
uint8_t second_flag = 0;
uint16_t second_count = 0;
volatile uint8_t c3_result, c2_result; // c3��ʼ���������������1  ��������Ӧ����0
int tcp_init_state = -1;               // tcp״̬
int run_wifi_step = 1;                 // wifi��ʼ������
uint16_t rec_len;                      // wifi�·����ݵĳ���
char rec_buffer[150];                  //
char output[100];
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

int main(void)
{
  // 初始化模块
  nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);
  nvic_irq_enable(USART0_IRQn, 2, 0); // WiFi?????
  nvic_irq_enable(USART2_IRQn, 1, 0); // ZigBee?????
  uart_init(USART0);                  // WiFi
  uart_init(USART2);                  // ZigBee
  timer3_init();
  gd_eval_com_init(EVAL_COM0);
  // 初始化状态变量
  c3_result = c3_init();
  c2_result = c2_init(0x01);

  // 主循环
  while (1)
  {
    // 从WiFi接收数据并转发到ZigBee
    if (second_flag)
    {
      second_flag = 0;
      c3_wifi_tcp_lead(&run_wifi_step, &tcp_init_state);
    }
    rec_len = c3_wifi_tcp_receive(rec_buffer, sizeof(rec_buffer));
    if (rec_len > 0)
    {
      parse_wifi_data(rec_buffer, output);
      c2_broadcast_data(output, 0x01); // 广播到所有终端
      memset(rec_buffer, 0, sizeof(rec_buffer));
      memset(output, 0, sizeof(output));
    }
    delay_ms(200);
    // 从ZigBee接收数据并转发到WiFi
    rec_len = c2_rec_data(rec_buffer, sizeof(rec_buffer), 300);
    if (rec_len > 0)
    {
      c3_wifi_tcp_send(rec_buffer); // 通过WiFi发送
      memset(rec_buffer, 0, sizeof(rec_buffer));
    }
  }
}
