#include <stdlib.h>

#include "networking/backend.h"
#include "networking/http_website.h"
#include "networking/wifi_provision.h"

#include "peripherals/led.h"
#include "peripherals/streaming.h"

#include "utilities/factory_reset.h"
#include "utilities/runtime.h"
#include "utilities/storage.h"

#include "esp_task_wdt.h"

void app_main(void) {
	init_system_and_storage();
	setup_factory_reset();

	init_threads();
	init_leds();
	setup_wifi();

	if (claim_token.size > 0) {
		register_device_with_backend();
	}

	free(wifi_name.data);
	prepare_runtime();

	char receive_buff[1025];
	receive_buff[1024] = 0;

	int client_fd_tcp;

	struct esp_tls *tls = startup_phase(receive_buff, &client_fd_tcp);
	if (!tls) {
		return;
	}

	print_system_info();
	esp_task_wdt_delete(NULL);

	while (1) {
		if (capture_frame(tls) < 0) return;

		int cmd_ret = sync_and_handle_command(tls, receive_buff);
		if (cmd_ret < 0) return;
		if (cmd_ret > 0) continue; // frame already released by stop handler

		if (send_frame_len(tls) < 0) return;
		if (send_frame_buf(tls) < 0) return;

		release_frame();
	}
}
