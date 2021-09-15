/*
 * Helper functions
 */

#ifndef HELPER_H
#define HELPER_H

static inline size_t roundpow2(size_t n);

/*
 * template for assign function:
 *
 * void assign_<type>(void* dest, void* src)
 * {
 * 	*((<type>*)dest) = *((<type>*)src);
 * }
 *
 */

void* gmemcpy(void* dest, void* src, size_t n, void(*assign)(void*, void*));

char* mystrcat(char* dest, char* src);

char* strtokstr(char* str, char* sep);

char* strappend(char* str, char* app);

char* strappend_r(char* str, char* app);

char* strsub(char* s, char* ns, char* ss);
#endif
