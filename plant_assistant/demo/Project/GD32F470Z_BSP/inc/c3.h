#ifndef C3_H
#define C3_H

#include "gd32f4xx.h"

#define out

/* ========================== WiFi 配置 ========================== */
#define WIFI_SSID       "Xiaomi_9539"
#define WIFI_PASSWORD   "1895@xiaomi"
#define SERVER_IP       "192.168.31.228"
#define SERVER_PORT     "8080"

/* ========================== 浇水提醒服务器路径 ========================== */
#define WATER_ALERT_PATH  "/water_alert"

/* ========================== 函数声明 ========================== */
uint8_t c3_init(void);
void c3_wifi_tcp_lead(int* wifi_run_state, int* tcp_status);
uint8_t c3_wifi_tcp_send(char* send_data);
uint16_t c3_wifi_tcp_receive(char* rec_data, uint16_t wait_time);

// HTTP GET 请求
void c3_send_water_alert(const char* plant_name);

#endif // C3_H