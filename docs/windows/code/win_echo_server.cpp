/**
 * Windows Echo Server using I/O Multiplexing (select)
 * 
 * This demo shows how to implement a server that can handle multiple client
 * connections simultaneously using the select() function for I/O multiplexing.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "8888"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Function to display error message and clean up Winsock
void ErrorCleanup(const char* message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    WSACleanup();
    exit(1);
}

int main() {
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
    hints.ai_flags = AI_PASSIVE;     // For wildcard IP address

    // Resolve the local address and port to be used by the server
    struct addrinfo* addrResult = NULL;
    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
    if (result != 0) {
        ErrorCleanup("getaddrinfo failed");
    }

    // Create a socket for the server
    SOCKET listenSocket = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        freeaddrinfo(addrResult);
        ErrorCleanup("socket creation failed");
    }

    // Bind the socket to the address and port
    result = bind(listenSocket, addrResult->ai_addr, (int)addrResult->ai_addrlen);
    if (result == SOCKET_ERROR) {
        freeaddrinfo(addrResult);
        closesocket(listenSocket);
        ErrorCleanup("bind failed");
    }

    // We don't need the addrinfo structure anymore
    freeaddrinfo(addrResult);

    // Start listening for client connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        ErrorCleanup("listen failed");
    }

    printf("Echo Server started. Listening on port %s...\n", DEFAULT_PORT);

    // Array to keep track of all client sockets
    SOCKET clientSockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientSockets[i] = INVALID_SOCKET;
    }

    // Main server loop
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    while (true) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the listening socket to the set
        FD_SET(listenSocket, &readfds);

        // Add client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientSockets[i] != INVALID_SOCKET) {
                FD_SET(clientSockets[i], &readfds);
            }
        }

        // Wait for activity on any of the sockets (no timeout)
        int activity = select(0, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            ErrorCleanup("select failed");
        }

        // Check if there is activity on the listening socket (new connection)
        if (FD_ISSET(listenSocket, &readfds)) {
            SOCKET newClientSocket = accept(listenSocket, NULL, NULL);
            if (newClientSocket == INVALID_SOCKET) {
                ErrorCleanup("accept failed");
            }

            // Add the new client to the array of client sockets
            bool clientAdded = false;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientSockets[i] == INVALID_SOCKET) {
                    clientSockets[i] = newClientSocket;
                    printf("New client connected. Socket FD: %d\n", (int)newClientSocket);
                    clientAdded = true;
                    break;
                }
            }

            // If we couldn't add the client, close the connection
            if (!clientAdded) {
                printf("Too many clients connected. Connection rejected.\n");
                closesocket(newClientSocket);
            }
        }

        // Check for activity on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = clientSockets[i];

            if (sd != INVALID_SOCKET && FD_ISSET(sd, &readfds)) {
                // Read data from the client
                ZeroMemory(buffer, BUFFER_SIZE);
                int bytesReceived = recv(sd, buffer, BUFFER_SIZE, 0);

                if (bytesReceived == SOCKET_ERROR) {
                    int error = WSAGetLastError();
                    if (error == WSAECONNRESET) {
                        // Client disconnected
                        printf("Client disconnected unexpectedly. Socket FD: %d\n", (int)sd);
                        closesocket(sd);
                        clientSockets[i] = INVALID_SOCKET;
                    } else {
                        ErrorCleanup("recv failed");
                    }
                } else if (bytesReceived == 0) {
                    // Client closed the connection gracefully
                    printf("Client disconnected. Socket FD: %d\n", (int)sd);
                    closesocket(sd);
                    clientSockets[i] = INVALID_SOCKET;
                } else {
                    // Echo the data back to the client
                    printf("Received from client %d: %s\n", (int)sd, buffer);
                    int bytesSent = send(sd, buffer, bytesReceived, 0);
                    if (bytesSent == SOCKET_ERROR) {
                        ErrorCleanup("send failed");
                    }
                    printf("Echoed back to client %d: %s\n", (int)sd, buffer);
                }
            }
        }
    }

    // Clean up
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientSockets[i] != INVALID_SOCKET) {
            closesocket(clientSockets[i]);
        }
    }
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}