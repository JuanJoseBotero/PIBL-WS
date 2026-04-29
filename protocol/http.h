#ifndef HTTP_H
#define HTTP_H

#define MAX_HEADERS    50
#define MAX_HEADER_LEN 256
#define MAX_METHOD_LEN 8
#define MAX_PATH_LEN   512
#define MAX_BODY_LEN   65536

// Tipo de dato para los metodos HTTP - solo GET y POST por el momento
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_UNKNOWN
} HttpMethod;

// Tipo de dato para un header HTTP - con su clave y valor
typedef struct {
    char key[MAX_HEADER_LEN];
    char value[MAX_HEADER_LEN];
} HttpHeader;

// Tipo de dato para un request HTTP - con método, path, versión, headers y body
typedef struct {
    HttpMethod method;
    char path[MAX_PATH_LEN];
    char version[16];
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char body[MAX_BODY_LEN];
    int body_length;
} HttpRequest;

// Tipo de dato para una respuesta HTTP - con código, texto, headers y body
typedef struct {
    int status_code;
    char status_text[64];
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char body[MAX_BODY_LEN];
} HttpResponse;

// ─────────────────────────────────────────
// FUNCIONES PÚBLICAS
// ─────────────────────────────────────────

// Parsea el request completo desde el string raw
int http_parse_request (const char *raw, HttpRequest *req);

// Busca el valor de un header por su clave (case-insensitive)
const char* http_get_header (HttpRequest *req, const char *key);

// Maneja un request HTTP y llena la respuesta
void http_handle_request (const char *raw, HttpResponse *res);

// Construye el string de respuesta HTTP completo a partir de HttpResponse
void http_build_response (HttpResponse *res, char *out, int out_size);

#endif