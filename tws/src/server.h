#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <pthread.h>


typedef struct {
    int client_fd;
    char root_dir[256];
    char client_ip[INET_ADDRSTRLEN];
} ClientArgs;

int  create_server_socket(int port);
void run_server(int server_fd, const char *root_dir);

#endif