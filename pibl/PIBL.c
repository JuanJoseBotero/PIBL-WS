#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "config/config.c"
#include "cache/cache.c"
#include "load_balancer/load_balancer.c"
#include "logger/logger.c"

#define BUFFER_SIZE 4096

typedef struct {
    int client_socket;
} ClientArgs;

void *handle_client(void *arg) {
    ClientArgs *args = (ClientArgs *)arg;
    int client_socket = args->client_socket;
    free(arg);

    // Recibir request del cliente
    char buffer_request[BUFFER_SIZE];
    int bytes = recv(client_socket, buffer_request, BUFFER_SIZE - 1, 0);

    // Si no se reciben bytes, cerrar conexión
    if (bytes <= 0) {
        close(client_socket);
        return NULL;
    }

    // Asegurar que el buffer es un string válido
    buffer_request[bytes] = '\0';

    // Extraer method y path del request
    char method[16], path[256];
    sscanf(buffer_request, "%15s %255s", method, path);

    // Buscar en cache usando el path como clave
    char cached[BUFFER_SIZE];
    int  cached_size = 0;

    if (cache_get(path, cached, &cached_size)) {
        // Cache HIT → responder directo sin ir al servidor
        send(client_socket, cached, cached_size, 0);
        log_transaction(method, path, buffer_request, 200, cached_size, 1);
        close(client_socket);
        return NULL;
    }

    // Cache MISS → seleccionar servidor con round-robin
    Server web_server = get_next_server();
    char *server_ip   = web_server.ip;
    int   server_port = web_server.port;

    // Crear socket hacia el webserver elegido
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Conectar al servidor
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "[ERROR] No se pudo conectar al servidor %s:%d\n", server_ip, server_port);

        char *error_response =
            "HTTP/1.1 502 Bad Gateway\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body>"
            "<h1>502 Bad Gateway</h1>"
            "<p>No se pudo conectar al servidor backend.</p>"
            "</body></html>\r\n";

        log_transaction(method, path, buffer_request, 502, strlen(error_response), 0);
        send(client_socket, error_response, strlen(error_response), 0);
        close(server_socket);
        close(client_socket);
        return NULL;
    }

    // Enviar request al webserver
    send(server_socket, buffer_request, bytes, 0);

    // Recibir respuesta en bloques, reenviar al cliente y acumular para cache
    int  resp_bytes;
    char buffer_response[BUFFER_SIZE];
    char cache_buffer[BUFFER_SIZE];
    int  cache_size = 0;

    while ((resp_bytes = recv(server_socket, buffer_response, BUFFER_SIZE - 1, 0)) > 0) {
        // Reenviar al cliente
        send(client_socket, buffer_response, resp_bytes, 0);

        // Acumular respuesta para guardar en cache (si cabe en el buffer)
        if (cache_size + resp_bytes < BUFFER_SIZE) {
            memcpy(cache_buffer + cache_size, buffer_response, resp_bytes);
            cache_size += resp_bytes;
        }
    }

    // Extraer status code de la respuesta
    int status_code = 0;
    sscanf(cache_buffer, "HTTP/%*s %d", &status_code);

    // Loguear la respuesta
    log_transaction(method, path, buffer_request, status_code, cache_size, 0);

    // Guardar respuesta en cache
    if (cache_size > 0) {
        cache_set(path, cache_buffer, cache_size);
    }

    // Cerrar conexiones
    close(server_socket);
    close(client_socket);
    return NULL;
}

int main() {
    Config config;

    // Cargar configuración desde archivo JSON
    if (config_load("./config-pibl.json", &config) != 0) {
        fprintf(stderr, "[ERROR] No se pudo cargar config-pibl.json\n");
        return 1;
    }

    // Inicializar cache, load balancer y logger
    cache_init(config.ttl);
    lb_init(config.servers, config.num_servers);
    logger_init("pibl.log");

    // Inicializar sockets
    int proxy_socket;
    struct sockaddr_in proxy_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    // Socket del proxy
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Configurar dirección del proxy
    proxy_addr.sin_family      = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port        = htons(config.proxy_port);

    // bind y listen
    bind(proxy_socket, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
    listen(proxy_socket, 5);

    printf("[PIBL] Escuchando en puerto %d | TTL cache: %ds | Servidores: %d\n",
        config.proxy_port, config.ttl, config.num_servers);

    while (1) {
        // Aceptar cliente y crear thread
        ClientArgs *args = malloc(sizeof(ClientArgs));
        args->client_socket = accept(proxy_socket, (struct sockaddr*)&client_addr, &addrlen);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, args);
        pthread_detach(thread);
    }

    close(proxy_socket);
    logger_close();
    return 0;
}