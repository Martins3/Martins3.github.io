#!/usr/bin/env python3
import argparse
import json
import os
import socket
import uuid

host_ip = ""
host_port = 9999


def send_request(request):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        client_socket.connect((host_ip, host_port))
        client_socket.sendall(json.dumps(request).encode("utf-8"))
        response = client_socket.recv(1024).decode("utf-8")
        print(f"Response from Host B: {response}")
    except Exception as e:
        print(f"Error while sending request: {e}")
        exit(1)
    finally:
        client_socket.close()


def parse_args():
    parser = argparse.ArgumentParser(description="Client for Host B")
    parser.add_argument(
        "--operation",
        choices=["setup_nbd_disk", "migrate"],
        required=True,
        help="Operation type: setup, query or migrate",
    )
    parser.add_argument("--host", required=True, help="Host B IP address")
    parser.add_argument(
        "--disk", type=int, help="Disk index (required for setup_nbd_disk)"
    )
    parser.add_argument("--dir", help="vm dir")
    args = parser.parse_args()

    if args.operation == "setup_nbd_disk" or args.operation == "migrate":
        if args.dir is None:
            parser.error("migrate operation requires --dir")

    if args.operation == "setup_nbd_disk":
        if args.disk is None:
            parser.error("setup_nbd_disk operation requires --disk")

    return args


def extract_id(vm_dir):
    if not os.path.isabs(vm_dir):
        print("abs path needed")
        exit(1)

    if not os.path.isdir(vm_dir + "/" + "opt"):
        print("not a vm dir")
        exit(1)

    guest_uuid = ""
    guest_id = ""

    with open(f"{vm_dir}/opt/uuid", "r", encoding="utf-8") as f:
        guest_uuid = f.read().strip()

    with open(f"{vm_dir}/opt/id", "r", encoding="utf-8") as f:
        guest_id = f.read().strip()

    uuid.UUID(guest_uuid)
    int(guest_id)

    return guest_id, guest_uuid


def setup_nbd_disk_server(vm_dir, disk_idx):
    guest_id, guest_uuid = extract_id(vm_dir)
    execute_request = {
        "type": "setup_nbd_disk",
        "uuid": guest_uuid,
        "id": guest_id,
        "disk_idx": disk_idx,
    }

    print(f"Sending execute request: {execute_request}")
    send_request(execute_request)


def query_server_status():
    query_request = {"type": "query", "command": "count_servers"}

    print(f"\nSending query request: {query_request}")
    return send_request(query_request)


def migrate_vm(vm_dir):
    file_path = f"{vm_dir}/cmd.sh"
    config = files_to_json(f"{vm_dir}/opt")

    guest_id, guest_uuid = extract_id(vm_dir)

    with open(file_path, "rb") as f:
        file_data = f.read()
        file_size = len(file_data)

    migrate_request = {
        "type": "migrate",
        "id": guest_id,
        "uuid": guest_uuid,
        "config": config,
        "vm_name": os.path.basename(vm_dir),
        "file_size": file_size,
    }

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((host_ip, host_port))

        print(migrate_request)
        request_data = json.dumps(migrate_request).encode("utf-8")
        client_socket.sendall(request_data)

        # 为什么需要 recv 一下
        # TODO 这样直接发送会有问题吗?
        response = client_socket.recv(1024).decode("utf-8")
        if response != "ready":
            raise Exception(f"Server not ready: {response}")

        client_socket.sendall(file_data)

        response = client_socket.recv(1024).decode("utf-8")
        print(f"Response from Host B: {response}")
    except Exception as e:
        print(f"Error while migrating : {e}")
    finally:
        client_socket.close()


def files_to_json(folder_path):
    result = {}

    for filename in os.listdir(folder_path):
        if filename == "README.md":
            continue
        file_path = os.path.join(folder_path, filename)
        if os.path.isfile(file_path):
            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()
                result[filename] = content

    return result


if __name__ == "__main__":
    args = parse_args()
    host_ip = args.host

    if args.operation == "setup_nbd_disk":
        setup_nbd_disk_server(args.dir, args.disk)
    elif args.operation == "migrate":
        migrate_vm(args.dir)
    else:
        query_server_status()
