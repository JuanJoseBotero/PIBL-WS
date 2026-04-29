#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

// ─────────────────────────────────────────
// UTILIDADES PRIVADAS
// ─────────────────────────────────────────

// Elimina espacios al inicio y al final de una cadena
static void trim(char *str) {
    char *start = str;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) memmove(str, start, strlen(start) + 1);

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }
}

// Agrega un header a la respuesta
static void set_header(HttpResponse *res, const char *key, const char *value) {
    if (res->header_count >= MAX_HEADERS) return;
    strncpy(res->headers[res->header_count].key,   key,   MAX_HEADER_LEN - 1);
    strncpy(res->headers[res->header_count].value, value, MAX_HEADER_LEN - 1);
    res->header_count++;
}

// ─────────────────────────────────────────
// PARSEO
// ─────────────────────────────────────────

// Parsea la línea de request (e.g. "GET /index.html HTTP/1.1")
static int parse_request_line(const char *line, HttpRequest *req) {
    char method[MAX_METHOD_LEN];
    if (sscanf(line, "%7s %511s %15s", method, req->path, req->version) != 3) {
        return -1;
    }

    if      (strcmp(method, "GET")  == 0) req->method = HTTP_GET;
    else if (strcmp(method, "POST") == 0) req->method = HTTP_POST;
    else                                  req->method = HTTP_UNKNOWN;

    return 0;
}

// Parsea una línea de header (e.g. "Content-Type: text/html")
static int parse_header_line(const char *line, HttpHeader *header) {
    const char *colon = strchr(line, ':');
    if (!colon) return -1;

    int key_len = colon - line;
    if (key_len >= MAX_HEADER_LEN) key_len = MAX_HEADER_LEN - 1;

    strncpy(header->key,   line,      key_len);
    header->key[key_len] = '\0';
    strncpy(header->value, colon + 1, MAX_HEADER_LEN - 1);
    header->value[MAX_HEADER_LEN - 1] = '\0';

    trim(header->key);
    trim(header->value);

    return 0;
}

// Parsea el request completo desde el string raw
int http_parse_request(const char *raw, HttpRequest *req) {
    if (!raw || !req) return -1;

    memset(req, 0, sizeof(HttpRequest));

    char copy[8192];
    strncpy(copy, raw, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *cursor = copy;
    char *line_end;
    int   first  = 1;

    while ((line_end = strstr(cursor, "\r\n")) != NULL) {
        *line_end = '\0';

        if (strlen(cursor) == 0) {
            cursor = line_end + 2;
            if (req->method == HTTP_POST && strlen(cursor) > 0) {
                strncpy(req->body, cursor, MAX_BODY_LEN - 1);
                req->body[MAX_BODY_LEN - 1] = '\0';
                req->body_length = strlen(req->body);
            }
            break;
        }

        if (first) {
            if (parse_request_line(cursor, req) != 0) return -1;
            first = 0;
        } else {
            if (req->header_count < MAX_HEADERS) {
                parse_header_line(cursor, &req->headers[req->header_count]);
                req->header_count++;
            }
        }

        cursor = line_end + 2;
    }

    return 0;
}

// Busca el valor de un header por su clave (case-insensitive)
const char* http_get_header(HttpRequest *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (_stricmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

// ─────────────────────────────────────────
// RESPUESTAS PRIVADAS
// ─────────────────────────────────────────

// Responde con un 200 OK y el contenido dado
static void respond_200(HttpResponse *res, const char *content, long size) {
    res->status_code = 200;
    strcpy(res->status_text, "OK");
    set_header(res, "Content-Type", "text/html");
    char len_str[16];
    snprintf(len_str, sizeof(len_str), "%ld", size);
    set_header(res, "Content-Length", len_str);
    memcpy(res->body, content, size);
}

// Responde con un 400 Bad Request
static void respond_400(HttpResponse *res) {
    res->status_code = 400;
    strcpy(res->status_text, "Bad Request");
    set_header(res, "Content-Type", "text/plain");
    strcpy(res->body, "400 Bad Request");
    set_header(res, "Content-Length", "15");
}

// Responde con un 404 Not Found
static void respond_404(HttpResponse *res) {
    res->status_code = 404;
    strcpy(res->status_text, "Not Found");
    set_header(res, "Content-Type", "text/plain");
    strcpy(res->body, "404 Not Found");
    set_header(res, "Content-Length", "13");
}

// ─────────────────────────────────────────
// FLUJO PRINCIPAL
// ─────────────────────────────────────────

void http_handle_request(const char *raw, HttpResponse *res) {
    memset(res, 0, sizeof(HttpResponse));

    // 1. Parsear el request
    HttpRequest req;
    if (http_parse_request(raw, &req) != 0) {
        respond_400(res);
        printf("[400] No se pudo parsear el request\n");
        return;
    }

    // 2. Validar método
    if (req.method == HTTP_UNKNOWN) {
        respond_400(res);
        printf("[400] Método no soportado\n");
        return;
    }

    // 3. Validar path
    if (strlen(req.path) == 0) {
        respond_400(res);
        printf("[400] Path vacío\n");
        return;
    }

    // 4. Validar POST con body
    if (req.method == HTTP_POST && req.body_length == 0) {
        respond_400(res);
        printf("[400] POST sin body\n");
        return;
    }

    // 5. Resolver path → archivo
    char filepath[MAX_PATH_LEN];
    if (strcmp(req.path, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "index.html");
    } else {
        snprintf(filepath, sizeof(filepath), "%s.html", req.path + 1);
    }

    // 6. Buscar el archivo
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        respond_404(res);
        printf("[404] %s → %s no encontrado\n", req.path, filepath);
        return;
    }

    // 7. Leer el archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char content[MAX_BODY_LEN];
    fread(content, 1, file_size, file);
    fclose(file);

    // 8. Responder 200 con el contenido
    respond_200(res, content, file_size);
    printf("[200] %s → %s\n", req.path, filepath);
}

// ─────────────────────────────────────────
// CONSTRUIR STRING DE RESPUESTA
// ─────────────────────────────────────────

void http_build_response(HttpResponse *res, char *out, int out_size) {
    int written = snprintf(out, out_size,
        "HTTP/1.1 %d %s\r\n",
        res->status_code, res->status_text);

    for (int i = 0; i < res->header_count; i++) {
        written += snprintf(out + written, out_size - written,
            "%s: %s\r\n",
            res->headers[i].key,
            res->headers[i].value);
    }

    snprintf(out + written, out_size - written, "\r\n%s", res->body);
}