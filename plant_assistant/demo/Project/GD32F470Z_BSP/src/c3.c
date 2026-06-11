#include "uart.h"
#include "c3.h"
#include <stdio.h>
#include <string.h>

/* ========================== 内部辅助函数 ========================== */

static int Uart_write(char* sendbuf, int len) {
    return uart_send_bytes(USART0, (uint8_t*)sendbuf, len);
}

static int Uart_read(char* recvbuf, int len, int timeout) {
    return uart_rece_bytes(USART0, (uint8_t*)recvbuf, len, timeout);
}

static uint8_t at_cmd(char* send_data, char* expected, uint16_t wait_ms) {
    char recv_buf[80] = {0};
    int recv_cnt;

    if (Uart_write(send_data, strlen(send_data)) <= 0)
        return 0;

    recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), wait_ms);
    if (recv_cnt <= 0) return 0;

    return (strstr(recv_buf, expected) != NULL) ? 1 : 0;
}

/* ========================== C3 WiFi 驱动 ========================== */

/**
 * 初始化 C3 模块, 发送 AT 确认模块在线
 */
uint8_t c3_init(void) {
    char recv_buf[64];
    int recv_cnt = 0;

    for (int i = 0; i < 5; i++) {
        Uart_write("AT\r\n", 4);
        recv_cnt += Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 100);
        if (strstr(recv_buf, "OK") != NULL) {
            return 1;
        }
    }
    return 0;
}

/**
 * WiFi 连接状态机
 * wifi_run_state: 状态步骤 (1-8)
 * tcp_status: TCP连接状态 (-1=未连接, 0=已连接)
 */
void c3_wifi_tcp_lead(int* wifi_run_state, int* tcp_status) {
    char recv_buf[80] = {0};
    int recv_cnt;

    if (*tcp_status != -1) return;

    switch (*wifi_run_state) {
        case 1:  // 关闭回显
            if (at_cmd("ATE0\r\n", "OK", 300)) { (*wifi_run_state)++; }
            else { *wifi_run_state = 8; }
            break;

        case 2:  // 设置 Station 模式
            if (at_cmd("AT+CWMODE=1\r\n", "OK", 300)) { (*wifi_run_state)++; }
            else { *wifi_run_state = 8; }
            break;

        case 3:  // 检查是否已连接WiFi
            delay_ms(1000);
            Uart_write("AT+CWJAP?\r\n", 12);
            recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 300);
            if (strstr(recv_buf, "+CWJAP") != NULL) {
                *wifi_run_state = 5;  // 已连接, 跳到获取IP
                return;
            }
            if (strstr(recv_buf, "OK") != NULL) { (*wifi_run_state)++; }
            break;

        case 4:  // 连接 WiFi
            {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
                Uart_write(cmd, strlen(cmd));
                for (int x = 15; x > 0; x--) {
                    recv_cnt += Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 200);
                    if (strstr(recv_buf, "ERROR") != NULL) { *wifi_run_state = 8; return; }
                    if (strstr(recv_buf, "OK") != NULL || strstr(recv_buf, "WIFI GOT IP") != NULL) {
                        (*wifi_run_state)++;
                        return;
                    }
                }
                *wifi_run_state = 8;
            }
            break;

        case 5:  // 查询IP
            Uart_write("AT+CIFSR\r\n", 10);
            recv_cnt = Uart_read(recv_buf, sizeof(recv_buf), 300);
            if (strstr(recv_buf, "+CIFSR:STAIP,\"0.0.0.0\"") != NULL) {
                *wifi_run_state = 8; return;
            }
            if (strstr(recv_buf, "OK") != NULL) { (*wifi_run_state)++; }
            break;

        case 6:  // 连接 TCP 服务器
            {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", SERVER_IP, SERVER_PORT);
                Uart_write(cmd, strlen(cmd));
                for (int x = 10; x > 0; x--) {
                    recv_cnt += Uart_read(&recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt, 200);
                    if (strstr(recv_buf, "ERROR") != NULL) { *wifi_run_state = 8; return; }
                    if (strstr(recv_buf, "CONNECT") != NULL || strstr(recv_buf, "OK") != NULL) {
                        *tcp_status = 0;
                        *wifi_run_state = 1;
                        return;
                    }
                }
                *wifi_run_state = 8;
            }
            break;

        case 7: break;

        case 8:  // 重启模块
            Uart_write("AT+RST\r\n", 7);
            delay_ms(3000);
            *wifi_run_state = 1;
            break;

        default: break;
    }
}

/**
 * 通过 WiFi TCP 发送数据
 */
uint8_t c3_wifi_tcp_send(char* send_data) {
    char send_buffer[256];
    char recv_buf[80] = {0};
    int len = strlen(send_data);

    snprintf(send_buffer, sizeof(send_buffer), "AT+CIPSEND=%d\r\n", len);
    Uart_write(send_buffer, strlen(send_buffer));
    delay_ms(200);

    Uart_write(send_data, len);
    Uart_read(recv_buf, sizeof(recv_buf), 500);

    return (strstr(recv_buf, "SEND OK") != NULL) ? 1 : 0;
}

/**
 * 通过 WiFi TCP 接收数据
 */
uint16_t c3_wifi_tcp_receive(char* rec_data, uint16_t wait_time) {
    return Uart_read(rec_data, 300, wait_time);
}

/**
 * 发送浇水提醒 HTTP GET 请求
 */
void c3_send_water_alert(const char* plant_name) {
    char http_request[256];
    snprintf(http_request, sizeof(http_request),
             "GET %s?plant=%s&alert=water HTTP/1.1\r\nHost: %s:%s\r\nConnection: close\r\n\r\n",
             WATER_ALERT_PATH, plant_name, SERVER_IP, SERVER_PORT);
    c3_wifi_tcp_send(http_request);
}

/**
 * 发送通用 HTTP GET 请求 (完整版 - 带自定义查询参数)
 * path:        请求路径 (如 "/api/water_alert")
 * query_params: 查询参数字符串 (如 "plant=绿萝&temp=25.0&humi=60.0")
 */
void c3_send_http_get(const char* path, const char* query_params) {
    char http_request[512];
    snprintf(http_request, sizeof(http_request),
             "GET %s?%s HTTP/1.1\r\nHost: %s:%s\r\nConnection: close\r\n\r\n",
             path, query_params, SERVER_IP, SERVER_PORT);
    c3_wifi_tcp_send(http_request);
}