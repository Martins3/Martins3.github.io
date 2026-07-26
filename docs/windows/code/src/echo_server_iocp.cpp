// Linux analogy: epoll/io_uring are the closest mental neighbors, but IOCP is a
// completion queue: workers consume completed operations, not readiness events.

#include "winsock_util.h"
#include "win32_util.h"

#include <process.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

constexpr unsigned short kDefaultPort = 8888;
constexpr size_t kBufferSize = 4096;

enum class Operation {
    Recv,
    Send,
};

struct IoOperation {
    WSAOVERLAPPED overlapped{};
    WSABUF wsabuf{};
    SOCKET socket = INVALID_SOCKET;
    Operation operation = Operation::Recv;
    DWORD flags = 0;
    std::array<char, kBufferSize + 1> buffer{};

    IoOperation(SOCKET s, Operation op) : socket(s), operation(op) {
        wsabuf.buf = buffer.data();
        wsabuf.len = static_cast<ULONG>(kBufferSize);
    }
};

std::atomic<bool> g_stopping = false;
HANDLE g_iocp = nullptr;
SOCKET g_listen_socket = INVALID_SOCKET;

bool post_recv(SOCKET socket) {
    auto* op = new IoOperation(socket, Operation::Recv);
    DWORD bytes = 0;
    int rc = WSARecv(socket, &op->wsabuf, 1, &bytes, &op->flags, &op->overlapped, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cerr << "WSARecv failed: " << WSAGetLastError() << "\n";
        delete op;
        return false;
    }
    return true;
}

bool post_send(SOCKET socket, const char* data, DWORD length) {
    auto* op = new IoOperation(socket, Operation::Send);
    std::memcpy(op->buffer.data(), data, length);
    op->wsabuf.len = length;

    DWORD bytes = 0;
    int rc = WSASend(socket, &op->wsabuf, 1, &bytes, 0, &op->overlapped, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cerr << "WSASend failed: " << WSAGetLastError() << "\n";
        delete op;
        return false;
    }
    return true;
}

unsigned __stdcall worker_thread(void* param) {
    HANDLE iocp = static_cast<HANDLE>(param);

    while (true) {
        DWORD transferred = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* overlapped = nullptr;
        BOOL ok = GetQueuedCompletionStatus(iocp, &transferred, &key, &overlapped, INFINITE);

        if (overlapped == nullptr) {
            if (g_stopping) {
                break;
            }
            continue;
        }

        auto* op = CONTAINING_RECORD(overlapped, IoOperation, overlapped);
        SOCKET socket = static_cast<SOCKET>(key);

        if (!ok || transferred == 0) {
            closesocket(socket);
            delete op;
            continue;
        }

        if (op->operation == Operation::Recv) {
            DWORD echo_len = std::min<DWORD>(transferred, static_cast<DWORD>(kBufferSize));
            op->buffer[echo_len] = '\0';
            std::cout << "recv " << echo_len << " bytes: " << op->buffer.data() << "\n";
            bool sent = post_send(socket, op->buffer.data(), echo_len);
            delete op;
            if (!sent) {
                closesocket(socket);
            }
        } else {
            delete op;
            if (!post_recv(socket)) {
                closesocket(socket);
            }
        }
    }

    return 0;
}

unsigned __stdcall accept_thread(void*) {
    while (!g_stopping) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
        SOCKET client = accept(g_listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client == INVALID_SOCKET) {
            if (!g_stopping) {
                std::cerr << "accept failed: " << WSAGetLastError() << "\n";
            }
            continue;
        }

        if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(client), g_iocp, static_cast<ULONG_PTR>(client), 0) == nullptr) {
            std::cerr << "CreateIoCompletionPort(associate) failed: " << GetLastError() << "\n";
            closesocket(client);
            continue;
        }

        char ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        std::cout << "accepted " << ip << ":" << ntohs(client_addr.sin_port) << "\n";

        if (!post_recv(client)) {
            closesocket(client);
        }
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    unsigned short port = argc > 1 ? static_cast<unsigned short>(std::atoi(argv[1])) : kDefaultPort;

    demo::winsock_session winsock;
    if (!winsock.ok()) {
        std::cerr << "WSAStartup failed: " << winsock.error() << "\n";
        return 1;
    }

    g_listen_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (g_listen_socket == INVALID_SOCKET) {
        std::cerr << "WSASocketW failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(g_listen_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR ||
        listen(g_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "bind/listen failed: " << WSAGetLastError() << "\n";
        closesocket(g_listen_socket);
        return 1;
    }

    g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (g_iocp == nullptr) {
        std::wcerr << L"CreateIoCompletionPort failed: " << demo::win32_error_message();
        closesocket(g_listen_socket);
        return 1;
    }

    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    DWORD worker_count = std::max<DWORD>(1, std::min<DWORD>(info.dwNumberOfProcessors * 2, 4));
    std::array<HANDLE, 4> workers{};
    for (DWORD i = 0; i < worker_count; ++i) {
        workers[i] = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, worker_thread, g_iocp, 0, nullptr));
    }

    HANDLE acceptor = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, accept_thread, nullptr, 0, nullptr));
    std::cout << "IOCP echo server listening on " << port << " with " << worker_count << " workers\n";
    std::cout << "Press Enter to stop\n";
    std::cin.get();

    g_stopping = true;
    closesocket(g_listen_socket);
    WaitForSingleObject(acceptor, 1000);
    CloseHandle(acceptor);

    for (DWORD i = 0; i < worker_count; ++i) {
        PostQueuedCompletionStatus(g_iocp, 0, 0, nullptr);
    }
    WaitForMultipleObjects(worker_count, workers.data(), TRUE, 1000);
    for (DWORD i = 0; i < worker_count; ++i) {
        CloseHandle(workers[i]);
    }
    CloseHandle(g_iocp);
    return 0;
}
