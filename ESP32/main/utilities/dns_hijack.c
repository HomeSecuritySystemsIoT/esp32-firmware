#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <string.h>

static void dns_hijack_task(void *pvParameters) {
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		ESP_LOGE("dns_hijack", "socket() failed: %d", errno);
		vTaskDelete(NULL);
		return;
	}

	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(53),
		.sin_addr.s_addr = htonl(INADDR_ANY),
	};
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		ESP_LOGE("dns_hijack", "bind() failed: %d", errno);
		close(sock);
		vTaskDelete(NULL);
		return;
	}
	ESP_LOGI("dns_hijack", "listening on UDP port 53");

	uint8_t buf[512];
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	while (1) {
		int len = recvfrom(sock, buf, sizeof(buf), 0,
						   (struct sockaddr *)&client_addr, &client_len);
		if (len < 12) continue;

		// Build a DNS response pointing everything to 192.168.4.1
		uint8_t resp[512];
		memcpy(resp, buf, len);
		resp[2] = 0x81;
		resp[3] = 0x80; // flags: response, no error
		resp[6] = 0x00;
		resp[7] = 0x01; // 1 answer

		int resp_len = len;
		// Pointer back to the question name
		resp[resp_len++] = 0xC0;
		resp[resp_len++] = 0x0C;
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x01; // type A
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x01; // class IN
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x3C; // TTL 60s
		resp[resp_len++] = 0x00;
		resp[resp_len++] = 0x04; // rdlength 4
		// 192.168.4.1
		resp[resp_len++] = 192;
		resp[resp_len++] = 168;
		resp[resp_len++] = 4;
		resp[resp_len++] = 1;

		sendto(sock, resp, resp_len, 0, (struct sockaddr *)&client_addr,
			   client_len);
	}
	close(sock);
	vTaskDelete(NULL);
}

void start_dns_hijack(void) {
	static int started = 0;
	if (started) return;
	started = 1;
	ESP_LOGI("dns_hijack", "start dns hijack task");
	xTaskCreate(dns_hijack_task, "dns_hijack", 4096, NULL, 5, NULL);
}
