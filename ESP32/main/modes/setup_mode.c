#include "setup_mode.h"
#include "http_website.h"
#include "led.h"
#include "wifi_common.h"
#include "str.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UTF8_MAX_SIZE        4
#define EXAMPLE_WIFI_SSID    CONFIG_ESP_WIFI_SSID
#define EXAMPLE_WIFI_PASS    CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "setup_mode";

// Globals required by http_website.c (declared extern there)
vstr wifi_name     = {NULL, 0};
vstr wifi_password = {NULL, 0};
int has_wifi       = 0;
static httpd_handle_t server_handle = NULL;
httpd_handle_t *SERVER = &server_handle;

static int handler_registered = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " connected, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " disconnected, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

static void wifi_init_softap(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    switch (wifi_mode) {
    case 0:
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_netif = esp_netif_create_default_wifi_ap();
        if (!wifi_netif) exit(800);
        wifi_mode = 1;
        break;
    case 2:
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy(wifi_netif);
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        wifi_netif = esp_netif_create_default_wifi_ap();
        if (!wifi_netif) exit(800);
        wifi_mode = 1;
        break;
    }

    if (!handler_registered) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
        handler_registered = 1;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid        = EXAMPLE_WIFI_SSID,
            .ssid_len    = strlen(EXAMPLE_WIFI_SSID),
            .channel     = EXAMPLE_WIFI_CHANNEL,
            .password    = EXAMPLE_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode    = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else
            .authmode    = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg     = { .required = true },
        },
    };
    if (strlen(EXAMPLE_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(TAG, "AP started — SSID: %s  channel: %d", EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_CHANNEL);
}

void run_setup_mode(void) {
    ESP_LOGI(TAG, "Entering setup mode");
    configure_led();

    wifi_name.data = malloc(98 * UTF8_MAX_SIZE);
    if (!wifi_name.data) exit(556);
    wifi_password.data = wifi_name.data + 33 * UTF8_MAX_SIZE;
    wifi_name.size     = 0;
    wifi_password.size = 0;

    wifi_init_softap();
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server_handle));

    // Blink LED while waiting for the user to connect and submit credentials
    while (!has_wifi) {
        blink_led();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI(TAG, "Credentials received — SSID: %.*s", wifi_name.size, wifi_name.data);

    stop_webserver(server_handle);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler);

    // Persist credentials to flash
    FILE *f = fopen("/spiffs/wf", "wb");
    if (!f) exit(557);
    fwrite(&wifi_name.size,     1, 1,              f);
    fwrite(&wifi_password.size, 1, 1,              f);
    fwrite(wifi_name.data,      wifi_name.size,     1, f);
    fwrite(wifi_password.data,  wifi_password.size, 1, f);
    fclose(f);
    ESP_LOGI(TAG, "Credentials saved. Restarting into normal mode...");

    free(wifi_name.data);
    esp_restart();
}
