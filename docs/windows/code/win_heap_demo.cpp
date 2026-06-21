#include <windows.h>
#include <iostream>

int main()
{
    // Get the process heap
    HANDLE hHeap = GetProcessHeap();
    if (hHeap == NULL)
    {
        std::cerr << "GetProcessHeap failed." << std::endl;
        return 1;
    }

    // Allocate memory from the process heap
    // HEAP_ZERO_MEMORY initializes the allocated memory to zero
    PVOID pMemory = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);
    if (pMemory == NULL)
    {
        std::cerr << "HeapAlloc failed." << std::endl;
        return 1;
    }

    std::cout << "Memory allocated successfully from process heap." << std::endl;
    std::cout << "Allocated size: " << HeapSize(hHeap, 0, pMemory) << " bytes." << std::endl;

    // Use the allocated memory (e.g., write some data)
    // For demonstration, we'll just print its address
    std::cout << "Allocated memory address: " << pMemory << std::endl;

    // Free the allocated memory
    if (!HeapFree(hHeap, 0, pMemory))
    {
        std::cerr << "HeapFree failed." << std::endl;
        return 1;
    }

    std::cout << "Memory freed successfully." << std::endl;

    // Create a private heap
    // HEAP_CREATE_ENABLE_EXECUTE allows execution from the heap (use with caution)
    // 0, 0 for initial and maximum size means the system determines the size
    HANDLE hPrivateHeap = HeapCreate(0, 0, 0);
    if (hPrivateHeap == NULL)
    {
        std::cerr << "HeapCreate failed." << std::endl;
        return 1;
    }

    std::cout << "Private heap created successfully." << std::endl;

    // Allocate memory from the private heap
    PVOID pPrivateMemory = HeapAlloc(hPrivateHeap, HEAP_ZERO_MEMORY, 512);
    if (pPrivateMemory == NULL)
    {
        std::cerr << "HeapAlloc from private heap failed." << std::endl;
        // Destroy the private heap even if allocation fails
        HeapDestroy(hPrivateHeap);
        return 1;
    }

    std::cout << "Memory allocated successfully from private heap." << std::endl;
    std::cout << "Allocated size from private heap: " << HeapSize(hPrivateHeap, 0, pPrivateMemory) << " bytes." << std::endl;
    std::cout << "Allocated private memory address: " << pPrivateMemory << std::endl;

    // Free memory from the private heap
    if (!HeapFree(hPrivateHeap, 0, pPrivateMemory))
    {
        std::cerr << "HeapFree from private heap failed." << std::endl;
        // Destroy the private heap even if freeing fails
        HeapDestroy(hPrivateHeap);
        return 1;
    }

    std::cout << "Memory freed successfully from private heap." << std::endl;

    // Destroy the private heap
    if (!HeapDestroy(hPrivateHeap))
    {
        std::cerr << "HeapDestroy failed." << std::endl;
        return 1;
    }

    std::cout << "Private heap destroyed successfully." << std::endl;

    return 0;
}
