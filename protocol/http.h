#ifndef HTTP_H
#define HTTP_H

#define MAX_METHOD_LEN 8    // Largo máximo del método HTTP (e.g., "GET", "POST")
#define MAX_HEADERS 50      // Máxima cantidad de headers
#define MAX_HEADER_LEN 256  // Largo máximo de cada header
#define MAX_PATH_LEN 512    // Largo máximo del path
#define MAX_BODY_LEN 4096   // Largo máximo del body (Este solo aparece en POST)

// Métodos HTTP soportados - Defino un nuevo tipo de dato llamado HttpMethod, que es un enum con los métodos HTTP soportados (GET, POST, UNKNOWN)
typedef enum {
    HTTP_GET,       // Método GET, get = 0
    HTTP_POST,      // Método POST, post = 1
    HTTP_UNKNOWN    // Metodo desconocido, unknown = 2  
} HttpMethod;

// Estructura de un header individual - Defino una estructura llamada HttpHeader, que tiene dos campos: key y value, ambos son strings de tamaño MAX_HEADER_LEN
typedef struct {
    char key[MAX_HEADER_LEN];
    char value[MAX_HEADER_LEN];
} HttpHeader;

// Estructura del request completo
typedef struct {
    HttpMethod method;                  // GET o POST
    char path[MAX_PATH_LEN];            // "/ruta"
    char version[16];                   // "HTTP/1.1"
    HttpHeader headers[MAX_HEADERS];    // arreglo de 50 headers
    int header_count;                   // cuántos headers llegaron realmente
    char body[MAX_BODY_LEN];            // contenido del POST
    int body_length;                    // tamaño del body
} HttpRequest;

// Estructura de la respuesta
typedef struct {
    int status_code;
    char status_text[64];
    HttpHeader headers[MAX_HEADERS];
    int header_count;
    char body[MAX_BODY_LEN];
} HttpResponse;

// FUNCIONES PUBLICAS - Declaración de funciones públicas para parsear requests, obtener headers, construir respuestas, etc.

// Parsear un request HTTP crudo en una estructura HttpRequest
int http_parse_request(const char *raw, HttpRequest *req);

// Buscar un header por nombre (case-insensitive)
const char* http_get_header(HttpRequest *req, const char *key);

// Agregar un header a la respuesta
void http_build_response(HttpResponse *res, char *out, int out_size);

// Agregar un header a la respuesta
void http_set_header(HttpResponse *res, const char *key, const char *value);

#endif