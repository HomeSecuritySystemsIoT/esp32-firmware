#include "utilities/storage.h"

#include "../networking/soft_ap_sub.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include <stdio.h>

void init_system_and_storage(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
		ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_netif_set_hostname(wifi_netif, "myesp32.xws");

	esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
								  .partition_label = NULL,
								  .max_files = 1,
								  .format_if_mount_failed = true};
	ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		printf("esp_vfs_spiffs_register(): %u\n", ret);
	}

	puts("esp_vfs_spiffs_register(): OK");
}
