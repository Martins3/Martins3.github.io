// Linux analogy: sem_open/sem_wait/sem_post with a POSIX named semaphore.
// Key difference: Windows named semaphores are named kernel objects. There is
// no sem_unlink equivalent; the object disappears after the last handle closes.

#include "win32_util.h"

#include <windows.h>

#include <cwchar>
#include <iostream>

namespace {

constexpr const wchar_t* kSemaphoreName = L"Local\\vn_demo_named_semaphore";

void usage(const wchar_t* prog) {
    std::wcerr << L"Usage:\n"
               << L"  " << prog << L" wait\n"
               << L"  " << prog << L" post\n"
               << L"  " << prog << L" create\n\n"
               << L"Run one terminal with `named_semaphore_demo wait`, then another with `named_semaphore_demo post`.\n";
}

demo::unique_handle create_or_open_semaphore(LONG initial_count) {
    SetLastError(ERROR_SUCCESS);
    demo::unique_handle semaphore(CreateSemaphoreW(
        nullptr,
        initial_count,
        1,
        kSemaphoreName));
    if (!semaphore) {
        std::wcerr << L"CreateSemaphoreW failed: " << demo::win32_error_message();
    }
    return semaphore;
}

demo::unique_handle open_existing_semaphore() {
    demo::unique_handle semaphore(OpenSemaphoreW(
        SYNCHRONIZE | SEMAPHORE_MODIFY_STATE,
        FALSE,
        kSemaphoreName));
    if (!semaphore) {
        std::wcerr << L"OpenSemaphoreW failed: " << demo::win32_error_message()
                   << L"Run `named_semaphore_demo create` or `named_semaphore_demo wait` first.\n";
    }
    return semaphore;
}

int wait_mode() {
    demo::unique_handle semaphore = create_or_open_semaphore(0);
    if (!semaphore) {
        return 1;
    }

    std::wcout << L"waiting on named semaphore " << kSemaphoreName << L"\n";
    DWORD result = WaitForSingleObject(semaphore.get(), INFINITE);
    if (result != WAIT_OBJECT_0) {
        std::wcerr << L"WaitForSingleObject failed/result=" << result << L"\n";
        return 1;
    }

    std::wcout << L"acquired semaphore\n";
    return 0;
}

int post_mode() {
    demo::unique_handle semaphore = open_existing_semaphore();
    if (!semaphore) {
        return 1;
    }

    LONG previous_count = 0;
    if (!ReleaseSemaphore(semaphore.get(), 1, &previous_count)) {
        std::wcerr << L"ReleaseSemaphore failed: " << demo::win32_error_message();
        return 1;
    }

    std::wcout << L"posted semaphore, previous_count=" << previous_count << L"\n";
    return 0;
}

int create_mode() {
    demo::unique_handle semaphore = create_or_open_semaphore(0);
    if (!semaphore) {
        return 1;
    }

    DWORD last_error = GetLastError();
    std::wcout << L"named semaphore " << kSemaphoreName
               << (last_error == ERROR_ALREADY_EXISTS ? L" already existed\n" : L" created\n");
    return 0;
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    if (std::wcscmp(argv[1], L"wait") == 0) {
        return wait_mode();
    }
    if (std::wcscmp(argv[1], L"post") == 0) {
        return post_mode();
    }
    if (std::wcscmp(argv[1], L"create") == 0) {
        return create_mode();
    }

    usage(argv[0]);
    return 1;
}
