#include "modes/normal_mode.h"
#include "modes/setup_mode.h"
#include "wifi_common.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

// Definitions for the shared WiFi state (declared extern in wifi_common.h)
esp_netif_t *wifi_netif = NULL;
int wifi_mode = 0;

static bool has_saved_credentials(void) {
    FILE *f = fopen("/spiffs/wf", "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path          = "/spiffs",
        .partition_label    = NULL,
        .max_files          = 2,
        .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));

    if (has_saved_credentials()) {
        run_normal_mode();
    } else {
        run_setup_mode();
    }
}
