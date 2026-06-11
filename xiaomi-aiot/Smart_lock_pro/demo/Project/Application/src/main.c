#include "gd32f4xx.h"
#include "gd32f470z_eval.h"
#include "main.h"
#include "i2c.h"
#include "stdio.h"
#include "string.h"
#include "s1.h"
#include "s2.h"
#include "s5.h"
#include "s7.h"
#include "e1.h"
#include "e2.h"
#include "e3.h"
#include "c2.h"
#include "uart.h"
#include "c3.h"

// 定义 Zigbee 模式
#define TERMINAL    0
#define COORDINATOR 1

// 设备地址结构体定义
i2c_addr_def s1_key_addr;
i2c_addr_def s2_th_addr;
i2c_addr_def s2_ss_addr;
i2c_addr_def s5_addr;
i2c_addr_def s7_addr;
i2c_addr_def e1_nixie_tube_addr;
i2c_addr_def e1_rgb_led_addr;
i2c_addr_def e2_fan_addr;
i2c_addr_def e3_curtain_addr;

// Zigbee 参数
uint8_t  zigbee_mode          = COORDINATOR;
volatile uint8_t c2_result;
uint8_t  rec_data[100];
uint8_t  len;
uint8_t  zigbee_active_flag   = 0;
uint16_t zigbee_active_time   = 0;
uint8_t  voice_authorized_flag = 0;
uint16_t voice_authorized_time = 0;

// WiFi 参数
int      tcp_init_state      = -1;
int      run_wifi_step       = 1;
volatile uint8_t c3_result;
uint16_t rec_len             = 0;
char     rec_buffer[100];
char     output[100];
uint8_t  wifi_flag           = 0;
uint8_t  wifi_active_flag    = 0;
uint16_t wifi_active_time    = 0;

// S1 参数（按键相关）
uint8_t  s1_key_value;
uint8_t  s1_pressed_flag     = 0;
uint8_t  botton_flag         = 0;
uint8_t  botton_input[4]     = {0, 0, 0, 0};
uint8_t  password[4]         = {6, 6, 6, 6};
uint8_t  e1_dis[4]           = {NODIS, NODIS, NODIS, NODIS};
uint8_t  dis_pos             = 0;
uint8_t  reset_dis[4]        = {NODIS, NODIS, NODIS, NODIS};
uint8_t  reset_dis_pos       = 0;
uint8_t  password_retry_count = 0;
uint8_t  temporary_password[4] = {0};
uint8_t  temporary_password_flag = 0;
uint32_t temporary_password_time = 0;

// S2 参数（温湿度相关）
s2_para s2_value;
float   ss_value;

// S5 参数（NFC 相关）
uint8_t  s5_nfc_detected    = 0;
uint8_t  nfc_enable_flag    = 0;
uint8_t  DefaultKey[6]      = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t  ucArray_ID[6];
uint8_t  wd_buffer[20]      = {0};
uint8_t  read_flag          = 1;
uint8_t  write_flag         = 0;
uint8_t  user_id[255]       = {0};
uint16_t user_id_len        = 100;
uint8_t  nfc_authorized_flag = 0;
uint16_t nfc_authorized_time = 0;

// S7 参数（人体检测相关）
uint8_t  s7_human_detected   = 0;
uint8_t  human_detect_flag   = 0;
uint16_t human_detect_count  = 0;
uint8_t  human_long_stay_flag = 0;

// 系统参数
uint16_t time_count         = 0;
uint8_t  system_state       = 0;
uint8_t  system_locked_flag = 0;
uint16_t auto_lock_time     = 100;
uint8_t  active_flag        = 0;
uint16_t unlock_failure_count = 0;
uint8_t  locked_in_flag     = 0;

// 函数声明
void init_addr(void);
void detect_human(void);
void detect_botton_input(void);
void detect_nfc(void);
void detect_wifi(void);
void detect_zigbee(void);
void detect_all(void);
void handle_system_state(void);
void handle_detected(void);
void handle_botton_input(void);
void handle_nfc(void);
void handle_wifi(void);
void parse_wifi_data(const char *raw_data, char *parsed_data);
void handle_zigbee(void);
void handle_reset_password(void);
uint8_t handler_cmd(char *args);
uint8_t cmd_lock(char *args);
uint8_t cmd_unlock(char *args);
uint8_t cmd_reset(char *args);
uint8_t cmd_check(char *args);
uint8_t cmd_test(char *args);
uint8_t cmd_nfc(char *args);
uint8_t cmd_tempkey(char *args);
uint8_t cmd_help(char *args);
void state0_display(void);
void state1_display(void);
void unlock_success_display(void);
void unlock_failure(void);
void reset_password_display_led(void);
void reset_password_display_tube(void);
void state5_display(void);

// 命令表
static struct {
    char *name;
    char *description;
    uint8_t (*handler)(char *);
} cmd_table[] = {
    {"lock",   "Lock the door",                cmd_lock},
    {"unlock", "Unlock the door",              cmd_unlock},
    {"reset",  "Reset password",               cmd_reset},
    {"check",  "Check whether the door is open or closed", cmd_check},
    {"test",   "Test the System",              cmd_test},
    {"nfc",    "NFC authentication",           cmd_nfc},
    {"tempkey", "Temporary password",          cmd_tempkey},
    {"help",   "Help",                         cmd_help},
};

int main(void) {
    // 配置中断优先级
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(USART0_IRQn, 2, 0); // WiFi
    nvic_irq_enable(USART2_IRQn, 1, 0); // ZigBee

    // 初始化串口
    uart_init(USART0);
    uart_init(USART2);

    // 初始化定时器
    timer3_init();

    // 初始化评估板串口
    gd_eval_com_init(EVAL_COM0);

    // 初始化 I2C
    init_i2c();

    // 初始化设备地址
    init_addr();

    // 初始化用户 ID
    for (int i = 0; i <= user_id_len; i++) {
        user_id[i] = i;
    }

    while (1) {
        detect_wifi();
        detect_zigbee();
        handle_wifi();
        handle_zigbee();
        handle_system_state();
    }
}

// 初始化设备地址
void init_addr(void) {
    c3_result = c3_init();
    c2_result = c2_init(zigbee_mode);
    s1_key_addr = s1_init(HT16K33_ADDRESS_S1);
    s2_th_addr = s2_init(TH_ADDRESS_S2);
    s2_ss_addr = s2_init(S1_ADDRESS_S2);
    s5_addr = s5_init(MS523_ADDRESS_S5);
    s7_addr = s7_init(PCA9557_ADDRESS_S7);
    e1_rgb_led_addr = e1_init(PCA9685_ADDRESS_E1);
    e1_nixie_tube_addr = e1_init(HT16K33_ADDRESS_E1);
    e2_fan_addr = e2_init(PCA9685_ADDRESS_E2);
    e3_curtain_addr = e3_init(CURTAIN_ADDRESS_E3);
}

// 检测函数
void detect_human(void) {
    if (s7_addr.flag && human_detect_flag) {
        human_detect_flag = 0;
        if (s7_get_status(s7_addr.periph, s7_addr.addr)) {
            s7_human_detected = 1;
            active_flag = 1;
        }
    }
}

void detect_botton_input(void) {
    if (s1_key_addr.flag && botton_flag) {
        botton_flag = 0;
        s1_key_value = s1_key_scan(s1_key_addr.periph, s1_key_addr.addr);
        if (s1_key_value != SWN) {
            s1_pressed_flag = 1;
            active_flag = 1;
        }
    }
}

void detect_nfc(void) {
    if (s5_addr.flag && nfc_enable_flag) {
        nfc_enable_flag = 0;
        if (s5_detect(s5_addr.periph, s5_addr.addr, ucArray_ID)) {
            if (s5_verify(s5_addr.periph, s5_addr.addr, PICC_AUTHENT1A, 5, DefaultKey, ucArray_ID) == 0) {
                s5_nfc_detected = 1;
                memset(wd_buffer, 0, 16);
                if (s5_read_data(s5_addr.periph, s5_addr.addr, 6, wd_buffer) == 0) {
                    read_flag = 1;
                    active_flag = 1;
                }
            }
        }
    }
}

void detect_wifi(void) {
    if(c3_result == 0){
        c3_result = c3_init();
    }
    if (wifi_flag && tcp_init_state) {
        wifi_flag = 0;
        c3_wifi_tcp_lead(&run_wifi_step, &tcp_init_state);
    }
    rec_len = c3_wifi_tcp_receive(rec_buffer, 100);
}

void detect_zigbee(void) {
    if(c2_result == 0){
        c2_result = c2_init(zigbee_mode);
    }
    memset(rec_data, 0, 100);
    len = c2_rec_data(rec_data, 50, 100);
}

void detect_all(void) {
    detect_human();
    detect_botton_input();
    detect_nfc();
}

// 显示函数
void state0_display(void) {
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, NODIS, NODIS, NODIS, NODIS);
    e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, 0);
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 0);
}

void state1_display(void) {
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 0, 60);
}

void unlock_success_display(void) {
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, NODIS, 0, 18, NODIS);
    e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, 100);
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
}

void unlock_failure(void) {
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, 0xE, 17, 17, NODIS);
    e3_set_position(e3_curtain_addr.periph, e3_curtain_addr.addr, 0);
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 0, 0);
}

void reset_password_display_led(void) {
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 0);
}

void reset_password_display_tube(void) {
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 0);
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, password[0], password[1], password[2], password[3]);
}

void state5_display(void) {
    e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 0);
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, NODIS, 0, 18, NODIS);
}

// 处理函数
void handle_detected(void) {
    system_state = 1;
    active_flag = 1;
}

void handle_botton_input(void) {
    e1_dis[dis_pos++] = s1_key_value;
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, e1_dis[0], e1_dis[1], e1_dis[2], e1_dis[3]);
    if (dis_pos == 4) {
        int right_flag = 1;
        dis_pos = 0;
        for (int i = 0; i < 4; i++) {
            right_flag = right_flag && (e1_dis[i] == password[i]);
        }

        if (temporary_password_flag) {
            right_flag = 1;
            for (int i = 0; i < 4; i++) {
                right_flag = right_flag && (e1_dis[i] == temporary_password[i]);
            }
        }

        if (right_flag) {
            system_state = 2;
            password_retry_count = 0;
        } else {
            c2_broadcast_data("pwfalse", 0x01);
            system_state = 3;
            if (++password_retry_count >= 6) {
                c2_broadcast_data("pwmultifalse", 0x01);
                c3_wifi_tcp_send("Too many wrong attempts! Please wait for 30 seconds and try again");
                system_locked_flag = 1;
            }
        }

        for (int j = 0; j < 4; j++) {
            e1_dis[j] = 0x10;
        }
    }
}

void handle_reset_password(void) {
    reset_dis[reset_dis_pos++] = s1_key_value;
    e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, reset_dis[0], reset_dis[1], reset_dis[2], reset_dis[3]);
    if (reset_dis_pos == 4) {
        for (int i = 0; i < 4; i++) {
            password[i] = reset_dis[i];
            reset_dis[i] = 0x10;
        }
        reset_dis_pos = 0;
        e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, password[0], password[1], password[2], password[3]);
        e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 0, 60, 0);
        system_state = 0;
    }
}

void handle_nfc(void) {
    if (read_flag) {
        if (voice_authorized_flag && nfc_authorized_flag) {
            system_state = 2;
            return;
        }
        for (int i = 0; i < user_id_len; i++) {
            if (wd_buffer[0] == user_id[i]) {
                c2_broadcast_data("nfc", 0x01);
                system_state = 5;
                nfc_authorized_flag = 1;
                return;
            }
        }
        c2_broadcast_data("nfcfalse", 0x01);
        system_state = 3;
    }
}

void handle_wifi(void) {
    if (rec_len > 0) {
        wifi_active_flag = 1;
        auto_lock_time = 100;
        parse_wifi_data(rec_buffer, output);
        uint8_t result = handler_cmd((char *)output);
        if (result == 0) {
            c3_wifi_tcp_send("Operation success!");
        } else if (result == 1) {
            c3_wifi_tcp_send("Invalid format! Enter 'help' for help.");
        } else if (result == 2) {
            c3_wifi_tcp_send("Invalid parameter!");
        } else if (result == 3) {
            c3_wifi_tcp_send("New Password should be 4 digits!");
        } else if (result == 4) {
            c3_wifi_tcp_send("Please enter password!");
        } else if (result == 5) {
            c3_wifi_tcp_send("Wrong password!");
        } else if (result == 6) {
            c3_wifi_tcp_send("Door Locked! Someone is in front of the door!");
        } else if (result == 7) {
            c3_wifi_tcp_send("Unlocked!");
        } else if (result == 8) {
            c3_wifi_tcp_send("User already exists!");
        } else if (result == 9) {
            c3_wifi_tcp_send("User not found!");
        } else if (result == 10) {
            c3_wifi_tcp_send(
                "Available commands:\n"
                "lock <pwd> - Lock door\n"
                "unlock - Unlock door\n"
                "reset <org_pwd> <new_pwd> - Reset password\n");
            c3_wifi_tcp_send(
                "check - Check door status\n"
                "nfc <add|remove> <id> - NFC authentication\n"
                "tempkey <pwd> - Set temp-password\n"
                "help - Show help");
        } else if (result == 11) {
            c3_wifi_tcp_send("Please set time limit!");
        } else if (result == 12) {
            c3_wifi_tcp_send("Door Locked! No one is near the door!");
        } else if (result == 13) {
            c3_wifi_tcp_send("Inside-locked mode has been activated!");
        } else if (result == 14) {
            c3_wifi_tcp_send("Inside-locked mode has been deactivated!");
        }
    } else {
        wifi_active_flag = 0;
    }
}

void parse_wifi_data(const char *raw_data, char *parsed_data) {
    const char *data_start = strstr(raw_data, "+IPD,");
    if (data_start) {
        data_start += strlen("+IPD,");
        const char *colon_pos = strchr(data_start, ':');
        if (colon_pos) {
            strcpy(parsed_data, colon_pos + 1);
            char *end = strstr(parsed_data, "\r\n");
            if (end) {
                *end = '\0';
            }
        }
    }
}

void handle_zigbee(void) {
    if (len) {
        zigbee_active_flag = 1;
        auto_lock_time = 100;
        uint8_t result = rec_data[0] - '0';
        if (result == 0) { // unlocked
            if (nfc_authorized_flag && voice_authorized_flag && system_state == 5) {
                system_state = 2;
                return;
            }
            system_state = 5;
            voice_authorized_flag = 1;
            c2_broadcast_data("voice", 0x01);
            c3_wifi_tcp_send("Voice authentication success!");
        } else if (result == 3) { // reset password to default
            for (int i = 0; i < 4; i++) {
                password[i] = 6;
            }
            c3_wifi_tcp_send("Password Reset success!");
            c2_broadcast_data("reset", 0x01);
        } else if (result == 4) { // change password
            system_state = 4;
        }
    } else {
        zigbee_active_flag = 0;
    }
}

// 命令处理函数
uint8_t handler_cmd(char *args) {
    char *str_end = args + strlen(args);
    char *cmd = strtok(args, " ");

    if (cmd != NULL) {
        args = cmd + strlen(cmd) + 1;
        if (args > str_end) {
            args = NULL;
        }
        for (int i = 0; i < sizeof(cmd_table) / sizeof(cmd_table[0]); i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                return cmd_table[i].handler(args);
            }
        }
    }
    return 1;
}

uint8_t cmd_lock(char *args) {
    if (args == NULL) {
        system_state = 0;
        return 0; // success
    } 

    int right_flag = 1;
    if (strlen(args) == 4) {
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            right_flag = right_flag && (temp == password[i]);
        }
        if (!right_flag) {
            return 5; // wrong password
        }
    } else {
        return 3; // password should be 4 digits
    }

    args = strtok(NULL, " ");
    else if (strcmp(args, "in") == 0) {
        locked_in_flag = 1;
        return 13; // success
    } else if (strcmp(args, "out") == 0) {
        locked_in_flag = 0;
        return 14; // success
    } else {
        return 1; // error
    }
}

uint8_t cmd_unlock(char *args) {
    if (strlen(args) == 4) {
        int right_flag = 1;
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            if (temporary_password_flag) {
                right_flag = right_flag && ((temp == temporary_password[i] || temp == password[i]));
            } else {
                right_flag = right_flag && (temp == password[i]);
            }
        }
        if (right_flag) {
            system_state = 2;
            return 0;
        } else {
            system_state = 3;
            return 5; // wrong password
        }
    } else {
        return 3; // password should be 4 digits
    }
}

uint8_t cmd_reset(char *args) {
    args = strtok(NULL, " ");
    if (args == NULL) {
        return 4; // no password input
    }

    int right_flag = 1;
    if (strlen(args) == 4) {
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            right_flag = right_flag && (temp == password[i]);
        }
        if (!right_flag) {
            return 5; // wrong password
        }
    } else {
        return 3; // password should be 4 digits
    }
    args = strtok(NULL, " ");

    if (strlen(args) == 4) {
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            password[i] = temp;
        }
        e1_digital_display(e1_nixie_tube_addr.periph, e1_nixie_tube_addr.addr, password[0], password[1], password[2], password[3]);
        e1_rgb_control(e1_rgb_led_addr.periph, e1_rgb_led_addr.addr, 60, 60, 0);
        delay_ms(1500);
        system_state = 0;
        return 0; // reset successfully
    } else {
        return 3; // password should be 4 digits
    }
}

uint8_t cmd_check(char *args) {
    args = strtok(NULL, " ");
    if (args == NULL) {
        if (system_state != 2 && human_long_stay_flag == 1) {
            return 6; // door is locked and someone there
        } else if (system_state != 2 && human_long_stay_flag == 0) {
            return 12; // door is locked and no one there
        } else if (system_state == 2) {
            return 7; // door is unlocked
        }
    } else {
        return 2; // invalid parameter
    }
}

uint8_t cmd_test(char *args) {
    LED_OFF;
    delay_ms(500);
    LED_ON;
    delay_ms(500);
    LED_OFF;
    delay_ms(500);
    LED_ON;
    delay_ms(500);
    return 0;
}

uint8_t cmd_nfc(char *args) {
    args = strtok(NULL, " ");
    if (args == NULL) {
        return 1;
    } else {
        if (strcmp(args, "add") == 0) {
            args = strtok(NULL, " ");
            if (args == NULL) {
                return 1;
            }
            uint8_t value = atoi(args);
            for (int i = 0; i < user_id_len; i++) {
                if (value == user_id[i]) {
                    return 8; // user already exists
                }
            }
            user_id[user_id_len++] = value;
            return 0; // add user successfully
        } else if (strcmp(args, "remove") == 0) {
            args = strtok(NULL, " ");
            if (args == NULL) {
                return 1;
            }
            uint8_t value = atoi(args);
            for (int i = 0; i < user_id_len; i++) {
                if (value == user_id[i]) {
                    user_id[i] = 0;
                    return 0; // remove user successfully
                }
            }
            return 9; // user not found
        }
    }
    return 1;
}

uint8_t cmd_tempkey(char *args) {
    args = strtok(NULL, " ");
    if (args == NULL) {
        return 4; // no password input
    }

    int right_flag = 1;
    if (strlen(args) == 4) {
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            right_flag = right_flag && (temp == password[i]);
        }
        if (!right_flag) {
            return 5; // wrong password
        }
    } else {
        return 3; // password should be 4 digits
    }
    args = strtok(NULL, " ");

    if (strlen(args) == 4) {
        for (int i = 0; i < 4; i++) {
            int temp = args[i] - '0';
            if (temp < 0 || temp > 9) {
                return 2; // invalid parameter
            }
            temporary_password[i] = temp;
        }
    } else {
        return 3; // password should be 4 digits
    }

    args = strtok(NULL, " ");
    if (args == NULL) {
        return 11; // no time limit input
    }
    uint32_t time_limit = atoi(args);
    if (time_limit >= 1 && time_limit <= 60) {
        time_limit = time_limit * 1000 * 60;
        temporary_password_flag = 1;
        return 0; // set successfully
    } else {
        return 3; // invalid time limit
    }
}

uint8_t cmd_help(char *args) {
    args = strtok(NULL, " ");
    if (args == NULL) {
        return 10; // show help
    }
    return 2; // invalid parameter
}

uint8_t send_flag = 0;

void handle_system_state(void) {
    detect_all();
    if(locked_in_flag) return; //若反锁，则不处理
    if(human_long_stay_flag){
         c3_wifi_tcp_send("Someone is in front of the door!");
         human_long_stay_flag = 0;
    }
    uint8_t detected = s7_human_detected || s1_pressed_flag || s5_nfc_detected || zigbee_active_flag || wifi_active_flag;
    if (s7_human_detected) s7_human_detected = 0;
    if (auto_lock_time >= 15000) {
        system_state = 0;
        auto_lock_time = 0;
    }
    switch (system_state) {
        case 0: // locked, interactive
            send_flag = 0;
            state0_display();
            if (detected) system_state = 1;
            else active_flag = 0;
            break;

        case 1: // waiting for input, interactive
            if (!detected) {
                active_flag = 0;
            } else {
                state1_display();
                active_flag = 1;
                if (s1_pressed_flag) {
                    s1_pressed_flag = 0;
                    if (s1_key_value >= 0x00 && s1_key_value <= 0x09) {
                        state1_display();
                        handle_botton_input();
                    }
                }
                if (s5_nfc_detected) {
                    s5_nfc_detected = 0;
                    handle_nfc();
                }
            }
            break;

        case 2: // unlocked, uninteractive
            if (!detected) {
                active_flag = 0;
            }
            if (send_flag == 0) {
                c2_broadcast_data("backhome", 0x01);
                send_flag = 1;
            }
            unlock_success_display();
            if (s1_pressed_flag) {
                s1_pressed_flag = 0;
                if (s1_key_value == SWA || s1_key_value == SWC) {
                    system_state = 3;
                }
            }
            if (auto_lock_time >= 12000) {
                system_state = 0;
                auto_lock_time = 0;
            }
            break;

        case 3: // unlock failure
            if (!detected) {
                active_flag = 0;
            }
            unlock_failure();
            if (system_locked_flag) {
                delay_ms(30000);
                system_locked_flag = 0;
            }
            break;

        case 4: // reset password
            if (!detected) {
                active_flag = 0;
            }
            reset_password_display_led();
            handle_reset_password();
            break;

        case 5: // nfc + voice
            if (!detected) {
                active_flag = 0;
            }
            state5_display();
            if (voice_authorized_flag && nfc_authorized_flag) {
                system_state = 2;
            }
            if (s5_nfc_detected && system_state == 5) {
                s5_nfc_detected = 0;
                handle_nfc();
            }
            break;
    }
}
