"""
植物管家 - PC前端控制面板
通过串口(UART0)与U1计算子板通信，提供Web界面控制和状态展示
"""
import json
import threading
import time
import serial
import serial.tools.list_ports
from flask import Flask, render_template, jsonify, request

app = Flask(__name__)

# ==================== 全局状态 ====================
serial_port = None
serial_lock = threading.Lock()

device_status = {
    "connected": False,
    "port": "",
    "baudrate": 115200,
    "plant_name": "---",
    "plant_id": 0,
    "temperature": 0.0,
    "humidity": 0.0,
    "light": 0.0,
    "total_score": 0,
    "temp_score": 0,
    "humi_score": 0,
    "light_score": 0,
    "evaporation": 0.0,
    "water_threshold": 0.0,
    "distance": 0,
    "human": False,
    "curtain_pos": 0,
    "curtain_auto": True,
    "fan_speed": 0,
    "fan_auto": True,
    "auto_mode": True,
    "cooldown": False,
}

# ==================== 串口通信 ====================
def find_serial_port():
    """自动查找可用串口"""
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        # 优先匹配常见描述
        if "USB" in p.description or "COM" in p.device:
            return p.device
    return ports[0].device if ports else ""

def serial_send_command(cmd):
    """向设备发送串口命令"""
    with serial_lock:
        if serial_port and serial_port.is_open:
            try:
                serial_port.write((cmd + "\n").encode("utf-8"))
                response = serial_port.readline().decode("utf-8", errors="ignore").strip()
                return response
            except Exception as e:
                return f"ERROR:{e}"
    return "ERROR:Not connected"

def parse_status_response(raw):
    """解析 STATUS? 命令的响应"""
    try:
        lines = raw.split("\r\n")
        for line in lines:
            line = line.strip()
            if line.startswith("Plant:"):
                parts = line.split()
                if len(parts) >= 2:
                    device_status["plant_name"] = parts[1]
            elif line.startswith("Temp:"):
                parts = line.split()
                device_status["temperature"] = float(parts[1])
                device_status["temp_score"] = int(parts[3].split(":")[1])
            elif line.startswith("Humi:"):
                parts = line.split()
                device_status["humidity"] = float(parts[1])
                device_status["humi_score"] = int(parts[3].split(":")[1])
            elif line.startswith("Light:"):
                parts = line.split()
                device_status["light"] = float(parts[1])
                device_status["light_score"] = int(parts[3].split(":")[1])
            elif line.startswith("Total Score:"):
                parts = line.split()
                device_status["total_score"] = int(parts[2].split("/")[0])
            elif line.startswith("Evaporation:"):
                parts = line.split()
                device_status["evaporation"] = float(parts[1])
                device_status["water_threshold"] = float(parts[3])
            elif line.startswith("Curtain:"):
                parts = line.split()
                device_status["curtain_pos"] = int(parts[1].replace("%", ""))
                device_status["curtain_auto"] = "AUTO" in parts[2]
            elif line.startswith("Fan:"):
                parts = line.split()
                device_status["fan_speed"] = int(parts[1].replace("%", ""))
                device_status["fan_auto"] = "AUTO" in parts[2]
            elif line.startswith("Distance:"):
                parts = line.split()
                device_status["distance"] = int(parts[1])
            elif line.startswith("Human:"):
                parts = line.split()
                device_status["human"] = "YES" in parts[1]
            elif line.startswith("Auto Mode:"):
                parts = line.split()
                device_status["auto_mode"] = "ON" in parts[1]
            elif line.startswith("Cooldown:"):
                parts = line.split()
                device_status["cooldown"] = "YES" in parts[1]
    except Exception as e:
        print(f"Parse error: {e}")

def polling_thread():
    """后台轮询设备状态"""
    while True:
        if serial_port and serial_port.is_open:
            # 查询温度
            resp = serial_send_command("TEMP?")
            if resp.startswith("TEMP:"):
                try:
                    parts = resp.split()
                    device_status["temperature"] = float(parts[0].split(":")[1])
                except:
                    pass
            time.sleep(0.1)

            # 查询湿度
            resp = serial_send_command("HUMI?")
            if resp.startswith("HUMI:"):
                try:
                    parts = resp.split()
                    device_status["humidity"] = float(parts[0].split(":")[1])
                except:
                    pass
            time.sleep(0.1)

            # 查询完整状态
            resp = serial_send_command("STATUS?")
            if "PLANT ASSISTANT STATUS" in resp:
                parse_status_response(resp)

        time.sleep(2)  # 2秒轮询间隔

# ==================== Web路由 ====================
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/status")
def api_status():
    """获取设备状态的 JSON API"""
    return jsonify(device_status)

@app.route("/api/command", methods=["POST"])
def api_command():
    """向设备发送命令"""
    cmd = request.json.get("command", "")
    if not cmd:
        return jsonify({"error": "No command"}), 400
    response = serial_send_command(cmd)
    return jsonify({"response": response})

@app.route("/api/connect", methods=["POST"])
def api_connect():
    """连接/断开串口"""
    global serial_port
    action = request.json.get("action", "connect")
    port_name = request.json.get("port", "")

    if action == "disconnect":
        if serial_port and serial_port.is_open:
            serial_port.close()
        device_status["connected"] = False
        return jsonify({"connected": False})

    if action == "connect":
        try:
            if not port_name:
                port_name = find_serial_port()
            if not port_name:
                return jsonify({"error": "No serial port found"}), 400

            if serial_port and serial_port.is_open:
                serial_port.close()

            serial_port = serial.Serial(
                port=port_name,
                baudrate=115200,
                timeout=1
            )
            device_status["port"] = port_name
            device_status["connected"] = True
            return jsonify({"connected": True, "port": port_name})
        except Exception as e:
            return jsonify({"error": str(e)}), 500

@app.route("/api/ports")
def api_ports():
    """获取可用串口列表"""
    ports = [p.device for p in serial.tools.list_ports.comports()]
    return jsonify({"ports": ports})

# ==================== 启动 ====================
if __name__ == "__main__":
    print("=" * 50)
    print("  植物管家 - PC 前端控制面板")
    print("  访问 http://localhost:5000")
    print("=" * 50)

    # 启动后台轮询线程
    threading.Thread(target=polling_thread, daemon=True).start()

    app.run(host="0.0.0.0", port=5000, debug=False)