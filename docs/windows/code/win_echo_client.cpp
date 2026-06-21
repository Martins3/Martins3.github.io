/**
 * Windows Echo Client
 * 
 * This demo shows how to implement a client that connects to the echo server
 * and sends/receives messages.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "8888"
#define DEFAULT_SERVER "127.0.0.1"
#define BUFFER_SIZE 1024

// Function to display error message and clean up Winsock
void ErrorCleanup(const char* message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    WSACleanup();
    exit(1);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments for server address
    const char* serverAddr = DEFAULT_SERVER;
    if (argc > 1) {
        serverAddr = argv[1];
    }

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", result);
        return 1;
    }

    // Set up hints for getaddrinfo
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // TCP protocol

    // Resolve the server address and port
    struct addrinfo* addrResult = NULL;
    result = getaddrinfo(serverAddr, DEFAULT_PORT, &hints, &addrResult);
    if (result != 0) {
        ErrorCleanup("getaddrinfo failed");
    }

    // Create a socket for connecting to the server
    SOCKET connectSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        freeaddrinfo(addrResult);
        ErrorCleanup("socket creation failed");
    }

    // Connect to the server
    result = connect(connectSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
        freeaddrinfo(addrResult);
        ErrorCleanup("connection failed");
    }

    // We don't need the addrinfo structure anymore
    freeaddrinfo(addrResult);

    // Check if connection was successful
    if (connectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Connected to echo server at %s:%s\n", serverAddr, DEFAULT_PORT);
    printf("Type a message and press Enter to send. Type 'exit' to quit.\n");

    char sendBuffer[BUFFER_SIZE];
    char recvBuffer[BUFFER_SIZE];
    int bytesReceived;

    // Set up select for non-blocking input
    fd_set readfds;
    struct timeval tv;

    // Main client loop
    while (true) {
        // Clear the set and add stdin
        FD_ZERO(&readfds);
        FD_SET(connectSocket, &readfds);

        // Set timeout for select (100ms)
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        // Check for data from server
        result = select(0, &readfds, NULL, NULL, &tv);
        if (result == SOCKET_ERROR) {
            ErrorCleanup("select failed");
        }

        // Check if there is data to receive from server
        if (FD_ISSET(connectSocket, &readfds)) {
            ZeroMemory(recvBuffer, BUFFER_SIZE);
            bytesReceived = recv(connectSocket, recvBuffer, BUFFER_SIZE, 0);

            if (bytesReceived > 0) {
                // Data received from server
                printf("Server response: %s\n", recvBuffer);
            } else if (bytesReceived == 0) {
                // Connection closed by server
                printf("Server closed the connection\n");
                break;
            } else {
                // Error in recv
                ErrorCleanup("recv failed");
            }
        }

        // Check if there is input from the user
        if (_kbhit()) {
            ZeroMemory(sendBuffer, BUFFER_SIZE);
            printf("Enter message: ");
            fgets(sendBuffer, BUFFER_SIZE, stdin);

            // Remove newline character
            size_t len = strlen(sendBuffer);
            if (len > 0 && sendBuffer[len - 1] == '\n') {
                sendBuffer[len - 1] = '\0';
            }

            // Check if user wants to exit
            if (strcmp(sendBuffer, "exit") == 0) {
                printf("Exiting...\n");
                break;
            }

            // Send the message to the server
            result = send(connectSocket, sendBuffer, (int)strlen(sendBuffer), 0);
            if (result == SOCKET_ERROR) {
                ErrorCleanup("send failed");
            }
            printf("Sent to server: %s\n", sendBuffer);
        }

        // Small delay to prevent CPU hogging
        Sleep(10);
    }

    // Clean up
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}