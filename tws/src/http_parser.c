#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "http_parser.h"
#include "logger.h"

int read_full_request(int client_fd, char *buffer, int max_size) {
    int total = 0;

    while (total < max_size - 1) {
        int bytes = recv(client_fd, buffer + total, max_size - total - 1, 0);

        if (bytes <= 0) {
            return -1; // error o conexión cerrada
        }

        total += bytes;
        buffer[total] = '\0';

        // condición clave HTTP
        if (strstr(buffer, "\r\n\r\n") != NULL) {
            return total;
        }
    }

    return -1; // demasiado grande o incompleto
}


/* Estructura: Metodo SP Request-URI SP HTTP-Version CRLF */
int parse_http_request(int client_fd, HttpRequest *req) {
    char buffer[BUFFER_SIZE];
    int bytes_received; 

    /* Paso 1 — leer bytes crudos del socket */
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = read_full_request(client_fd, buffer, BUFFER_SIZE);
    if (bytes_received <= 0) {
        return -1;
    }

    /* Paso 3 — parsear la request line
    sscanf lee método, path y versión separados por espacios
    aunque solo se implementan 3 metodos se deja en 7 para que puede leer cualquiera 
    de los metodos HTTP comunes, y con la version lo mismo para http/1.1 se necesitan 
    8 caracteres por lo que el redondeo siguiente es 16*/

    if (sscanf(buffer, "%7s %511s %15s",
               req->method,
               req->path,
               req->version) != 3) {
        return -1;  /* responder 400 */
    }

    /* Validacion de metodos */
    if (strcmp(req->method, "GET") != 0 &&
        strcmp(req->method, "POST") != 0 &&
        strcmp(req->method, "HEAD") != 0) {
        return -1;  /* método no soportado */
    }

    /* Validacion de URI */
    if (strstr(req->path, "..") != NULL) {
        return -1;
    }

    /* Validacion de version */
    if (strcmp(req->version, "HTTP/1.1") != 0 &&
        strcmp(req->version, "HTTP/1.0") != 0) {
        return -1;
    }

    /* Guardar el buffer completo en el struct */
    memcpy(req->raw, buffer, bytes_received);

    /* Extrae Content-Length del header para saber cuántos bytes tiene el body */
    char *cl = strstr(buffer, "Content-Length:");
    if (cl == NULL) {
        cl = strstr(buffer, "content-length:"); /* algunos clientes usan minúsculas */
    }

    if (cl != NULL) {
        /* saltar hasta los dos puntos y luego espacios */
        char *valor = strchr(cl, ':');
        if (valor != NULL) {
            valor++;                    /* saltar los : */
            while (*valor == ' ') valor++; /* saltar espacios extra */
            req->content_length = atoi(valor);
        }
    }
    return 0;
}

const char *get_content_type(const char *filepath) {
    if (strstr(filepath, ".html")) return "text/html";
    if (strstr(filepath, ".css"))  return "text/css";
    if (strstr(filepath, ".js"))   return "application/javascript";
    if (strstr(filepath, ".jpg"))  return "image/jpeg";
    if (strstr(filepath, ".jpeg")) return "image/jpeg";
    if (strstr(filepath, ".png"))  return "image/png";
    if (strstr(filepath, ".gif"))  return "image/gif";
    if (strstr(filepath, ".ico"))  return "image/x-icon";
    return "application/octet-stream";
}

void send_400(int client_fd) {
    char *body =
        "<html><body><h1>400 Bad Request</h1></body></html>";

    /* 512 para asegurar que se imprima toda la respuesta*/
    char response[512];

    snprintf(response, sizeof(response),
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body),
        body);

    send(client_fd, response, strlen(response), 0);
}


void send_404(int client_fd, int send_body) {
    char *body =
        "<html><body><h1>404 Not Found</h1></body></html>";

    char response[512];

    snprintf(response, sizeof(response),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body),
        send_body ? body : "");

    send(client_fd, response, strlen(response), 0);
}


void send_200(int client_fd, const char *filepath, int send_body) {
    FILE *file;
    char buffer[BUFFER_SIZE];
    long file_size;
    char headers[256];

    /* Abrir el archivo */
    file = fopen(filepath, "rb");
    if (file == NULL) {
        send_404(client_fd, send_body);
        return;
    }

    /* Calcular tamaño del archivo */
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* Construir y enviar headers */
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        get_content_type(filepath),
        file_size);

    send(client_fd, headers, strlen(headers), 0);

    /* Enviar el contenido del archivo en chunks */
    if (send_body) {
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_fd, buffer, bytes_read, 0);
        }
    }

    fclose(file);
}


void send_POST_response(int client_fd) {
    char *body =
        "<html><body><h1>POST recibido</h1></body></html>";
        
    char response[512];

    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body),
        body);

    send(client_fd, response, strlen(response), 0);
}


/* Decide qué responder según método y path */
void handle_request(int client_fd, HttpRequest *req, const char *root_dir, const char *client_ip) {
    char filepath[1024];

    /* Detectar HEAD una sola vez */
    int is_head = (strcmp(req->method, "HEAD") == 0);

    /* Construir ruta completa del archivo */
    snprintf(filepath, sizeof(filepath), "%s%s", root_dir, req->path);

    /* Si el path termina en / buscar index.html en ese directorio */
    if (req->path[strlen(req->path) - 1] == '/') {
        snprintf(filepath, sizeof(filepath), "%s%.500sindex.html",
             root_dir, req->path);
    }

    if (strcmp(req->method, "GET") == 0 || is_head) {
        FILE *test = fopen(filepath, "rb");
        if (test == NULL) {
            send_404(client_fd, !is_head);
            log_request(client_ip, req->method, req->path, 404);
            return;
        }
        fclose(test);
        send_200(client_fd, filepath, !is_head);
        log_request(client_ip, req->method, req->path, 200);
        return;
    }

    if (strcmp(req->method, "POST") == 0) {
        if (req->content_length > 0) {
            /* El body esta después de \r\n\r\n */
            char *body_start = strstr(req->raw, "\r\n\r\n");
            
            if (body_start != NULL) {
                /* saltar el \r\n\r\n */
                body_start += 4; 
                
                printf("POST body recibido (%d bytes): %s\n",
                    req->content_length, body_start);
            }
        }

        send_POST_response(client_fd);
        log_request(client_ip, req->method, req->path, 200);
        return;
    }

    /* Método no soportado */
    send_400(client_fd);
    log_request(client_ip, req->method, req->path, 400);
}