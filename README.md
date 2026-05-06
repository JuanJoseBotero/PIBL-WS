# PIBL-WS

## Arquitectura de Despliegue

El sistema fue desplegado en AWS usando 4 instancias EC2 en la región us-east-1, todas dentro de la misma VPC y subred para comunicación por red privada.

| Instancia | Rol | IP Privada | IP Pública |
|-----------|-----|------------|------------|
| pibl-server | Proxy Inverso + Balanceador | 172.31.16.223 | 98.88.100.236 |
| backend-1 | Servidor TWS | 172.31.21.179 | — |
| backend-2 | Servidor TWS | 172.31.29.96 | — |
| backend-3 | Servidor TWS | 172.31.26.98 | — |

Solo el PIBL tiene IP pública. Los backends únicamente son accesibles desde la red privada, específicamente desde el PIBL.

---

## Configuración de Red y Seguridad

Se crearon dos Security Groups en AWS:

**SG-PIBL** — aplicado a la instancia del proxy:
- Puerto 8080 abierto al público (0.0.0.0/0) para recibir tráfico de clientes
- Puerto 22 restringido a IP del administrador para SSH

**SG-Backend** — aplicado a las 3 instancias de backend:
- Puerto 8081 abierto únicamente desde SG-PIBL, bloqueando acceso directo desde internet
- Puerto 22 restringido a IP del administrador para SSH

---

## Instrucciones de Despliegue

### Prerrequisitos
- Cuenta AWS activa con permisos EC2
- Par de llaves `.pem` configurado con `chmod 400`
- Git instalado en las instancias

### 1. Clonar el repositorio en cada instancia

```bash
sudo apt update && sudo apt install -y git gcc make
git clone https://github.com/[REPO]/pibl-ws.git
```

### 2. Compilar y lanzar el TWS en cada backend
```bash
cd ~/pibl-ws/tws
make
./tws [PUERTO] [LOGFILE] [DOCUMENT_ROOT] &
```
Para verificar que está escuchando:
```bash
ss -tlnp | grep [PUERTO]
```

### 3. Verificar/Configurar el PIBL
Editar ```config-pibl.json``` con las IPs privadas reales de los backends:
```bash
{
    "port": 8080,
    "ttl": 60,
    "servers": [
        {"ip": "172.31.21.179", "port": 8081},
        {"ip": "172.31.29.96", "port": 8081},
        {"ip": "172.31.26.98", "port": 8081}
    ]
}
```

### 4. Compilar y lanzar el PIBL
```bash
cd ~/pibl-ws/pibl
make
screen -dmS pibl ./pibl config-pibl.json
ps aux | grep pibl      #Verificar que está corriendo
```

### 5. Porbar y Verificar el Sistema Completo
curl -v http://98.88.100.236:8080/      # Esperado: HTTP/1.1 200 OK

# Resultados de Pruebas — PIBL-WS
## Pruebas de Protocolo HTTP
| Prueba                  | Comando                                      | Resultado         | Observaciones                    |
| ----------------------- | -------------------------------------------- | ----------------- | -------------------------------- |
| GET /                   | `curl -v http://98.88.100.236:8080/`         | ✅ 200 OK          | Recurso entregado correctamente  |
| HEAD /                  | `curl -v -X HEAD http://98.88.100.236:8080/` | ⚠️ 200 OK         | Headers correctos, body no vacío |
| POST /                  | `curl -v -X POST ... -d "campo=valor"`       | ✅ 200 OK          | Body procesado correctamente     |
| GET recurso inexistente | `curl -v http://.../recurso.html`            | ✅ 404 Not Found   | Respuesta limpia                 |
| Método inválido         | `echo "INVALIDO /" \| nc ...`                | ✅ 400 Bad Request | Servidor no crashea              |

## Prueba de Round Robin
Se enviaron 9 peticiones consecutivas con URLs únicas para evitar caché. Distribución resultante:
| Backend   | IP Privada    | Peticiones recibidas |
| --------- | ------------- | -------------------- |
| backend-1 | 172.31.21.179 | req=2, req=5, req=8  |
| backend-2 | 172.31.29.96  | req=3, req=6, req=9  |
| backend-3 | 172.31.26.98  | req=1, req=4, req=7  |
Distribución exacta: 3 peticiones por backend. Round Robin funcionando correctamente.

## Prueba de Caché y TTL
TTL configurado: 60 segundos.
| Petición        | Fuente   | Observación                                            |
| --------------- | -------- | ------------------------------------------------------ |
| Primera         | SERVIDOR | Recurso solicitado al backend y almacenado en disco    |
| Segunda (< 60s) | CACHE    | Recurso servido desde disco sin contactar backend      |
| Tercera (> 60s) | SERVIDOR | TTL expirado, recurso solicitado nuevamente al backend |
Caché persistente ubicada en: ~/pibl-ws/pibl/cache_storage/

## Prueba de Concurrencia
Se lanzaron 20 peticiones simultáneas usando procesos en background:

```bash
for i in {1..20}; do
  curl -s "http://98.88.100.236:8080/?c=$i" -o /dev/null -w "Petición $i: %{http_code}\n" &
done
wait
```
Resultado: 20/20 peticiones respondidas. El servidor no presentó crashes ni timeouts bajo carga concurrente.

## Bugs Identificados Durante Pruebas
Durante la fase de pruebas se identificaron los siguientes bugs, reportados al equipo de desarrollo:

Bug 1 — TWS no sirve recursos en subdirectorios
El servidor responde 404 para cualquier path con subdirectorios (/caso1/index.html) aunque el archivo exista en disco. GET / funciona correctamente. Causa probable: concatenación incorrecta de DocumentRoot con el path de la petición.

Bug 2 — HEAD devuelve body
La respuesta al método HEAD incluye el body completo, violando la especificación RFC 2616 que exige cero bytes en el cuerpo. Los headers son correctos.

Bug 3 — PIBL realiza doble conexión por petición
Cada petición aparece registrada dos veces en el log de los backends, sugiriendo que el PIBL abre dos conexiones hacia el backend por cada petición recibida.