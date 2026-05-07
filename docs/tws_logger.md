## Descripción

Este módulo implementa el sistema de logging del servidor TWS.

Su función principal es registrar todas las peticiones HTTP procesadas por el servidor, escribiéndolas tanto en consola como en un archivo de log (`tws.log`).

El módulo está diseñado para ser **thread-safe**, utilizando un `pthread_mutex_t` para evitar que múltiples hilos escriban simultáneamente sobre el archivo o la salida estándar.

---

### Librerías Utilizadas

```c
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "logger.h"
```

| Librería | Propósito |
|---|---|
| `stdio.h` | Funciones de entrada/salida (`printf`, `fprintf`, `snprintf`) |
| `time.h` | Manejo de fecha y hora |
| `pthread.h` | Mutex y concurrencia |
| `logger.h` | Declaraciones del módulo logger |

---

### Mutex Global

```c
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
```
Este mutex protege la sección crítica del logger.

Sin mutex:

- Dos hilos podrían escribir simultáneamente
- Las líneas del log podrían mezclarse
- El archivo podría corromperse

Ejemplo incorrecto sin mutex:

```text
[2025-05-06] 127.0.0.1 "GET /index.html"
[2025-05-[2025-05-06] 127.0.0.1 "POST /login"
```

Con mutex:

```text
[2025-05-06] 127.0.0.1 "GET /index.html" 200
[2025-05-06] 127.0.0.1 "POST /login" 200
```

---

### Función Principal
`log_request()`

```c
void log_request(const char *client_ip,
                 const char *method,
                 const char *path,
                 int status_code);
```

### Parámetros

| Parámetro | Descripción |
|---|---|
| `client_ip` | IP del cliente |
| `method` | Método HTTP (`GET`, `POST`, `HEAD`) |
| `path` | Recurso solicitado |
| `status_code` | Código HTTP retornado |

### Flujo de Funcionamiento

1. Obtener fecha y hora actual
2. Formatear timestamp
3. Construir línea de log
4. Bloquear mutex
5. Escribir en consola
6. Escribir en tws.log
7. Liberar mutex

---

### Obtención del Timestamp

### Obtener tiempo actual

```c
time_t now = time(NULL);
```
Obtiene el tiempo actual del sistema en segundos desde:

```text
1970-01-01 00:00:00 UTC
```

### Formatear fecha

```c
strftime(timestamp,
         sizeof(timestamp),
         "%Y-%m-%d %H:%M:%S",
         localtime(&now));
```

### Formato generado

```text
2026-05-06 22:30:15
```

---

### Construcción del Log

```c
snprintf(log_line,
         sizeof(log_line),
         "[%s] %s \"%s %s\" %d\n",
         timestamp,
         client_ip,
         method,
         path,
         status_code);
```

## Ejemplo generado

```text
[2026-05-06 22:30:15] 127.0.0.1 "GET /index.html" 200
```

---

### Protección con Mutex

### Bloqueo

```c
pthread_mutex_lock(&log_mutex);
```
Evita que múltiples hilos entren simultáneamente a la sección crítica.

### Liberación

```c
pthread_mutex_unlock(&log_mutex);
```

Permite que el siguiente hilo pueda escribir.

---

### Escritura en Archivo

### Abrir archivo

```c
FILE *log_file = fopen("tws.log", "a");
```

### Modo `"a"`

Append mode:

- No sobrescribe el archivo
- Agrega nuevas líneas al final

### Escribir línea

```c
fprintf(log_file, "%s", log_line);
```

### Cerrar archivo

```c
fclose(log_file);
```

### Conclusión

El módulo `logger.c` proporciona un sistema de logging simple, eficiente y thread-safe para el servidor TWS, garantiza:

- Correcta sincronización entre hilos
- Persistencia de logs
- Monitoreo en tiempo real
- Registro de peticiones HTTP