#include "string.h"

size_t k_strlen(const char *s)
{
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int k_strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static char lower(char c)
{
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

int k_strcasecmp(const char *a, const char *b)
{
    while (*a && (lower(*a) == lower(*b))) { a++; b++; }
    return (unsigned char)lower(*a) - (unsigned char)lower(*b);
}

int k_strncasecmp(const char *a, const char *b, size_t n)
{
    while (n && *a && (lower(*a) == lower(*b))) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)lower(*a) - (unsigned char)lower(*b);
}

char *k_strchr(const char *s, char c)
{
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    return 0;
}
