#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>

namespace demo {

class winsock_session {
public:
    winsock_session() {
        int rc = WSAStartup(MAKEWORD(2, 2), &data_);
        if (rc != 0) {
            error_ = rc;
        }
    }

    winsock_session(const winsock_session&) = delete;
    winsock_session& operator=(const winsock_session&) = delete;

    ~winsock_session() {
        if (error_ == 0) {
            WSACleanup();
        }
    }

    bool ok() const {
        return error_ == 0;
    }

    int error() const {
        return error_;
    }

private:
    WSADATA data_{};
    int error_ = 0;
};

inline void close_socket_if_valid(SOCKET socket) {
    if (socket != INVALID_SOCKET) {
        closesocket(socket);
    }
}

} // namespace demo
