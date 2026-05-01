#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "http_parser.h"

#define BACKLOG 10

int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    /* Crea el socket definiendo familia, tipo y protocolo */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() falló");
        return -1;
    }

    /* Evita el error "Address already in use" al reiniciar */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() falló");
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;      
    address.sin_addr.s_addr = INADDR_ANY;
    /* puerto en network byte order, conversion con htons */
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind() falló");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen() falló");
        close(server_fd);
        return -1;
    }

    printf("TWS escuchando en puerto %d\n", port);
    return server_fd;
}


void run_server(int server_fd, const char *root_dir) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

    while (1) {
        /* Se llena el struct con la información del cliente */
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);
        if (client_fd < 0) {
            perror("accept() falló");
            /* Sigue esperando, si no hay clientes no se consume CPU */
            continue;
        }

        printf("Cliente conectado: %s\n",
               inet_ntoa(client_addr.sin_addr));

        /* Crear estructura para el resultado del parser */
        HttpRequest req;

        memset(&req, 0, sizeof(req));

        if (parse_http_request(client_fd, &req) == 0) {
            handle_request(client_fd, &req, root_dir);
        } else {
            send_400(client_fd);
        }

        close(client_fd);
    }
}