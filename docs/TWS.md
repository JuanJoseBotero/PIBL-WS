# TWS - Servidor Web con Concurrencia y Logging
Este proyecto implementa un servidor web HTTP/1.1 desde cero en C, que incluye:

- Manejo de métodos HTTP: GET, HEAD y POST
- Servidor de archivos estáticos con detección de tipo de contenido (content-type)
- Manejo concurrente de clientes con hilos (pthreads)
- Sistema de logging en stdout y archivo .log
- Validaciones de seguridad (path traversal, versión HTTP, métodos)

El servidor recibe peticiones HTTP de clientes (o del PIBL), parsea la petición según RFC 2616, busca el recurso solicitado en el sistema de archivos y devuelve la respuesta correspondiente segun RFC 2616.

## Diagrama de flujo del TWS

![alt text](flujo_tws.svg)

## Estructura del proyecto

```
tws/
│
├── src/
│   ├── main.c           
│   ├── server.c            
│   ├── server.h
│   ├── http_parser.c       
│   ├── http_parser.h
│   ├── logger.c            
│   └── logger.h
│
├── tests/
│   └── test_completo.sh    
│
├── Makefile
├── tws
└── tws.log
```

El proyecto fue diseñado siguiendo un enfoque modular, separando cada responsabilidad en componentes independientes:

- **server.c** → Manejo de sockets TCP, creación de hilos, loop principal
- **http_parser.c** → Parsing de peticiones HTTP/1.1, construcción de respuestas, servicio de archivos
- **logger.c** → Registro de transacciones con mutex para concurrencia segura
- **main.c** → Punto de entrada, lectura de argumentos de línea de comandos

Esta división permite:

- Mejor mantenimiento del código
- Facilidad para probar cada componente de forma independiente
- Clara separación entre la capa de red y la capa de protocolo

Por último estos archivos permiten la compilación y el registro:

- **Makefile** → Punto de entrada, lectura de argumentos de línea de comandos
- **tws** → Carpeta tws compilada
- **tws.log** → Se genera en ejecución


## Funcionamiento principal

### 1. Inicialización

En `main()` se leen los argumentos de línea de comandos:

```bash
./tws --port 8081 --root ./webapp
```

Se inicializa el servidor con:
- `create_server_socket()` — crea el socket, hace bind y listen
- `run_server()` — inicia el loop principal de aceptación de conexiones

### 2. Loop principal — `run_server()`

```c
while (1) {
    client_fd = accept(...);          // espera cliente
    args = malloc(sizeof(ClientArgs)); // empaquetar fd + root_dir + IP
    pthread_create(..., handle_client, args); // crear hilo
    // loop vuelve a accept() inmediatamente
}
```

Cada cliente genera un hilo independiente. El loop principal nunca se bloquea procesando — solo acepta y delega.

### 3. Procesamiento por hilo — `handle_client()`

Cada hilo:
1. Recupera sus argumentos del `ClientArgs`
2. Llama a `pthread_detach()` para limpiarse solo al terminar
3. Parsea la petición con `parse_http_request()`
4. Llama a `handle_request()` para responder
5. Cierra el socket del cliente

### 4. Parser HTTP — `parse_http_request()`

Implementa RFC 2616 5 — Request:

```
GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
Connection: close\r\n
\r\n
```

Pasos:
1. `read_full_request()` — lee del socket en loop hasta encontrar `\r\n\r\n`
2. `sscanf()` — extrae método, path y versión
3. Valida método, path (sin `..`) y versión HTTP
4. Extrae `Content-Length` para peticiones POST

### 5. Manejo de métodos — `handle_request()`

| Método | Comportamiento |
|--------|---------------|
| GET    | Sirve el archivo con headers y body |
| HEAD   | Sirve solo los headers, sin body (RFC 2616 9.4) |
| POST   | Lee el body y responde con confirmación |
| otro   | Responde 400 Bad Request |

### 6. Códigos de respuesta

| Código | Cuándo se usa |
|--------|--------------|
| 200 OK | Archivo encontrado y servido |
| 400 Bad Request | Petición malformada o método no soportado |
| 404 Not Found | Archivo no existe en el sistema |

### 7. Detección de Content-Type

El servidor detecta el tipo de archivo por extensión:

| Extensión | Content-Type |
|-----------|-------------|
| .html | text/html |
| .css | text/css |
| .js | application/javascript |
| .jpg / .jpeg | image/jpeg |
| .png | image/png |
| .gif | image/gif |
| .ico | image/x-icon |
| otro | application/octet-stream |

### 8. Sistema de logging — `log_request()`

Cada petición genera una línea de log con este formato:

```
[2026-05-01 20:35:02] 127.0.0.1 "GET /index.html" 200
```

El logging usa un mutex (`pthread_mutex_t`) para garantizar que dos hilos no escriban simultáneamente — evitando líneas de log corruptas. Escribe en:
- **stdout** — visible en la terminal del servidor
- **tws.log** — archivo persistente en disco

## Decisiones de diseño

| Decisión | Elección | Razón |
|----------|----------|-------|
| Conexiones | Connection: close | Simplifica el manejo de hilos — cada petición abre y cierra su socket |
| Concurrencia | Un hilo por cliente | Modelo simple, suficiente para las pruebas del enunciado |
| Parser | sscanf con límites | Previene buffer overflow en campos de longitud variable |
| Archivos | fread en chunks de 4096B | Permite servir archivos grandes (1MB+) sin cargarlos en memoria |
| thread cleanup | pthread_detach | El hilo se limpia solo — evita zombie threads sin necesitar join |

## Seguridad implementada

- **Path traversal**: peticiones con `..` en el path son rechazadas con 400
- **Validación de versión**: solo acepta HTTP/1.0 y HTTP/1.1
- **Validación de método**: rechaza métodos no implementados antes de procesar
- **Buffer limits**: todos los sscanf tienen límite de caracteres explícito

## Compilación y ejecución

```bash
# Compilar
cd tws
make

# Ejecutar con configuración por defecto (puerto 8081, carpeta ./webapp)
./tws

# Ejecutar con configuración personalizada
./tws --port 8081 --root ./webapp

# Limpiar binarios
make clean
```

## Pruebas

```bash
# Ejecutar suite de pruebas completa
bash tests/test_completo.sh

# Pruebas manuales
curl http://localhost:8081/index.html          # GET 200
curl http://localhost:8081/noexiste.html       # GET 404
curl -I http://localhost:8081/index.html       # HEAD sin body
curl -X POST http://localhost:8081/ \
     -d '{"test":"ok"}'                        # POST con body

# Prueba de concurrencia — 10 clientes simultáneos
for i in $(seq 1 10); do
    curl -s -o /dev/null http://localhost:8081/index.html &
done
wait
```

## Casos de prueba del enunciado

| Caso | Descripción | URL |
|------|-------------|-----|
| Caso 1 | Página con hipertextos e imagen | `/caso1/` |
| Caso 2 | Página con múltiples imágenes | `/caso2/` |
| Caso 3 | Archivo de aproximadamente 1MB | `/caso3/` |
| Caso 4 | Múltiples archivos de ~1MB total | `/caso4/` |

## Limitaciones conocidas

- No implementa `Connection: keep-alive` — cada petición abre y cierra su socket (decisión de diseño)
- No implementa compresión (gzip)
- El manejo de POST no persiste datos — solo confirma recepción
- No implementa virtual hosts

## Referencias

- RFC 2616 — Hypertext Transfer Protocol HTTP/1.1: https://www.rfc-editor.org/rfc/rfc2616
- POSIX Threads Programming: https://hpc-tutorials.llnl.gov/posix/
- Durante el desarrollo del TWS se utilizó Claude (Anthropic) como asistente de 
programación para orientación sobre la API POSIX Sockets, manejo de mutex e hilos con 
pthreads, y depuración de bugs. Todo el código fue escrito, revisado y validado por el equipo.
