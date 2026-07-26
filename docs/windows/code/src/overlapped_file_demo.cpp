// Linux analogy: aio/io_uring in spirit, not in interface.
// Key difference: OVERLAPPED carries the file offset and completion state.

#include "win32_util.h"

#include <windows.h>

#include <array>
#include <iostream>

int wmain() {
    constexpr wchar_t kPath[] = L"overlapped_demo.tmp";
    constexpr char kText[] = "overlapped write\n";

    demo::unique_handle file(CreateFileW(
        kPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr));
    if (!file) {
        std::wcerr << L"CreateFileW failed: " << demo::win32_error_message();
        return 1;
    }

    demo::unique_handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    OVERLAPPED write_ov{};
    write_ov.hEvent = event.get();

    DWORD ignored = 0;
    BOOL ok = WriteFile(file.get(), kText, sizeof(kText) - 1, &ignored, &write_ov);
    if (!ok && GetLastError() != ERROR_IO_PENDING) {
        std::wcerr << L"WriteFile overlapped failed: " << demo::win32_error_message();
        return 1;
    }

    DWORD transferred = 0;
    if (!GetOverlappedResult(file.get(), &write_ov, &transferred, TRUE)) {
        std::wcerr << L"GetOverlappedResult(write) failed: " << demo::win32_error_message();
        return 1;
    }
    std::wcout << L"async write bytes=" << transferred << L"\n";

    std::array<char, 64> buffer{};
    ResetEvent(event.get());
    OVERLAPPED read_ov{};
    read_ov.hEvent = event.get();
    ok = ReadFile(file.get(), buffer.data(), static_cast<DWORD>(buffer.size() - 1), &ignored, &read_ov);
    if (!ok && GetLastError() != ERROR_IO_PENDING) {
        std::wcerr << L"ReadFile overlapped failed: " << demo::win32_error_message();
        return 1;
    }

    if (!GetOverlappedResult(file.get(), &read_ov, &transferred, TRUE)) {
        std::wcerr << L"GetOverlappedResult(read) failed: " << demo::win32_error_message();
        return 1;
    }
    std::cout << "async read: " << buffer.data();

    file.reset();
    DeleteFileW(kPath);
    return 0;
}
