#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT 8080

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // 1. Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Configurar dirección
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3. Bind
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // 4. Escuchar
    listen(server_fd, 5);

    printf("Servidor escuchando en http://localhost:%d\n", PORT);

    while (1) {
        // 5. Aceptar conexión
        client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        // 6. Leer request
        read(client_socket, buffer, 1024);
        printf("Request recibido:\n%s\n", buffer);

        // 7. Respuesta HTTP
        char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<html><body><h1>Hola desde C</h1></body></html>";

        // 8. Enviar respuesta
        write(client_socket, response, strlen(response));

        // 9. Cerrar conexión
        close(client_socket);
    }

    return 0;
}
