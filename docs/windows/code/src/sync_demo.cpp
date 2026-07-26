// Linux analogy: pthread_mutex, pthread_cond, sem_t, eventfd.
// Key difference: many Windows synchronization primitives are waitable handles
// and can be combined with WaitForMultipleObjects.

#include "win32_util.h"

#include <windows.h>

#include <iostream>
#include <thread>
#include <vector>

namespace {

CRITICAL_SECTION g_cs;
SRWLOCK g_srw = SRWLOCK_INIT;
CONDITION_VARIABLE g_cv = CONDITION_VARIABLE_INIT;
int g_counter = 0;
bool g_ready = false;

void critical_section_worker() {
    EnterCriticalSection(&g_cs);
    ++g_counter;
    LeaveCriticalSection(&g_cs);
}

void condition_waiter() {
    AcquireSRWLockExclusive(&g_srw);
    while (!g_ready) {
        SleepConditionVariableSRW(&g_cv, &g_srw, INFINITE, 0);
    }
    std::wcout << L"condition variable woke\n";
    ReleaseSRWLockExclusive(&g_srw);
}

DWORD WINAPI event_waiter(void* param) {
    HANDLE event = static_cast<HANDLE>(param);
    WaitForSingleObject(event, INFINITE);
    std::wcout << L"manual-reset event signaled\n";
    return 0;
}

DWORD WINAPI semaphore_waiter(void* param) {
    HANDLE semaphore = static_cast<HANDLE>(param);
    WaitForSingleObject(semaphore, INFINITE);
    std::wcout << L"semaphore acquired\n";
    return 0;
}

} // namespace

int wmain() {
    InitializeCriticalSection(&g_cs);
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(critical_section_worker);
    }
    for (auto& thread : threads) {
        thread.join();
    }
    DeleteCriticalSection(&g_cs);
    std::wcout << L"critical section counter=" << g_counter << L"\n";

    std::thread waiter(condition_waiter);
    Sleep(50);
    AcquireSRWLockExclusive(&g_srw);
    g_ready = true;
    WakeConditionVariable(&g_cv);
    ReleaseSRWLockExclusive(&g_srw);
    waiter.join();

    demo::unique_handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    demo::unique_handle event_thread(CreateThread(nullptr, 0, event_waiter, event.get(), 0, nullptr));
    SetEvent(event.get());
    WaitForSingleObject(event_thread.get(), INFINITE);

    demo::unique_handle semaphore(CreateSemaphoreW(nullptr, 0, 1, nullptr));
    demo::unique_handle sem_thread(CreateThread(nullptr, 0, semaphore_waiter, semaphore.get(), 0, nullptr));
    ReleaseSemaphore(semaphore.get(), 1, nullptr);
    WaitForSingleObject(sem_thread.get(), INFINITE);

    return 0;
}
