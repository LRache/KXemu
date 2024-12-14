#include <klib.h>
#include <stdint.h>

size_t strlen(const char *s) {
    size_t i = 0;
    while (*s++) i++;
    return i;
}

char *strcpy(char *dst, const char *src) {
    char *p = (char*)src, *d = dst;
    while (*p) *(d++) = *(p++);
    *d = 0;
    return dst;  
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    while (i < n) dst[i++] = 0;
    return dst;
}

char *strcat(char *dst, const char *src) {
    char *d = dst;
    while (*d) d++;
    for (char *p = (char *)src; *p; p++, d++) {
        *d = *p;
    }
    *d = 0;
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    unsigned char *p1 = (unsigned char *)s1, *p2 = (unsigned char *)s2;
    for (; *p1 && *p2; p1++, p2++) {
        if (*p1 != *p2) return *p1 - *p2;
    }
    if (*p1) return 1;
    if (*p2) return -1;
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    unsigned char *p1 = (unsigned char *)s1, *p2 = (unsigned char *)s2;
    size_t i;
    for (i = 0; i < n && *p1 && *p2; i++, p1++, p2++) {
        if (*p1 != *p2) return *p1 - *p2;
    }
    if (i == n) return 0;
    if (*p1) return 1;
    if (*p2) return -1;
    return 0;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    for (size_t i = 0; i < n; i++, p++) *p = (unsigned char) c;
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char p[n];
    unsigned char *t = p, *d = dst, *s = (unsigned char *)src;
    for (int i = 0; i < n; i++, t++, s++) *t = *s;
    t = p;
    for (int i = 0; i < n; i++, d++, t++) *d = *t;
    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    unsigned char *o = out, *i = (unsigned char *)in;
    for (size_t j = 0; j < n; j++, o++, i++) *o = *i;
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    unsigned char *p1 = (unsigned char *)s1, *p2 = (unsigned char *)s2;
    for (size_t i = 0; i < n; i++, p1++, p2++) {
        if (*p1 != *p2) return *p1 - *p2;
    }
    return 0;
}
