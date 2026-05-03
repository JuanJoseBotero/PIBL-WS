#ifndef CACHE_H
#define CACHE_H

void cache_init(int ttl_segundos);
int  cache_get(const char *key, char *buffer, int *size);  // 1 = HIT, 0 = MISS
void cache_set(const char *key, const char *data, int size);

#endif