#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <regex.h>  // 新增正则表达式库
#include "i2c_s1_key.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "s6.h"
#include "c3.h"
#include <stdbool.h>
#include <ctype.h>  // 添加这行

FUNC_PARAM param_e1_led;
FUNC_PARAM param_e1_lcd;
FUNC_PARAM param_s1;
FUNC_PARAM param_e2;
FUNC_PARAM param_e3;
FUNC_PARAM param_s6;


int mode = 0;
volatile int voice_thread_running = 1;
pthread_t voice_thread;
pthread_mutex_t command_mutex;
pthread_cond_t command_ready;  // 新增条件变量
char latest_command[100] = {0};
int command_available = 0;     // 新增命令状态标志

uint8_t c3_result;        // C3模块操作结果
int tcp_init_state = -1;  // TCP连接状态(-1:未连接, 0:已连接)
int run_wifi_state = 1;   // WiFi状态机当前状态(1-8)

char SSID[128];           // 存储WiFi SSID
char PWD[128];            // 存储WiFi密码
char ip[15];              // 存储服务器IP地址
char port[5];             // 存储服务器端口号

uint16_t rec_len;         // 接收到的数据长度
char rec_buffer[200];     // 接收数据缓冲区


char wifi_cmd[100] = {0};  // 新增：存储WiFi接收到的命令
int cmd_ready = 0;         // 新增：WiFi命令就绪标志


// 在接收函数中添加硬件状态检查
void check_uart_health(int fd) {
    // 1. 发送AT测试命令
    const char *test_cmd = "AT\r\n";
    write(fd, test_cmd, strlen(test_cmd));
    
    // 2. 接收检查
    uint8_t buf[32];
    int len = read(fd, buf, sizeof(buf));
    if(len <= 0 || buf[0] == 0) {
        printf("硬件通信故障！建议：\n");
        printf("1. 检查TX/RX接线\n");
        printf("2. 确认波特率设置\n");
        printf("3. 重启WiFi模块\n");
        exit(1);
    }
}

void ultral_control(void)
{
    unsigned short dis = 0;
    unsigned char data = 0;
    char context[4] = {'0', '1', '2', '3'};
    dis = distance(param_s6);
    //printf("dis is %d\n", dis);
    unsigned char speed;
    unsigned short tmp = dis;
    for (int i = 0; i < 4; i++)
    {
        data = tmp % 10;
        context[3 - i] = data + 48;
        tmp = tmp / 10;
    }
    xianshi(param_e1_lcd.i2c_fd, param_e1_lcd.i2c_addr, context);
    if (mode)
    {
        if (dis < 1000)
        {
            speed = 1;
        }
        else if (dis < 1500)
        {
            speed = 2;
        }
        else if (dis < 2000)
        {
            speed = 3;
        }
        else
        {
            speed = 0;
        }
        printf("speed is %d\n", speed);
        pc9685_motor_control(param_e2.i2c_fd, param_e2.i2c_addr, speed);
        usleep(500000);
    }
}
// 定义播放反馈音的函数
void play_feedback_sound(const char* wav_file) {
    char command[256];
    snprintf(command, sizeof(command), "aplay %s", wav_file);
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "播放失败: %s\n", wav_file);
    } else {
        printf("已播放反馈音: %s\n", wav_file);
    }
}
// 处理语音命令
void handle_voice_command(const char *command) {
    if (strstr(command, "打开窗帘") != NULL) {
        printf("语音指令: 打开窗帘\n");
        curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 100);
        play_feedback_sound("1749544948724.wav");  // 播放反馈音
    }
    else if (strstr(command, "关闭窗帘") != NULL) {
        printf("语音指令: 关闭窗帘\n");
        curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 0);
        play_feedback_sound("1749544948724.wav");  // 播放反馈音
    }
    else if (strstr(command, "切换颜色") != NULL) {
        static unsigned char color = 0;
        printf("语音指令: 切换颜色\n");
        color++;
        if (color > 3)
            color = 0;
        pc9685_rgb_led_control(param_e1_led.i2c_fd, param_e1_led.i2c_addr, color);
        play_feedback_sound("1749544948724.wav");  // 播放反馈音
    }
    else if (strstr(command, "切换模式") != NULL) {
        printf("语音指令: 切换模式\n");
        if (mode == 0)
            mode = 1;
        else
            mode = 0;
        play_feedback_sound("1749544948724.wav");  // 播放反馈音
    }
    else if (strstr(command, "切换风扇档位") != NULL) {
        static unsigned char fan = 0;
        printf("语音指令: 切换风扇档位\n");
        fan++;
        if (fan > 3)
            fan = 0;
        pc9685_motor_control(param_e2.i2c_fd, param_e2.i2c_addr, fan);
        play_feedback_sound("1749544948724.wav");  // 播放反馈音
    }
    // 可以根据需要添加更多指令
}


void handle_wifi_command(const char *cmd) {
    char processed_cmd[300] = {0};
    
    // 1. 解析命令
    strcpy(processed_cmd, cmd);
    printf("%s\n",processed_cmd);
    // 2. 只有解析出有效内容才处理
    if (strlen(processed_cmd) == 0) {
        printf("无效命令\n");
        return;
    }
    // 3. 命令处理
    if (strstr(processed_cmd, "LED_ON") != NULL) {
        pc9685_rgb_led_control(param_e1_led.i2c_fd, param_e1_led.i2c_addr, 1);
    } 
    else if (strstr(processed_cmd, "LED_OFF") != NULL) {
        pc9685_rgb_led_control(param_e1_led.i2c_fd, param_e1_led.i2c_addr, 0);
    }
    else if (strstr(processed_cmd, "CURTAIN_OPEN") != NULL) {
        curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 100);
        printf("CURTAIN_OPEN");
    }
    else if (strstr(processed_cmd, "CURTAIN_CLOSE") != NULL) {
        curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 0);
        printf("CURTAIN_OPEN");
    }
    else if (strncmp(processed_cmd, "FAN_SPEED_", 10) == 0) {
        int speed = atoi(processed_cmd + 10);
        if (speed >= 1 && speed <= 3) {
            pc9685_motor_control(param_e2.i2c_fd, param_e2.i2c_addr, speed);
            printf("风扇速度设置为%d档\n", speed);
        } else {
            printf("无效的风扇速度\n");
        }
    }
    else if (strstr(processed_cmd, "FAN_OFF") != NULL) {
        pc9685_motor_control(param_e2.i2c_fd, param_e2.i2c_addr, 0);
        printf("风扇已关闭\n");
    }
    else if (strstr(processed_cmd, "MODE_SWITCH") != NULL) {
        mode = !mode;
        printf("模式切换: %s\n", mode ? "自动" : "手动");
    }
    else {
        printf("未知WiFi命令: %s\n", processed_cmd);
        
        // 调试输出原始命令的十六进制
        printf("原始命令十六进制: ");
        for (const char *p = cmd; *p; p++) {
            printf("%02X ", (unsigned char)*p);
        }
        printf("\n");
    }
}
// 语音识别线程函数 - 实时获取输出，不使用正则表达式
void* voice_recognition_thread(void* arg)
{
    FILE* pipe = NULL;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    const char* command = "stdbuf -oL ./sherpa-ncnn-alsa-1 ./smallmodel/tokens.txt ./smallmodel/encoder_jit_trace-pnnx.ncnn.param ./smallmodel/encoder_jit_trace-pnnx.ncnn.bin ./smallmodel/decoder_jit_trace-pnnx.ncnn.param ./smallmodel/decoder_jit_trace-pnnx.ncnn.bin ./smallmodel/joiner_jit_trace-pnnx.ncnn.param ./smallmodel/joiner_jit_trace-pnnx.ncnn.bin \"hw:0,1\" 4 greedy_search 2>&1";

    // 只执行一次命令，保持在线识别
    pipe = popen(command, "r");
    if (!pipe)
    {
        perror("popen failed");
        return NULL;
    }
    
    printf("语音识别已启动，等待命令...\n");
    
    // 使用getline动态读取每一行，避免缓冲区溢出
    while (voice_thread_running)
    {
		read = getline(&line, &len, pipe);
		if (read == -1) {
		    if (feof(pipe)) {
			    printf("管道已关闭\n");
		    } else {
			    perror("getline 失败");
		    }
		    break;
        }
        printf("识别输出: %s", line);
        
        // 查找冒号位置
        char* colon_pos = strchr(line, ':');
        if (!colon_pos)
            continue; // 没有找到冒号，不是有效命令行
        
        // 跳过数字部分，定位到命令起始位置
        if (colon_pos == line)
            continue; // 冒号在开头，格式错误
        
        // 提取命令部分
        char* command_start = colon_pos + 1;
        
        // 去除行末的换行符
        size_t command_len = strlen(command_start);
        if (command_len > 0 && command_start[command_len - 1] == '\n')
            command_start[command_len - 1] = '\0';
        
        // 保存最新命令并通知主线程
        pthread_mutex_lock(&command_mutex);
        
        // 确保不超过目标缓冲区大小
        size_t copy_len = (command_len < sizeof(latest_command) - 1) ? command_len : sizeof(latest_command) - 1;
        strncpy(latest_command, command_start, copy_len);
        latest_command[copy_len] = '\0';
        
        command_available = 1;
        pthread_cond_signal(&command_ready);  // 发送信号通知主线程
        pthread_mutex_unlock(&command_mutex);
        
        printf("识别到命令: %s\n", command_start);
    }
    
    // 线程停止时清理资源
    if (line)
        free(line);  // 释放getline分配的内存
    pclose(pipe);
    printf("语音识别线程已停止\n");
    
    return NULL;
}

void s1_key_func(int fd)
{
    int ret;
    unsigned char key;
    

    I2c_s1_key_init(param_s1.i2c_fd, param_s1.i2c_addr);
    e1_init(param_e1_led);
    
    // 启动语音识别线程
    pthread_create(&voice_thread, NULL, voice_recognition_thread, NULL);
    
    while (1)
    {   
        ultral_control();
        
        // 网络连接管理
        if(tcp_init_state != 0) {
            c3_wifi_tcp_lead(fd, &run_wifi_state, &tcp_init_state);
            sleep(1);
            continue;
        }
        ;


          // 接收WiFi命令
        if(tcp_init_state == 0) {
            uint16_t rec_len;	
            char rec_buffer[300];
            rec_len = read(fd,rec_buffer,300);
            printf("hello\n");
            printf("%s\n",rec_buffer);
            printf("WORLD\n");
            handle_wifi_command(rec_buffer);
        }
        // 处理按键事件
        ret = I2c_s1_key_get(param_s1.i2c_fd, param_s1.i2c_addr, &key);
        if (!ret)
        {
            printf("key %d pressed\n", key);
            switch (key)
            {
                case 1:
                    printf("key 1 start\n");
                    static unsigned char color = 0;
                    color++;
                    if (color > 3)
                        color = 0;
                    pc9685_rgb_led_control(param_e1_led.i2c_fd, param_e1_led.i2c_addr, color);
                    usleep(500000);
                    break;
                case 2:
                    printf("key 2 start\n");
                    curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 100);
                    usleep(500000);
                    break;
                case 3:
                    printf("key 3 start\n");
                    curtain_control(param_e3.i2c_fd, param_e3.i2c_addr, 0);
                    usleep(500000);
                    break;
                case 4:
                    printf("key 4 start\n");
                    if (mode == 0)
                        mode = 1;
                    else
                        mode = 0;
                    usleep(500000);
                    break;
                case 5:
                    printf("key 5 start\n");
                    static unsigned char fan = 0;
                    fan++;
                    if (fan > 3)
                        fan = 0;
                    pc9685_motor_control(param_e2.i2c_fd, param_e2.i2c_addr, fan);
                    break;
                case 6:
                    printf("key 6 start\n");
                    break;
                default:
                    break;
            }
        }

        // 检查是否有新的语音命令（非阻塞方式）
        pthread_mutex_lock(&command_mutex);
        if (command_available) {
            handle_voice_command(latest_command);
            command_available = 0;
        }
        pthread_mutex_unlock(&command_mutex);
        
        usleep(100000);
    }
}

// 信号处理函数
void signal_handler(int signum)
{
    printf("收到信号 %d，准备退出...\n", signum);
    voice_thread_running = 0;
    
    // 唤醒可能正在等待的线程
    pthread_mutex_lock(&command_mutex);
    pthread_cond_signal(&command_ready);
    pthread_mutex_unlock(&command_mutex);
    
    pthread_join(voice_thread, NULL);
    exit(0);
}



/*
 * main
 */
int main(int argc, char **argv)
{
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化互斥锁和条件变量
    pthread_mutex_init(&command_mutex, NULL);
    pthread_cond_init(&command_ready, NULL);


     int ch;  // 存储当前处理的参数选项
    bool s = false, w = false, i = false, p = false;  // 参数存在标志
    
    // 参数检查：必须包含全部4个参数
    if(argc < 9) {
        printf("Usage:\n.\\main -s [ssid] -w [password] -i [controller's ip] -p [port].\n");
        return 1;
    }

          while ((ch = getopt(argc, argv, "s:w:i:p:")) != -1) {
        switch (ch) {
        case 's':  // WiFi SSID
            s = true;
            printf("ssid:%s\n", optarg);
            strcpy(SSID, optarg);
            break;
        case 'w':  // WiFi密码
            w = true;
            printf("password:%s\n", optarg); 
            strcpy(PWD, optarg);
            break;
        case 'i':  // 服务器IP
            i = true;
            printf("controller's ip:%s\n", optarg);
            strcpy(ip, optarg);
            break; 
        case 'p':  // 服务器端口
            p = true;
            printf("port:%s\n", optarg);
            strcpy(port, optarg);
            break;    
        default:
            break;
        }
    }


        // 检查是否所有必需参数都已提供
    if(!(s && w && i && p)) {
        printf("Usage:\n.\\main -s [ssid] -w [password] -i [controller's ip] -p [port].\n");
        return 1;
    }


    char devname[16];
    /*e1 text*/
    
    memset(&param_e1_led, 0, sizeof(param_e1_led));
    int  fid_e1_led = jiance_e1_led(&param_e1_led);
    if (fid_e1_led < 2)
    {
        printf("not find e1\n");
    }
    printf("e1 fid:%d\n", fid_e1_led);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_e1_led);
    
    if (fid_e1_led >= 2)
    {
        printf("e1 is open\n");
        I2c_open(devname);
    }
    //S1状态检测
    
    memset(&param_s1, 0, sizeof(param_s1));
    int fid_s1 = jiance_s1(&param_s1);
    if (fid_s1 < 2)
    {
        printf("not find s1\n");
    }
    printf("s1 fid:%d\n", fid_s1);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_s1);
    if (fid_s1 >= 2)
    {
        printf("s1 is open\n");
        I2c_open(devname);
    }
    //e2状态检测
    
    memset(&param_e2, 0, sizeof(param_e2));
    int fid_e2 = jiance_e2(&param_e2);
    if (fid_e2 < 2)
    {
        printf("not find e2\n");
    }
    printf("e2 fid:%d\n", fid_e2);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_e2);
    if (fid_e2 >= 2)
    {
        printf("e2 is open\n");
        I2c_open(devname);
    }
    //e1_lcd状态检测
    
    memset(&param_e1_lcd, 0, sizeof(param_e1_lcd));
    int fid_e1_lcd = jiance_e1_lcd(&param_e1_lcd);
    if (fid_e1_lcd < 2)
    {
        printf("not find e1_lcd\n");
    }
    printf("e1_lcd fid:%d\n", fid_e1_lcd);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_e1_lcd);
    if (fid_e1_lcd >= 2)
    {
        printf("e1_lcd is open\n");
        I2c_open(devname);
    }
    //e3状态检测
    
    memset(&param_e3, 0, sizeof(param_e3));
    int fid_e3 = jiance_e3(&param_e3);
    if (fid_e3 < 2)
    {
        printf("not find e3\n");
    }
    printf("e3 fid:%d\n", fid_e3);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_e3);
    if (fid_e3 >= 2)
    {
        printf("e3 is open\n");
        I2c_open(devname);
    }
    //s6 connect
    
    memset(&param_s6, 0, sizeof(param_s6));
    int fid_s6 = jiance_s6(&param_s6);
    if (fid_s6 < 2)
    {
        printf("not find s6\n");
    }
    printf("s6 fid:%d\n", fid_s6);
    sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, fid_s6);
    if (fid_s6 >= 2)
    {
        printf("s6 is open\n");
        I2c_open(devname);
    }

    
        // 4. 初始化WiFi模块（仅在此处初始化一次）
    int fd;
    if(c3_init(&fd) == 0) {
        printf("WiFi模块初始化失败\n");
        return 1;
    }
    printf("WiFi模块初始化成功\n");

    // 5. 等待网络连接就绪
    printf("正在建立网络连接...\n");
    while(tcp_init_state != 0) {
        c3_wifi_tcp_lead(fd, &run_wifi_state, &tcp_init_state);
        sleep(1);
    }
    printf("网络连接就绪!\n");
    
    // 主循环：处理网络连接和数据通信

    s1_key_func(fd);

    check_uart_health(fd);
    // 清理资源
    pthread_mutex_destroy(&command_mutex);
    pthread_cond_destroy(&command_ready);
    
    return 0;
}