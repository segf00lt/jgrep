#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const size_t CHARSIZE = sizeof(char);

inline size_t roundpow2(size_t n) {
	size_t i = n;

	i--;
	i |= i >> 1;
	i |= i >> 2;
	i |= i >> 4;
	i |= i >> 8;
	i |= i >> 16;
	i++;

	size_t j = n >> 1;

	return (i - n) > (n - j) ? j : i;
}

char* mystrcat(char* dest, char* src) {
	while(*dest) ++dest;
	while(*dest++ = *src++);
	return --dest;
}

/*
 * Credit to ipserc
 * github: https://github.com/ipserc/strrep
 */

char* strtokstr(char* str, char* sep) {
	static char* ptr_1;
	static char* ptr_2;
	size_t sep_len = strlen(sep);
	
	if(!*sep) return str;
	if(str) ptr_1 = str;
	else {
		if(!ptr_2) return ptr_2;
		ptr_1 = ptr_2 + sep_len;
	}

	if(ptr_1) {
		ptr_2 = strstr(ptr_1, sep);
		if(ptr_2) memset(ptr_2, 0, sep_len); 
	}

	return ptr_1;
}

char* strappend(char* str, char* app) {
	char * newstr = (char*)malloc((strlen(str) + strlen(app) + 1) * CHARSIZE);
	sprintf(newstr, "%s%s", str, app);
	return newstr;
}

char* strappend_r(char* str, char* app) {
	size_t str_len = strlen(str);
	size_t app_len = strlen(app);
	str = (char*)realloc(str, (str_len + app_len + 1) * CHARSIZE);
	sprintf(str, "%s%s", str, app);
	return str;
}

char* strsub(char* s, char* ns, char* ss) {
	char* buf = NULL;
	char* cur = NULL;
	char* res = NULL;
	size_t s_len = strlen(s);

	buf = (char*)malloc(s_len + 1 * CHARSIZE);
	sprintf(buf, "%s", s);
	if(!*ns) return buf;

	cur = strtokstr(buf, ss);

	res = (char*)calloc(s_len + 1, CHARSIZE);

	while(cur) {
		res = strappend_r(res, cur);
		cur = strtokstr(NULL, ss);
		if(cur) res = strappend_r(res, ns);
	}

	free(buf);

	return res;
}
