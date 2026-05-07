# Módulo: Load Balancer (load_balancer.c)
## Descripción

Este módulo implementa el sistema de balanceo de carga del servidor proxy.

Su función es distribuir las peticiones entrantes entre múltiples servidores backend utilizando el algoritmo Round Robin.

El módulo fue diseñado para funcionar en un entorno concurrente, utilizando mutex para garantizar acceso seguro a las variables compartidas.

## Características principales
- Balanceo de carga Round Robin
- Soporte para múltiples servidores backend
- Protección concurrente mediante mutex
- Selección rápida del siguiente servidor disponible

## Variables globales
````c
Server servers[MAX_SERVERS];
static int num_servers = 0;
static int current = 0;
static pthread_mutex_t lb_mutex = PTHREAD_MUTEX_INITIALIZER;
````

- ``servers[]``: Lista de servidores backend disponibles
- ``num_servers``: Cantidad de servidores registrados
- ``current``: Índice del siguiente servidor a utilizar
- ``lb_mutex``: Mutex utilizado para proteger el acceso concurrente

## 1. Inicialización del balanceador
``lb_init(ServerConfig *s, int num)``

Inicializa la lista de servidores backend.

````c
void lb_init(ServerConfig *s, int num) {
    num_servers = num;
    for (int i = 0; i < num; i++) {
        strncpy(servers[i].ip, s[i].ip, 16);
        servers[i].port = s[i].port;
    }
}
````
### Funcionalidad

1. Guarda la cantidad de servidores disponibles
2. Copia:
    - Dirección IP
    - Puerto
3. Almacena la información en el arreglo global servers[]

## 2.Selección de servidor
``get_next_server()``

Selecciona el siguiente servidor backend utilizando Round Robin.

````c
Server get_next_server() {
    pthread_mutex_lock(&lb_mutex);

    int selected = current;
    current = (current + 1) % num_servers; // round-robin

    // Unlock antes del return para no dejar el mutex trabado
    pthread_mutex_unlock(&lb_mutex);
    return servers[selected];
}
````

### Algoritmo Round Robin
````c
int selected = current;
current = (current + 1) % num_servers;
````

El algoritmo selecciona servidores de manera secuencial:

````
Servidor 1 → Servidor 2 → Servidor 3 → Servidor 1 → ...
````

El operador módulo (%) permite reiniciar el índice cuando se alcanza el último servidor.

### Concurrencia

El acceso a la variable current está protegido mediante mutex:

````c
pthread_mutex_lock(&lb_mutex);
pthread_mutex_unlock(&lb_mutex);
````
Esto evita:

- Condiciones de carrera
- Selecciones inconsistentes
- Acceso simultáneo por múltiples threads