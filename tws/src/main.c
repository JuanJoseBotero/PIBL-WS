#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

int main(int argc, char *argv[]) {
    int port = 8081;
    char *root_dir = "./webapp";

    if (argc == 5) {
        /* ./tws --port 8081 --root ./webapp */
        if (strcmp(argv[1], "--port") == 0)
            port = atoi(argv[2]);
        if (strcmp(argv[3], "--root") == 0)
            root_dir = argv[4];
    } else if (argc == 3 && strcmp(argv[1], "--port") == 0) {
        port = atoi(argv[2]);
    }

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto inválido: %d\n", port);
        return 1;
    }
    
    int server_fd = create_server_socket(port);
    if (server_fd < 0) {
        fprintf(stderr, "No se pudo iniciar el servidor\n");
        return 1;
    }

    run_server(server_fd, root_dir);
    return 0;
}