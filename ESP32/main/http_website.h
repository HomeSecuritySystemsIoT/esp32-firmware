/*
 * http_website.h
 *
 *  Created on: 11 kwi 2026
 *      Author: Dawid
 */

#ifndef MAIN_HTTP_WEBSITE_H_
#define MAIN_HTTP_WEBSITE_H_
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"
#include "str.h"
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
extern int has_wifi;
esp_err_t stop_webserver(httpd_handle_t server);
extern httpd_handle_t *SERVER;
#endif /* MAIN_HTTP_WEBSITE_H_ */
