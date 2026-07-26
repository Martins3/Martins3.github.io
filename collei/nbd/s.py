#!/usr/bin/env python3
import json
import os
import socket
import subprocess
import threading
from collections import defaultdict

images_dir = ""
all_vm_dir = ""
is_storage_node = False

running_processes = defaultdict()


def execute_qemu_nbd(params):
    guest_id = int(params.get("id"))
    uuid = params.get("uuid")
    disk_idx = int(params.get("disk_idx"))

    if not guest_id or not uuid or disk_idx is None:
        print(f"{guest_id} {uuid} {disk_idx}")
        return "Error: guest_id, uuid and disk_idx are required"

    base_dir = f"{images_dir}/{guest_id}"
    uuid_file = os.path.join(base_dir, "uuid.txt")
    disk_path = f"{base_dir}/{disk_idx}.qcow2"

    if os.path.exists(base_dir):
        if os.path.exists(uuid_file):
            with open(uuid_file, "r") as f:
                stored_uuid = f.read().strip()
                if stored_uuid != uuid:
                    return f"Error: Directory already exists with different UUID: {stored_uuid}"
    else:
        os.makedirs(base_dir, exist_ok=True)
        with open(uuid_file, "w") as f:
            f.write(uuid)
        print("ok !")

    if not os.path.exists(disk_path):
        create_cmd = ["qemu-img", "create", "-f", "qcow2", disk_path, "180G"]
        try:
            subprocess.run(create_cmd, check=True, capture_output=True, text=True)
        except subprocess.CalledProcessError as e:
            return f"Error creating disk: {e.stderr}"

    cmd = [
        "qemu-nbd",
        # TODO 使用相同的 port 会有问题吗?
        "--export-name",
        "martins3",
        # collei/scripts/bash-archive/collei-lib.sh 中 get_tcp_port
        # 使用 guest_id 生成唯一的端口号
        "--port",
        str(50000 + 6 + disk_idx),
        "--persistent",
        "--shared",
        "2",
        "--format",
        "qcow2",
        disk_path,  # 使用构建好的磁盘路径
    ]

    try:
        process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        running_processes[guest_id] = process
        return f"Started qemu-nbd with PID {process.pid}"
    except Exception as e:
        return f"Error executing qemu-nbd: {e}"


def count_servers():
    """查询当前运行的 server 数量（qemu-nbd 实例）"""
    result = ""
    for k, p in running_processes.items():
        if not running_processes[k]:
            continue
        return_code = p.poll()
        if return_code is None:
            print(k)
            result = result + f"{k} is running"
        else:
            result = result + f"client {k} has finished with return code: {return_code}"
            _, stderr = p.communicate()
            result = result + stderr
            running_processes[k] = None
    return result


# uuid 是唯一标志
# 1. 如果有相同的 uuid ，如果其他的都不同，那么需要手动修改，并且警告
# 2. 如果没有相同的 uuid ，但凡有相同的 id 或者名称，那么警告
def find_vm_dir(source_id, source_uuid, source_vm_name):
    target_vm_dir = ""
    for vm in os.listdir(all_vm_dir):
        vm_dir = os.path.join(all_vm_dir, vm)
        if not os.path.isdir(vm_dir + "/" + "opt"):
            continue

        with open(f"{vm_dir}/opt/uuid", "r", encoding="utf-8") as f:
            target_uuid = f.read().strip()

        if source_uuid == target_uuid:
            target_vm_dir = vm_dir
            break

    if target_vm_dir != "":
        target_vm_name = os.path.basename(target_vm_dir)
        with open(f"{target_vm_dir}/opt/id", "r", encoding="utf-8") as f:
            target_id = f.read().strip()
        if target_id != source_id or target_vm_name != source_vm_name:
            return f"conflicted with [{target_id} {target_vm_name}]\n"
        # 获取到完全匹配
        return target_vm_dir
    else:
        for vm in os.listdir(all_vm_dir):
            vm_dir = os.path.join(all_vm_dir, vm)
            if not os.path.isdir(vm_dir + "/" + "opt"):
                continue

            vm_name = os.path.basename(vm_dir)
            with open(f"{vm_dir}/opt/id", "r", encoding="utf-8") as f:
                id = f.read().strip()
            if vm_name == source_vm_name or id == source_id:
                return f"conflicted with <{id} {vm_name}>\n"
        return ""


def handle_migrate(request, client_socket):
    vm_name = request.get("vm_name")
    guest_id = request.get("id")
    guest_uuid = request.get("uuid")
    print(vm_name)

    vm_dir = find_vm_dir(guest_id, guest_uuid, vm_name)

    # 没有找到
    if vm_dir == "":
        vm_dir = all_vm_dir + "/" + vm_name
        os.mkdir(vm_dir)

    # 有冲突
    if vm_dir.startswith("conflicted"):
        client_socket.sendall(vm_dir.encode("utf-8"))
        return "Abort"

    # 完全匹配或者就是没有这个目录
    client_socket.sendall("ready".encode("utf-8"))

    file_size = request.get("file_size")
    config_dir = os.path.join(vm_dir, "opt")
    os.makedirs(config_dir, exist_ok=True)
    for filename, content in request["config"].items():
        with open(os.path.join(config_dir, filename), "w") as f:
            f.write(content)

    received_data = b""
    remaining_size = file_size
    while remaining_size > 0:
        chunk = client_socket.recv(min(remaining_size, 8192))
        if not chunk:
            break
        received_data += chunk
        remaining_size -= len(chunk)

    file_path = os.path.join(vm_dir, "cmd.sh")
    with open(file_path, "wb") as f:
        f.write(received_data)

    return "migrate setup successfully"


def handle_client(client_socket):
    try:
        data = client_socket.recv(1024).decode("utf-8")
        request = json.loads(data)
        print(f"Received request: {request}")

        if request["type"] == "setup_nbd_disk":
            if not is_storage_node:
                response = "Not a storage node"
            else:
                response = execute_qemu_nbd(request)
        elif request["type"] == "query":
            response = count_servers()
        elif request["type"] == "migrate":
            response = handle_migrate(request, client_socket)
        else:
            response = f"Unsupported query command: {request['type']}"

        client_socket.sendall(response.encode("utf-8"))
    except Exception as e:
        error_msg = f"Error handling request: {e}"
        client_socket.sendall(error_msg.encode("utf-8"))
    finally:
        client_socket.close()


def start_server(host, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Server listening on {host}:{port}")

    try:
        while True:
            client_socket, addr = server_socket.accept()
            print(f"Connection from {addr}")
            client_thread = threading.Thread(
                target=handle_client, args=(client_socket,)
            )
            client_thread.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        os._exit(0)


def set_images_dir(home_dir):
    file_path = os.path.join(home_dir, ".config", "collei", "storage")
    if not os.path.exists(file_path):
        return

    global is_storage_node
    is_storage_node = True
    global images_dir
    images_dir = f"{all_vm_dir}/images"

    if not os.path.exists(images_dir):
        os.makedirs(images_dir, exist_ok=True)


def get_vm_images_location(home_dir):
    file_path = os.path.join(home_dir, ".config", "collei", "vm")

    with open(file_path, "r", encoding="utf-8") as file:
        content = file.read().replace("\n", "")
        global all_vm_dir
        all_vm_dir = content
        print(f"all_vm_dir={all_vm_dir}")

    if not os.path.exists(all_vm_dir):
        print(f"${all_vm_dir} is not setup")
        exit(1)


def setup_dirs():
    home_dir = os.environ.get("HOME")
    if not home_dir:
        print("$HOME is None")
        exit(1)
    get_vm_images_location(home_dir)
    set_images_dir(home_dir)


if __name__ == "__main__":
    setup_dirs()
    start_server("0.0.0.0", 9999)
