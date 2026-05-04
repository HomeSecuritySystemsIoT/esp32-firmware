#include "includes.h"
#include "soft_ap_sub.h"
#include "station_wifi.h"
#include "tls.h"

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
	// goto huj;
	// if(!remove("/spiffs/wf")){
	//	puts("removed");
	// }
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

static void setup_wifi(void) {
	FILE *wififile = fopen("/spiffs/wf", "rb");
	wifi_name.data = (char *)malloc(98 * UTF8_MAX_SIZE);
	if (!wifi_name.data) exit(556);

	wifi_password.data = wifi_name.data + 33 * UTF8_MAX_SIZE;
	wifi_name.size = 0;
	wifi_password.size = 0;

	int wifi_getter = 1;

	// the esp32 has wifi credential to connect to the internet
	if (wififile) {
		puts("Found cache file");
		fread(&wifi_name.size, 1, 1, wififile);
		fread(&wifi_password.size, 1, 1, wififile);
		fread(wifi_name.data, wifi_name.size, 1, wififile);
		fread(wifi_password.data, wifi_password.size, 1, wififile);
		fclose(wififile);

		wifi_name.data[wifi_name.size] = 0;
		wifi_password.data[wifi_password.size] = 0;

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
			fwrite(wifi_name.data, wifi_name.size, 1, wififile);
			fwrite(wifi_password.data, wifi_password.size, 1, wififile);
			fclose(wififile);
			puts("Saved to temp file");
		}
	}

	free(wifi_name.data);
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

	if (init_camera_and_send_status(tls, receive_buff) != 0) return NULL;

	return tls;
}

void handle_command(struct esp_tls *tls, camera_fb_t *fb, char *receive_buff) {
	switch (*receive_buff) {
	case 'P':
		// just to keep alive the connection
		puts("Pong");
		break;
	case 'I':
		// puts("0");
		break;
	case 'G':
		// puts("th0 attempts to unlock mutex");
		// if(th1_command==0)
		//	pthread_mutex_unlock(&snapshot_muted);
		// puts("th0 unlocked mutex");
		break;
	case 'M':
		// turns on motion detection
		// th1_command=0;
		puts("Motion detection turned on");
		// puts("th0 attempts to unlock mutex");
		// pthread_mutex_unlock(&snapshot_muted);
		// puts("th0 unlocked mutex");
		break;
	case 'N':
		// turns off motion detection
		puts("Motion detection turned off");
		// th1_command=1;
		// analysis_init=0;
		break;
	case 'S':
		// esp_camera_fb_return(fb);
		// esp_camera_deinit();
		// fb=NULL;
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
			case 'M':
				// turns on motion detection
				puts("Motion detection turned on");
				// th1_command=0;
				break;
			case 'N':
				puts("Motion detection turned off");
				// turns off motion detection
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

	prepare_runtime();

	char receive_buff[1025];
	receive_buff[1024] = 0;

	// socket(AF_INET,SOCK_DGRAM,ip_protocol)
	// socket(AF_INET,SOCK_STREAM,ip_protocol)
	int client_fd_tcp;

	struct esp_tls *tls = startup_phase(receive_buff, &client_fd_tcp);
	if (!tls) {
		return;
	}

	// pthread_mutex_init(&snapshot_muted, NULL);

	/*
	xTaskCreatePinnedToCore(
		(TaskFunction_t)th1_func, // Your function (remember to cast it)
		"CameraAnalyzer",         // Name for the debugger
		16384,                    // STACK SIZE (in bytes for the S3)
		(void*)client_fd_udp,     // Argument
		5,                        // Priority
		NULL,                     // Task handle
		1                         // Core (Core 1)
	);
	*/

	print_system_info();
	esp_task_wdt_delete(NULL);

	while (1) {
		int ret2 = capture_frame(tls);

		if (ret2 < 0) return;
		if (ret2 > 0) continue;

		if (sync_and_handle_command(tls, receive_buff) < 0) return;
		if (send_frame_len(tls) < 0) return;
		if (send_frame_buf(tls) < 0) return;
		release_frame();
	}
}
