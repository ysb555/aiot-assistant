#include "c3.h"
#include <sys/select.h>

char SSID[128];
char PWD[128];
char ip[15];
char port[5];
int c3_wifi_tcp_init(int fd, int wifi_run_state, char *rec_date);
int c3_wifi_net_status_get(char *recv_buf);

// const char* uart_path="/dev/ttyS0";

/**@brief 初始化C3子板与U2主板的连接。
 * @param[in] *fd 保存C3子板设备的文件标识符
 * @return 函数执行结果
 * - 0 执行出现错误
 * - 1 初始化成功
 */
uint8_t c3_init(int *fd, char _SSID[128], char _PWD[128], char _ip[15],
                char _port[5])
{
    memcpy(SSID, _SSID, sizeof(SSID));
    memcpy(PWD, _PWD, sizeof(SSID));
    memcpy(ip, _ip, sizeof(SSID));
    memcpy(port, _port, sizeof(SSID));
    int i;
    int recv_cnt = 0;
    char recv_buf[64] = {0};
    (*fd) = open("/dev/ttyS3", O_RDWR | O_NONBLOCK | O_NDELAY);
    if ((*fd) < 0)
    {
        printf("open failed.\n");
        return 0;
    }
    if (fcntl((*fd), F_SETFL, 0) < 0)
    {
        printf("fcntl check failure\n");
        return 0;
    }
    if (0 == isatty(*fd))
    {
        printf("[ttyS3:%d] is not a terminal device\n", *fd);
        return 0;
    }
    struct termios term;
    memset(&term, 0, sizeof(term));
    int err;

    if (tcgetattr((*fd), &term) == -1)
    {
        printf("Cannot get standard input descriptermn");
        return 0;
    }

    // term.c_lflag = (ICANON | ECHO | ECHOE);
    /* Modify the control mode to ensure that the program does not occupy the serial port */
    term.c_cflag |= CLOCAL;

    /* Start the receiver and read the input data from the serial port */
    term.c_cflag |= CREAD;

    /* CSIZE character size mask that will be associated with setting databits to the Peugeot position zero */
    term.c_cflag &= ~CSIZE;

    /*
     * ICANON: standard input
     * ECHO:   the entered characters are displayed
     * ECHOE:  If the ICANON flag is set, ERASE deletes the previous character and WERASE deletes the previous word
     * ISIG:   When the INTR/QUIT/SUSP/DSUSP characters are received, a corresponding signal is generated
     */
    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /*
     * BRKINT: BREAK discards the data in the input and output queues (flush)
     * ICRNL:  Convert CR in the input to NL
     * INPCK:  Parity check is allowed
     * ISTRIP: Strip the eighth bit
     * IXON:   Stripping the 8th bit allows XON/XOF flow control on the output
     */
    term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* OPOST: Represents the output after processing, according to the original data output */
    term.c_oflag &= ~OPOST;

    term.c_cflag |= CS8;

    term.c_cflag &= ~PARENB;
    term.c_iflag &= ~INPCK;

    term.c_cflag &= ~CSTOPB;

    cfsetispeed(&term, B115200);
    cfsetospeed(&term, B115200);

    term.c_lflag &= ~ECHO;

    if (tcsetattr((*fd), TCSANOW, &term) < 0)
    {
        printf("set error.\n");
        return 0;
    }

    for (i = 0; i < 5; i++) // 读取返回数据
    {
        write((*fd), "AT\r\n", strlen("AT\r\n")); // 检测AT命令是否正常
        sleep(1);
        recv_cnt += read((*fd), &recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt);
        if (strstr(recv_buf, "OK") != NULL)
        {
            return 1;
        }
    }
    return 0;
}

/**@brief 接收电脑发送到开发板的信息
 * @param[in] fd C3子板设备的文件标识符
 * @param[in] *buffer 储存返回消息的缓冲区字符串的头指针。
 * @param[in] buf_len 缓冲区大小
 * @return 读取的消息长度
 */
uint16_t c3_wifi_tcp_receive(int fd, char *buffer, size_t buf_len)
{
    uint16_t rec_len;
    char rec_buffer[300];
    rec_len = read(fd, rec_buffer, buf_len);
    printf("============================>\n%s\n<=============================\n", rec_buffer);
    return rec_len;
}

/**@brief 初始化开发板到电脑的TCP连接
 * @param[in] fd C3子板设备的文件标识符
 * @param[in] *wifi_run_state 储存当前wifi中tcp连接建立的状态的指针
 * @param[in] *tcp_status 储存当前tcp状态的指针
 */
void c3_wifi_tcp_lead(int fd, int *wifi_run_state, int *tcp_status)
{
    char rec_buf[80] = {0};
    int net_state;

    if (*tcp_status == -1)
    {
        int wifi_rec = 0;
        wifi_rec = c3_wifi_tcp_init(fd, *wifi_run_state, rec_buf); // 连接服务器
        if (wifi_rec == 0)
        {
            if (*wifi_run_state == 6)
            {
                *tcp_status = 0;
                *wifi_run_state = 1;
            }
            else
            {
                *wifi_run_state = *wifi_run_state + 1;
            }
        }
        else if (wifi_rec == 1)
        {
            *wifi_run_state = 5;
        }
        else if (wifi_rec == -1)
        {
            *wifi_run_state = 8;
        }
        else if (wifi_rec == -2)
        {
            *wifi_run_state = 1;
        }
        net_state = c3_wifi_net_status_get(rec_buf); /////
        if (net_state == -1)
            *wifi_run_state = 8;
        else if (net_state == -3)
            *wifi_run_state = 1;
    }
}

int c3_wifi_tcp_init(int fd, int wifi_run_state, char *rec_data)
{
    int recv_cnt;
    char recv_buf[80] = {0};
    printf("%d\n", wifi_run_state);
    switch (wifi_run_state)
    {
    case 1: // 关闭回显
    {
        if (write(fd, "ATE0\r\n", strlen("ATE0\r\n")) <= 0)
        {
            goto WIFI_ERR;
        }
        usleep(100000);
        // 读取返回数据
        recv_cnt = read(fd, recv_buf, sizeof(recv_buf));

        if (recv_cnt <= 0)
        {
            goto WIFI_ERR;
        }
        if (strstr(recv_buf, "OK") != NULL)
        {
            goto WIFI_OK;
        }
        break;
    }
    case 2: // 设置模式
    {
        if (write(fd, "AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n")) <= 0)
        {
            goto WIFI_ERR;
        }
        // 读取返回数据
        recv_cnt = read(fd, recv_buf, sizeof(recv_buf));
        usleep(100000);
        if (recv_cnt <= 0)
        {
            goto WIFI_ERR;
        }
        if (strstr(recv_buf, "OK") != NULL)
        {
            goto WIFI_OK;
        }
        break;
    }
    case 3: // 是否连接到网络
    {
        recv_cnt = 0;
        sleep(1);
        if (write(fd, "AT+CWJAP?\r\n", strlen("AT+CWJAP?\r\n")) <= 0)
        {
            goto WIFI_ERR;
        }
        // 读取返回数据
        recv_cnt = read(fd, recv_buf, sizeof(recv_buf));
        usleep(100000);
        char *rec = strstr(recv_buf, "+CWJAP");
        if (rec != NULL)
        {
            memcpy(rec_data, recv_buf, recv_cnt);
            return 1; // 网络连接成功  跳过网络配置
        }
        if (recv_cnt <= 0)
        {
            goto WIFI_ERR;
        }
        if (strstr(recv_buf, "OK") != NULL)
        {
            goto WIFI_OK;
        }
        break;
    }
    case 4: // 配置WIFI信息
    {
        recv_cnt = 0;
        char order[300] = {0};
        sprintf(order, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PWD);
        if (write(fd, order, strlen(order)) <= 0)
        {
            goto WIFI_ERR;
        }
        printf("wifi connecting.\n");
        // 读取返回数据
        for (int x = 10; x != 0; x--)
        {
            recv_cnt += read(fd, &recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt);
            usleep(100000);
            if (strstr(recv_buf, "ERROR") != NULL)
            {
                break;
            }
            if (strstr(recv_buf, "OK") != NULL)
            {
                printf("wifi connected.\n");
                goto WIFI_OK;
            }
        }
        goto WIFI_ERR;
    }
    case 5: // 查询网络注册是否成功
    {
        recv_cnt = 0;
        if (write(fd, "AT+CIFSR\r\n", strlen("AT+CIFSR\r\n")) <= 0)
        {
            goto WIFI_ERR;
        }
        sleep(1);
        // 读取返回数据
        recv_cnt = read(fd, &recv_buf[recv_cnt], sizeof(recv_buf) - recv_cnt);
        if (recv_cnt <= 0)
        {
            break;
        }
        char *rec = strstr(recv_buf, "+CIFSR:STAIP,\"0.0.0.0\"");
        if (rec != NULL)
        {
            goto WIFI_ERR;
        }
        if (strstr(recv_buf, "OK") != NULL)
        {
            goto WIFI_OK;
        }
        break;
    }
    case 6: // 配置网络IP和端口
    {
        recv_cnt = 0;
        char order[80] = {0};
        sprintf(order, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", ip, port);
        if (write(fd, order, strlen(order)) <= 0)
        {
            goto WIFI_ERR;
        }
        // 读取返回数据
        for (int x = 10; x != 0; x--)
        {
            recv_cnt += read(fd, &recv_buf[recv_cnt], (sizeof(recv_buf) - recv_cnt));
            if (strstr(recv_buf, "ERROR") != NULL)
            {
                printf("ip&port setting failed.\n");
                goto WIFI_ERR;
                // goto WIFI_OK;
            }
            if (strstr(recv_buf, "CONNECT") != NULL)
            {
                printf("connected.\n");
                // break;
                goto WIFI_OK;
            }
            if (strstr(recv_buf, "OK") != NULL)
            {
                printf("connected.\n");
                goto WIFI_OK;
            }
        }
        break;
    }
    case 8:
    {
        write(fd, "AT+RST\r\n", strlen("AT+RST\r\n"));
        sleep(2);
        return -2;
    }
    default:
        break;
    }
WIFI_ERR:
    memcpy(rec_data, recv_buf, recv_cnt);
    return -1;

WIFI_OK:
    memcpy(rec_data, recv_buf, recv_cnt);
    return 0;
}

int c3_wifi_net_status_get(char *recv_buf)
{
    // 读取返回数据
    char *rec = strstr(recv_buf, "ERROR");
    if (rec != NULL)
    {
        // printf("C3 ERROR\n");
    }

    rec = strstr(recv_buf, "+IPD,");
    if (rec == NULL)
    {
        rec = strstr(recv_buf, "CLOSED");
        if (rec != NULL)
        {
            return -1;
        }
    }

    rec = strstr(recv_buf, "ready\r\n");
    if (rec != NULL)
    {
        return -3;
    }

    return 0;
}

/**@brief 从开发板发送信息到电脑
 * @param[in] fd C3子板设备的文件标识符
 * @param[in] *send_data 储存发送消息的缓冲区字符串的头指针。
 * @return 函数执行结果
 * - 0 发送出现错误
 * - 1 发送成功
 */
uint8_t c3_wifi_tcp_send(int fd, char *send_data)
{
    uint8_t len;
    char send_buffer[200];
    char recv_buf[80] = {0};
    int flags, nbytes;
    struct timeval timeout;

    len = strlen(send_data);
    memset(send_buffer, 0, 200);

    // 构造AT命令
    sprintf(send_buffer, "AT+CIPSEND=%d\r\n", len);
    write(fd, send_buffer, strlen(send_buffer));
    usleep(100000);

    // 发送数据
    memset(send_buffer, 0, 200);
    strncpy(send_buffer, (const char *)send_data, len);
    write(fd, send_buffer, len);

    // 设置文件描述符为非阻塞模式
    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    // 等待服务器响应，设置500ms超时
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    nbytes = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (nbytes > 0)
    {
        // 有数据可读，读取响应
        int bytes_read = read(fd, recv_buf, sizeof(recv_buf));
        if (bytes_read > 0)
        {
            recv_buf[bytes_read] = '\0'; // 确保字符串终止
        }
    }
    else
    {
        // 超时或错误
        fprintf(stderr, "Timeout waiting for server response\n");
        // 恢复阻塞模式
        fcntl(fd, F_SETFL, flags);
        return 0;
    }

    // 恢复阻塞模式
    fcntl(fd, F_SETFL, flags);

    usleep(200000);
    if (strstr(recv_buf, "SEND OK") != NULL)
        return 1;
    else
        return 0;
}