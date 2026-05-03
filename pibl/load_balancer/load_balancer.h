#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "../config/config.h"

#define MAX_SERVERS 10

typedef struct {
    char ip[16];
    int port;
} Server;

void lb_init(ServerConfig *s, int num);  // ← agregar
Server get_next_server();

extern Server servers[MAX_SERVERS];

#endif