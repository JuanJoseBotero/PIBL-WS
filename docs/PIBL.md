# PIBL - Proxy Inteligente con Balanceo y Cache
Este proyecto implementa un servidor proxy concurrente en C que incluye:

- Balanceo de carga (Round Robin)
- Sistema de cache con TTL
- Registro de logs (logger)
- Manejo concurrente de clientes con hilos (pthreads)

El proxy recibe solicitudes HTTP de clientes, decide si responder desde cache o reenviar la petición a un servidor backend, y devuelve la respuesta al cliente.

## Acá pongo algun diagrama y explico como es el flujo del pibl

## Estructura del proyecto

```
/project
│
├── cache/
│ ├── cache.c
│ └── cache.h
│
├── cache_storage/
│ └── (archivos de almacenamiento de cache en disco)
│
├── config/
│ ├── config.c
│ └── config.h
│
├── load_balancer/
│ ├── load_balancer.c
│ └── load_balancer.h
│
├── logger/
│ ├── logger.c
│ └── logger.h
│
├── config-pibl.json
├── Makefile
├── pibl.c
└── pibl.log
```

El proyecto fue diseñado siguiendo un enfoque modular, separando cada responsabilidad en componentes independientes:

- **cache/** → Manejo del sistema de cache con TTL  
- **load_balancer/** → Selección de servidores (Round Robin)  
- **config/** → Lectura y manejo de configuración desde JSON  
- **logger/** → Registro de transacciones del proxy  
- **cache_storage/** → (Opcional) Persistencia de datos en cache  

Esta división permite:

- Mejor mantenimiento del código  
- Mayor escalabilidad  
- Facilidad para probar cada componente de forma independiente  
- Reutilización de módulos en otros proyectos 

## Funcionamiento Principal

### 1. Inicialización

En ```main()``` se carga la configuración desde config-pibl.json

Se inicializan:
- Cache (cache_init)
- Balanceador (lb_init)
- Logger (logger_init)
- Se crea el socket del proxy
- Se pone en modo escucha (listen)