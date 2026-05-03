#ifndef LOGGER_H
#define LOGGER_H

void logger_init(const char *filepath);
void logger_close();
void log_transaction(const char *method, const char *path, const char *full_request, int status_code, int bytes, int from_cache);

#endif