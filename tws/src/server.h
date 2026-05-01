#ifndef SERVER_H
#define SERVER_H

typedef struct {
    int client_fd;
    char root_dir[256];
} ClientArgs;

int  create_server_socket(int port);
void run_server(int server_fd, const char *root_dir);

#endif