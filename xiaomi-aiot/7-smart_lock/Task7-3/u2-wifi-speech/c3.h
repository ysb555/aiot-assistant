#ifndef C3_H
#define C3_H

#include "stdio.h"
#include "string.h"
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <getopt.h>

extern char SSID[128];
extern char PWD[128];
extern char ip[15];
extern char port[5];

uint8_t c3_init(int *fd);
void c3_wifi_tcp_lead(int fd,int *wifi_run_state,int *tcp_status);
uint16_t c3_wifi_tcp_receive(int fd,char* buffer,int buf_len);
uint8_t c3_wifi_tcp_send(int fd,char *send_data);

#endif