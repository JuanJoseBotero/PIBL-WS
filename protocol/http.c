#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

// Utilidad: elimina espacios al inicio y fin
static void trim(char *str) {
    // Trim izquierda
    char *start = str;
    while (*start == ' ' || *start == '\t') start++;

    // Mover al inicio
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // Trim derecha
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

// Parsear la primera línea: "GET /path HTTP/1.1"
static int parse_request_line(const char *line, HttpRequest *req) {
    char method[MAX_METHOD_LEN];    // Temporal para el método
    if (sscanf(line, "%7s %511s %15s", method, req->path, req->version) != 3) {     // Si no se pudieron parsear los 3 componentes, es un request inválido
        return -1;
    }

    // Determinar el método HTTP
    if (strcmp(method, "GET") == 0) {
        req->method = HTTP_GET;
    } else if (strcmp(method, "POST") == 0) {
        req->method = HTTP_POST;
    } else {
        req->method = HTTP_UNKNOWN;
    }

    return 0;
}

// Parsear un header individual: "Content-Type: application/json"
static int parse_header_line(const char *line, HttpHeader *header) {
    // Buscar el colon que separa clave y valor
    const char *colon = strchr(line, ':');
    if (!colon) return -1;

    // Se calcula la longitud de key para copiar solo esa parte, y se asegura que no exceda MAX_HEADER_LEN - 1 para dejar espacio al \0
    int key_len = colon - line;
    if (key_len >= MAX_HEADER_LEN) key_len = MAX_HEADER_LEN - 1;

    // La clave es la parte antes del colon, y se le asigna \0 al final para asegurar que es un string válido, al final se hace trim para eliminar espacios
    strncpy(header->key, line, key_len);
    header->key[key_len] = '\0';
    trim(header->key);

    // El valor empieza después del colon, y se le asigna \0 al final para asegurar que es un string válido, al final se hace trim para eliminar espacios
    strncpy(header->value, colon + 1, MAX_HEADER_LEN - 1);
    header->value[MAX_HEADER_LEN - 1] = '\0';
    trim(header->value);

    return 0;
}

// Función principal: parsea el request HTTP completo
int http_parse_request(const char *raw, HttpRequest *req) {
    if (!raw || !req) return -1;

    // Limpiar estructura
    memset(req, 0, sizeof(HttpRequest));

    char copy[8192];
    strncpy(copy, raw, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *cursor = copy;
    char *line_end;
    int first_line = 1;

    // Recorrer línea por línea
    while ((line_end = strstr(cursor, "\r\n")) != NULL) {
        *line_end = '\0';

        // Línea vacía = separador entre headers y body
        if (strlen(cursor) == 0) {
            cursor = line_end + 2;

            // Leer body si es POST
            if (req->method == HTTP_POST && strlen(cursor) > 0) {
                strncpy(req->body, cursor, MAX_BODY_LEN - 1);
                req->body[MAX_BODY_LEN - 1] = '\0';
                req->body_length = strlen(req->body);
            }
            break;
        }

        if (first_line) {
            // Parsear "GET /path HTTP/1.1"
            if (parse_request_line(cursor, req) != 0) return -1;
            first_line = 0;
        } else {
            // Parsear headers
            if (req->header_count < MAX_HEADERS) {
                parse_header_line(cursor, &req->headers[req->header_count]);
                req->header_count++;
            }
        }

        cursor = line_end + 2;
    }

    return 0;
}

// Buscar un header por nombre (case-insensitive)
const char* http_get_header(HttpRequest *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (_stricmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

// Agregar un header a la respuesta
void http_set_header(HttpResponse *res, const char *key, const char *value) {
    if (res->header_count >= MAX_HEADERS) return;
    strncpy(res->headers[res->header_count].key, key, MAX_HEADER_LEN - 1);
    strncpy(res->headers[res->header_count].value, value, MAX_HEADER_LEN - 1);
    res->header_count++;
}

// Construir el string de respuesta HTTP
void http_build_response(HttpResponse *res, char *out, int out_size) {
    // Línea de estado
    int written = snprintf(out, out_size,
        "HTTP/1.1 %d %s\r\n",
        res->status_code,
        res->status_text
    );

    // Headers
    for (int i = 0; i < res->header_count; i++) {
        written += snprintf(out + written, out_size - written,
            "%s: %s\r\n",
            res->headers[i].key,
            res->headers[i].value
        );
    }

    // Línea en blanco + body
    written += snprintf(out + written, out_size - written, "\r\n%s", res->body);
}