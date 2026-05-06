# Módulo: Configuración (config.c)
## Descripción

Este módulo se encarga de cargar la configuración del servidor proxy desde un archivo externo en formato JSON (config-pibl.json).

Permite definir dinámicamente:

- **Puerto del proxy**
- **Tiempo de vida del cache (TTL)**
- **Lista de servidores backend**

Esto facilita la flexibilidad del sistema sin necesidad de recompilar el código.

## Estructura de configuración

El archivo esperado tiene un formato similar a:

````json
{
  "port": 8080,
  "ttl": 60,
  "servers": [
    { "ip": "127.0.0.1", "port": 8081 },
    { "ip": "127.0.0.1", "port": 8082 }
  ]
}
````
Cada campo tiene el siguiente propósito dentro del servidor proxy:

- `port`: Puerto en el que el proxy escucha las peticiones de los clientes.
- `ttl`: Tiempo de vida (en segundos) de los elementos almacenados en cache. Una vez superado este tiempo, los datos se consideran expirados y se eliminan.
- `servers`: Lista de servidores backend disponibles.
    - `ip`: Dirección IP del servidor.
    - `port`: Puerto del servidor backend.

Estos servidores son utilizados por el balanceador de carga para distribuir las peticiones entrantes mediante el algoritmo Round Robin.

## Función principal
`config_load(const char *filepath, Config *config)`

Esta función se encarga de cargar y parsea la configuración desde un archivo.

````c
int config_load(const char *filepath, Config *config);
````

### Flujo de funcionamiento
- Apertura del archivo
    - Se intenta abrir el archivo en modo lectura
        ````c
        FILE *f = fopen(filepath, "r");
        ````
    - Si falla → retorna error
        ````c
        if (!f) {
            printf("[CONFIG] No se pudo abrir %s\n", filepath);
            return -1;
        }
        ````

- Lectura línea por línea

    ````c
    while (fgets(line, sizeof(line), f))
    ````

    Se procesa el archivo como texto plano, sin usar un parser JSON formal.

- Extracción de parámetros
    - Puerto del proxy
        ````c
        if (!in_servers && strstr(line, "\"port\"")) {
            sscanf(line, " \"port\": %d", &config->proxy_port);
        }
        ````
    - TTL del cache
        ````c
        if (strstr(line, "\"ttl\"")) {
            sscanf(line, " \"ttl\": %d", &config->ttl);
        }
        ````
    - Servidores backend
        Se detecta la sección "servers" y luego se extraen IP y puerto:
        ````c
        if (in_servers && strstr(line, "\"ip\"")) {
            char ip_buf[16];
            int p;
            if (sscanf(line, " {\"ip\": \"%15[^\"]\", \"port\": %d}", ip_buf, &p) == 2) {
                strncpy(config->servers[config->num_servers].ip, ip_buf, 16);
                config->servers[config->num_servers].port = p;
                config->num_servers++;
            }
        }
        ````
## Variables clave
- `in_servers`: Indica si se está dentro del bloque "servers" del JSON
- `num_servers`: Contador de servidores cargados
- `line`: Buffer para leer cada línea del archivo

## Retorno
- 0 → Configuración cargada correctamente
- -1 → Error al abrir el archivo
