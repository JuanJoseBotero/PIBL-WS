## Descripción

Este módulo implementa el parsing de peticiones HTTP/1.1 y la construcción de respuestas según RFC 2616, se encarga de:

- Leer bytes crudos desde el socket
- Extraer método, path y versión HTTP
- Validar peticiones
- Construir respuestas HTTP correctas
- Servir archivos estáticos

El módulo es **thread-safe**, ya que cada hilo trabaja con su propia instancia de `HttpRequest` en el stack, sin compartir datos entre hilos.

---

## Características principales

- Lectura completa de peticiones HTTP hasta `\r\n\r\n`
- Parsing de método, path y versión usando `sscanf`
- Validaciones de seguridad:
  - Path traversal
  - Métodos soportados
  - Versiones HTTP válidas
- Detección automática de `Content-Type`
- Servicio de archivos en chunks de 4096 bytes
- Soporte para:
  - `GET`
  - `HEAD`
  - `POST`

---

### Constantes

| Constante | Descripción |
|---|---|
| `BUFFER_SIZE` | Tamaño del buffer para peticiones y archivos |
| `METHOD_SIZE` | Tamaño máximo del método HTTP |
| `PATH_SIZE` | Tamaño máximo del path |
| `VERSION_SIZE` | Tamaño máximo de la versión HTTP |

---

### Struct principal

| Campo | Descripción |
|---|---|
| `method` | Método HTTP (`GET`, `HEAD`, `POST`) |
| `path` | Ruta solicitada |
| `version` | Versión HTTP |
| `raw` | Buffer completo de la petición |
| `content_length` | Valor del header `Content-Length` |

---

### Lectura completa de petición
`read_full_request()`

```c
int read_full_request(int client_fd, char *buffer, int max_size);
```

Lee la petición HTTP completa desde el socket hasta encontrar:

```text
\r\n\r\n
```

### Funcionamiento

- Usa `recv()` en loop
- Acumula bytes en el buffer
- Detecta fin de headers
- Retorna error si:
  - El cliente se desconecta
  - La petición es demasiado grande
  - Hay error de lectura

---

### Parsing HTTP
`parse_http_request()`

```c
int parse_http_request(int client_fd, HttpRequest *req);
```

Parsea la petición y llena el struct `HttpRequest`.

### Flujo de funcionamiento

### 1. Leer petición completa

```c
memset(buffer, 0, BUFFER_SIZE);

bytes_received = read_full_request(
    client_fd,
    buffer,
    BUFFER_SIZE
);

if (bytes_received <= 0)
    return -1;
```

### 2. Extraer método, path y versión

```c
sscanf(buffer,
       "%7s %511s %15s",
       req->method,
       req->path,
       req->version);
```

Los límites previenen **buffer overflows**.

### 3. Validar método HTTP

```c
if (strcmp(req->method, "GET")  != 0 &&
    strcmp(req->method, "POST") != 0 &&
    strcmp(req->method, "HEAD") != 0) {
    return -1;
}
```

Solo se aceptan los métodos implementados.

### 4. Validar path

```c
if (strstr(req->path, "..") != NULL)
    return -1;
```

Previene ataques de:

```text
../../etc/passwd
```

### 5. Validar versión HTTP

```c
if (strcmp(req->version, "HTTP/1.1") != 0 &&
    strcmp(req->version, "HTTP/1.0") != 0) {
    return -1;
}
```

### 6. Extraer `Content-Length`

```c
char *cl = strstr(buffer, "Content-Length:");
```

Se almacena en:

```c
req->content_length
```

---

### Detección de Content-Type
`get_content_type()`

```c
const char *get_content_type(const char *filepath);
```

Detecta el tipo MIME según la extensión del archivo.

### Extensiones soportadas

| Extensión | Content-Type |
|---|---|
| `.html` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.jpg` / `.jpeg` | `image/jpeg` |
| `.png` | `image/png` |
| `.gif` | `image/gif` |
| `.ico` | `image/x-icon` |

Por defecto:

```text
application/octet-stream
```

---

### Respuestas HTTP
`send_400()`

Envía:

```http
HTTP/1.1 400 Bad Request
```

Usado cuando:

- La petición es inválida
- El método no está soportado
- La versión HTTP es inválida


`send_404()`

Envía:

```http
HTTP/1.1 404 Not Found
```

`send_200()`

Envía:

```http
HTTP/1.1 200 OK
```

---

### GET y HEAD

Comparten la misma lógica:

- Verificar existencia del archivo
- Responder `404` si no existe
- Responder `200` si existe

## Diferencia

| Método | Body |
|---|---|
| `GET` | Sí |
| `HEAD` | No |



### POST

- Lee el body desde `req->raw`
- Busca `\r\n\r\n`
- Extrae contenido del body
- Responde con `200 OK`