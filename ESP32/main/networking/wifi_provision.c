#include "networking/wifi_provision.h"

#include "networking/http_website.h"
#include "networking/soft_ap_sub.h"
#include "networking/station_wifi.h"
#include "peripherals/led.h"
#include "utilities/dns_hijack.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>

#define UTF8_MAX_SIZE 4

void setup_wifi(void) {
	FILE *wififile = fopen("/spiffs/wf", "rb");
	wifi_name.data = (char *)malloc(98 * UTF8_MAX_SIZE);
	if (!wifi_name.data) exit(556);

	wifi_password.data = wifi_name.data + 33 * UTF8_MAX_SIZE;
	wifi_name.size = 0;
	wifi_password.size = 0;

	claim_token.data = malloc(65);
	claim_token.size = 0;
	backend_url.data = malloc(129);
	backend_url.size = 0;

	int wifi_getter = 1;

	// the esp32 has wifi credential to connect to the internet
	if (wififile) {
		puts("Found cache file");
		uint8_t token_len = 0, url_len = 0;
		fread(&wifi_name.size, 1, 1, wififile);
		fread(&wifi_password.size, 1, 1, wififile);
		// Try to read token_len and url_len (new format); old files won't have
		// them
		if (fread(&token_len, 1, 1, wififile) == 1 &&
			fread(&url_len, 1, 1, wififile) == 1) {
			fread(wifi_name.data, wifi_name.size, 1, wififile);
			fread(wifi_password.data, wifi_password.size, 1, wififile);
			if (token_len > 0 && token_len <= 64) {
				fread(claim_token.data, token_len, 1, wififile);
				claim_token.size = token_len;
			}
			if (url_len > 0 && url_len <= 128) {
				fread(backend_url.data, url_len, 1, wififile);
				backend_url.size = url_len;
			}
		} else {
			// Old format: rewind and read without length bytes
			fseek(wififile, 2, SEEK_SET);
			fread(wifi_name.data, wifi_name.size, 1, wififile);
			fread(wifi_password.data, wifi_password.size, 1, wififile);
		}
		fclose(wififile);

		wifi_name.data[wifi_name.size] = 0;
		wifi_password.data[wifi_password.size] = 0;
		if (claim_token.size < 65) claim_token.data[claim_token.size] = 0;
		if (backend_url.size < 129) backend_url.data[backend_url.size] = 0;

		wifi_getter = wifi_init_sta(&wifi_name, &wifi_password);
		if (wifi_getter) {
			vTaskDelay(pdMS_TO_TICKS(5000));
			wifi_getter = wifi_init_sta(&wifi_name, &wifi_password);
		}
	}

	httpd_handle_t server = NULL;

	// the esp32 waits for connection to receive wifi credentials
	while (wifi_getter) {
		has_wifi = 0;
		wifi_init_softap();
		start_dns_hijack();

		ESP_ERROR_CHECK(esp_event_handler_register(
			WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));

		while (!has_wifi) {
			blink_led();
			vTaskDelay(pdMS_TO_TICKS(10));
		}

		stop_webserver(*SERVER);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED,
									 &connect_handler);

		wifi_destroy();
		*SERVER = NULL;

		vTaskDelay(pdMS_TO_TICKS(5000));
		printf("Attempting to find : %s with password %s\n", wifi_name.data,
			   wifi_password.data);

		wifi_getter = wifi_init_sta(&wifi_name, &wifi_password);
		if (wifi_getter) {
			vTaskDelay(pdMS_TO_TICKS(16000));
		} else {
			wififile = fopen("/spiffs/wf", "wb");
			if (!wififile) {
				exit(557);
			}

			esp_wifi_set_ps(WIFI_PS_NONE);
			fwrite(&wifi_name.size, 1, 1, wififile);
			fwrite(&wifi_password.size, 1, 1, wififile);
			fwrite(&claim_token.size, 1, 1, wififile);
			fwrite(&backend_url.size, 1, 1, wififile);
			fwrite(wifi_name.data, wifi_name.size, 1, wififile);
			fwrite(wifi_password.data, wifi_password.size, 1, wififile);
			if (claim_token.size > 0)
				fwrite(claim_token.data, claim_token.size, 1, wififile);
			if (backend_url.size > 0)
				fwrite(backend_url.data, backend_url.size, 1, wififile);
			fclose(wififile);
			puts("Saved to temp file");
		}
	}

	set_led(0, 0, 0);
}
