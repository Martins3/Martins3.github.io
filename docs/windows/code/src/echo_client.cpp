// Linux analogy: getaddrinfo/socket/connect/send/recv are intentionally close
// to BSD sockets. Windows still requires WSAStartup and closesocket.

#include "winsock_util.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* host = argc > 1 ? argv[1] : "127.0.0.1";
    const char* port = argc > 2 ? argv[2] : "8888";
    const char* message = argc > 3 ? argv[3] : "hello echo";

    demo::winsock_session winsock;
    if (!winsock.ok()) {
        std::cerr << "WSAStartup failed: " << winsock.error() << "\n";
        return 1;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    int rc = getaddrinfo(host, port, &hints, &result);
    if (rc != 0) {
        std::cerr << "getaddrinfo failed: " << rc << "\n";
        return 1;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    rc = connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);
    if (rc == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        return 1;
    }

    rc = send(sock, message, static_cast<int>(std::strlen(message)), 0);
    if (rc == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        return 1;
    }

    std::array<char, 1024> buffer{};
    rc = recv(sock, buffer.data(), static_cast<int>(buffer.size() - 1), 0);
    if (rc == SOCKET_ERROR) {
        std::cerr << "recv failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        return 1;
    }

    std::cout << "echo: " << buffer.data() << "\n";
    closesocket(sock);
    return 0;
}
