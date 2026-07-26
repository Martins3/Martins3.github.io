// Linux analogy: sched_setaffinity for process/thread CPU affinity.
// Key difference: classic affinity masks are per processor group, so machines
// with more than 64 logical CPUs need group-aware APIs.

#include "win32_util.h"

#include <windows.h>

#include <iostream>

namespace {

int count_bits(DWORD_PTR mask) {
    int count = 0;
    while (mask != 0) {
        mask &= mask - 1;
        ++count;
    }
    return count;
}

void print_mask(const wchar_t* label, DWORD_PTR mask) {
    std::wcout << label << L" mask=0x" << std::hex << mask << std::dec
               << L" cpu_count=" << count_bits(mask) << L"\n";
}

DWORD_PTR lowest_cpu_mask(DWORD_PTR mask) {
    return mask & (~mask + 1);
}

DWORD WINAPI worker(void*) {
    DWORD cpu = GetCurrentProcessorNumber();
    std::wcout << L"worker started on logical cpu " << cpu << L"\n";
    Sleep(100);
    std::wcout << L"worker ended on logical cpu " << GetCurrentProcessorNumber() << L"\n";
    return 0;
}

} // namespace

int wmain() {
    std::wcout << L"Windows CPU affinity demo\n";

    DWORD_PTR process_mask = 0;
    DWORD_PTR system_mask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        std::wcerr << L"GetProcessAffinityMask failed: " << demo::win32_error_message();
        return 1;
    }

    print_mask(L"current process", process_mask);
    print_mask(L"system", system_mask);

    DWORD_PTR target_mask = lowest_cpu_mask(process_mask);
    if (target_mask == 0) {
        std::wcerr << L"process has no available CPU in its affinity mask\n";
        return 1;
    }

    std::wcout << L"pin current process to one available CPU\n";
    if (!SetProcessAffinityMask(GetCurrentProcess(), target_mask)) {
        std::wcerr << L"SetProcessAffinityMask failed: " << demo::win32_error_message();
        return 1;
    }

    DWORD_PTR changed_process_mask = 0;
    DWORD_PTR changed_system_mask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &changed_process_mask, &changed_system_mask)) {
        std::wcerr << L"GetProcessAffinityMask after set failed: " << demo::win32_error_message();
        return 1;
    }
    print_mask(L"changed process", changed_process_mask);

    demo::unique_handle thread(CreateThread(nullptr, 0, worker, nullptr, CREATE_SUSPENDED, nullptr));
    if (!thread) {
        std::wcerr << L"CreateThread failed: " << demo::win32_error_message();
        return 1;
    }

    DWORD_PTR previous_thread_mask = SetThreadAffinityMask(thread.get(), target_mask);
    if (previous_thread_mask == 0) {
        std::wcerr << L"SetThreadAffinityMask failed: " << demo::win32_error_message();
        return 1;
    }

    ResumeThread(thread.get());
    WaitForSingleObject(thread.get(), INFINITE);

    if (!SetProcessAffinityMask(GetCurrentProcess(), process_mask)) {
        std::wcerr << L"restore SetProcessAffinityMask failed: " << demo::win32_error_message();
        return 1;
    }
    std::wcout << L"restored original process affinity\n";
    return 0;
}
