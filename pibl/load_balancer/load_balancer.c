#include "load_balancer.h"
#include <string.h>
#include <pthread.h>

Server servers[MAX_SERVERS];
static int num_servers = 0;
static int current = 0;
static pthread_mutex_t lb_mutex = PTHREAD_MUTEX_INITIALIZER;

void lb_init(ServerConfig *s, int num) {
    num_servers = num;
    for (int i = 0; i < num; i++) {
        strncpy(servers[i].ip, s[i].ip, 16);
        servers[i].port = s[i].port;
    }
}

Server get_next_server() {
    pthread_mutex_lock(&lb_mutex);

    int selected = current;
    current = (current + 1) % num_servers; // round-robin

    // Unlock antes del return para no dejar el mutex trabado
    pthread_mutex_unlock(&lb_mutex);
    return servers[selected];
}