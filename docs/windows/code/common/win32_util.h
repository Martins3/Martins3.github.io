#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <string>
#include <utility>

namespace demo {

class unique_handle {
public:
    unique_handle() = default;
    explicit unique_handle(HANDLE handle) : handle_(handle) {}

    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    unique_handle(unique_handle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

    unique_handle& operator=(unique_handle&& other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.handle_, nullptr));
        }
        return *this;
    }

    ~unique_handle() {
        reset();
    }

    HANDLE get() const {
        return handle_;
    }

    HANDLE* put() {
        reset();
        return &handle_;
    }

    HANDLE release() {
        return std::exchange(handle_, nullptr);
    }

    void reset(HANDLE handle = nullptr) {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
        handle_ = handle;
    }

    explicit operator bool() const {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE handle_ = nullptr;
};

inline std::wstring win32_error_message(DWORD error = GetLastError()) {
    wchar_t* buffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::wstring message = size == 0 ? L"(FormatMessageW failed)" : std::wstring(buffer, size);
    if (buffer != nullptr) {
        LocalFree(buffer);
    }
    return message;
}

} // namespace demo
