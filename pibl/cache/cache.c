#include "cache.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

static int ttl_global = 60; // Valor por defecto, se sobreescribe con cache_init()

// Inicializa la cache creando el directorio de almacenamiento y configurando el TTL
void cache_init(int ttl_segundos) {
    ttl_global = ttl_segundos; // Se le pone el valor del TTL que viene del config, si no se especifica, se queda con el valor por defecto de 60 segundos
    mkdir("cache_storage", 0755); // Crear carpeta cache si no existe
    printf("[CACHE] Inicializado | TTL: %ds | Directorio: ./cache_storage/\n", ttl_global);
}

// Convierte el path a nombre de archivo válido
// Ej: "/index.html" → "cache/_index.html"
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

/*
Esta función lo que hace es:
1. Convertir la clave (path del request) a un nombre de archivo válido usando key_to_filepath().
2. Verificar si el archivo existe en disco usando stat(). Si no existe, es un cache MISS.
3. Si el archivo existe, verificar su edad comparando el tiempo actual con la última modificación del archivo. Si es más viejo que el TTL, considerarlo expirado, eliminarlo del disco y retornar cache MISS.
4. Si el archivo es válido (existe y no está expirado), abrirlo, leer su contenido en el buffer, cerrar el archivo y retornar cache HIT
*/
int cache_get(const char *key, char *buffer, int *size) {
    char filepath[512];
    key_to_filepath(key, filepath);

    // Verificar si el archivo existe en disco
    struct stat file_metadata;
    if (stat(filepath, &file_metadata) != 0) { // stat(filepath, &file_metadata), filepath es el nombre del archivo a verificar, file_metadata es una variable donde se guarda metadata del archivo (tamaño, fecha de modificación, etc). 
        printf("[CACHE] MISS (no existe): %s\n", key);
        return 0;
    }

    // Verificar TTL comparando tiempo actual con última modificación del archivo
    time_t now = time(NULL);
    double edad = difftime(now, file_metadata.st_mtime); // difftime devuelve la diferencia en segundos entre el tiempo actual y la última modificación del archivo.

    // Si el archivo es más viejo que el TTL, considerarlo expirado
    if (edad > ttl_global) {
        remove(filepath); // Expirado → eliminar del disco
        printf("[CACHE] MISS (expirado hace %.0fs): %s\n", edad - ttl_global, key);
        return 0;
    }

    // Leer archivo del disco y cargar en buffer
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        printf("[CACHE] MISS (error al abrir): %s\n", key);
        return 0;
    }

    *size = fread(buffer, 1, 4096, f);
    fclose(f);

    printf("[CACHE] HIT (edad: %.0fs / TTL: %ds): %s\n", edad, ttl_global, key);
    return 1;
}

/*  
Esta funcion basicamente lo que hace es:

1. Convertir la clave (path del request) a un nombre de archivo válido usando key_to_filepath().
2. Abrir el archivo en modo escritura binaria ("wb"). Si no se puede abrir, imprime un error y retorna.
3. Escribir los datos en el archivo usando fwrite().
4. Cerrar el archivo.
5. Imprimir un mensaje indicando que se ha guardado en disco, mostrando el tamaño de los datos, la clave original y el filepath donde se guardó.
*/
void cache_set(const char *key, const char *data, int size) {
    char filepath[512];
    key_to_filepath(key, filepath);

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        printf("[CACHE] ERROR al guardar: %s\n", key);
        return;
    }

    fwrite(data, 1, size, f);
    fclose(f);

    printf("[CACHE] Guardado en disco (%d bytes): %s → %s\n", size, key, filepath);
}