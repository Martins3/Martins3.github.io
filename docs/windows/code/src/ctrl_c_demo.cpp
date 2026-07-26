// Linux analogy: SIGINT from a terminal, but Windows delivers console control
// events to console processes instead of Unix signals to one specific pid.
// Key difference: a CTRL_C_EVENT is handled on a new system-created thread.
//
// 简单分析下，似乎是合理的，windows 为程序提供了一个 thread 来处理 ctrl c
// 之类的功能
//
//  \Users\97936\data\vn\docs\windows\code\build\bin\Debug\ctrl_c_demo.exe
//  Windows Ctrl+C demo
//  pid=46652, main_tid=43124
//  Press Ctrl+C in this console. In pass mode the default handler exits the process.
//  alive tick=1
//  alive tick=2
//  console_ctrl_handler backtrace:
//  #00 0x00007FF6CAF41BC2 `anonymous namespace'::print_current_backtrace+0x52 (C:\Users\97936\data\vn\docs\windows\code\src\ctrl_c_demo.cpp:46)
//  #01 0x00007FF6CAF41E69 `anonymous namespace'::console_ctrl_handler+0x19 (C:\Users\97936\data\vn\docs\windows\code\src\ctrl_c_demo.cpp:93)
//  #02 0x00007FFAE2023867 CtrlRoutine+0x127
//  #03 0x00007FFAE423259D BaseThreadInitThunk+0x1d
//  #04 0x00007FFAE4D4AF78 RtlUserThreadStart+0x28
//  handler saw CTRL_C_EVENT; returning FALSE so default handler exits the process

#include "win32_util.h"

#include <dbghelp.h>
#include <windows.h>

#include <cstdio>
#include <cwchar>
#include <cstring>
#include <iostream>

namespace {

enum class CtrlCMode {
    pass_to_default,
    graceful_exit,
    ignore
};

HANDLE g_stop_event = nullptr;
CtrlCMode g_mode = CtrlCMode::pass_to_default;

void write_console_line(const wchar_t* message) {
    DWORD written = 0;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleW(output, message, static_cast<DWORD>(std::wcslen(message)), &written, nullptr);
    WriteConsoleW(output, L"\n", 1, &written, nullptr);
}

void write_stdout(const char* message) {
    DWORD written = 0;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(output, message, static_cast<DWORD>(std::strlen(message)), &written, nullptr);
}

void write_stdout_line(const char* message) {
    write_stdout(message);
    write_stdout("\n");
}

void print_current_backtrace() {
    void* frames[32] = {};
    const USHORT frame_count = CaptureStackBackTrace(0, static_cast<DWORD>(std::size(frames)), frames, nullptr);

    write_stdout_line("console_ctrl_handler backtrace:");

    HANDLE process = GetCurrentProcess();
    char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    for (USHORT i = 0; i < frame_count; ++i) {
        DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
        DWORD64 symbol_displacement = 0;
        char line_buffer[1024] = {};

        if (SymFromAddr(process, address, &symbol_displacement, symbol)) {
            std::snprintf(line_buffer,
                          sizeof(line_buffer),
                          "  #%02hu 0x%p %s+0x%llx",
                          i,
                          frames[i],
                          symbol->Name,
                          static_cast<unsigned long long>(symbol_displacement));
        } else {
            std::snprintf(line_buffer, sizeof(line_buffer), "  #%02hu 0x%p <symbol unavailable>", i, frames[i]);
        }

        IMAGEHLP_LINE64 source_line = {};
        source_line.SizeOfStruct = sizeof(source_line);
        DWORD line_displacement = 0;
        if (SymGetLineFromAddr64(process, address, &line_displacement, &source_line)) {
            std::snprintf(line_buffer + std::strlen(line_buffer),
                          sizeof(line_buffer) - std::strlen(line_buffer),
                          " (%s:%lu)",
                          source_line.FileName,
                          source_line.LineNumber);
        }

        write_stdout_line(line_buffer);
    }
}

BOOL WINAPI console_ctrl_handler(DWORD event) {
    if (event != CTRL_C_EVENT) {
        return FALSE;
    }

    print_current_backtrace();

    switch (g_mode) {
    case CtrlCMode::pass_to_default:
        write_console_line(L"handler saw CTRL_C_EVENT; returning FALSE so default handler exits the process");
        return FALSE;
    case CtrlCMode::graceful_exit:
        write_console_line(L"handler saw CTRL_C_EVENT; returning TRUE and asking main thread to exit");
        SetEvent(g_stop_event);
        return TRUE;
    case CtrlCMode::ignore:
        write_console_line(L"handler saw CTRL_C_EVENT; returning TRUE so the process keeps running");
        return TRUE;
    }

    return FALSE;
}

void print_usage() {
    std::wcout << L"usage: ctrl_c_demo [pass|graceful|ignore]\n"
               << L"  pass     handler returns FALSE; default console handler terminates the process\n"
               << L"  graceful handler returns TRUE; main thread exits after seeing an event\n"
               << L"  ignore   handler returns TRUE; Ctrl+C is swallowed\n";
}

bool parse_mode(int argc, wchar_t* argv[]) {
    if (argc <= 1 || std::wcscmp(argv[1], L"pass") == 0) {
        g_mode = CtrlCMode::pass_to_default;
        return true;
    }
    if (std::wcscmp(argv[1], L"graceful") == 0) {
        g_mode = CtrlCMode::graceful_exit;
        return true;
    }
    if (std::wcscmp(argv[1], L"ignore") == 0) {
        g_mode = CtrlCMode::ignore;
        return true;
    }
    return false;
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (!parse_mode(argc, argv)) {
        print_usage();
        return 2;
    }

    demo::unique_handle stop_event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!stop_event) {
        std::wcerr << L"CreateEventW failed: " << demo::win32_error_message();
        return 1;
    }
    g_stop_event = stop_event.get();

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE)) {
        std::wcerr << L"SymInitialize failed: " << demo::win32_error_message();
        return 1;
    }

    if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE)) {
        std::wcerr << L"SetConsoleCtrlHandler failed: " << demo::win32_error_message();
        SymCleanup(GetCurrentProcess());
        return 1;
    }

    std::wcout << L"Windows Ctrl+C demo\n"
               << L"pid=" << GetCurrentProcessId()
               << L", main_tid=" << GetCurrentThreadId() << L"\n"
               << L"Press Ctrl+C in this console. In pass mode the default handler exits the process.\n";

    int tick = 0;
    while (WaitForSingleObject(stop_event.get(), 1000) == WAIT_TIMEOUT) {
        std::wcout << L"alive tick=" << ++tick << L"\n";
    }

    std::wcout << L"main thread observed stop event and exits normally\n";
    SymCleanup(GetCurrentProcess());
    return 0;
}
