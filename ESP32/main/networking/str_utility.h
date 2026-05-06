#ifndef NETWORKING_STR_UTILITY
#define NETWORKING_STR_UTILITY
#include <stddef.h>
struct vstr_t {
	char *data;
	size_t size;
};
#define vstr struct vstr_t

struct qvstr_t {
	size_t size;
	char data[];
};
#endif
