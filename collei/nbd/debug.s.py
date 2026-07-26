#!/usr/bin/env python3
import os
import socket
import threading
import time


def handle_client():
    for _ in range(0, 100):
        time.sleep(1)
        print(".")


def start_server(host, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)
    print(f"Server listening on {host}:{port}")

    try:
        while True:
            _, addr = server_socket.accept()
            print(f"Connection from {addr}")
            client_thread = threading.Thread(target=handle_client, args=())
            client_thread.daemon = True
            client_thread.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        # https://stackoverflow.com/questions/1489669/how-to-exit-the-entire-application-from-a-python-thread
        os._exit(0)


if __name__ == "__main__":
    start_server("0.0.0.0", 9998)
