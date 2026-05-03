#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Obtiene el timestamp actual formateado
static void get_timestamp(char *buf, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", t);
}

// Escribe en consola y archivo de una sola vez con mutex
static void write_log(const char *line) {
    pthread_mutex_lock(&log_mutex);

    printf("%s", line);
    if (log_file) {
        fprintf(log_file, "%s", line);
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mutex);
}

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

// Loguea request y response juntos en una sola escritura atómica
void log_transaction(const char *method, const char *path, const char *full_request,
                     int status_code, int bytes, int from_cache) {
    char log_entry[4096];
    char response_part[512];

    build_log_request(method, path, full_request, log_entry, sizeof(log_entry));
    build_log_response(path, status_code, bytes, from_cache, response_part, sizeof(response_part));
    strncat(log_entry, response_part, sizeof(log_entry) - strlen(log_entry) - 1);

    write_log(log_entry);
}

// Inicializa el logger abriendo el archivo en modo append
void logger_init(const char *filepath) {
    log_file = fopen(filepath, "a");
}

// Cierra el archivo de log
void logger_close() {
    if (log_file) fclose(log_file);
}