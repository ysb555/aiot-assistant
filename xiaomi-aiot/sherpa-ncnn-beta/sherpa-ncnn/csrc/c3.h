#ifndef C3_H
#define C3_H

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#include "stdio.h"
#include "string.h"
#include "time.h"

#define SSID "Xiaomi_9539"
#define PASSWD "1895@xiaomi"
#define PORT "8080"

uint8_t c3_init(int *fd, char _SSID[128], char _PWD[128], char _ip[15],
                char _port[5]);
void c3_wifi_tcp_lead(int fd, int *wifi_run_state, int *tcp_status);
uint16_t c3_wifi_tcp_receive(int fd, char *buffer, size_t buf_len);
uint8_t c3_wifi_tcp_send(int fd, char *send_data);

#endif