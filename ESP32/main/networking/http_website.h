/*
 * http_website.h
 *
 *  Created on: 11 kwi 2026
 *      Author: Dawid
 */

#ifndef NETWORKING_HTTP_WEBSITE_H_
#define NETWORKING_HTTP_WEBSITE_H_
#include "str_utility.h"
#include <esp_event.h>
#include <esp_https_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>
size_t cast_hex(size_t t);
void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
					 void *event_data);
extern vstr wifi_name, wifi_password;
extern vstr claim_token;
extern vstr backend_url;
extern int has_wifi;
httpd_handle_t start_webserver(void);
esp_err_t stop_webserver(httpd_handle_t server);
extern httpd_handle_t *SERVER;
#endif /* NETWORKING_HTTP_WEBSITE_H_ */
