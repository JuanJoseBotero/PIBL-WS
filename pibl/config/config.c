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

int in_servers = 0;

while (fgets(line, sizeof(line), f)) {
    if (strstr(line, "\"servers\"")) {
        in_servers = 1;
    }

    if (!in_servers && strstr(line, "\"port\"")) {
        sscanf(line, " \"port\": %d", &config->proxy_port);
    }

    if (strstr(line, "\"ttl\"")) {
        sscanf(line, " \"ttl\": %d", &config->ttl);
    }

    if (in_servers && strstr(line, "\"ip\"")) {
        char ip_buf[16];
        int p;
        if (sscanf(line, " {\"ip\": \"%15[^\"]\", \"port\": %d}", ip_buf, &p) == 2) {
            strncpy(config->servers[config->num_servers].ip, ip_buf, 16);
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