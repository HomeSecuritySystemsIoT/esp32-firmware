/*
 * tls.h
 *
 *  Created on: 3 maj 2026
 *      Author: Dawid
 */

#ifndef MAIN_TLS_H_
#define MAIN_TLS_H_

#include "esp_tls.h"
#include "esp_log.h"

esp_tls_cfg_t get_tls_cfg();
struct esp_tls* tls_connect(const char* hostname,int hostname_len,int port,esp_tls_cfg_t*cfg);

#endif /* MAIN_TLS_H_ */
