## Descripción

Implementa la capa de red del servidor, se encarga de:
- Crear socket TCP
- Escuchar conexiones
- Crear un hilo por cliente
- Delegar procesamiento

El loop principal nunca procesa peticiones directamente.

---

## Características principales

- Socket TCP con `SO_REUSEADDR`
- Loop infinito de `accept()`
- Un hilo por cliente
- Limpieza automática de hilos
- Conversión thread-safe de IPs

---

### Constantes

Cantidad máxima de conexiones pendientes.
```c
#define BACKLOG 10
```

### Struct del hilo
| Campo | Descripción |
|---|---|
| `client_fd` | Socket del cliente |
| `root_dir` | Directorio raíz |
| `client_ip` | IP del cliente |

---

### Creación del socket servidor
`create_server_socket()`

```c
int create_server_socket(int port);
```

Crea y configura el socket TCP.

### Flujo

### 1. Crear socket

```c
socket(AF_INET, SOCK_STREAM, 0);
```
- `AF_INET` → IPv4
- `SOCK_STREAM` → TCP


### 2. Configurar `SO_REUSEADDR`

```c
setsockopt(server_fd,
           SOL_SOCKET,
           SO_REUSEADDR,
           &opt,
           sizeof(opt));
```
Permite reutilizar el puerto rápidamente.

### 3. Bind

```c
bind(server_fd,
     (struct sockaddr *)&address,
     sizeof(address));
```

Usa:

```c
INADDR_ANY
```
para aceptar conexiones en cualquier IP.


### 4. Listen

```c
listen(server_fd, BACKLOG);
```

Marca el socket como servidor pasivo.

---

### Función del hilo
`handle_client()`

```c
void *handle_client(void *arg);
```

Procesa completamente una conexión cliente.

### Flujo

### Recuperar argumentos

```c
ClientArgs *args = (ClientArgs *)arg;
```

### Liberar memoria

```c
free(args);
```

Evita memory leaks.


### Detach del hilo

```c
pthread_detach(pthread_self());
```

Evita zombie threads.


### Parsear petición

```c
HttpRequest req;
memset(&req, 0, sizeof(req));
```

Cada hilo tiene su propio `HttpRequest`.

### Manejo de errores

```c
if (parse_http_request(...) != 0)
    send_400(client_fd);
```

### Cerrar socket

```c
close(client_fd);
```

Libera recursos del SO.

---

### Loop principal
`run_server()`

```c
void run_server(int server_fd,
                const char *root_dir);
```

Loop infinito que:

1. Acepta clientes
2. Crea hilo
3. Continúa escuchando

### Flujo

### Accept

```c
client_fd = accept(server_fd,
                   ...);
```
Devuelve un socket exclusivo para el cliente.

### Reservar memoria

```c
ClientArgs *args =
    malloc(sizeof(ClientArgs));
```

Se usa heap porque el hilo necesita datos persistentes.

### Convertir IP a texto

```c
inet_ntop(AF_INET,
          &client_addr.sin_addr,
          args->client_ip,
          sizeof(args->client_ip));
```

`inet_ntop()` es thread-safe.

### Crear hilo

```c
pthread_create(&thread_id,
               NULL,
               handle_client,
               args);
```

Cada cliente se procesa concurrentemente.

---

### Concurrencia

### Diseño thread-safe

Cada hilo tiene:

- Su propio `HttpRequest`
- Su propio `client_fd`
- Su propia copia de argumentos

No hay datos compartidos durante el parsing o servicio de archivos.

# Arquitectura concurrente

```text
Loop principal
    |
    +--> accept()
            |
            +--> pthread_create()
                    |
                    +--> handle_client()
                            |
                            +--> parse_http_request()
                            +--> handle_request()
                            +--> close()
```

Cada cliente es procesado de manera independiente y concurrente.