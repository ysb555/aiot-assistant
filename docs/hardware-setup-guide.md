# 小米 AIoT 智能植物养护助手 - 硬件搭建与运行指南

---

## 1. 系统概述

### 1.1 项目定位

基于 **GD32F470** 主控 + 小米 AIoT 实训箱扩展子板，搭建一套智能植物养护系统。核心功能：

- **环境监测**：温度、湿度、光照、人体红外、超声波距离
- **智能评分**：以植物档案为基准，对温/湿/光三项环境指标打分（0-100）
- **自动遮阳**：根据光照强度自动开/关窗帘电机
- **自动通风**：根据湿度自动调节风扇转速
- **浇水提醒**：基于蒸发指数，超阈值后通过 WiFi / Zigbee 推送提醒
- **植物切换**：4 种方式 — 按键选择、NFC 刷卡、串口命令、视觉识别（u2p_vision）
- **Zigbee 无线**：C2 (E18-MS1-PCB) 模块广播传感器状态，支持远程接收
- **WiFi 通信**：C3 模块通过 HTTP GET 发送浇水提醒到服务器
- **前端控制**：UART0 串口交互 + E5 触摸屏 UART 控制

### 1.2 软件架构

```
main.c (主循环 - 轮询调度)
├── 50ms 周期: 按键扫描 (S1 矩阵键盘)
├── 10ms 周期: 前端命令检测 (uart_get_line)
└── 1s 周期:
    ├── WiFi 连接状态机 (c3_wifi_tcp_lead)
    ├── 传感器读取 (sensors_read)
    ├── 评分计算 (score_calculate)
    ├── 显示更新 (display_update / rgb_update)
    ├── 自动控制 (auto_shade/vent/light_control)
    ├── 蒸发指数更新 (evaporation_update)
    ├── 浇水提醒检查 (watering_check)
    ├── 视觉识别处理 (vision_uart_process)
    └── Zigbee 收发 (zigbee_recv_process / zigbee_send_status)
```

---

## 2. 硬件清单

### 2.1 核心硬件

| 编号 | 模块 | 芯片/型号 | I2C 地址 (8位) | 备注 |
|------|------|-----------|---------------|------|
| **U1P** | GD32F470 主控底板 | GD32F470ZKT6 | - | ARM Cortex-M4F, 200MHz |
| **S1** | 矩阵按键子板 | HT16K33 | **0xE4** (键扫) | 8 个 GPIO 按键 |
| **S2** | 光照传感器子板 | BH1750 | **0x46** | 1-65535 lux |
| **S5** | NFC 子板 | MS523 | **0x52** | 刷卡植物切换 |
| **S6** | 超声波子板 | - | **0xB2** | 距离 2-450cm |
| **S7** | 人体红外子板 | - | **0xB8** | GPIO 电平读取 |
| **S8** | 温湿度子板 | SHT35 | **0x88** | ±0.1°C, ±1.5%RH |
| **E1** | RGB+数码管子板 | PCA9685 + HT16K33 | **0x80** / **0xE2** | 3色 LED + 4位 8段 |
| **E2** | 风扇子板 | PCA9685 | **0x82** | PWM 调速 0-100% |
| **E3** | 窗帘电机子板 | PCA9685 | **0x84** | 正反转, 限位 0-100% |
| **E4** | 功放子板 | WM8978 | **0x34** | I2S + 蜂鸣器 |
| **E5** | 触摸屏 | 串口屏 | UART | 界面交互（详见第 7 节） |
| **C2** | Zigbee 模块 | E18-MS1-PCB | UART2 | 广播组网 |
| **C3** | WiFi 模块 | ESP8266 | UART0 | HTTP AT 指令 |

### 2.2 关键器件参数速查

| 器件 | 工作电压 | 通信接口 | 速率/频率 |
|------|---------|---------|----------|
| BH1750 | 3.3V | I2C | 400kHz |
| SHT35 | 3.3V | I2C | 400kHz |
| MS523 NFC | 3.3V | I2C | 400kHz |
| PCA9685 | 5V 供电 / 3.3V 逻辑 | I2C | 400kHz |
| HT16K33 | 3.3V | I2C | 400kHz |
| WM8978 | 3.3V | I2C + I2S | 400kHz |
| E18-MS1-PCB | 3.3V | UART | 115200 bps |
| ESP8266 | 3.3V | UART | 115200 bps |
| 超声波模块 | 5V | I2C (模拟) | - |
| 触摸屏 E5 | 5V | UART | 9600/115200 bps |

---

## 3. UART 端口分配

| 串口 | 引脚 | 用途 | 管理方式 |
|------|------|------|---------|
| **UART0** | PA9 (TX), PA10 (RX) | **C3 WiFi** + **调试/前端控制** | uart.c (中断缓冲) + 前端命令解析 |
| **USART1** | PA2 (TX), PA3 (RX) | **u2p_vision 视觉识别** | main.c 独立中断 |
| **USART2** | PC10 (TX), PC11 (RX) | **C2 Zigbee (E18-MS1-PCB)** | c2.c 轮询收发 |

> **注意**：UART0 兼顾两个角色 — C3 WiFi 模块的 AT 指令通信 和 人类用户的前端串口命令。由 `uart.c` 统一管理中断缓冲区，`c3.c` 通过 `uart_rece_bytes` 读 AT 响应，`main.c` 通过 `uart_get_line` 提取前端命令行。

---

## 4. I2C 地址系统

### 4.1 地址计算规则

所有 I2C 地址使用 **8 位移位地址**（左移 1 位）：

```
8位地址 = 7位地址 << 1
```

| 模块 | 芯片 | 7位地址 | 8位地址 | 拨码配置 |
|------|------|---------|---------|---------|
| S2 BH1750 | 光照 | 0x23 | **0x46** | ADDR=L |
| S8 SHT35 | 温湿度 | 0x44 | **0x88** | ADDR=L |
| S5 MS523 | NFC | 0x29 | **0x52** | - |
| S6 超声波 | 模拟I2C | - | **0xB2** | - |
| S7 人体红外 | 模拟I2C | - | **0xB8** | - |
| E1 PCA9685 | RGB灯 | 0x40 | **0x80** | 拨码 000000 |
| E1 HT16K33 | 数码管 | 0x71 | **0xE2** | 拨码 111 或固定地址 |
| E2 PCA9685 | 风扇 | 0x41 | **0x82** | 拨码 000001 |
| E3 PCA9685 | 窗帘电机 | 0x42 | **0x84** | 拨码 000010 |
| S1 HT16K33 | 按键 | 0x72 | **0xE4** | 拨码 或固定地址 |
| E4 WM8978 | 功放 | 0x1A | **0x34** | 固定地址 |

> **重要**：每块 PCA9685 子板通过底部拨码开关设置唯一地址，确保无冲突。

---

## 5. 硬件搭建步骤

### 5.1 搭建准备

所需工具和材料：
- 小米 AIoT 实训箱 1 套（含 U1P 主控底板）
- 子板：S1、S2、S5、S6、S7、S8、E1、E2、E3、E4、C2、C3
- E5 触摸屏 1 块
- u2p_vision 视觉模块 1 块（含摄像头）
- NFC 标签卡 3 张（对应 3 种植物）
- USB Type-C 数据线 1 根
- 杜邦线若干
- PC 电脑 1 台（Windows）

### 5.2 子板插接

GD32F470 底板提供多个子板插座，I2C 总线已在底板上并联。插入顺序无严格要求，建议布局：

```
                  ┌─────────────────────────────────┐
                  │          E5 触摸屏 (串口)         │
                  │                                 │
   ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
   │  S1  │  │  S2  │  │  S5  │  │  S6  │  │  S7  │
   │ 按键 │  │ 光照 │  │ NFC  │  │超声波│  │红外  │
   └──────┘  └──────┘  └──────┘  └──────┘  └──────┘
   ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
   │  S8  │  │  E1  │  │  E2  │  │  E3  │
   │温湿度│  │RGB灯 │  │ 风扇 │  │ 窗帘 │
   └──────┘  └──────┘  └──────┘  └──────┘
   ┌──────┐  ┌──────┐  ┌──────┐
   │  E4  │  │  C2  │  │  C3  │
   │功放  │  │Zigbee│  │ WiFi │
   └──────┘  └──────┘  └──────┘
```

### 5.3 I2C 地址拨码设置

**PCA9685 子板（E1、E2、E3）** 必须通过底部拨码设置为不同地址：

| 子板 | 拨码位置 (A5-A0) | 8位 I2C 地址 |
|------|-------------------|-------------|
| E1-PCA9685 (RGB灯) | 000000 | 0x80 |
| E2-PCA9685 (风扇) | 000001 | 0x82 |
| E3-PCA9685 (窗帘) | 000010 | 0x84 |

**S1 按键子板** 的 HT16K33 通常固定为 0x72 (7位) / 0xE4 (8位)。

### 5.4 C2 Zigbee 模块接线

C2 E18-MS1-PCB 接入 USART2：

| C2 模块引脚 | 底板引脚 | 说明 |
|------------|---------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 地 |
| TXD | PC11 (USART2_RX) | C2 发送 → GD32 接收 |
| RXD | PC10 (USART2_TX) | C2 接收 ← GD32 发送 |

### 5.5 C3 WiFi 模块接线

C3 ESP8266 接入 UART0（与调试串口共用）：

| C3 模块引脚 | 底板引脚 | 说明 |
|------------|---------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 地 |
| TXD | PA10 (UART0_RX) | C3 发送 → GD32 接收 |
| RXD | PA9 (UART0_TX) | C3 接收 ← GD32 发送 |

> **注意**：UART0 同时连接 C3 WiFi 模块和 PC 调试串口。正常情况下 PC 端不会干扰 AT 指令通信。如需在 PC 上使用前端命令（TEMP? 等），UART0 的 RX 需要同时连接 C3 的 TX 和 USB 转串口的 TX，可使用模拟开关或直接并联（ESP8266 的 TX 在非透传模式下只在响应 AT 命令时发送数据，不会持续输出）。

### 5.6 USART1 接线 (u2p_vision)

| 视觉模块引脚 | 底板引脚 | 说明 |
|-------------|---------|------|
| TX | PA3 (USART1_RX) | 视觉模块发送 → GD32 接收 |
| RX | PA2 (USART1_TX) | 视觉模块接收 ← GD32 发送 |
| VCC | 5V / 3.3V | 供电 |
| GND | GND | 地 |

---

## 6. 编译与烧录

### 6.1 环境要求

| 工具 | 版本/说明 |
|------|---------|
| CMake | >= 3.20 |
| GNU Arm Embedded Toolchain | arm-none-eabi-gcc >= 10 |
| J-Link / DAP-Link 调试器 | 用于烧录固件 |
| Windows PowerShell | 编译脚本 |

### 6.2 编译步骤

```powershell
# 进入项目目录
cd c:\plant_assistant\plant_assistant\demo\Project\CMAKE_Project

# 创建 build 目录并配置
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=..\arm-gcc-toolchain.cmake

# 编译
cmake --build . -j4
```

编译产物：
- `build/PLANT_ASSISTANT.elf` — ELF 调试文件
- `build/PLANT_ASSISTANT.hex` — Intel HEX 烧录文件
- `build/PLANT_ASSISTANT.bin` — 二进制烧录文件

### 6.3 烧录

#### 使用 J-Link

```powershell
JLink.exe -device GD32F470ZK -if SWD -speed 4000 -autoconnect 1 -CommanderScript flash.jlink
```

#### 使用 DAP-Link + OpenOCD

```powershell
openocd -f interface/cmsis-dap.cfg -f target/gd32f4xx.cfg -c "program build/PLANT_ASSISTANT.hex verify reset exit"
```

---

## 7. 前端控制与 E5 触摸屏

### 7.1 通用串口命令（UART0, 115200bps）

拼好硬件后，在 PC 上打开串口终端（波特率 115200），发送以下命令，每行以 `\r\n` 结尾：

#### 查询类命令

| 命令 | 说明 | 示例响应 |
|------|------|---------|
| `TEMP?` | 查询温度 | `TEMP:25.3 C` |
| `HUMI?` | 查询湿度 | `HUMI:62.1 %` |
| `LIGHT?` | 查询光照 | `LIGHT:8500 lux` |
| `SCORE?` | 查询评分明细 | `SCORE:T=85 H=72 L=90 Total=80` |
| `STATUS?` | 全面状态报告 | 多行 JSON-like 输出 |
| `PLANT?` | 当前植物档案 | `PLANT:绿萝 (ID=1)` |
| `DISTANCE?` | 超声波距离 | `DISTANCE:250 mm` |
| `HUMAN?` | 人体红外检测 | `HUMAN:YES` |
| `HELP` | 帮助信息 | 列出所有命令 |

#### 控制类命令

| 命令 | 说明 |
|------|------|
| `PLANT:1` / `PLANT:2` / `PLANT:3` | 切换到对应植物 |
| `PLANT:NEXT` | 切换到下一个植物 |
| `CURTAIN:OPEN` | 手动开启窗帘 |
| `CURTAIN:CLOSE` | 手动关闭窗帘 |
| `CURTAIN:POS,50` | 设置窗帘到指定位置 (0-100%) |
| `CURTAIN:AUTO` | 恢复窗帘自动模式 |
| `FAN:ON` | 手动开启风扇 |
| `FAN:OFF` | 手动关闭风扇 |
| `FAN:SPEED,60` | 设置风扇转速 (0-100%) |
| `FAN:AUTO` | 恢复风扇自动模式 |
| `AUTO:ON` | 开启全自动模式（风扇+窗帘+RGB） |
| `AUTO:OFF` | 关闭全自动模式（全部手动） |
| `RGB:255,0,0` | 设置 RGB 灯颜色 (R,G,B 0-255) |
| `BEEP` | 测试蜂鸣器 |
| `ZIGBEE:Hello` | 通过 C2 Zigbee 广播消息 |

### 7.2 E5 触摸屏集成

E5 触摸屏通过 **独立串口** 与 GD32F470 通信。触摸屏作为"上位机"发送命令，GD32 作为"下位机"返回数据。

#### 7.2.1 连接方式

E5 触摸屏通常使用自身引出的串口，可连接到 GD32 的一个空闲 UART/USART。推荐方案：

**方案 A：通过 UART0 并联（与调试串口共享）**

- E5 触摸屏 TX → PA10 (UART0_RX)
- E5 触摸屏 RX → PA9 (UART0_TX)
- 此时 E5 发送的命令由 `uart_get_line` 提取并解析，响应同时返回给 E5 和 PC 终端

**方案 B：独立 UART（推荐量产方案）**

- 若底板有多余 UART 引出（如 USART2 已给 Zigbee 但可用额外的 USART5），可将 E5 独立连接
- 需在代码中增加对应的 UART 初始化和中断处理

#### 7.2.2 E5 触摸屏界面设计建议

E5 是串口指令屏，需通过上位机软件（如 USART HMI、Nextion Editor）设计界面。建议页面结构：

```
┌──────────────────────────────────┐
│   小米 AIoT 智能植物养护助手       │
├──────────────────────────────────┤
│  当前植物: [绿萝]  [切换]         │
│                                  │
│  温度: 25.3°C     [适宜 ✓]       │
│  湿度: 62.1%      [适宜 ✓]       │
│  光照: 8500 lux   [适宜 ✓]       │
│                                  │
│  综合评分: 80/100                 │
│  ━━━━━━━━━━━━░░░░                │
│                                  │
│  蒸发指数: 320 / 500             │
│  [浇水提醒]                       │
│                                  │
│  [遮阳: 开] [通风: 关]           │
│  [RGB调色] [自动/手动]           │
└──────────────────────────────────┘
```

E5 触摸屏发送的串口命令可直接复用第 7.1 节的命令协议，例如：
- 按钮"切换植物" → 发送 `PLANT:NEXT\r\n`
- 按钮"查询状态" → 发送 `STATUS?\r\n`
- 按钮"开窗帘" → 发送 `CURTAIN:OPEN\r\n`

#### 7.2.3 E5 接收数据解析

E5 收到 GD32 返回的字符串后，可在触摸屏脚本中通过 `usart_recv` 解析。例如收到 `TEMP:25.3 C\r\n` 后，解析出 `25.3` 显示在温度数值组件上。

---

## 8. Zigbee (C2) 通信

### 8.1 模块参数

| 参数 | 值 |
|------|-----|
| 模块型号 | E18-MS1-PCB |
| 频段 | 2.4 GHz |
| 通信接口 | UART (115200-8-N-1) |
| 工作模式 | 透传 / 广播 |
| 默认角色 | 终端 (Terminal) |

### 8.2 Zigbee 数据流

```
GD32F470 (USART2) → E18-MS1-PCB → Zigbee 网络广播
GD32F470 (USART2) ← E18-MS1-PCB ← Zigbee 网络消息
```

### 8.3 数据广播内容（每 10 秒）

```
PLANT:绿萝|T:25.3|H:62.1|L:8500|S:80|E:320
```

字段含义：植物名, 温度(℃), 湿度(%), 光照(lux), 评分(0-100), 蒸发指数

### 8.4 Zigbee 接收处理

其他 Zigbee 节点发来的消息存入 `g_sys.zigbee_rx_buf`，可被前端命令 (`ZIGBEE:MSG`) 转发或用作远程控制指令。

---

## 9. WiFi (C3) 通信

### 9.1 模块参数

| 参数 | 值 |
|------|-----|
| 模块型号 | ESP8266 (AT 固件) |
| WiFi 配置 | 代码中宏定义 (`c3.h`) |
| 默认 SSID | `Xiaomi_9539` |
| 默认密码 | `1895@xiaomi` |
| 服务器 IP | `192.168.31.228` |
| 服务器端口 | `8080` |
| 浇水提醒路径 | `/water_alert` |

### 9.2 浇水提醒 HTTP 请求

当蒸发指数超过当前植物的 `water_threshold` 时：

```
GET /water_alert?plant=绿萝&alert=water HTTP/1.1
Host: 192.168.31.228:8080
Connection: close
```

或带完整传感器数据的版本：

```
GET /water_alert?plant=绿萝&temp=25.3&humi=62.1&light=8500&evap=320 HTTP/1.1
Host: 192.168.31.228:8080
Connection: close
```

### 9.3 WiFi 连接状态机

`main.c` 维护了两个状态变量：
- `g_wifi_run_state`：WiFi 是否处于运行状态
- `g_tcp_status`：TCP 连接状态（-1=未连接, 0=连接中, 1=已连接）

`c3_wifi_tcp_lead()` 每秒检查一次，自动处理 AT 命令的交互流程。

---

## 10. 完整运行流程

### 10.1 上电启动

1. 确认所有子板已插入底板对应插座
2. 确认 I2C 拨码地址无冲突
3. C2/C3 天线连接好
4. E5 触摸屏已接线
5. 通过 USB Type-C 或调试器供电

### 10.2 启动过程

```
1. 系统时钟初始化 (systick_config)
2. 硬件初始化 (device_init):
   ├── I2C 总线初始化 (init_i2c)
   ├── Timer3 1ms 定时器 (timer3_init)
   ├── UART0 初始化 (uart_init, C3 WiFi + 前端)
   ├── USART1 初始化 (usart1_init, u2p_vision)
   ├── C3 WiFi AT 初始化 (c3_init)
   └── 植物逻辑初始化 (plant_logic_init):
       ├── 植物档案加载
       ├── I2C 传感器地址确认
       ├── E1/E2/E3/E4 初始化
       ├── C2 Zigbee 初始化
       └── USART0 启动横幅
```

### 10.3 常态运行

```
[每 10ms]  检查串口前端命令
[每 50ms]  按键扫描
[每 1s]:
  ├── WiFi 状态维护
  ├── 传感器数据采集 (S2/S8/S6/S7)
  ├── 环境评分
  ├── 数码管+RGB 刷新
  ├── 遮阳/通风/灯光自动控制
  ├── 蒸发指数累加
  ├── 浇水判断 → 触发蜂鸣+RGB闪烁+WiFi推送
  ├── Zigbee 数据广播（每10s）
  └── NFC 卡片检测（每2s）
```

### 10.4 植物切换流程

4 种切换入口，最终调用同一个 `plant_switch()`：

```
┌─────────────┐
│ 方式1: 按键  │ KEY_1/KEY_2/KEY_3
├─────────────┤
│ 方式2: NFC  │ nfc_card_process()
├─────────────┤
│ 方式3: 串口  │ uart_command_parse("PLANT:N")
├─────────────┤
│ 方式4: 视觉  │ vision_uart_process()
└──────┬──────┘
       ▼
  plant_switch(id)
  ├── 更新 g_sys.current_plant
  ├── 蒸发指数清零
  ├── 重置浇水冷却标记
  ├── 数码管显示新的植物编号
  └── USART0 输出 "PLANT:Switched to xxx"
```

---

## 11. 项目文件结构

```
plant_assistant/
├── docs/
│   └── hardware-setup-guide.md        ← 本文档
├── task.md                            ← 原始任务描述
├── 具体说明.md                         ← 硬件详细说明
├── xiaomi-aiot/                       ← 参考代码 (小米 AIoT 实训箱)
└── plant_assistant/
    └── demo/
        └── Project/
            ├── Application/
            │   ├── inc/
            │   │   ├── main.h              ← 主程序头文件
            │   │   └── plant_logic.h       ← 植物逻辑数据结构 + 函数声明
            │   └── src/
            │       ├── main.c              ← 主循环 + 中断 + 前端命令调度
            │       └── plant_logic.c       ← 传感器/执行器/评分/控制/通信逻辑
            ├── GD32F470Z_BSP/
            │   ├── inc/
            │   │   ├── i2c.h               ← I2C 总线驱动头文件
            │   │   ├── uart.h              ← UART 公用驱动头文件
            │   │   ├── s1.h / s2.h / s5.h   ← 传感器驱动头文件
            │   │   ├── s6.h / s7.h / s8.h   ← 传感器驱动头文件
            │   │   ├── e1.h / e2.h / e3.h   ← 执行器驱动头文件
            │   │   ├── e4.h                ← 功放驱动头文件
            │   │   ├── c2.h                ← C2 Zigbee 驱动头文件
            │   │   ├── c3.h                ← C3 WiFi 驱动头文件
            │   │   └── systick.h           ← 系统定时器头文件
            │   └── src/
            │       ├── i2c.c               ← I2C 总线驱动实现
            │       ├── uart.c              ← UART 公用驱动 (中断缓冲+命令行提取)
            │       ├── s1.c ~ s8.c         ← 传感器驱动实现
            │       ├── e1.c ~ e4.c         ← 执行器驱动实现
            │       ├── c2.c                ← C2 Zigbee 初始化+广播+接收
            │       ├── c3.c                ← C3 WiFi AT+HTTP 发送
            │       └── systick.c           ← 系统定时器
            └── CMAKE_Project/
                ├── CMakeLists.txt          ← CMake 编译配置
                ├── gd32f470zk.ld           ← 链接脚本
                └── startup_gd32f470_gcc.S  ← 启动文件
```

---

## 12. 常见问题排查

### 12.1 I2C 地址冲突

**症状**：某个传感器/执行器无响应，读数为 0
**排查**：
1. 检查拨码开关位置是否与代码中地址一致
2. 用 I2C 扫描程序确认所有地址

### 12.2 C3 WiFi 连接失败

**症状**：WiFi 连接不上，浇水提醒未发出
**排查**：
1. 确认 SSID 和密码与路由器一致
2. 确认服务器 IP 可达（同一局域网）
3. 检查天线是否连接好

### 12.3 C2 Zigbee 无数据

**症状**：Zigbee 广播无数据或接收不到
**排查**：
1. 确认 USART2 接线正确（交叉连接）
2. 检查 `c2_init()` 是否成功
3. 确认 PAN ID 和 Group ID 与目标节点一致

### 12.4 UART0 串口乱码

**症状**：前端命令无响应或乱码
**排查**：
1. 确认波特率 115200
2. 确认数据位 8, 停止位 1, 无校验
3. 检查 C3 模块是否在持续输出透传数据干扰 UART

---

## 13. 与小米 AIoT 实训箱的对照关系

本项目的硬件驱动基于 `xiaomi-aiot/` 参考代码移植，主要适配点：

| 参考文件 | 移植后文件 | 变更说明 |
|---------|-----------|---------|
| `xiaomi-aiot/bsp_driver/i2c.c` | `i2c.c` | GD32F4xx 标准库风格 |
| `xiaomi-aiot/Project_Individual/S2/includes.h` | `s2.h/s2.c` | BH1750 从 Arduino 风格移植到 GD32 库 |
| `xiaomi-aiot/...` (其他子板示例) | `s8.c/e1.c/e2.c/e3.c/e4.c` | I2C 地址全部转换为 8 位移位格式 |
| `xiaomi-aiot/C2/` | `c2.c` | E18-MS1-PCB 的 AT 指令封装 |
| `xiaomi-aiot/C3/` | `c3.c` | ESP8266 AT 指令 + HTTP GET 封装 |

> **关键变化**：参考代码中 I2C 地址使用 Arduino 格式（7位），本项目统一使用 GD32 标准库的 8 位移位地址（左移 1 位），驱动器初始化时无需再做移位操作。

---

*最后更新: 2026-06-11*