# PIBL-WS — Proxy Inverso + Balanceador de Carga + Web Server

**Curso:** Telemática/Internet: Arquitectura y Protocolos  
**Universidad:** EAFIT  
**Integrantes:** Juan José Botero · Samuel Villa · Felipe Castro Jaiems  
**Fecha de entrega:** Mayo 6 de 2026

---

## Documentación por componente

| Componente | Descripción | Documentación |
|------------|-------------|---------------|
| PIBL | Proxy Inverso + Balanceador de Carga | [pibl/README.md](./docs/PIBL.md) |
| TWS | Telematics Web Server | [tws/README.md](./tws/README.md) |
| AWS | Infraestructura y despliegue | [aws/README.md](./aws/README.md) |

---

## i. Introducción

Este proyecto implementa un sistema distribuido cliente/servidor compuesto por un Proxy Inverso con Balanceador de Carga (PIBL) y un servidor web propio denominado Telematics Web Server (TWS), desplegado sobre infraestructura de Amazon Web Services.

El sistema fue desarrollado en lenguaje C utilizando la API POSIX Sockets, siguiendo la especificación del protocolo HTTP/1.1 definida en el RFC 2616. La arquitectura replica el modelo de operación de sistemas de producción reales: un punto de entrada único que distribuye el tráfico hacia múltiples servidores de backend, con soporte para caché persistente, logging completo y manejo concurrente de clientes.

El objetivo central del proyecto es desarrollar habilidades en la programación de aplicaciones distribuidas, comprendiendo en profundidad los mecanismos de comunicación cliente/servidor, el protocolo HTTP/1.1, la gestión de concurrencia mediante hilos POSIX, y el despliegue de sistemas en infraestructura cloud.

---

## ii. Desarrollo

### Arquitectura general

El sistema está compuesto por cuatro instancias EC2 en AWS, todas dentro de la misma VPC y subred para comunicación por red privada:

| Instancia | Rol | IP Privada | IP Pública |
|-----------|-----|------------|------------|
| pibl-server | Proxy Inverso + Balanceador | 172.31.16.223 | 98.88.100.236 |
| backend-1 | Servidor TWS | 172.31.21.179 | — |
| backend-2 | Servidor TWS | 172.31.29.96 | — |
| backend-3 | Servidor TWS | 172.31.26.98 | — |

Solo el PIBL tiene IP pública. Los backends son accesibles únicamente desde la red privada, a través del PIBL.

### Componente PIBL

El Proxy Inverso + Balanceador de Carga recibe todas las peticiones HTTP entrantes en el puerto 8080 y las distribuye hacia los tres backends usando el algoritmo Round Robin. Para cada petición, el PIBL abre un nuevo socket cliente hacia el backend seleccionado, reenvía la petición íntegra, espera la respuesta y la retorna al cliente original.

El PIBL implementa caché persistente en disco: cada respuesta recibida de un backend se almacena en el directorio `cache_storage/` con un TTL configurable (60 segundos por defecto). Las peticiones subsecuentes al mismo recurso dentro del TTL son atendidas directamente desde disco sin consultar el backend. Todas las peticiones y respuestas se registran en stdout y en archivo de log con marca de tiempo, fuente (SERVIDOR o CACHE) y código de estado.

La configuración del PIBL se define en `config-pibl.json`, permitiendo parametrizar el puerto de escucha, el TTL del caché y la lista de backends con sus IPs y puertos.

### Componente TWS

El Telematics Web Server implementa un servidor HTTP/1.1 capaz de procesar los métodos GET, HEAD y POST. Cada conexión entrante es atendida por un hilo POSIX independiente (`pthread`), lo que permite el manejo concurrente de múltiples clientes sin bloqueo.

El servidor analiza la línea de petición HTTP, valida el método y la versión del protocolo, construye la ruta del archivo solicitado concatenando el DocumentRoot con el path de la URL, y responde con el recurso o con el código de error correspondiente (200, 400 o 404). El método HEAD responde con los mismos headers que GET pero sin body, siguiendo estrictamente la especificación RFC 2616. Todas las peticiones se registran en un archivo de log con IP del cliente, método, path y código de respuesta.

### Concurrencia

Ambos componentes implementan manejo concurrente mediante hilos POSIX. El servidor principal acepta conexiones en un loop continuo y delega cada conexión a un hilo independiente, que se encarga de parsear la petición, procesarla y cerrar el socket al finalizar. Los hilos se configuran en modo detached para liberar recursos automáticamente al terminar.

### Despliegue en AWS

La infraestructura fue desplegada en AWS EC2 con Ubuntu Server 22.04 LTS. La segmentación de red se implementó mediante dos Security Groups: SG-PIBL, que expone el puerto 8080 al público, y SG-Backend, que restringe el acceso al puerto 8081 exclusivamente desde SG-PIBL, bloqueando cualquier acceso directo desde internet hacia los backends.

Para más detalles de despliegue, configuración y scripts de automatización, ver [docs/Deploy_&_Testing.md](./docs/Deploy_&_Testing.md).

---

## iii. Conclusiones

El desarrollo de este proyecto permitió comprender en profundidad el funcionamiento del protocolo HTTP/1.1 a nivel de implementación, más allá de su uso como protocolo de aplicación. Parsear manualmente la línea de petición, los headers y el body, y construir respuestas bien formadas según el RFC 2616, exige una precisión que no es evidente cuando se trabaja con frameworks de alto nivel.

La implementación del caché persistente en disco reveló la importancia de definir con precisión el alcance de cada componente: el PIBL debe diferenciar entre métodos HTTP al momento de cachear, para evitar servir una respuesta GET cacheada ante una petición HEAD. Este tipo de casos borde son difíciles de anticipar en el diseño pero críticos en la práctica.

El despliegue en AWS demostró que la infraestructura de red tiene implicaciones directas sobre el comportamiento del software: la forma en que se pasan los argumentos al servidor, los permisos de los Security Groups, y la distinción entre IPs públicas y privadas son detalles que afectan el funcionamiento del sistema de extremo a extremo.

Finalmente, la experiencia de trabajar con un equipo de tres personas en un proyecto de sistemas distribuidos reforzó la importancia de definir interfaces claras entre componentes desde el inicio, de modo que cada integrante pueda desarrollar y probar su parte de forma independiente antes de la integración.

---

## iv. Referencias

- Fielding, R. et al. (1999). *RFC 2616 — Hypertext Transfer Protocol HTTP/1.1*. IETF. https://datatracker.ietf.org/doc/rfc2616/
- Stevens, W. R., Fenner, B., & Rudoff, A. M. (2003). *UNIX Network Programming, Volume 1: The Sockets Networking API* (3rd ed.). Addison-Wesley.
- Kerrisk, M. (2010). *The Linux Programming Interface*. No Starch Press.
- Amazon Web Services. (2024). *Amazon EC2 User Guide*. https://docs.aws.amazon.com/ec2/
- IEEE POSIX. *pthreads(7) — POSIX threads*. Linux man-pages. https://man7.org/linux/man-pages/man7/pthreads.7.html