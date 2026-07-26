// Linux analogy: FIFO or Unix domain socket for local IPC.
// Key difference: Windows named pipes live in the NT object namespace and can be
// message- or byte-oriented.

#include "win32_util.h"

#include <windows.h>

#include <array>
#include <iostream>
#include <thread>

namespace {

constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\windows_api_demo_pipe";

void client_thread() {
    Sleep(50);
    demo::unique_handle pipe(CreateFileW(
        kPipeName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));
    if (!pipe) {
        std::wcerr << L"pipe client CreateFileW failed: " << demo::win32_error_message();
        return;
    }

    constexpr char message[] = "hello pipe";
    DWORD written = 0;
    WriteFile(pipe.get(), message, sizeof(message), &written, nullptr);
}

} // namespace

int wmain() {
    demo::unique_handle server(CreateNamedPipeW(
        kPipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        nullptr));
    if (!server) {
        std::wcerr << L"CreateNamedPipeW failed: " << demo::win32_error_message();
        return 1;
    }

    std::thread client(client_thread);
    BOOL connected = ConnectNamedPipe(server.get(), nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected) {
        std::wcerr << L"ConnectNamedPipe failed: " << demo::win32_error_message();
        client.join();
        return 1;
    }

    std::array<char, 128> buffer{};
    DWORD read = 0;
    ReadFile(server.get(), buffer.data(), static_cast<DWORD>(buffer.size() - 1), &read, nullptr);
    std::cout << "server read: " << buffer.data() << "\n";

    DisconnectNamedPipe(server.get());
    client.join();
    return 0;
}
