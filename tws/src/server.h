#ifndef SERVER_H
#define SERVER_H

int  create_server_socket(int port);
void run_server(int server_fd, const char *root_dir);

#endif