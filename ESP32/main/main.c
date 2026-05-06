#include <stdio.h>
#include <sys/types.h>

#include "includes.h"

#include "networking/backend.h"
#include "networking/http_website.h"
#include "networking/wifi_provision.h"

#include "peripherals/camera.h"
#include "peripherals/led.h"

#include "utilities/factory_reset.h"
#include "utilities/jpeg.h"
#include "utilities/runtime.h"
#include "utilities/storage.h"
#include "utilities/tls.h"

#include "esp_task_wdt.h"

static int send_init_status(struct esp_tls *tls, char *receive_buff,
							size_t packet_size) {
	size_t packet_i = 0;
	int ret;

	while (packet_i != 4) {
		ret = esp_tls_conn_write(tls, ((char *)&packet_size) + packet_i,
								 4 - packet_i);
		if (ret <= 0) {
			puts("L385 send() tcp error");
			esp_tls_conn_destroy(tls);
			esp_camera_fb_return(fb);
			return -1;
		}
		packet_i += ret;
	}
	puts("Size sent");

	packet_i = 0;
	while (packet_i != packet_size) {
		ret = esp_tls_conn_write(tls, receive_buff + packet_i,
								 packet_size - packet_i);
		if (ret <= 0) {
			puts("L395 send() tcp error");
			esp_tls_conn_destroy(tls);
			esp_camera_fb_return(fb);
			return -1;
		}
		packet_i += ret;
	}
	puts("Buff sent");

	return 0;
}

static int init_camera_and_send_status(struct esp_tls *tls,
									   char *receive_buff) {
	esp_err_t ret = init_camera();
	vTaskDelay(pdMS_TO_TICKS(200));
	printf("Entering camera mode B %i\n", ret);

	size_t packet_size = sprintf(receive_buff, "init_camera() %i\n", ret);
	return send_init_status(tls, receive_buff, packet_size);
}

static struct esp_tls *startup_phase(char *receive_buff, int *client_fd_tcp) {
	prepare_runtime();

	struct esp_tls *tls = connect_tls_server();
	if (!tls) return NULL;

	configure_tls_socket(tls, client_fd_tcp);

	// Identify handshake: server sends 'I', we reply with our MAC address +
	// '\n'
	char cmd = 0;
	esp_tls_conn_read(tls, &cmd, 1);
	if (cmd == 'I') {
		uint8_t mac[6];
		esp_wifi_get_mac(WIFI_IF_STA, mac);
		char mac_str[20];
		snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X\n",
				 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		esp_tls_conn_write(tls, mac_str, strlen(mac_str));
	}

	if (init_camera_and_send_status(tls, receive_buff) != 0) return NULL;

	return tls;
}

void handle_command(struct esp_tls *tls, camera_fb_t *fb, char *receive_buff) {
	switch (*receive_buff) {
	case 'G':
		break;
	case 'M':
		break;
	case 'S':
		puts("Camera feed turned off");
		// th1_command=1;
		int wait = 1;
		while (wait) {
			ssize_t ret = esp_tls_conn_read(tls, receive_buff, 1);
			if (ret <= 0) {
				esp_camera_fb_return(fb);
				esp_tls_conn_destroy(tls);
				exit(400);
			}
			switch (*receive_buff) {
			case 'G':
				wait = 0;
				puts("Camera feed turned on");
				break;
			}
		}
		break;

	default:
		printf("Unknown command: %c\n", *receive_buff);
		puts("Unknown command");
		break;
	}
}

static int capture_frame(struct esp_tls *tls) {
	esp_err_t ret;

	fb = esp_camera_fb_get();
	if (!fb) {
		puts("esp_camera_fb_get() failed");
		ret = esp_tls_conn_write(tls, "\xff\xff\xff\xff", 4);
		if (ret <= 0) {
			esp_tls_conn_destroy(tls);
			esp_camera_fb_return(fb);
			return -1;
		}
		return 1;
	}

	return 0;
}

static int sync_and_handle_command(struct esp_tls *tls, char *receive_buff) {
	esp_err_t ret = esp_tls_conn_read(tls, receive_buff, 1);
	if (ret <= 0) {
		puts("Sync error");
		esp_tls_conn_destroy(tls);
		esp_camera_fb_return(fb);
		return -1;
	}

	handle_command(tls, fb, receive_buff);
	return 0;
}

static int send_frame_len(struct esp_tls *tls) {
	esp_err_t ret;
	size_t packet_i = 0;

	while (packet_i != 4) {
		ret = esp_tls_conn_write(tls, ((char *)&fb->len) + packet_i,
								 4 - packet_i);
		if (ret <= 0) {
			esp_camera_fb_return(fb);
			esp_tls_conn_destroy(tls);
			return -1;
		}
		packet_i += ret;
	}

	return 0;
}

static int send_frame_buf(struct esp_tls *tls) {
	esp_err_t ret;
	size_t packet_i = 0;

	while (packet_i != fb->len) {
		ret = esp_tls_conn_write(tls, fb->buf + packet_i, fb->len - packet_i);
		if (ret <= 0) {
			esp_camera_fb_return(fb);
			esp_tls_conn_destroy(tls);
			return -1;
		}
		packet_i += ret;
	}

	return 0;
}

static void release_frame(void) { esp_camera_fb_return(fb); }

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
		int ret2 = capture_frame(tls);

		if (ret2 < 0) return;

		if (sync_and_handle_command(tls, receive_buff) < 0) {
			// return;
		}

		if (send_frame_len(tls) < 0) return;
		if (send_frame_buf(tls) < 0) return;

		release_frame();
	}
}
