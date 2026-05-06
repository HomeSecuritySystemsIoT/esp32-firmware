#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "includes.h"
#include "soft_ap_sub.h"
#include "station_wifi.h"
#include "tls.h"
#include "utilities/factory_reset.h"
#include <stdio.h>

// server information for tcp/tls connection
#define SERVER_IP "51.210.107.234"
#define SERVER_IP_LENGTH 14
#define SERVER_PORT 7893

static void init_system_and_storage(void) {
	// nvs_flash_erase();
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

static void init_threads(void) {
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

static void register_device_with_backend(void) {
	if (claim_token.size == 0 || backend_url.size == 0) {
		puts("No claim token, skipping registration");
		return;
	}

	uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	char device_id[18];
	snprintf(device_id, sizeof(device_id), "%02X:%02X:%02X:%02X:%02X:%02X",
			 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	char body[220];
	int body_len = snprintf(body, sizeof(body),
							"{\"device_id\":\"%s\",\"claim_token\":\"%.*s\"}",
							device_id, (int)claim_token.size, claim_token.data);

	char url[220];
	snprintf(url, sizeof(url), "%.*s/api/iot/register", (int)backend_url.size,
			 backend_url.data);

	printf("Registering device %s with %s\n", device_id, url);

	esp_http_client_config_t config = {
		.url = url,
		.method = HTTP_METHOD_POST,
		.crt_bundle_attach = esp_crt_bundle_attach,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (!client) {
		puts("http_client_init failed");
		return;
	}

	esp_http_client_set_header(client, "Content-Type", "application/json");
	esp_http_client_set_post_field(client, body, body_len);

	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		int status = esp_http_client_get_status_code(client);
		printf("Registration response: %d\n", status);
		if (status == 200) {
			puts("Registration successful, clearing claim token");
			claim_token.size = 0;
			backend_url.size = 0;
			// Rewrite SPIFFS without the token
			FILE *wf = fopen("/spiffs/wf", "wb");
			if (wf) {
				uint8_t zero = 0;
				fwrite(&wifi_name.size, 1, 1, wf);
				fwrite(&wifi_password.size, 1, 1, wf);
				fwrite(&zero, 1, 1, wf); // token_len = 0
				fwrite(&zero, 1, 1, wf); // url_len = 0
				fwrite(wifi_name.data, wifi_name.size, 1, wf);
				fwrite(wifi_password.data, wifi_password.size, 1, wf);
				fclose(wf);
			}
		}
	} else {
		printf("Registration failed: %s\n", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
}

static void setup_wifi(void) {
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

static void sincronize_time_sntp(void) {
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org"); // Time server
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

static void print_system_info(void) {

	printf("Free SRAM: %zu bytes\n",
		   heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

	// Free PSRAM memory (external - here you have 8MB in N16R8)
	printf("Free PSRAM: %zu bytes\n",
		   heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

	// Largest contiguous block (important if malloc returns NULL despite free
	// memory)
	printf("Largest SRAM block: %zu bytes\n",
		   heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

static void prepare_runtime(void) {
	sleep(10);
	sincronize_time_sntp();
}

static struct esp_tls *connect_tls_server(void) {
	esp_tls_cfg_t tls_cfg = get_tls_cfg();
	struct esp_tls *tls =
		tls_connect(SERVER_IP, SERVER_IP_LENGTH, SERVER_PORT, &tls_cfg);

	if (!tls) {
		puts("tls=NULL");
		return NULL;
	}

	return tls;
}

static void configure_tls_socket(struct esp_tls *tls, int *client_fd_tcp) {
	if (esp_tls_get_conn_sockfd(tls, client_fd_tcp) == ESP_OK) {
		int enable = 1;
		if (setsockopt(*client_fd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable,
					   sizeof(enable)) == 0) {
			printf("TCP_NODELAY enabled\n");
		} else {
			printf("Failed to set TCP_NODELAY\n");
		}
	}
}

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
	case 'S':
		// turns off camera feed
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
				// init_camera();
				break;
			}
		}
		break;

	default:
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
	init_threads();
	init_leds();
	setup_wifi();
	if (claim_token.size > 0) {
		register_device_with_backend();
	}
	free(wifi_name.data);
	prepare_runtime();

	xTaskCreate(reset_button_task, "reset_button_task", 4096, NULL, 5, NULL);

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
		if (send_frame_len(tls) < 0) {
			// return;
		}
		if (send_frame_buf(tls) < 0) {
			// return;
		}
		release_frame();
	}
}
