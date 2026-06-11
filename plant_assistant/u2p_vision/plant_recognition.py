import cv2
import requests
import base64
import serial
import time
import sys
import signal

# ==================== 配置区域 ====================
# 百度API配置（请替换成你自己的）
API_KEY = "o4MC3DQQBFGhnCbyrKvPT30m"
SECRET_KEY = "9e3Ufi15KJZBDvBG6uySPgzUYa1S79LQ"

# 串口配置（U2P -> U1P）
UART_PORT = "/dev/ttyS0"      # 地平线X3上UART0的路径
BAUD_RATE = 115200

# 摄像头配置
CAMERA_INDEX = 8               # MIPI摄像头通常为8，USB摄像头为0
CAPTURE_INTERVAL = 10          # 拍照识别间隔（秒）

# 临时图片保存路径
TEMP_IMAGE = "/tmp/plant_capture.jpg"
TEST_IMAGE = "/home/sunrise/plant_assistant/plant_assistant/test.jpg"

# ==================== 全局变量 ====================
ser = None
cap = None
running = True

# ==================== 信号处理 ====================
def signal_handler(sig, frame):
    global running
    print("\n正在退出...")
    running = False

# ==================== 百度API相关函数 ====================
def get_access_token():
    """获取百度API的access_token"""
    url = "https://aip.baidubce.com/oauth/2.0/token"
    params = {
        "grant_type": "client_credentials",
        "client_id": API_KEY,
        "client_secret": SECRET_KEY
    }
    
    try:
        response = requests.post(url, params=params, timeout=10)
        if response.status_code == 200:
            token = response.json().get("access_token")
            if token:
                print(f"[INFO] 获取Token成功")
                return token
            else:
                print(f"[ERROR] 响应中无token: {response.text}")
        else:
            print(f"[ERROR] 获取Token失败: {response.status_code}")
    except Exception as e:
        print(f"[ERROR] 获取Token异常: {e}")
    
    return None

def identify_plant(image_path, token):
    """调用百度植物识别API"""
    # 正确的API地址
    url = f"https://aip.baidubce.com/rest/2.0/image-classify/v1/plant?access_token={token}"
    
    # 读取图片并转base64
    try:
        with open(image_path, "rb") as f:
            img_base64 = base64.b64encode(f.read()).decode("utf-8")
    except Exception as e:
        print(f"[ERROR] 读取图片失败: {e}")
        return None, 0.0
    
    # 构造请求
    data = {
        'image': img_base64,
        'baike_num': 1
    }
    
    try:
        response = requests.post(url, data=data, timeout=10)
        
        if response.status_code == 200:
            result = response.json()
            
            # 检查是否有错误
            if "error_code" in result:
                print(f"[ERROR] API错误: {result.get('error_msg')} (code: {result.get('error_code')})")
                return None, 0.0
            
            # 解析结果
            if "result" in result and len(result["result"]) > 0:
                top_result = result["result"][0]
                name = top_result.get("name", "未知")
                score = top_result.get("score", 0.0)
                return name, score
            else:
                print(f"[WARN] 未识别到植物")
                return None, 0.0
        else:
            print(f"[ERROR] API请求失败: {response.status_code}")
            return None, 0.0
            
    except requests.exceptions.Timeout:
        print("[ERROR] API请求超时")
        return None, 0.0
    except Exception as e:
        print(f"[ERROR] API请求异常: {e}")
        return None, 0.0

# ==================== 摄像头相关函数 ====================
def init_camera():
    """初始化摄像头"""
    global cap
    
    # 尝试多个摄像头索引
    indices_to_try = [CAMERA_INDEX, 0, 1, 2, 4, 6]
    
    for idx in indices_to_try:
        print(f"[INFO] 尝试打开摄像头索引: {idx}")
        cap = cv2.VideoCapture(idx)
        if cap.isOpened():
            print(f"[INFO] 成功打开摄像头索引: {idx}")
            # 设置分辨率
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            # 测试读取一帧
            ret, frame = cap.read()
            if ret and frame is not None:
                print(f"[INFO] 摄像头工作正常")
                return True
            else:
                print(f"[WARN] 摄像头索引{idx}能打开但无法读取画面")
                cap.release()
                cap = None
    
    print("[ERROR] 无法打开任何摄像头")
    return False

def capture_image():
    """拍照并保存（已清空缓冲区，确保获取最新帧）"""
    global cap
    
    if cap is None or not cap.isOpened():
        print("[ERROR] 摄像头未初始化")
        return False

    # 清空缓冲区：连续读取并丢弃几帧，确保拿到最新画面
    for _ in range(4):
        cap.read()

    ret, frame = cap.read()
    if not ret or frame is None:
        print("[ERROR] 拍照失败")
        return False
    
    # 保存图片（可适当降低质量以减小文件体积）
    cv2.imwrite(TEMP_IMAGE, frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
    print(f"[INFO] 拍照成功，已保存至: {TEMP_IMAGE}")
    return True
# ==================== 串口相关函数 ====================
def init_serial():
    """初始化串口"""
    global ser
    
    try:
        ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=1)
        print(f"[INFO] 串口 {UART_PORT} 已打开，波特率 {BAUD_RATE}")
        return True
    except Exception as e:
        print(f"[ERROR] 无法打开串口 {UART_PORT}: {e}")
        return False

def send_to_u1p(plant_name, confidence):
    """通过串口发送结果给U1P"""
    global ser
    
    if ser is None or not ser.is_open:
        print("[ERROR] 串口未打开")
        return False
    
    # 格式：植物名|置信度\n
    # 置信度保留2位小数
    message = f"{plant_name}|{confidence:.2f}\n"
    
    try:
        ser.write(message.encode('utf-8'))
        print(f"[INFO] 发送: {message.strip()}")
        return True
    except Exception as e:
        print(f"[ERROR] 串口发送失败: {e}")
        return False

# ==================== 主函数 ====================
def main():
    global running
    
    print("=" * 50)
    print("U2P 植物识别程序启动")
    print("=" * 50)
    
    # 1. 注册信号处理
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # 2. 初始化串口
    if not init_serial():
        print("[ERROR] 串口初始化失败，程序退出")
        sys.exit(1)
    
    # 3. 初始化摄像头
    if not init_camera():
        print("[ERROR] 摄像头初始化失败，程序退出")
        sys.exit(1)
    
    # 4. 获取Access Token
    print("[INFO] 正在获取Access Token...")
    token = get_access_token()
    if not token:
        print("[ERROR] 获取Token失败，请检查API Key和网络")
        sys.exit(1)
    
    print("[INFO] 程序就绪，开始识别循环...")
    print(f"[INFO] 拍照间隔: {CAPTURE_INTERVAL}秒")
    print("[INFO] 按 Ctrl+C 退出\n")
    
    # 5. 主循环
    last_capture_time = 0
    
    try:
        while running:
            current_time = time.time()
            
            # 每隔CAPTURE_INTERVAL秒执行一次
            if current_time - last_capture_time >= CAPTURE_INTERVAL:
                last_capture_time = current_time
                
                # 拍照
                if not capture_image():
                    continue
                
                # 识别
                print("[INFO] 正在识别...")
                plant_name, confidence = identify_plant(TEMP_IMAGE, token)
                
                if plant_name:
                    print(f"[INFO] 识别结果: {plant_name} (置信度: {confidence:.2f})")
                    # 发送给U1P
                    send_to_u1p(plant_name, confidence)
                else:
                    print("[WARN] 未识别到植物，跳过本次发送")
                
                print("-" * 40)
            
            # 短暂休眠，避免CPU占用过高
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n[INFO] 收到中断信号")
    finally:
        # 清理资源
        print("[INFO] 正在清理资源...")
        if cap is not None:
            cap.release()
        if ser is not None and ser.is_open:
            ser.close()
        print("[INFO] 程序退出")

if __name__ == "__main__":
    main()