#include <alsa/asoundlib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <sys/wait.h>

#include "c2.h"
#include "c3.h"
#include "sherpa-ncnn/csrc/display.h"
#include "sherpa-ncnn/csrc/recognizer.h"
#include "uart.h"
#define BASE_DEVICE "/dev/ttyS"
#define VOICE_PATH "/userdata/wavs/"

// 新增线程通信相关定义
const int MAX_C2_PACKET_SIZE = 32;
// std::mutex queue_mutex;
// std::condition_variable data_cond;
bool thread_running = true;
pthread_t c2_recv_thread;

// 其他原有定义保持不变
const int32_t SAMPLE_RATE = 16000;
const int32_t CHANNELS = 4;
const int32_t CHANNEL_TO_PROCESS = 4;
const int32_t PERIOD_SIZE = 1600;

char command[100];
int fd;
bool stop = false;

static void playVoiceFork(const std::string &command)
{
  pid_t pid = fork();

  if (pid < 0)
  {
    fprintf(stderr, "Fork failed");
    return;
  }

  if (pid == 0)
  {
    // 子进程
    execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
    // 如果execl返回，说明执行失败
    fprintf(stderr, "Failed to execute: %s", command.c_str());
    exit(EXIT_FAILURE);
  }
  else
  {
    // 父进程，不等待子进程结束
    fprintf(stderr, "Voice playback started in background (PID: %d)\n", pid);
    int status;
    pid_t wpid = waitpid(pid, &status, WNOHANG);
    if (wpid == pid)
    {
      fprintf(stderr, "Command completed, status: %d\n", WEXITSTATUS(status));
    }
    sleep(1);
  }
}
static void IntHandler(int sig)
{
  stop = true;
  playVoiceFork("aplay " VOICE_PATH "shutdown.wav");
  fprintf(stderr, "\nCaught Ctrl + C. Exiting...\n");
};

static void SegvHandler(int sig)
{
  stop = true;
  playVoiceFork("aplay " VOICE_PATH "abnormal-shutdown.wav");
  c3_wifi_tcp_send(fd, "\nCaught Segmentfault. Exiting...\n");
  printf("\nCaught Segmentfault. Exiting...\n");
  exit(0);
}

short voice_text_handler(const std::string &text)
{
  if (text == "芝麻开门")
  {
    // playVoiceFork("aplay " VOICE_PATH "comeback.wav");
    return 0;
  }
  else if (text == "举头望明月")
  {
    // playVoiceFork("aplay " VOICE_PATH "reset.wav");
    return 3;
  }
  else if (text == "灵锁" || text == "零锁" || text == "领索")
  {
    playVoiceFork("aplay " VOICE_PATH "zai.wav");
  }
  return 10;
}

static struct
{
  char *name;
  char *file_name;
} files[] = {
    {"backhome", "comeback.wav"},
    {"reset", "reset.wav"},
    {"nfc", "nfc.wav"},
    {"voice", "voice.wav"},
    {"nfcfalse", "nfcfalse.wav"},
    {"pwfalse", "pwfalse.wav"},
    {"pwmultifalse", "pwmultifalse.wav"},
};

// 线程函数：接收c2数据
void *c2_recv_thread_func(void *arg)
{
  // 设置线程为分离状态，避免资源泄漏
  pthread_detach(pthread_self());

  // 优化线程优先级，低于主循环
  struct sched_param param;
  param.sched_priority = 5;
  pthread_setschedparam(pthread_self(), SCHED_RR, &param);

  char buffer[MAX_C2_PACKET_SIZE];

  while (thread_running)
  {
    // 非阻塞接收c2数据
    int recv_len = c2_rec_data_nonblock((uint8_t *)buffer, MAX_C2_PACKET_SIZE);

    if (recv_len > 0)
    {
      // 分配内存并复制数据
      for (int i = 0; i < sizeof(files) / sizeof(files[0]); i++)
      {
        if (strncmp(buffer, files[i].name, strlen(files[i].name)) == 0)
        {
          snprintf(command, sizeof(command), "aplay %s%s", VOICE_PATH,
                   files[i].file_name);
          playVoiceFork(command);
          break;
        }
      }
    }
    else
    {
      // 短暂休眠，减少CPU占用
      usleep(1000); // 1ms
    }
  }

  return NULL;
}

// 初始化接收线程
bool init_c2_recv_thread()
{
  int ret = pthread_create(&c2_recv_thread, NULL, c2_recv_thread_func, NULL);
  if (ret != 0)
  {
    perror("Failed to create c2 recv thread");
    return false;
  }
  return true;
}

// 停止接收线程
void stop_c2_recv_thread()
{
  thread_running = false;
  // 通知条件变量，让线程退出
  // data_cond.notify_one();
  // 等待线程结束
  pthread_join(c2_recv_thread, NULL);
}

int main(int32_t argc, char *argv[])
{
  if (argc < 9 || argc > 12)
  {
    const char *usage = R"usage(
Usage:
  ./bin/sherpa-ncnn-alsa \
    /path/to/tokens.txt \
    /path/to/encoder.ncnn.param \
    /path/to/encoder.ncnn.bin \
    /path/to/decoder.ncnn.param \
    /path/to/decoder.ncnn.bin \
    /path/to/joiner.ncnn.param \
    /path/to/joiner.ncnn.bin \
    device_name \
    [num_threads] [decode_method] [hotwords_file] [hotwords_score]

新增说明：当前支持4声道设备，固定处理第4个声道
)usage";

    fprintf(stderr, "%s\n", usage);
    return 0;
  }

  signal(SIGINT, IntHandler);
  // 异常错误处理
  signal(SIGSEGV, SegvHandler);

  // 初始化配置部分保持不变
  sherpa_ncnn::RecognizerConfig config;
  config.model_config.tokens = argv[1];
  config.model_config.encoder_param = argv[2];
  config.model_config.encoder_bin = argv[3];
  config.model_config.decoder_param = argv[4];
  config.model_config.decoder_bin = argv[5];
  config.model_config.joiner_param = argv[6];
  config.model_config.joiner_bin = argv[7];

  const char *device_name = argv[8];

  int num_threads = 4;
  if (argc >= 10 && atoi(argv[9]) > 0)
  {
    num_threads = atoi(argv[9]);
  }

  config.model_config.encoder_opt.num_threads = num_threads;
  config.model_config.decoder_opt.num_threads = num_threads;
  config.model_config.joiner_opt.num_threads = num_threads;

  if (argc >= 11)
  {
    std::string method = argv[10];
    if (method == "greedy_search" || method == "modified_beam_search")
    {
      config.decoder_config.method = method;
    }
  }
  // NOTE: IP
  std::string ip = "192.168.31.228";

  if (argc >= 12)
  {
    std::string ip = argv[11];
  }

  // if (argc >= 12) {
  //   config.hotwords_file = argv[11];
  // }

  // if (argc == 13) {
  //   config.hotwords_score = atof(argv[12]);
  // }

  // 修改端点检测配置
  config.enable_endpoint = true;
  config.endpoint_config.rule1.min_trailing_silence = 2.4;
  config.endpoint_config.rule2.min_trailing_silence = 1.2;
  config.endpoint_config.rule3.min_utterance_length = 300;

  config.feat_config.sampling_rate = SAMPLE_RATE;
  config.feat_config.feature_dim = 80;

  fprintf(stderr, "%s\n", config.ToString().c_str());

  // 初始化ALSA设备
  snd_pcm_t *pcm_handle;
  int err = snd_pcm_open(&pcm_handle, device_name, SND_PCM_STREAM_CAPTURE, 0);
  if (err < 0)
  {
    fprintf(stderr, "Cannot open audio device %s (%s)\n", device_name,
            snd_strerror(err));
    return -1;
  }

  // 配置硬件参数
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_hw_params_alloca(&hw_params);

  snd_pcm_hw_params_any(pcm_handle, hw_params);
  snd_pcm_hw_params_set_access(pcm_handle, hw_params,
                               SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle, hw_params, CHANNELS);

  unsigned int actual_rate = SAMPLE_RATE;
  snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &actual_rate, 0);

  snd_pcm_uframes_t period_size = PERIOD_SIZE;
  snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &period_size,
                                         0);

  if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0)
  {
    fprintf(stderr, "Cannot set parameters (%s)\n", snd_strerror(err));
    return -1;
  }

  fprintf(stderr, "ALSA configured:\n");
  fprintf(stderr, "  Channels: %d (using channel %d)\n", CHANNELS,
          CHANNEL_TO_PROCESS);
  fprintf(stderr, "  Sample rate: %d Hz\n", actual_rate);
  fprintf(stderr, "  Period size: %lu frames\n", period_size);

  // 准备缓冲区
  int16_t buffer[PERIOD_SIZE * CHANNELS];
  float samples[PERIOD_SIZE];

  // 初始化识别器
  sherpa_ncnn::Recognizer recognizer(config);
  auto s = recognizer.CreateStream();

  std::string last_text;
  std::string send_buffer;
  int32_t segment_index = 0;
  int32_t last_index = segment_index;
  sherpa_ncnn::Display display;
  playVoiceFork("aplay " VOICE_PATH "oxp.wav");
  sleep(1);
  // 初始化c2模块
  printf("begin init\n");

  if (c3_init(&fd, SSID, PASSWD, (char *)ip.c_str(), PORT) ==
      0)
  {
    printf("C3 init error.\n");
    snd_pcm_close(pcm_handle);
    return -1;
  }
  printf("C3 Ready.\n");
  char data[2];
  int run_wifi_state = 1;
  int tcp_init_state = -1;

  // 轮询串口部分保持不变
  while (1)
  {
    int serial_fd = -1;
    char device[10] = BASE_DEVICE;
    char dev_num[2] = {0};
    int init_success = 0;

    for (int i = 0; i < 6; i++)
    {
      dev_num[0] = '0' + i;
      dev_num[1] = '\0';

      strcpy(device, BASE_DEVICE);
      strcat(device, dev_num);

      serial_fd = init_serial(device);
      if (serial_fd == -1)
      {
        printf("无法打开串口 %s\n", device);
        continue;
      }

      if (c2_init(0x02) != 0)
      {
        init_success = 1;
        break;
      }

      printf("不是串口%s\n", device);
      close(serial_fd);
      serial_fd = -1;
    }

    if (!init_success)
    {
      printf("所有串口初始化失败，5秒后重试...\n");
      sleep(5);
      continue;
    }

    printf("uart_init success:serial_fd=%d,dev_num=%c\n", serial_fd,
           dev_num[0]);
    break;
  }
  c2_broadcast_data("Hello world!\n", 0x01);

  // 初始化wifi，tcp模块
  // sprintf(SSID, "Xiaomi_9539");
  // sprintf(PWD, "1895@xiaomi");
  // sprintf(ip, "192.168.31.228");
  // sprintf(port, "8080");

  // 启动c2接收线程
  if (!init_c2_recv_thread())
  {
    fprintf(stderr, "Failed to initialize c2 recv thread\n");
    snd_pcm_close(pcm_handle);
    return -1;
  }
  bool starts = false;

  while (!stop)
  {
    c3_wifi_tcp_lead(fd, &run_wifi_state, &tcp_init_state);
    if (tcp_init_state == 0)
    {
      if (!starts)
      {
        playVoiceFork("aplay " VOICE_PATH "net-initial.wav");
        starts = true;
        sleep(2);
      }
      // 读取原始音频数据
      int frames = snd_pcm_readi(pcm_handle, buffer, PERIOD_SIZE);
      if (frames < 0)
      {
        fprintf(stderr, "Audio read error: %s\n", snd_strerror(frames));
        break;
      }

      // 提取第四个声道并转换格式
      for (int i = 0; i < frames; ++i)
      {
        samples[i] = buffer[i * CHANNELS + (CHANNEL_TO_PROCESS - 1)] / 32768.0f;
      }

      // 送入识别器
      s->AcceptWaveform(SAMPLE_RATE, samples, frames);

      while (recognizer.IsReady(s.get()))
      {
        recognizer.DecodeStream(s.get());
      }

      bool is_endpoint = recognizer.IsEndpoint(s.get());
      auto text = recognizer.GetResult(s.get()).text;

      // 显示结果逻辑保持不变
      if (!text.empty() && last_text != text)
      {
        last_text = text;
        std::transform(text.begin(), text.end(), text.begin(),
                       [](auto c)
                       { return std::tolower(c); });
        display.Print(segment_index, text);
      }

      if (is_endpoint)
      {
        if (!text.empty())
        {
          if (c3_wifi_tcp_send(fd, (char *)text.c_str()) < 0)
          {
            fprintf(stderr, "Failed to send data to server\n");
          }
          data[0] = '0' + voice_text_handler(text);
          data[1] = '\0';

          c2_broadcast_data(data, 0x01);

          ++segment_index;
        }
        recognizer.Reset(s.get());
      }
    }
  }

  // 清理资源
  stop_c2_recv_thread();
  snd_pcm_close(pcm_handle);
  return 0;
}