/*
 * server_reacher.h
 *
 *  Created on: 26 kwi 2026
 *      Author: Dawid
 */

#ifndef MAIN_SERVER_REACHER_H_
#define MAIN_SERVER_REACHER_H_

int tcp_connect_server(const char *host, const char *port);

int udp_connect_server(const char *host, const char *port);

#endif /* MAIN_SERVER_REACHER_H_ */
