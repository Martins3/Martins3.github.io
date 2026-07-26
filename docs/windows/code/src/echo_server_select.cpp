// Linux analogy: select-based TCP echo server.
// Key difference: Winsock fd_set contains SOCKET handles and nfds is ignored.

#include "winsock_util.h"

#include <array>
#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* port = argc > 1 ? argv[1] : "8888";

    demo::winsock_session winsock;
    if (!winsock.ok()) {
        std::cerr << "WSAStartup failed: " << winsock.error() << "\n";
        return 1;
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;
    int rc = getaddrinfo(nullptr, port, &hints, &result);
    if (rc != 0) {
        std::cerr << "getaddrinfo failed: " << rc << "\n";
        return 1;
    }

    SOCKET listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        freeaddrinfo(result);
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    BOOL reuse = TRUE;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    rc = bind(listen_socket, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);
    if (rc == SOCKET_ERROR || listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "bind/listen failed: " << WSAGetLastError() << "\n";
        closesocket(listen_socket);
        return 1;
    }

    std::array<SOCKET, FD_SETSIZE> clients{};
    clients.fill(INVALID_SOCKET);
    std::cout << "select echo server listening on " << port << "\n";

    while (true) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(listen_socket, &read_set);
        for (SOCKET client : clients) {
            if (client != INVALID_SOCKET) {
                FD_SET(client, &read_set);
            }
        }

        rc = select(0, &read_set, nullptr, nullptr, nullptr);
        if (rc == SOCKET_ERROR) {
            std::cerr << "select failed: " << WSAGetLastError() << "\n";
            break;
        }

        if (FD_ISSET(listen_socket, &read_set)) {
            SOCKET client = accept(listen_socket, nullptr, nullptr);
            if (client != INVALID_SOCKET) {
                bool stored = false;
                for (SOCKET& slot : clients) {
                    if (slot == INVALID_SOCKET) {
                        slot = client;
                        stored = true;
                        break;
                    }
                }
                if (!stored) {
                    closesocket(client);
                }
            }
        }

        std::array<char, 1024> buffer{};
        for (SOCKET& client : clients) {
            if (client == INVALID_SOCKET || !FD_ISSET(client, &read_set)) {
                continue;
            }

            rc = recv(client, buffer.data(), static_cast<int>(buffer.size()), 0);
            if (rc <= 0) {
                closesocket(client);
                client = INVALID_SOCKET;
                continue;
            }
            send(client, buffer.data(), rc, 0);
        }
    }

    for (SOCKET client : clients) {
        demo::close_socket_if_valid(client);
    }
    closesocket(listen_socket);
    return 0;
}
