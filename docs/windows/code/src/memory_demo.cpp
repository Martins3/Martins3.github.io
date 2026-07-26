// Linux analogy: malloc/free plus mmap/munmap for page-granular virtual memory.
// Key difference: Windows exposes Heap* and VirtualAlloc as separate common APIs.

#include "win32_util.h"

#include <windows.h>

#include <iostream>

int wmain() {
    HANDLE process_heap = GetProcessHeap();
    void* heap_block = HeapAlloc(process_heap, HEAP_ZERO_MEMORY, 1024);
    if (heap_block == nullptr) {
        std::wcerr << L"HeapAlloc failed: " << demo::win32_error_message();
        return 1;
    }
    std::wcout << L"process heap block=" << heap_block
               << L" size=" << HeapSize(process_heap, 0, heap_block) << L"\n";
    HeapFree(process_heap, 0, heap_block);

    HANDLE private_heap = HeapCreate(0, 4096, 0);
    if (private_heap == nullptr) {
        std::wcerr << L"HeapCreate failed: " << demo::win32_error_message();
        return 1;
    }

    void* private_block = HeapAlloc(private_heap, 0, 512);
    if (private_block == nullptr) {
        std::wcerr << L"private HeapAlloc failed: " << demo::win32_error_message();
        HeapDestroy(private_heap);
        return 1;
    }
    std::wcout << L"private heap block=" << private_block << L"\n";
    HeapFree(private_heap, 0, private_block);
    HeapDestroy(private_heap);

    void* pages = VirtualAlloc(nullptr, 64 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (pages == nullptr) {
        std::wcerr << L"VirtualAlloc failed: " << demo::win32_error_message();
        return 1;
    }
    std::wcout << L"virtual pages=" << pages << L"\n";
    VirtualFree(pages, 0, MEM_RELEASE);

    return 0;
}
