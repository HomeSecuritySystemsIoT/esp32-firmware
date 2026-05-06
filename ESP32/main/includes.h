/*
 * includes.h
 *
 *  Created on: 1 maj 2026
 *      Author: Dawid
 */

#ifndef MAIN_INCLUDES_H_
#define MAIN_INCLUDES_H_

#include "server_reacher.h"

#include "esp_pthread.h"
#include "esp_task_wdt.h"
#define PTRSIZE sizeof(void *)
#include <string.h>
// #include <sys/_pthreadtypes.h>
#include "camera.h"
#include "jpeg.h"
#include "led.h"
#include "station_wifi.h"
#include "wifi_common.h"
#define UTF8_MAX_SIZE 4
#include "esp_camera.h"
#include "esp_event.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_website.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define CONFIG_EXAMPLE_CONNECT_WIFI

#endif /* MAIN_INCLUDES_H_ */
