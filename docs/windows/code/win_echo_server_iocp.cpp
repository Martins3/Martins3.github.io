/**
 * Windows Echo Server using I/O Completion Port (IOCP)
 * 
 * This demo shows how to implement a server that can handle multiple client
 * connections simultaneously using the IOCP for I/O multiplexing.
 * IOCP provides better scalability than select() for handling many concurrent connections.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 8888
#define BUFFER_SIZE 4096
#define MAX_WORKER_THREADS 4

// Structure for per-connection information
typedef struct {
    SOCKET socket;
    SOCKADDR_IN clientAddr;
    CHAR buffer[BUFFER_SIZE];
    WSAOVERLAPPED overlapped;
    WSABUF dataBuf;
    DWORD bytesReceived;
    DWORD bytesSent;
    DWORD flags;
} PER_IO_DATA, *LPPER_IO_DATA;

// Global variables
HANDLE g_hIOCP = NULL;
SOCKET g_listenSocket = INVALID_SOCKET;

// Function to display error message and clean up Winsock
void ErrorCleanup(const char* message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    if (g_listenSocket != INVALID_SOCKET) {
        closesocket(g_listenSocket);
    }
    WSACleanup();
    exit(1);
}

// Worker thread function to process I/O completion packets
unsigned int WINAPI WorkerThread(LPVOID lpParam) {
    HANDLE hIOCP = (HANDLE)lpParam;
    DWORD bytesTransferred;
    LPPER_IO_DATA perIoData;
    LPOVERLAPPED overlapped;
    SOCKET socket;
    DWORD flags;
    BOOL result;
    char clientIP[INET_ADDRSTRLEN];

    while (true) {
        // Get a completed I/O request from the completion port
        result = GetQueuedCompletionStatus(hIOCP, &bytesTransferred, (PULONG_PTR)&socket, &overlapped, INFINITE);
        
        // Get the per-IO data structure
        perIoData = (LPPER_IO_DATA)CONTAINING_RECORD(overlapped, PER_IO_DATA, overlapped);

        // Check for errors or connection closed
        if (!result || bytesTransferred == 0) {
            inet_ntop(AF_INET, &(perIoData->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            printf("Client disconnected. IP: %s, Socket: %d\n", clientIP, (int)socket);
            closesocket(socket);
            free(perIoData);
            continue;
        }

        // Successfully received data
        perIoData->buffer[bytesTransferred] = '\0';
        inet_ntop(AF_INET, &(perIoData->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        printf("Received from client %s: %s\n", clientIP, perIoData->buffer);

        // Echo the data back to the client
        perIoData->dataBuf.len = bytesTransferred;
        perIoData->bytesSent = 0;
        perIoData->flags = 0;

        if (WSASend(socket, &(perIoData->dataBuf), 1, NULL, 0, &(perIoData->overlapped), NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                printf("WSASend failed with error: %d\n", WSAGetLastError());
                closesocket(socket);
                free(perIoData);
                continue;
            }
        }

        printf("Echoed back to client %s: %s\n", clientIP, perIoData->buffer);

        // Reset for the next receive
        ZeroMemory(&(perIoData->overlapped), sizeof(WSAOVERLAPPED));
        perIoData->bytesReceived = 0;
        perIoData->bytesSent = 0;
        perIoData->flags = 0;
        perIoData->dataBuf.len = BUFFER_SIZE;

        // Post another receive
        if (WSARecv(socket, &(perIoData->dataBuf), 1, NULL, &(perIoData->flags), &(perIoData->overlapped), NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                printf("WSARecv failed with error: %d\n", WSAGetLastError());
                closesocket(socket);
                free(perIoData);
                continue;
            }
        }
    }

    return 0;
}

// Thread function to accept client connections
unsigned int WINAPI AcceptThread(LPVOID lpParam) {
    SOCKET listenSocket = (SOCKET)lpParam;
    SOCKET acceptSocket;
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);
    LPPER_IO_DATA perIoData;
    char clientIP[INET_ADDRSTRLEN];

    while (true) {
        // Accept a client connection
        acceptSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
        if (acceptSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            continue;
        }

        // Create per-IO data structure
        perIoData = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
        if (!perIoData) {
            printf("Memory allocation failed\n");
            closesocket(acceptSocket);
            continue;
        }

        ZeroMemory(perIoData, sizeof(PER_IO_DATA));
        perIoData->socket = acceptSocket;
        perIoData->clientAddr = clientAddr;
        perIoData->dataBuf.buf = perIoData->buffer;
        perIoData->dataBuf.len = BUFFER_SIZE;
        perIoData->flags = 0;

        // Associate the accepted socket with the completion port
        if (CreateIoCompletionPort((HANDLE)acceptSocket, g_hIOCP, (ULONG_PTR)acceptSocket, 0) == NULL) {
            printf("CreateIoCompletionPort failed with error: %d\n", GetLastError());
            closesocket(acceptSocket);
            free(perIoData);
            continue;
        }

        // Print client connection information
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        printf("New client connected. IP: %s, Socket: %d\n", clientIP, (int)acceptSocket);

        // Post an initial receive
        if (WSARecv(acceptSocket, &(perIoData->dataBuf), 1, NULL, &(perIoData->flags), &(perIoData->overlapped), NULL) == SOCKET_ERROR) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                printf("WSARecv failed with error: %d\n", WSAGetLastError());
                closesocket(acceptSocket);
                free(perIoData);
                continue;
            }
        }
    }

    return 0;
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return 1;
    }

    // Create a socket for the server
    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) {
        ErrorCleanup("socket creation failed");
    }

    // Set up the server address
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(DEFAULT_PORT);

    // Bind the socket to the address and port
    result = bind(g_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        ErrorCleanup("bind failed");
    }

    // Start listening for client connections
    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        ErrorCleanup("listen failed");
    }

    // Create an I/O completion port
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (g_hIOCP == NULL) {
        ErrorCleanup("CreateIoCompletionPort failed");
    }

    // Determine the number of processors
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DWORD numThreads = min(systemInfo.dwNumberOfProcessors * 2, MAX_WORKER_THREADS);

    // Create worker threads to process I/O completion packets
    for (DWORD i = 0; i < numThreads; i++) {
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, g_hIOCP, 0, NULL);
        if (hThread == NULL) {
            ErrorCleanup("Failed to create worker thread");
        }
        CloseHandle(hThread);
    }

    // Create a thread to accept client connections
    HANDLE hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, (LPVOID)g_listenSocket, 0, NULL);
    if (hAcceptThread == NULL) {
        ErrorCleanup("Failed to create accept thread");
    }
    CloseHandle(hAcceptThread);

    printf("Echo Server (IOCP) started. Listening on port %d...\n", DEFAULT_PORT);
    printf("Using %d worker threads.\n", numThreads);
    printf("Press Enter to exit...\n");

    // Wait for user input to exit
    getchar();

    // Clean up
    closesocket(g_listenSocket);
    CloseHandle(g_hIOCP);
    WSACleanup();

    return 0;
}