#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

DWORD WINAPI receive_messages(LPVOID socket_fd) {
    SOCKET sock = *(SOCKET *)socket_fd;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection to server failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connected to server. Type your messages below:\n");

    // Create a thread to receive messages
    CreateThread(NULL, 0, receive_messages, &sock, 0, NULL);

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        send(sock, message, strlen(message), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
