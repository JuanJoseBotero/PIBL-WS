# Módulo: Logger (logger.c)
## Descripción

Este módulo implementa el sistema de logging del servidor proxy.

Su función principal es registrar en un archivo la request que el cliente solicitó con parte de información de la response que da el servidor backend 

El logger escribe simultáneamente:

- En consola
- En archivo (`pibl.log`)

Tambien se asegura que no hayan 2 hilos guardando al mismo tiempo utilizando mutex. Esto para evitar conflictos entre múltiples hilos escribiendo al mismo tiempo y que se combinen requests o responses.

## Variables Globales
````c
static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
````
- `log_file`: Archivo donde se almacenan los logs
- ``log_mutex``: Mutex utilizado para sincronizar escrituras concurrentes

## 1. Inicialización del logger
`logger_init(const char *filepath)`

Inicializa el sistema de logging abriendo el archivo en modo append.

````c
// Inicializa el logger abriendo el archivo en modo append
void logger_init(const char *filepath) {
    log_file = fopen(filepath, "a");
}
````
El modo "a" permite:

- Conservar logs anteriores
- Agregar nuevas entradas al final del archivo

## 2. Cierre del logger
`logger_close()`

La función principal de esto es cerrar el archivo de logs

````c
// Cierra el archivo de log
void logger_close() {
    if (log_file) fclose(log_file);
}
````

## 3. Generación de timestamp
`get_timestamp(char *buf, int size)`

Esta función es utilizada para obtener la fechan y hora actual formateada, siendo útil para hacer lo registros con mayor claridad

````c
// Obtiene el timestamp actual formateado
static void get_timestamp(char *buf, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

````

**Formato utilizado**
````c
YYYY-MM-DD HH:MM:SS
````

> ⚠️ **IMPORTANTE:** Para la creación de esta función se uso `#include <time.h>` que nos ayuda para poder tomar el tiempo actual

## 4. Construcción del log del request

`build_log_request(...)`

Esta función utiliza la request que manda el cliente y crea un formato de como va a ser almacenada en el `pibl.log`

````c
// Arma el string del request en el buffer out
static void build_log_request(const char *method, const char *path, const char *full_request,
                               char *out, int out_size) {
    char ts[20];
    get_timestamp(ts, sizeof(ts));

    snprintf(out, out_size,
        "\n[%s] ── REQUEST ──────────────────────────\n"
        "  %s %s\n"
        "  Raw:\n%s\n",
        ts, method, path, full_request);
}
````
La información que se registra es: 
- Timestamp
- Método HTTP
- Path solicitado
- Request completo (raw request)

**Ejemplo**:
````
[2026-05-06 18:20:31] ── REQUEST ──────────────────────────
  GET /index.html
  Raw:
GET /index.html HTTP/1.1
Host: localhost
````

## 5. Construcción del log del response
`build_log_response(...)`

Esta función utiliza la response que se genera apartir de la request y crea un formato de como va a ser almacenada en el `pibl.log`

````c
// Arma el string del response en el buffer out
static void build_log_response(const char *path, int status_code, int bytes, int from_cache,
                                char *out, int out_size) {
    char ts[20];
    get_timestamp(ts, sizeof(ts));

    snprintf(out, out_size,
        "[%s] ── RESPONSE ─────────────────────────\n"
        "  Path    : %s\n"
        "  Status  : %d\n"
        "  Bytes   : %d\n"
        "  Fuente  : %s\n"
        "─────────────────────────────────────────\n",
        ts, path, status_code, bytes, from_cache ? "CACHE" : "SERVIDOR");
}
````
La información que se registra es: 
- Path solicitado
- Código HTTP
- Cantidad de bytes enviados
- Fuente de la respuesta:
    - CACHE
    - SERVIDOR

**Ejemplo**
````
[2026-05-06 18:20:31] ── RESPONSE ─────────────────────────
  Path    : /index.html
  Status  : 200
  Bytes   : 1024
  Fuente  : CACHE
─────────────────────────────────────────
````




## 6. Escritura segura de logs
`static void write_log(const char *line)`

Su función es escribir en el archivo y en consola el log que se necesita guardar.

````c
static void write_log(const char *line) {
    pthread_mutex_lock(&log_mutex);

    printf("%s", line);
    if (log_file) {
        fprintf(log_file, "%s", line);
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mutex);
}
````
La escritura está protegida mediante mutex, garantizando que cada transacción se escriba de forma completa y ordenada:

````c
pthread_mutex_lock(&log_mutex);
pthread_mutex_unlock(&log_mutex);
````

## 7. Registro completo de transacciones
`void log_transaction(...)`

Función principal del módulo.

````c
void log_transaction(const char *method, const char *path, const char *full_request,
                     int status_code, int bytes, int from_cache) {
    char log_entry[4096];
    char response_part[512];

    build_log_request(method, path, full_request, log_entry, sizeof(log_entry));
    build_log_response(path, status_code, bytes, from_cache, response_part, sizeof(response_part));
    strncat(log_entry, response_part, sizeof(log_entry) - strlen(log_entry) - 1);

    write_log(log_entry);
}
````

### Funcionamiento
- Construye el bloque del request
    ````c
    build_log_request(method, path, full_request, log_entry, sizeof(log_entry));
    ````
- Construye el bloque del response
    ````c
    build_log_response(path, status_code, bytes, from_cache, response_part, sizeof(response_part));
    ````
- Une ambas partes en una sola entrada
    ````c
    strncat(log_entry, response_part, sizeof(log_entry) - strlen(log_entry) - 1);
    ````
- Escribe el log de forma atómica
    ````c
    write_log(log_entry);
    ````
