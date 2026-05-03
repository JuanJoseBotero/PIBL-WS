#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "config/config.c"
#include "cache/cache.c"
// #include "load_balancer.h"
 
#define BUFFER_SIZE 4096
 
int main() {
    Config config;
 
    // Cargar configuración desde archivo JSON
    if (config_load("config-pibl.json", &config) != 0) {
        fprintf(stderr, "[ERROR] No se pudo cargar config-pibl.json\n");
        return 1;
    }
 
    // Inicializar cache con TTL del config
    cache_init(config.ttl);
 
    // Inicializar load balancer con servidores del config
    // lb_init(config.servers, config.num_servers);
 
    // Inicializar sockets
    int proxy_socket, client_socket, server_socket;
    struct sockaddr_in proxy_addr, client_addr, server_addr;
    socklen_t addrlen = sizeof(client_addr);
 
    // Socket del proxy
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
 
    // Configurar dirección del proxy
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(config.proxy_port);
 
    // bind y listen - el proxy queda a la espera de clientes
    bind(proxy_socket, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
    listen(proxy_socket, 5);
 
    // Imprimir información que confirma que el proxy está listo para recibir conexiones
    printf("[PIBL] Escuchando en puerto %d | TTL cache: %ds | Servidores: %d\n",
        config.proxy_port, config.ttl, config.num_servers);
 
    while (1) {
        // Aceptar cliente
        client_socket = accept(proxy_socket, (struct sockaddr*)&client_addr, &addrlen);
 
        // Se crea un buffer para recibir el request del cliente
        char buffer_request[BUFFER_SIZE];
        int bytes = recv(client_socket, buffer_request, BUFFER_SIZE - 1, 0);
 
        // Si no se reciben bytes, cerrar conexión y esperar otro cliente
        if (bytes <= 0) {
            close(client_socket);
            continue;
        }
 
        // Asegurar que el buffer es un string válido para imprimir
        buffer_request[bytes] = '\0';
        printf("\n[REQUEST]\n%s\n", buffer_request);
 
        // Extraer path del request para usar como clave del cache
        char method[16], path[256];
        sscanf(buffer_request, "%15s %255s", method, path);
 
        // Verificar cache
        char cached[BUFFER_SIZE];
        int  cached_size = 0;
 
        // Buscar en cache usando el path como clave
        if (cache_get(path, cached, &cached_size)) {
            // ✅ Cache HIT → responder directo sin ir al servidor
            send(client_socket, cached, cached_size, 0);
            printf("[CACHE] HIT → Respondido desde cache: %s\n", path);
            close(client_socket);
            continue;
        }
 
        // ❌ Cache MISS → seleccionar servidor con round-robin
        //int idx = get_next_server();













        // Obtener IP y puerto del servidor seleccionado
        char* server_ip = "127.0.0.1";
        int server_port = 8081;

        // Imprimir información del servidor al que se va a conectar
        printf("[LB] Enviando a servidor %s:%d\n", "127.0.0.1", 8081);
 
        // Crear socket hacia el webserver elegido
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
 
        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(server_port);
        inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
 
        if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            fprintf(stderr, "[ERROR] No se pudo conectar al servidor %s:%d\n",
                server_ip, server_port);
            close(server_socket);
            close(client_socket);
            continue;
        }
 
        // 📤 Enviar request al webserver
        send(server_socket, buffer_request, bytes, 0);
 
        // 📥 Recibir respuesta, reenviar al cliente y acumular para cache
        int  resp_bytes;
        char buffer_response[BUFFER_SIZE];
        char cache_buffer[BUFFER_SIZE];
        int  cache_size = 0;
 
        // Recibir respuesta en bloques y reenviar al cliente
        while ((resp_bytes = recv(server_socket, buffer_response, BUFFER_SIZE - 1, 0)) > 0) {
            // Reenviar al cliente
            send(client_socket, buffer_response, resp_bytes, 0);
 
            // Asegurar que el buffer es un string válido para imprimir
            buffer_response[resp_bytes] = '\0';
            printf("%s", buffer_response);
 
            // Acumular respuesta para guardar en cache (si cabe en el buffer)
            if (cache_size + resp_bytes < BUFFER_SIZE) {
                // Copiar la respuesta al buffer de cache
                memcpy(cache_buffer + cache_size, buffer_response, resp_bytes);
                cache_size += resp_bytes;
            }
        }
 
        // 💾 Guardar respuesta en cache
        if (cache_size > 0) {
            cache_set(path, cache_buffer, cache_size);
        }
 
        // 🔚 Cerrar conexiones
        close(server_socket);
        close(client_socket);
    }
 
    close(proxy_socket);
    return 0;
}
 