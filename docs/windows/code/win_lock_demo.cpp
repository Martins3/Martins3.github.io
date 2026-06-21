#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>

// Example using a Critical Section
CRITICAL_SECTION g_criticalSection;
int g_sharedResource = 0;

void CriticalSectionThreadFunc()
{
    EnterCriticalSection(&g_criticalSection);
    // Critical section: only one thread can access this at a time
    g_sharedResource++;
    std::cout << "Critical Section Thread: g_sharedResource = " << g_sharedResource << std::endl;
    LeaveCriticalSection(&g_criticalSection);
}

// Example using a Mutex
HANDLE g_hMutex;
int g_sharedResourceMutex = 0;

DWORD WINAPI MutexThreadFunc(LPVOID lpParam)
{
    // Wait for ownership of the mutex object
    DWORD dwWaitResult = WaitForSingleObject(
        g_hMutex,    // handle to mutex
        INFINITE);   // no time-out interval

    switch (dwWaitResult)
    {
        // The thread got ownership of the mutex
        case WAIT_OBJECT_0:
            {
                // RAII for mutex release
                struct MutexLocker {
                    HANDLE hMutex;
                    MutexLocker(HANDLE hM) : hMutex(hM) {}
                    ~MutexLocker() {
                        if (!ReleaseMutex(hMutex))
                        {
                            std::cerr << "ReleaseMutex error: " << GetLastError() << std::endl;
                        }
                    }
                } locker(g_hMutex);

                // Critical section: only one thread can access this at a time
                g_sharedResourceMutex++;
                std::cout << "Mutex Thread: g_sharedResourceMutex = " << g_sharedResourceMutex << std::endl;
            }
            break;

        // Cannot get mutex ownership due to time-out
        case WAIT_TIMEOUT:
            std::cerr << "Mutex wait timed out." << std::endl;
            break;

        default:
            std::cerr << "Mutex wait failed: " << GetLastError() << std::endl;
            break;
    }
    return 0;
}

int main()
{
    std::cout << "Windows Lock Mechanisms Demo" << std::endl;

    // --- Critical Section Demo ---
    std::cout << "\n--- Critical Section Demo ---" << std::endl;
    InitializeCriticalSection(&g_criticalSection);

    std::vector<std::thread> csThreads;
    for (int i = 0; i < 5; ++i)
    {
        csThreads.emplace_back(CriticalSectionThreadFunc);
    }

    for (auto& t : csThreads)
    {
        t.join();
    }

    DeleteCriticalSection(&g_criticalSection);
    std::cout << "Final g_sharedResource (Critical Section): " << g_sharedResource << std::endl;

    // --- Mutex Demo ---
    std::cout << "\n--- Mutex Demo ---" << std::endl;
    // Create a mutex with no initial owner
    g_hMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

    if (g_hMutex == NULL)
    {
        std::cerr << "CreateMutex error: " << GetLastError() << std::endl;
        return 1;
    }

    std::vector<HANDLE> mutexThreads;
    for (int i = 0; i < 5; ++i)
    {
        HANDLE hThread = CreateThread(
            NULL,                   // default security attributes
            0,                      // default stack size
            MutexThreadFunc,        // thread function
            NULL,                   // argument to thread function
            0,                      // default creation flags
            NULL);                  // receive thread identifier

        if (hThread == NULL)
        {
            std::cerr << "CreateThread error: " << GetLastError() << std::endl;
            CloseHandle(g_hMutex);
            return 1;
        }
        mutexThreads.push_back(hThread);
    }

    // Wait for all threads to terminate
    WaitForMultipleObjects(mutexThreads.size(), mutexThreads.data(), TRUE, INFINITE);

    for (HANDLE hThread : mutexThreads)
    {
        CloseHandle(hThread);
    }

    CloseHandle(g_hMutex);
    std::cout << "Final g_sharedResourceMutex (Mutex): " << g_sharedResourceMutex << std::endl;

    return 0;
}