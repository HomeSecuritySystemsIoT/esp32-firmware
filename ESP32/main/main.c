

#include "includes.h"
#include "soft_ap_sub.h"
#include "station_wifi.h"
#include "tls.h"

void app_main(void) {
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

	esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
	cfg.stack_size = 16384;
	cfg.prio = 5;
	cfg.inherit_cfg = false;
	esp_pthread_set_cfg(&cfg);

	pthread_t th1;
	pthread_attr_t attr_detached;
	pthread_attr_init(&attr_detached);
	pthread_attr_setdetachstate(&attr_detached, PTHREAD_CREATE_DETACHED);

	pthread_attr_setstacksize(&attr_detached, 16384);

	puts("esp_vfs_spiffs_register(): OK");
	FILE *wififile = fopen("/spiffs/wf", "rb");
	wifi_name.data = (char *)malloc(98 * UTF8_MAX_SIZE);
	if (!wifi_name.data)
		exit(556);
	wifi_password.data = wifi_name.data + 33 * UTF8_MAX_SIZE;
	wifi_name.size = 0;
	wifi_password.size = 0;
	int wifi_getter = 1;
	configure_led();
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
	// Initialize NVS

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
	// huj:
	// wifi_init_softap();

	time_t start, end;
	sleep(10);

	char receive_buff[1025];
	int err;

	// socket(AF_INET,SOCK_DGRAM,ip_protocol)
	// socket(AF_INET,SOCK_STREAM,ip_protocol)
	int client_fd_tcp;
	receive_buff[1024] = 0;

	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org"); // Serwer czasu
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

	esp_tls_cfg_t tls_cfg = get_tls_cfg();
	struct esp_tls *tls = tls_connect("51.210.107.234", 14, 7893, &tls_cfg);

	if (!tls) {
		puts("tls=NULL");
		return;
	}

	if (esp_tls_get_conn_sockfd(tls, &client_fd_tcp) == ESP_OK) {
		int enable = 1;
		if (setsockopt(client_fd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable,
					   sizeof(enable)) == 0) {
			printf("TCP_NODELAY enabled\n");
		} else {
			printf("Failed to set TCP_NODELAY\n");
		}
	}

	// if (client_fd_udp == -1) {
	//	puts("client_fd_udp=-1");
	//	return;
	// }

	size_t packet_size, packet_i = 0;
	int r, g, b;

	ret = init_camera();
	vTaskDelay(pdMS_TO_TICKS(200));
	printf("Entering camera mode B %i\n", ret);
	packet_size = sprintf(receive_buff, "init_camera() %i\n", ret);

	/*recv(client_fd_tcp, receive_buff+1023, 1, 0);
	if(receive_buff[1023]=='2'){
		puts("HandShake");
	}
	else{
		puts("Nima hendszejka");
	}*/
	// send(client_fd_tcp,"4",1,0);
	// printf("client_fd_tcp %i\n",client_fd_tcp);
	packet_i = 0;
	while (packet_i != 4) {
		ret = esp_tls_conn_write(tls, ((char *)&packet_size) + packet_i,
								 4 - packet_i);
		if (ret <= 0) {
			puts("L385 send() tcp error");
			esp_tls_conn_destroy(tls);
			esp_camera_fb_return(fb);
			return;
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
			return;
		}
		packet_i += ret;
	}
	puts("Buff sent");

	// jpeg_init();

	printf("Wolna SRAM: %d bajtow\n",
		   heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

	// Wolna pamięć PSRAM (zewnętrzna - tu masz 8MB w N16R8)
	printf("Wolna PSRAM: %d bajtow\n",
		   heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

	// Największy ciągły blok (ważne, jeśli malloc zwraca NULL mimo wolnego
	// miejsca)
	printf("Największy blok SRAM: %d bajtów\n",
		   heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

	// pthread_mutex_init(&snapshot_muted, NULL);

	/*
	xTaskCreatePinnedToCore(
(TaskFunction_t)th1_func, // Twoja funkcja (pamiętaj o rzutowaniu)
"CameraAnalyzer",         // Nazwa dla debuggera
16384,                    // ROZMIAR STOSU (w bajtach dla S3)
(void*)client_fd_udp,     // Argument
5,                        // Priorytet
NULL,                     // Task handle
1                         // Rdzeń (Core 1)
);*/
	esp_task_wdt_delete(NULL);
	// puts("th1 created");

	// th1_command=1;

	while (1) {
		// start=clock();
		// if(th1_command==0){
		// puts("th0 attempts to lock mutex");
		// pthread_mutex_lock(&snapshot_muted);
		// puts("th0 locked mutex");
		//}

		fb = esp_camera_fb_get();
		if (!fb) {
			puts("esp_camera_fb_get() failed");
			ret = esp_tls_conn_write(tls, "\xff\xff\xff\xff", 4);
			if (ret <= 0) {
				esp_tls_conn_destroy(tls);
				esp_camera_fb_return(fb);
				return;
			}
			// if(th1_command==0){
			// puts("th0 attempts to unlock mutex");
			//	pthread_mutex_unlock(&snapshot_muted);
			// puts("th0 unlocked mutex");
			//}
			continue;
		}
		// end=clock();
		// printf("snapshot %lli\n",end-start);
		// start=clock();
		// puts("L447");
		ret = esp_tls_conn_read(tls, receive_buff, 1);
		if (ret <= 0) {
			puts("Sync error");
			esp_tls_conn_destroy(tls);
			esp_camera_fb_return(fb);
			return;
		}
		// printf("Command : %c\n",*receive_buff);
		switch (*receive_buff) {
		case 'P':
			// puts("Pong!");
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
				ret = esp_tls_conn_read(tls, receive_buff, 1);
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
					// if(th1_command==0)
					//	pthread_mutex_unlock(&snapshot_muted);
					break;
				case 'M':
					// turns on motion detection
					puts("Motion detection turned on");
					// th1_command=0;
					break;
				case 'N':
					puts("Motion detection turned off");
					// turns off motion detection
					// th1_command=1;
					// analysis_init=0;
					break;
				}
			}
			// if(th1_command==0)
			//	pthread_mutex_unlock(&snapshot_muted);  //Necessary,but test
			break;

		default:
			puts("Unknown command");
			break;
		}

		// end=clock();
		// printf("sync %lli\n",end-start);

		// start=clock();

		packet_i = 0;
		while (packet_i != 4) {
			// puts("L535");
			ret = esp_tls_conn_write(tls, ((char *)&fb->len) + packet_i,
									 4 - packet_i);
			if (ret <= 0) {
				esp_camera_fb_return(fb);
				esp_tls_conn_destroy(tls);
				return;
			}
			packet_i += ret;
		}
		packet_i = 0;
		// memset(counts,0,1024);
		while (packet_i != fb->len) {
			// puts("L545");
			ret =
				esp_tls_conn_write(tls, fb->buf + packet_i, fb->len - packet_i);
			// printf("ret %i\n",ret);
			if (ret <= 0) {
				esp_camera_fb_return(fb);
				esp_tls_conn_destroy(tls);
				return;
			}
			// printf("%u / %u\n",packet_i,fb->len);
			packet_i += ret;
			// for (size_t i = packet_i-ret; i != packet_i; i++) {
			//		counts[fb->buf[i]]++;
			//}
		}
		// printf("%u / %u\n",fb->len,fb->len);
		// printf("%zu :",fb->len);
		// for (size_t i = 0; i != 256; i++) {
		///    	printf(" %zu:%i ",i,counts[i]);
		//}
		// puts("");

		// end=clock();

		esp_camera_fb_return(fb);
	}
}
