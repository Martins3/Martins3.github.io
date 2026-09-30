#!/usr/bin/env python3
import subprocess
import time
from collections import defaultdict

running_processes = defaultdict()  # 按命令类型存储进程
def execute_qemu_nbd(params, id : int):
    """执行 qemu-nbd 命令"""
    cmd = [
        "qemu-nbd",
        "--export-name", params["export_name"],
        "--port", str(params["port"]),
        "--persistent",
        "--shared", str(params["shared"]),
        "--format", params["format"],
        params["image"]
    ]

    try:
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        running_processes[id] = process
        print(f"Started qemu-nbd with PID {process.pid}")
    except Exception as e:
        print(f"Error executing qemu-nbd: {e}")


params={"export_name": "xueshi", "port": 6000,"shared": 2, "format": "qcow2", "image": "/home/martins3/1.qcow2"}
execute_qemu_nbd(params, 1 )
p = running_processes[1]

# 原来 poll 是需要多等待一会的，其实
while p.poll() is None:
    print("Process is still running...")
    time.sleep(1)  # 每隔1秒检查一次
    stdout, stderr = p.communicate()  # 阻塞直到进程结束
    print(f"Output: {stdout}")
    print(f"Error: {stderr}")
    print(f"Return code: {p.returncode}")
print(f"Process ended with return code: {p.poll()}")
