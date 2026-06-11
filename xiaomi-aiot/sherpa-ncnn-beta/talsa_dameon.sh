#!/bin/bash

# talsa守护脚本 - 确保命令持续运行
# 保存为 talsa_daemon.sh 并赋予执行权限: chmod +x talsa_daemon.sh

# 定义要执行的命令
COMMAND="./talsa ./smallmodel/tokens.txt ./smallmodel/encoder_jit_trace-pnnx.ncnn.param ./smallmodel/encoder_jit_trace-pnnx.ncnn.bin ./smallmodel/decoder_jit_trace-pnnx.ncnn.param ./smallmodel/decoder_jit_trace-pnnx.ncnn.bin ./smallmodel/joiner_jit_trace-pnnx.ncnn.param ./smallmodel/joiner_jit_trace-pnnx.ncnn.bin hw:0,1 4 greedy_search 2> /dev/null"

# 定义日志文件
LOG_FILE="talsa_daemon.log"

# 记录启动信息
echo "[$(date)] talsa守护进程已启动" >> "$LOG_FILE"

# 无限循环执行命令
while true; do
    # 执行命令
    echo "[$(date)] 启动talsa进程" >> "$LOG_FILE"
    eval $COMMAND
    
    # 命令退出后记录信息
    EXIT_CODE=$?
    echo "[$(date)] talsa进程退出，退出码: $EXIT_CODE" >> "$LOG_FILE"
    
    # 可选：添加短暂延迟避免CPU占用过高
    sleep 1
done