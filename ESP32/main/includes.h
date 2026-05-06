/*
 * includes.h
 *
 *  Created on: 1 maj 2026
 *      Author: Dawid
 */

#ifndef MAIN_INCLUDES_H_
#define MAIN_INCLUDES_H_
#define PTRSIZE sizeof(void *)
#define UTF8_MAX_SIZE 4

#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define CONFIG_EXAMPLE_CONNECT_WIFI
static const char *TAG = "wifi softAP";

#endif /* MAIN_INCLUDES_H_ */
