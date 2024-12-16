#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
CRITICAL_SECTION clients_mutex;

void broadcast_message(char *message, int sender_fd) {
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] != 0 && client_sockets[i] != sender_fd) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
    LeaveCriticalSection(&clients_mutex);
}

DWORD WINAPI handle_client(LPVOID arg) {
    SOCKET client_fd = *(SOCKET *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';
        broadcast_message(buffer, client_fd);
    }

    closesocket(client_fd);

    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_sockets[i] == client_fd) {
            client_sockets[i] = 0;
            break;
        }
    }
    LeaveCriticalSection(&clients_mutex);

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    InitializeCriticalSection(&clients_mutex);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed. Error Code: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_socket == INVALID_SOCKET) {
            printf("Accept failed. Error Code: %d\n", WSAGetLastError());
            continue;
        }

        EnterCriticalSection(&clients_mutex);
        int i;
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_socket;
                break;
            }
        }
        LeaveCriticalSection(&clients_mutex);

        if (i == MAX_CLIENTS) {
            printf("Maximum clients reached. Connection refused.\n");
            closesocket(new_socket);
            continue;
        }

        CreateThread(NULL, 0, handle_client, &new_socket, 0, NULL);
    }

    closesocket(server_fd);
    DeleteCriticalSection(&clients_mutex);
    WSACleanup();
    return 0;
}