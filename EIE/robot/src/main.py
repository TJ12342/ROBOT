import socket
import io
from PIL import Image
import cv2
import numpy as np
import os
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

# 配置服务器和本地端口
SERVER_IP = '192.168.4.1'  # 服务器IP
SEND_PORT = 10001  # 发送端口
LOCAL_PORT = 10000  # 接收端口
BUFFER_SIZE = 1024 * 1024 * 4  # 1 MB的缓冲区
HTTP_PORT = 8080  # 本地HTTP服务器端口

# 用于保存JPEG文件的目录
SAVE_DIR = 'saved_images'
os.makedirs(SAVE_DIR, exist_ok=True)  # 确保目录存在

# 全局变量
frame_buffer = []
frame_lock = threading.Lock()
save_image_flag = threading.Event()  # 用于触发保存图片

def udp_send(message):
    """发送消息到服务器"""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
            udp_socket.sendto(message.encode('utf-8'), (SERVER_IP, SEND_PORT))
    except Exception as e:
        print(f"Error sending message: {e}")

def udp_receive():
    """接收UDP数据流"""
    global frame_buffer
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
            udp_socket.bind(('', LOCAL_PORT))
            data_buffer = bytearray()

            while True:
                data, addr = udp_socket.recvfrom(BUFFER_SIZE)
                if not data:
                    break
                data_buffer.extend(data)

                # 处理数据流
                while True:
                    jpeg_data, data_buffer = process_jpeg_data(data_buffer)
                    if jpeg_data:
                        frame_lock.acquire()
                        frame_buffer.append(jpeg_data)
                        frame_lock.release()
                    else:
                        break
    except Exception as e:
        print(f"Error receiving data: {e}")

def process_jpeg_data(data_stream):
    """处理JPEG数据流"""
    start_marker = b'\xFF\xD8'  # JPEG起始标志
    end_marker = b'\xFF\xD9'  # JPEG结束标志

    # 查找JPEG起始标志
    start_pos = data_stream.find(start_marker)
    if start_pos == -1:
        return None, data_stream  # 没有找到起始标志，返回剩余数据

    # 查找JPEG结束标志
    end_pos = data_stream.find(end_marker, start_pos)
    if end_pos == -1:
        return None, data_stream  # 没有找到结束标志，返回剩余数据

    jpeg_data = data_stream[start_pos:end_pos + 2]  # 包含起始和结束标志
    remaining_data = data_stream[end_pos + 2:]  # 剩余的数据流

    return jpeg_data, remaining_data

def display_video():
    """显示JPEG图像"""
    global frame_buffer

    while True:
        if len(frame_buffer) > 0:
            frame_lock.acquire()
            jpeg_data = frame_buffer.pop(0)
            frame_lock.release()

            try:
                image = Image.open(io.BytesIO(jpeg_data))
                image_np = np.array(image)
                image_np = cv2.cvtColor(image_np, cv2.COLOR_RGB2BGR)  # 转换为OpenCV格式

                # 显示结果
                cv2.imshow('Video Stream', image_np)
                cv2.waitKey(1)  # 等待1毫秒

                # 保存图像
                if save_image_flag.is_set():
                    print("Save flag detected, saving image...")
                    timestamp = time.strftime("%Y%m%d_%H%M%S")
                    image_path = os.path.join(SAVE_DIR, f"image_{timestamp}.jpg")
                    image.save(image_path)
                    print(f"Image saved to {image_path}")
                    save_image_flag.clear()  # 清除标志
            
            except Exception as e:
                print(f"Error displaying image: {e}")
        else:
            time.sleep(0.01)  # 如果没有新帧，等待一小段时间以减少CPU使用率

class RequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        """处理GET请求"""
        if self.path == '/capture':
            print("保存图片")
            save_image_flag.set()  # 设置标志以保存图片
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"Image saved.")
        else:
            self.send_response(404)
            self.end_headers()

# 启动HTTP服务器
httpd = HTTPServer(('', HTTP_PORT), RequestHandler)

# 启动HTTP服务器线程
http_thread = threading.Thread(target=httpd.serve_forever)
http_thread.daemon = True
http_thread.start()

# 发送开始信号
print("发送信号以开始视频流...")
udp_send("START_VIDEO")  # 发送开始信号

# 启动接收线程
receive_thread = threading.Thread(target=udp_receive)
receive_thread.start()

# 启动显示视频线程
display_thread = threading.Thread(target=display_video)
display_thread.daemon = True
display_thread.start()

# 等待直到用户按下 'q' 键退出
while True:
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 关闭窗口
cv2.destroyAllWindows()
httpd.shutdown()