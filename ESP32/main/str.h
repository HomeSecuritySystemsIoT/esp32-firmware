/*
 * str.h
 *
 *  Created on: 1 kwi 2026
 *      Author: Dawid
 */

#ifndef MAIN_STR_H_
#define MAIN_STR_H_
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

#endif /* MAIN_STR_H_ */
