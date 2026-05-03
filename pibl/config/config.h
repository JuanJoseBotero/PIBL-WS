#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SERVERS 10

typedef struct {
    char ip[16];
    int port;
} ServerConfig;

typedef struct {
    int proxy_port;
    int ttl;
    ServerConfig servers[MAX_SERVERS];
    int num_servers;
} Config;

int config_load(const char *filepath, Config *config);

#endif