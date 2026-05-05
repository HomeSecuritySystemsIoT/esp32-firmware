// #include "../includes.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
// #include "esp_camera.h"
// #include "esp_rom_gpio.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <unistd.h>

#define RESET_BUTTON_GPIO GPIO_NUM_0 // typically BOOT, or choose another
#define RESET_HOLD_MS 5000

void factory_reset_wifi_file(void) {
	// SPIFFS must already be registered/mounted at "/spiffs"
	const char *path = "/spiffs/wf";

	printf("Attending to delete wifi credentials");
	int res = remove(path); // or unlink(path);
	if (res == 0) {
		ESP_LOGI("UTILITIES", "Deleted WiFi file: %s", path);
	} else {
		ESP_LOGW("UTILITIES", "Failed to delete %s (maybe it doesn't exist)",
				 path);
	}
}

void factory_reset() {
	factory_reset_wifi_file();
	printf("sleep for 2 seconds\n");
	sleep(2);
	esp_restart();
}

void reset_button_task(void *arg) {
	gpio_config_t io_conf = {.pin_bit_mask = 1ULL << RESET_BUTTON_GPIO,
							 .mode = GPIO_MODE_INPUT,
							 .pull_up_en = GPIO_PULLUP_ENABLE,
							 .pull_down_en = GPIO_PULLDOWN_DISABLE,
							 .intr_type = GPIO_INTR_DISABLE};

	gpio_config(&io_conf);

	while (1) {
		int level = gpio_get_level(RESET_BUTTON_GPIO);
		if (level == 0) { // assuming active-low
			printf("detected pulse of BOOT button, hold for 5 seconds to reset "
				   "the device\n");

			int64_t start = esp_timer_get_time(); // microseconds
			while (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
				if ((esp_timer_get_time() - start) / 1000 >= RESET_HOLD_MS) {
					printf("reseting device\n");
					factory_reset();
				}
				vTaskDelay(pdMS_TO_TICKS(50));
			}
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
