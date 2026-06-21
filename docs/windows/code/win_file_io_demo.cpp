#include <windows.h>
#include <iostream>
#include <string>

int main()
{
    std::cout << "Windows File I/O Demo" << std::endl;

    const std::string fileName = "example.txt";
    const std::string fileContent = "Hello, Windows File I/O!\nThis is a test file.";

    // --- Create and Write to a file ---
    std::cout << "\n--- Creating and Writing to file: " << fileName << " ---" << std::endl;
    HANDLE hFile = CreateFileA(
        fileName.c_str(),          // file name
        GENERIC_WRITE,             // open for writing
        0,                         // do not share
        NULL,                      // default security
        CREATE_ALWAYS,             // overwrite existing, create new if not exists
        FILE_ATTRIBUTE_NORMAL,     // normal file
        NULL);                     // no template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateFileA failed: " << GetLastError() << std::endl;
        return 1;
    }

    DWORD bytesWritten;
    if (!WriteFile(
        hFile,                     // file handle
        fileContent.c_str(),       // start of data to write
        static_cast<DWORD>(fileContent.length()), // number of bytes to write
        &bytesWritten,             // number of bytes that were written
        NULL))                     // no overlapped structure
    {
        std::cerr << "WriteFile failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 1;
    }

    std::cout << "Successfully wrote " << bytesWritten << " bytes to " << fileName << std::endl;
    CloseHandle(hFile);

    // --- Read from the file ---
    std::cout << "\n--- Reading from file: " << fileName << " ---" << std::endl;
    hFile = CreateFileA(
        fileName.c_str(),          // file name
        GENERIC_READ,              // open for reading
        FILE_SHARE_READ,           // share for reading
        NULL,                      // default security
        OPEN_EXISTING,             // open existing file
        FILE_ATTRIBUTE_NORMAL,     // normal file
        NULL);                     // no template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateFileA failed for reading: " << GetLastError() << std::endl;
        return 1;
    }

    char buffer[256];
    DWORD bytesRead;
    if (!ReadFile(
        hFile,                     // file handle
        buffer,                    // start of data to read into
        sizeof(buffer) - 1,        // number of bytes to read
        &bytesRead,                // number of bytes that were read
        NULL))                     // no overlapped structure
    {
        std::cerr << "ReadFile failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 1;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    std::cout << "Successfully read " << bytesRead << " bytes from " << fileName << std::endl;
    std::cout << "Content: \n" << buffer << std::endl;
    CloseHandle(hFile);

    // --- Delete the file ---
    std::cout << "\n--- Deleting file: " << fileName << " ---" << std::endl;
    if (!DeleteFileA(fileName.c_str()))
    {
        std::cerr << "DeleteFileA failed: " << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "Successfully deleted " << fileName << std::endl;

    return 0;
}