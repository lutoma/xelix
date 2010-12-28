#pragma once

#include <common/generic.h>


// STRING FUNCTIONS (similiar to the string.h C standard library)
int strcmp(const char *s1, const char *s2);
char *strcat(char *dest, const char *src);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char * str);
char* substr(char* src, size_t start, size_t len);
char * strtok(char *s, const char *delim);
char * strtok_r(char *s, const char *delim, char **last);
