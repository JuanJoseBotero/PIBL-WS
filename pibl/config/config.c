#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int config_load(const char *filepath, Config *config) {
    FILE *f = fopen(filepath, "r");
    if (!f) {
        printf("[CONFIG] No se pudo abrir %s\n", filepath);
        return -1;
    }

    char line[256];
    config->num_servers = 0;

    while (fgets(line, sizeof(line), f)) {
        // Puerto del proxy
        if (strstr(line, "\"port\"") && !strstr(line, "servers")) {
            sscanf(line, " \"port\": %d", &config->proxy_port);
        }
        // TTL
        if (strstr(line, "\"ttl\"")) {
            sscanf(line, " \"ttl\": %d", &config->ttl);
        }
        // Servidores backend
        if (strstr(line, "\"ip\"")) {
            sscanf(line, " \"ip\": \"%15[^\"]\"",
                config->servers[config->num_servers].ip);
        }
        if (strstr(line, "\"port\"") && config->num_servers < MAX_SERVERS) {
            int p;
            sscanf(line, " \"port\": %d", &p);
            // Solo asignar si ya tenemos ip (evitar confundir con proxy_port)
            if (strlen(config->servers[config->num_servers].ip) > 0) {
                config->servers[config->num_servers].port = p;
                config->num_servers++;
            }
        }
    }

    fclose(f);

    printf("[CONFIG] Puerto: %d | TTL: %ds | Servidores: %d\n",
        config->proxy_port, config->ttl, config->num_servers);

    return 0;
}