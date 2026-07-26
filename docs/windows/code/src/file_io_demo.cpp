// Linux analogy: open/read/write/lseek/stat/unlink.
// Key difference: CreateFile opens many kernel object types, not only files.

#include "win32_util.h"

#include <windows.h>

#include <array>
#include <iostream>

int wmain() {
    constexpr wchar_t kPath[] = L"file_io_demo.tmp";
    constexpr char kContent[] = "hello from CreateFile/WriteFile\n";

    demo::unique_handle file(CreateFileW(
        kPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));

    if (!file) {
        std::wcerr << L"CreateFileW failed: " << demo::win32_error_message();
        return 1;
    }

    DWORD written = 0;
    if (!WriteFile(file.get(), kContent, sizeof(kContent) - 1, &written, nullptr)) {
        std::wcerr << L"WriteFile failed: " << demo::win32_error_message();
        return 1;
    }

    LARGE_INTEGER offset{};
    if (!SetFilePointerEx(file.get(), offset, nullptr, FILE_BEGIN)) {
        std::wcerr << L"SetFilePointerEx failed: " << demo::win32_error_message();
        return 1;
    }

    std::array<char, 128> buffer{};
    DWORD read = 0;
    if (!ReadFile(file.get(), buffer.data(), static_cast<DWORD>(buffer.size() - 1), &read, nullptr)) {
        std::wcerr << L"ReadFile failed: " << demo::win32_error_message();
        return 1;
    }

    LARGE_INTEGER size{};
    GetFileSizeEx(file.get(), &size);
    std::wcout << L"wrote=" << written << L" read=" << read << L" size=" << size.QuadPart << L"\n";
    std::cout << buffer.data();

    file.reset();
    if (!DeleteFileW(kPath)) {
        std::wcerr << L"DeleteFileW failed: " << demo::win32_error_message();
        return 1;
    }

    return 0;
}
