#include "uart.h"

#define  out

#define WIFI_SSID 		"Mi 10"
#define WIFI_PASSWORD 	"12345678"

#define SERVER_IP 		"10.0.0.239"
#define SERVER_PORT 	"8080"


uint8_t c3_init(void);
int c3_wifi_tcp_init(int wifi_run_state,char *rec_data);
void c3_wifi_tcp_lead(int *wifi_run_state,int *tcp_status);
uint8_t c3_wifi_tcp_send(char *send_data);
uint16_t c3_wifi_tcp_receive(char *rec_data,uint16_t wait_time);
