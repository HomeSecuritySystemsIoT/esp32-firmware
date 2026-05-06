#include "utilities/runtime.h"

#include "esp_heap_caps.h"
#include "esp_pthread.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void init_threads(void) {
	esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
	cfg.stack_size = 16384;
	cfg.prio = 5;
	cfg.inherit_cfg = false;
	esp_pthread_set_cfg(&cfg);

	// pthread_t th1;
	pthread_attr_t attr_detached;
	pthread_attr_init(&attr_detached);
	pthread_attr_setdetachstate(&attr_detached, PTHREAD_CREATE_DETACHED);

	pthread_attr_setstacksize(&attr_detached, 16384);
}

static void sincronize_time_sntp(void) {
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_init();
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	while (timeinfo.tm_year < (2026 - 1900)) {
		vTaskDelay(pdMS_TO_TICKS(500));
		time(&now);
		localtime_r(&now, &timeinfo);
	}
	puts("Year is 2026");
}

void prepare_runtime(void) {
	sleep(10);
	sincronize_time_sntp();
}

void print_system_info(void) {
	printf("Free SRAM: %zu bytes\n",
		   heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
	printf("Free PSRAM: %zu bytes\n",
		   heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	printf("Largest SRAM block: %zu bytes\n",
		   heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}
