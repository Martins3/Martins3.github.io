// Linux analogy: fork/exec/waitpid/getpid, but Windows creates a new process
// directly with CreateProcess and returns kernel handles for the process/thread.
// Key difference: there is no fork-style copy of the current process image.

#include "win32_util.h"

#include <tlhelp32.h>
#include <windows.h>

#include <iostream>

namespace {

void list_some_processes() {
    demo::unique_handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!snapshot) {
        std::wcerr << L"CreateToolhelp32Snapshot failed: " << demo::win32_error_message();
        return;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot.get(), &entry)) {
        std::wcerr << L"Process32FirstW failed: " << demo::win32_error_message();
        return;
    }

    std::wcout << L"\nFirst 12 processes from Toolhelp snapshot:\n";
    int shown = 0;
    do {
        std::wcout << L"  pid=" << entry.th32ProcessID << L" exe=" << entry.szExeFile << L"\n";
        ++shown;
    } while (shown < 12 && Process32NextW(snapshot.get(), &entry));
}

int run_child_process() {
    wchar_t command_line[] = L"cmd.exe /C exit 7";

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process_info{};

    BOOL ok = CreateProcessW(
        nullptr,
        command_line,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startup,
        &process_info);

    if (!ok) {
        std::wcerr << L"CreateProcessW failed: " << demo::win32_error_message();
        return 1;
    }

    demo::unique_handle process(process_info.hProcess);
    demo::unique_handle thread(process_info.hThread);

    std::wcout << L"\nCreated child pid=" << process_info.dwProcessId
               << L", primary_tid=" << process_info.dwThreadId << L"\n";

    DWORD wait_result = WaitForSingleObject(process.get(), INFINITE);
    if (wait_result != WAIT_OBJECT_0) {
        std::wcerr << L"WaitForSingleObject failed/result=" << wait_result << L"\n";
        return 1;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process.get(), &exit_code)) {
        std::wcerr << L"GetExitCodeProcess failed: " << demo::win32_error_message();
        return 1;
    }

    std::wcout << L"Child exited with code " << exit_code << L"\n";
    return exit_code == 7 ? 0 : 1;
}

} // namespace

int wmain() {
    std::wcout << L"Windows process demo\n";
    std::wcout << L"Current pid=" << GetCurrentProcessId()
               << L", current pseudo handle=" << GetCurrentProcess() << L"\n";

    list_some_processes();
    return run_child_process();
}
