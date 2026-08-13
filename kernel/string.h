#ifndef STRING_H
#define STRING_H
#include <stddef.h>

size_t k_strlen(const char *s);
int k_strcmp(const char *a, const char *b);
int k_strcasecmp(const char *a, const char *b);
int k_strncasecmp(const char *a, const char *b, size_t n);
char *k_strchr(const char *s, char c);

#endif
