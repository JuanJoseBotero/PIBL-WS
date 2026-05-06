# Módulo: Cache (cache.c)
## Descripción

Este módulo implementa un sistema de cache basado en archivos en disco, el cual permite almacenar respuestas HTTP para reutilizarlas en futuras solicitudes.

El sistema utiliza un mecanismo de expiración mediante TTL (Time To Live) y está diseñado para ser thread-safe usando mutex, permitiendo su uso en un entorno concurrente.

## Características principales
- Almacenamiento en disco (cache_storage/)
- Expiración de cache mediante TTL
- Control de concurrencia con pthread_mutex
- Indexación basada en el path del request
- Eliminación automática de archivos expirados

## Variables globales
````c
static int ttl_global = 60;
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
````
`ttl_global:` Tiempo de vida de los elementos en cache (en segundos)

`cache_mutex:` Mutex para evitar condiciones de carrera en acceso concurrente

## Inicialización
`cache_init(int ttl_segundos)`

Inicializa el sistema de cache.

````c
void cache_init(int ttl_segundos) {
    ttl_global = ttl_segundos; // Se le pone el valor del TTL que viene del config, si no se especifica, se queda con el valor por defecto de 60 segundos
    mkdir("cache_storage", 0755); // Crear carpeta cache si no existe
    printf("[CACHE] Inicializado | TTL: %ds | Directorio: ./cache_storage/\n", ttl_global);
}
````
Esta función se encarga de:
- Configura el TTL global
- Crear el directorio cache_storage/ si no existe


## Generación de clave de almacenamiento
`key_to_filepath(const char *key, char *filepath)`

Convierte un path HTTP en un nombre de archivo válido.

````c
static void key_to_filepath(const char *key, char *filepath) {
    char sanitized[256];                                //  Se crea una copia del key para sanitizarlo sin modificar el original
    strncpy(sanitized, key, sizeof(sanitized) - 1);     // Copia lo que esté en key a sanitized, asegurando no exceder el tamaño del buffer
    sanitized[sizeof(sanitized) - 1] = '\0';            // Asegura que el string esté null-terminated

    // Para cada carácter del path, si es '/', reemplazar por '_'. Porque '/' no es válido en nombres de archivos.
    for (int i = 0; sanitized[i]; i++) {
        if (sanitized[i] == '/') sanitized[i] = '_';    // Reemplaza '/' por '_'
    }

    // Construir filepath final: "cache_storage/_index.html"
    snprintf(filepath, 512, "cache_storage/%s", sanitized);
}
````
El paso a paso que ejecuta esta función es:
- Copia el path original
- Asegurar que el string sea valido agregando al final el caracter null-terminated
- Revisa este path y con un ciclo pasa caracter por caracter, si encuentra un `/` lo reemplaza por `_`. Asegurando que sea un nombre valido para un archivo
- Construye la ruta final dentro de cache_storage/

**Ejemplo:**
/index.html → cache_storage/_index.html

## Obtención de datos desde cache
`cache_get(const char *key, char *buffer, int *size)`

Esta funcion se encarga de buscar un recurso en cache.

````c
int cache_get(const char *key, char *buffer, int *size);
````

### Flujo de funcionamiento:
- Bloquea el acceso con mutex
    ````c
    pthread_mutex_lock(&cache_mutex);
    ````
- Genera el filepath a partir del key
    ````c
    char filepath[512];
    key_to_filepath(key, filepath);
    ````
- Verifica si el archivo existe:
    ````c
    // Verificar si el archivo existe en disco
    struct stat file_metadata;
    if (stat(filepath, &file_metadata) != 0) { // stat(filepath, &file_metadata), filepath es el nombre del archivo a verificar, file_metadata es una variable donde se guarda metadata del archivo (tamaño, fecha de modificación, etc).
        printf("[CACHE] MISS (no existe): %s\n", key);
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }
    ````
Verifica si el archivo ha expirado:
- Calcula edad con difftime
    ````c
    // Verificar TTL comparando tiempo actual con última modificación del archivo
    time_t now = time(NULL);
    double edad = difftime(now, file_metadata.st_mtime);
    ````
- Si supera el TTL:
    ````c
    if (edad > ttl_global) {
        remove(filepath); // Expirado → eliminar del disco
        printf("[CACHE] MISS (expirado hace %.0fs): %s\n", edad - ttl_global, key);
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }
    ````
    - Elimina el archivo
    - Retorna CACHE MISS
- Si es válido:
    ````c
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("[CACHE] MISS (error al abrir): %s\n", key);
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }

    *size = fread(buffer, 1, 4096, f);
    fclose(f);

    printf("[CACHE] HIT (edad: %.0fs / TTL: %ds): %s\n", edad, ttl_global, key);
    pthread_mutex_unlock(&cache_mutex);
    return 1;
    ````
    - Abre el archivo
    - Lee su contenido en buffer
    - Retorna CACHE HIT
- Retorno:
    - 1 → Cache HIT
    - 0 → Cache MISS


## Almacenamiento en cache
`cache_set(const char *key, const char *data, int size)`

Esta funcion se encarga de tomar la respuesta que mando el servidor backend y guardarla en cache.

````c
void cache_set(const char *key, const char *data, int size);
````

### Flujo de funcionamiento:
- Bloquea el acceso con mutex
    ````c
    pthread_mutex_lock(&cache_mutex);
    ````
- Genera el filepath
    ````c
    char filepath[512];
    key_to_filepath(key, filepath);
    ````
- Abre archivo en modo escritura (wb)
    ````c
    FILE *f = fopen(filepath, "wb");
    ````
    - Si no se abre archivo reportar error
        ````c
        if (!f) {
            printf("[CACHE] ERROR al guardar: %s\n", key);
            pthread_mutex_unlock(&cache_mutex);
            return;
        }
        ````
- Escribe los datos con fwrite
    ````c
    fwrite(data, 1, size, f);
    ````
- Cierra el archivo
    ````c
    fclose(f);
    ````

## Concurrencia

El módulo utiliza `pthread_mutex_lock(&cache_mutex);` y `pthread_mutex_unlock(&cache_mutex);`

Esto garantiza que no haya múltiples escrituras simultáneas, no se lea un archivo mientras se está escribiendo y se eviten condiciones de carrera