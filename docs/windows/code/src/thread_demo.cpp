// Linux analogy: pthread_create/pthread_join and wait primitives.
// Key difference: Windows waits on kernel object handles, so a thread handle is
// also a waitable object.

#include "win32_util.h"

#include <windows.h>

#include <array>
#include <iostream>

namespace {

DWORD WINAPI worker(void* param) {
    int index = *static_cast<int*>(param);
    Sleep(20 * index);
    std::wcout << L"worker " << index << L" ran on tid=" << GetCurrentThreadId() << L"\n";
    return static_cast<DWORD>(100 + index);
}

} // namespace

int wmain() {
    std::wcout << L"Windows thread/wait demo\n";

    std::array<int, 3> indexes{1, 2, 3};
    std::array<demo::unique_handle, 3> threads;
    std::array<HANDLE, 3> raw_handles{};

    for (size_t i = 0; i < threads.size(); ++i) {
        DWORD tid = 0;
        HANDLE handle = CreateThread(nullptr, 0, worker, &indexes[i], 0, &tid);
        if (handle == nullptr) {
            std::wcerr << L"CreateThread failed: " << demo::win32_error_message();
            return 1;
        }
        threads[i].reset(handle);
        raw_handles[i] = handle;
        std::wcout << L"created tid=" << tid << L"\n";
    }

    DWORD wait_result = WaitForMultipleObjects(
        static_cast<DWORD>(raw_handles.size()),
        raw_handles.data(),
        TRUE,
        INFINITE);

    if (wait_result != WAIT_OBJECT_0) {
        std::wcerr << L"WaitForMultipleObjects failed/result=" << wait_result << L"\n";
        return 1;
    }

    for (const auto& thread : threads) {
        DWORD exit_code = 0;
        GetExitCodeThread(thread.get(), &exit_code);
        std::wcout << L"thread exit code=" << exit_code << L"\n";
    }

    return 0;
}
