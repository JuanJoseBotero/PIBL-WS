#include "load_balancer.h"
#include <string.h>

Server servers[MAX_SERVERS];
static int num_servers = 0;
static int current = 0;

void lb_init(ServerConfig *s, int num) {
    num_servers = num;
    for (int i = 0; i < num; i++) {
        strncpy(servers[i].ip, s[i].ip, 16);
        servers[i].port = s[i].port;
    }
}

Server get_next_server() {
    int selected = current;
    current = (current + 1) % num_servers; // round-robin
    return servers[selected];
}