#ifndef UNICODE
#define UNICODE
#endif
#define _WIN32_WINNT 0x0600 // For Windows Vista and later
#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <winbase.h>
#include <winnls.h>

// Function to list running processes
void ListProcesses()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        std::cerr << "Process32First failed: " << GetLastError() << std::endl;
        CloseHandle(hSnapshot);
        return;
    }

    std::cout << "\n--- Running Processes ---\n";
    do
    {
        std::wcout << L"Process Name: " << pe32.szExeFile << L", PID: " << pe32.th32ProcessID << std::endl;
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
}

int main()
{
    std::cout << "Windows Process Management Demo" << std::endl;

    // Example 1: Get current process ID
    DWORD currentPid = GetCurrentProcessId();
    std::cout << "Current Process ID: " << currentPid << std::endl;

    // Example 2: Open current process handle
    HANDLE hCurrentProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, currentPid);
    if (hCurrentProcess == NULL)
    {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "Successfully opened handle to current process." << std::endl;
        CloseHandle(hCurrentProcess);
    }

    // Example 3: List all running processes
    ListProcesses();

    // Example 4: Create a simple child process (e.g., notepad.exe)
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Note: For simplicity, we're launching notepad. In a real scenario, you might want to specify full path.
    // Also, CreateProcess requires a writable string for the command line.
    wchar_t cmdLine[] = L"notepad.exe";

    std::cout << "\nAttempting to create child process (notepad.exe)..." << std::endl;
    if (!CreateProcessW(
        NULL,           // No module name (use command line)
        cmdLine,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi             // Pointer to PROCESS_INFORMATION structure
    ))
    {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
    }
    else
    {
        std::cout << "Child process (notepad.exe) created successfully. PID: " << pi.dwProcessId << std::endl;
        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, 5000); // Wait for 5 seconds
        std::cout << "Child process waited for (or timed out)." << std::endl;

        // Close process and thread handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return 0;
}
