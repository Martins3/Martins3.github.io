// Linux analogy: mmap MAP_SHARED on a file.
// Key difference: Windows separates the file handle, mapping object, and view.

#include "win32_util.h"

#include <windows.h>

#include <cstring>
#include <iostream>

int wmain() {
    constexpr wchar_t kPath[] = L"mapping_demo.tmp";
    constexpr char kText[] = "mapped bytes";

    demo::unique_handle file(CreateFileW(
        kPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr));
    if (!file) {
        std::wcerr << L"CreateFileW failed: " << demo::win32_error_message();
        return 1;
    }

    LARGE_INTEGER size{};
    size.QuadPart = 4096;
    SetFilePointerEx(file.get(), size, nullptr, FILE_BEGIN);
    SetEndOfFile(file.get());

    demo::unique_handle mapping(CreateFileMappingW(file.get(), nullptr, PAGE_READWRITE, 0, 4096, nullptr));
    if (!mapping) {
        std::wcerr << L"CreateFileMappingW failed: " << demo::win32_error_message();
        return 1;
    }

    void* view = MapViewOfFile(mapping.get(), FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, 4096);
    if (view == nullptr) {
        std::wcerr << L"MapViewOfFile failed: " << demo::win32_error_message();
        return 1;
    }

    std::memcpy(view, kText, sizeof(kText));
    FlushViewOfFile(view, sizeof(kText));
    std::cout << "view content: " << static_cast<char*>(view) << "\n";

    UnmapViewOfFile(view);
    file.reset();
    DeleteFileW(kPath);
    return 0;
}
