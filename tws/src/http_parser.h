#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define BUFFER_SIZE  4096
#define METHOD_SIZE  8
#define PATH_SIZE    512
#define VERSION_SIZE 16


typedef struct {
    char method[METHOD_SIZE];
    char path[PATH_SIZE];
    char version[VERSION_SIZE];
    char raw[BUFFER_SIZE];
    int  content_length;
} HttpRequest;


int  parse_http_request(int client_fd, HttpRequest *req);
void handle_request(int client_fd, HttpRequest *req, const char *root_dir);
void send_200(int client_fd, const char *filepath);
void send_400(int client_fd);
void send_404(int client_fd);

#endif