#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "logger.h"

/* Mutex — evita que dos hilos escriban al mismo tiempo */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_request(const char *client_ip, const char *method,
                 const char *path,      int status_code) {
    /* Obtener timestamp */
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    /* Construir línea de log */
    char log_line[512];
    snprintf(log_line, sizeof(log_line),
             "[%s] %s \"%s %s\" %d\n",
             timestamp, client_ip, method, path, status_code);

    /* Mutex — solo un hilo escribe a la vez */
    pthread_mutex_lock(&log_mutex);

    /* Escribir en stdout */
    printf("%s", log_line);

    /* Escribir en archivo */
    FILE *log_file = fopen("tws.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "%s", log_line);
        fclose(log_file);
    }

    /* Liberar mutex para siguiente hilo */
    pthread_mutex_unlock(&log_mutex);
}