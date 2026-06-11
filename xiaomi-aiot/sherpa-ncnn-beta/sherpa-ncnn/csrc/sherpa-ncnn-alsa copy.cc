#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <alsa/asoundlib.h>  // 新增ALSA头文件

#include "sherpa-ncnn/csrc/display.h"
#include "sherpa-ncnn/csrc/recognizer.h"

// 新增常量定义
const int32_t SAMPLE_RATE = 16000;
const int32_t CHANNELS = 4;         // 总声道数
const int32_t CHANNEL_TO_PROCESS = 4; // 需要处理的声道（1-based）
const int32_t PERIOD_SIZE = 1600;    // 每次读取的样本数 (0.1秒)

bool stop = false;

static void Handler(int sig) {
  stop = true;
  fprintf(stderr, "\nCaught Ctrl + C. Exiting...\n");
};

int main(int32_t argc, char *argv[]) {
  if (argc < 9 || argc > 12) {  // 修改参数检查
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

  signal(SIGINT, Handler);

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
  if (argc >= 10 && atoi(argv[9]) > 0) {
    num_threads = atoi(argv[9]);
  }

  config.model_config.encoder_opt.num_threads = num_threads;
  config.model_config.decoder_opt.num_threads = num_threads;
  config.model_config.joiner_opt.num_threads = num_threads;

  if (argc >= 11) {
    std::string method = argv[10];
    if (method == "greedy_search" || method == "modified_beam_search") {
      config.decoder_config.method = method;
    }
  }

  if (argc >= 12) {
    config.hotwords_file = argv[11];
  }

  if (argc == 13) {
    config.hotwords_score = atof(argv[12]);
  }

  // 修改端点检测配置
  config.enable_endpoint = true;
  config.endpoint_config.rule1.min_trailing_silence = 2.4;
  config.endpoint_config.rule2.min_trailing_silence = 1.2;
  config.endpoint_config.rule3.min_utterance_length = 300;

  config.feat_config.sampling_rate = SAMPLE_RATE;
  config.feat_config.feature_dim = 80;

  fprintf(stderr, "%s\n", config.ToString().c_str());

  // 初始化ALSA设备（替换原来的Alsa类）
  snd_pcm_t *pcm_handle;
  int err = snd_pcm_open(&pcm_handle, device_name, SND_PCM_STREAM_CAPTURE, 0);
  if (err < 0) {
    fprintf(stderr, "Cannot open audio device %s (%s)\n", 
            device_name, snd_strerror(err));
    return -1;
  }

  // 配置硬件参数
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_hw_params_alloca(&hw_params);
  
  snd_pcm_hw_params_any(pcm_handle, hw_params);
  snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm_handle, hw_params, CHANNELS);
  
  unsigned int actual_rate = SAMPLE_RATE;
  snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &actual_rate, 0);
  
  snd_pcm_uframes_t period_size = PERIOD_SIZE;
  snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &period_size, 0);
  
  if ((err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0) {
    fprintf(stderr, "Cannot set parameters (%s)\n", snd_strerror(err));
    return -1;
  }

  fprintf(stderr, "ALSA configured:\n");
  fprintf(stderr, "  Channels: %d (using channel %d)\n", CHANNELS, CHANNEL_TO_PROCESS);
  fprintf(stderr, "  Sample rate: %d Hz\n", actual_rate);
  fprintf(stderr, "  Period size: %lu frames\n", period_size);

  // 准备缓冲区
  int16_t buffer[PERIOD_SIZE * CHANNELS];
  float samples[PERIOD_SIZE];

  // 初始化识别器
  sherpa_ncnn::Recognizer recognizer(config);
  auto s = recognizer.CreateStream();

  std::string last_text;
  int32_t segment_index = 0;
  sherpa_ncnn::Display display;

  while (!stop) {
    // 读取原始音频数据
    int frames = snd_pcm_readi(pcm_handle, buffer, PERIOD_SIZE);
    if (frames < 0) {
      fprintf(stderr, "Audio read error: %s\n", snd_strerror(frames));
      break;
    }

    // 提取第四个声道（索引3）并转换格式
    for (int i = 0; i < frames; ++i) {
      samples[i] = buffer[i * CHANNELS + (CHANNEL_TO_PROCESS-1)] / 32768.0f;
    }

    // 送入识别器
    s->AcceptWaveform(SAMPLE_RATE, samples, frames);
    
    while (recognizer.IsReady(s.get())) {
      recognizer.DecodeStream(s.get());
    }

    bool is_endpoint = recognizer.IsEndpoint(s.get());
    auto text = recognizer.GetResult(s.get()).text;

    // 显示结果逻辑保持不变
    if (!text.empty() && last_text != text) {
      last_text = text;
      std::transform(text.begin(), text.end(), text.begin(),
                     [](auto c) { return std::tolower(c); });
      display.Print(segment_index, text);
    }

    if (is_endpoint) {
      if (!text.empty()) {
        ++segment_index;
      }
      recognizer.Reset(s.get());
    }
  }

  // 清理资源
  snd_pcm_close(pcm_handle);
  return 0;
}