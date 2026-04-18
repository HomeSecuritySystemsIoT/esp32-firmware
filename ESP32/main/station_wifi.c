/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_wifi_default.h"
#define CONFIG_ESP_WIFI_AUTH_WPA2_PSK 1
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "str.h"
#include <string.h>

/* The examples use WiFi configuration that you can set via project
   configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static int force_stop = 0;
static esp_event_handler_instance_t instance_any_id = NULL,
									instance_got_ip = NULL;
#include "wifi_common.h"
extern int wifi_mode;
void wifi_destroy() {
	force_stop = 1;
	esp_wifi_disconnect();
	if (instance_any_id) {
		esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
											  instance_any_id);
		instance_any_id = NULL;
	}
	if (instance_got_ip) {

		esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
											  instance_got_ip);
		instance_got_ip = NULL;
	}

	esp_wifi_stop();
	esp_wifi_deinit();
	esp_wifi_set_mode(WIFI_MODE_NULL);
	esp_netif_destroy(wifi_netif);
	wifi_netif = NULL;
	wifi_mode = 0;
}
static int s_retry_num = 0;

static void station_event_handler(void *arg, esp_event_base_t event_base,
								  int32_t event_id, void *event_data) {
	int e = event_id;
	printf("EVENT RECEIVED: %i\n", e);
	if (force_stop)
		return;

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT &&
			   event_id == WIFI_EVENT_STA_DISCONNECTED) {
		// if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
		// esp_wifi_connect();
		//   s_retry_num++;
		//    ESP_LOGI("event_handler", "retry to connect to the AP");
		//} else {
		wifi_event_sta_disconnected_t *disconnected =
			(wifi_event_sta_disconnected_t *)event_data;
		ESP_LOGE("WIFI_STATUS", "WYRZUCILO BŁĄD. Kod powodu: %d",
				 disconnected->reason);
		xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		//    force_stop=1;
		// }
		ESP_LOGI("event_handler", "connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI("event_handler", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

int connected_to_wifi;

static int station_initialising = 0;
int wifi_init_sta(vstr *name, vstr *password) {
	if (!station_initialising) {
		station_initialising = 1;
	}

	if (!s_wifi_event_group) {
		s_wifi_event_group = xEventGroupCreate();
	}

	force_stop = 0;
	s_retry_num = 0;

	// ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	switch (wifi_mode) {
	case 0:
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));
		// esp_wifi_set_storage(WIFI_STORAGE_RAM);
		wifi_netif = esp_netif_create_default_wifi_sta();
		if (!wifi_netif)
			exit(800);
		wifi_mode = 2;
		break;
	case 1:
		esp_wifi_stop();
		esp_wifi_deinit();
		esp_netif_destroy(wifi_netif);
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));
		wifi_netif = esp_netif_create_default_wifi_sta();
		if (!wifi_netif)
			exit(800);
		wifi_mode = 2;
		break;
	}

	if (!instance_any_id) {

		ESP_ERROR_CHECK(esp_event_handler_instance_register(
			WIFI_EVENT, ESP_EVENT_ANY_ID, &station_event_handler, NULL,
			&instance_any_id));
	}
	if (!instance_got_ip) {
		ESP_ERROR_CHECK(esp_event_handler_instance_register(
			IP_EVENT, IP_EVENT_STA_GOT_IP, &station_event_handler, NULL,
			&instance_got_ip));
	}
	wifi_config_t wifi_config = {
		.sta =
			{
				//.ssid = EXAMPLE_ESP_WIFI_SSID,
				//.password = EXAMPLE_ESP_WIFI_PASS,
				/* Authmode threshold resets to WPA2 as default if password
				 * matches WPA2 standards (password len => 8). If you want to
				 * connect the device to deprecated WEP/WPA networks, Please set
				 * the threshold value to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and
				 * set the password with length and format matching to
				 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
				 */
				.threshold.authmode =
					WIFI_AUTH_WPA2_PSK, // Ustaw konkretnie
										//.sae_pwe_h2e = ESP_WIFI_SAE_MODE,
										//.sae_h2e_identifier =
										//EXAMPLE_H2E_IDENTIFIER,
				.scan_method = WIFI_ALL_CHANNEL_SCAN,
				.failure_retry_cnt = 0,
				.pmf_cfg = {.capable = true, .required = false},

			},

	};

	memcpy(wifi_config.sta.ssid, name->data, name->size + (name->size != 32));

	memcpy(wifi_config.sta.password, password->data,
		   password->size + (password->size != 64));
	printf("%s %s\n", wifi_config.sta.ssid, wifi_config.sta.password);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	// printf("esp_wifi_set_config() : %i\n",esp_wifi_set_config(WIFI_IF_STA,
	// &wifi_config) );
	connected_to_wifi = 0;
	xEventGroupClearBits(s_wifi_event_group,
						 WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
	ESP_ERROR_CHECK(esp_wifi_start());
	esp_wifi_set_ps(WIFI_PS_NONE);
	ESP_LOGI("wifi_init_sta", "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
	 * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
	 * The bits are set by event_handler() (see above) */

	EventBits_t bits = xEventGroupWaitBits(
		s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
		pdFALSE, pdMS_TO_TICKS(10000));

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we
	 * can test which event actually happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI("wifi_init_sta", "connected to ap SSID:%.*s password:%.*s",
				 name->size, name->data, password->size, password->data);
		// connected_to_wifi=1;
		return 0;
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI("wifi_init_sta",
				 "Failed to connect to SSID:%.*s, password:%.*s", name->size,
				 name->data, password->size, password->data);
		// connected_to_wifi=1;

	} else {
		ESP_LOGE("wifi_init_sta", "UNEXPECTED EVENT");
	}

	wifi_destroy();
	return 1;
}