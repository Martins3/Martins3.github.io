#!/usr/bin/env python3
import json
import socket


def send_request(request):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    client_socket.connect(("127.0.0.1", 9998))
    client_socket.sendall(json.dumps(request).encode("utf-8"))
    response = client_socket.recv(1024).decode("utf-8")
    print(f"Response from Host B: {response}")


if __name__ == "__main__":
    request = {"a": "b"}
    send_request(request)
