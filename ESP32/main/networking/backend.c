#include "networking/backend.h"

#include "networking/http_website.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include <stdint.h>
#include <stdio.h>

void register_device_with_backend(void) {
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
			FILE *wf = fopen("/spiffs/wf", "wb");
			if (wf) {
				uint8_t zero = 0;
				fwrite(&wifi_name.size, 1, 1, wf);
				fwrite(&wifi_password.size, 1, 1, wf);
				fwrite(&zero, 1, 1, wf);
				fwrite(&zero, 1, 1, wf);
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
