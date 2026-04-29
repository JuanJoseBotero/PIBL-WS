#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "http.c"

#define PORT 8080

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[8192] = {0};
    char response_str[65536] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Servidor en http://localhost:%d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        HttpResponse res;
        http_handle_request(buffer, &res);

        http_build_response(&res, response_str, sizeof(response_str));
        send(client_socket, response_str, strlen(response_str), 0);

        closesocket(client_socket);
        memset(buffer, 0, sizeof(buffer));
        memset(response_str, 0, sizeof(response_str));
    }

    WSACleanup();
    return 0;
}