#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "http_parser.h"
#include "logger.h"

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

/* Un hilo por cliente */
void *handle_client(void *arg) {
    /* Recuperar los argumentos (lo unico que aceptan los hilos, punteros genericos) */
    /* Se le pasa un puntero a la estructura ClientArgs */
    ClientArgs *args = (ClientArgs *)arg;
    int client_fd    = args->client_fd;
    char root_dir[256];
    char client_ip[INET_ADDRSTRLEN];

    /* Copia el string a la variable local */
    strncpy(root_dir, args->root_dir, sizeof(root_dir) - 1);
    strncpy(client_ip,  args->client_ip,  sizeof(client_ip)  - 1);
    /* liberar memoria — quien la pidió la libera, para no leer informacion que no es*/
    free(args);

    /* Basicamente le dice al so cuando este hilo termine, libera sus recursos autom. */
    pthread_detach(pthread_self());

    /* Parsear y responder */
    HttpRequest req;
    memset(&req, 0, sizeof(req));

    if (parse_http_request(client_fd, &req) == 0) {
        handle_request(client_fd, &req, root_dir, client_ip);
    } else {
        send_400(client_fd);
    }

    close(client_fd);
    return NULL;
}

void run_server(int server_fd, const char *root_dir) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;
    pthread_t thread_id;

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

        /* Calcular tamaño del struct para liberar este tañamo en el heap
        esto porque persiste hasta que explicitamente se libera y no como 
        con stack que se libera con una iteracion */
        ClientArgs *args = malloc(sizeof(ClientArgs));
        /* Si no hay memoria disponible (raro) */
        if (args == NULL) {
            perror("malloc falló");
            close(client_fd);
            continue;
        }
        /* Copiar y llenar el struct para leer la peticion */
        args->client_fd = client_fd;
        strncpy(args->root_dir, root_dir, sizeof(args->root_dir) - 1);

        /* Convertir IP binaria a texto y guardarla en args */
        /* INternet Network Presentation*/
        inet_ntop(AF_INET, &client_addr.sin_addr, args->client_ip, sizeof(args->client_ip));

        /* Crear hilo — el loop principal sigue inmediatamente */
        if (pthread_create(&thread_id, NULL, handle_client, args) != 0) {
            perror("pthread_create() falló");
            free(args);
            close(client_fd);
            continue;
        }
    }
}