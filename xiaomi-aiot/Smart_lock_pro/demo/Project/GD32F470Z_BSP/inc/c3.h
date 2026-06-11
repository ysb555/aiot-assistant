#include "uart.h"

#define  out

#define SERVER_IP      "192.168.31.288"
#define SERVER_PORT    "8080"


#define WIFI_SSID      "Xiaomi_9539"
#define WIFI_PASSWORD  "1895@xiaomi"


uint8_t c3_init(void);
int c3_wifi_tcp_init(int wifi_run_state,char *rec_data);
void c3_wifi_tcp_lead(int *wifi_run_state,int *tcp_status);
uint8_t c3_wifi_tcp_send(char *send_data);
uint16_t c3_wifi_tcp_receive(char *rec_data,uint16_t wait_time);
