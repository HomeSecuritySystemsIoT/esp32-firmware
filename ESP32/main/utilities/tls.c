#include "utilities/tls.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "lwip/sockets.h"
#include <stdio.h>
#include <string.h>

#define SERVER_IP "51.210.107.234"
#define SERVER_IP_LENGTH 14
#define SERVER_PORT 7893

// Odwołania do osadzonych plików
extern const uint8_t ca_cert_start[] asm("_binary_ca_cert_crt_start");
extern const uint8_t ca_cert_end[] asm("_binary_ca_cert_crt_end");
extern const uint8_t client_crt_start[] asm("_binary_esp32_cert_crt_start");
extern const uint8_t client_crt_end[] asm("_binary_esp32_cert_crt_end");
extern const uint8_t client_key_start[] asm("_binary_esp32_private_key_start");
extern const uint8_t client_key_end[] asm("_binary_esp32_private_key_end");

esp_tls_cfg_t get_tls_cfg() {

	esp_tls_cfg_t ret;
	memset(&ret, 0, sizeof(ret));
	ret.cacert_buf = ca_cert_start;
	ret.cacert_bytes = ca_cert_end - ca_cert_start;
	ret.clientcert_buf = client_crt_start;
	ret.clientcert_bytes = client_crt_end - client_crt_start;
	ret.clientkey_buf = client_key_start;
	ret.clientkey_bytes = client_key_end - client_key_start;
	ret.skip_common_name = 1;

	return ret;
}

struct esp_tls *tls_connect(const char *hostname, int hostname_len, int port,
							esp_tls_cfg_t *cfg) {

	struct esp_tls *tls = esp_tls_init();

	int ret = esp_tls_conn_new_sync(hostname, hostname_len, port, cfg, tls);
	if (ret == 1) {
		printf("Succesfully connected to %.*s:%i\n", hostname_len, hostname,
			   port);
	} else {
		int esp_tls_code = 0, mbedtls_code = 0;

		esp_tls_error_handle_t tls_error_handle;
		esp_tls_get_error_handle(tls, &tls_error_handle);
		esp_tls_get_and_clear_last_error(tls_error_handle, &esp_tls_code,
										 &mbedtls_code);

		printf("Failed to connect to %.*s:%i ESP-TLS: 0x%x, mbedTLS: 0x%x",
			   hostname_len, hostname, port, esp_tls_code, -mbedtls_code);
		esp_tls_conn_destroy(tls);
		return NULL;
	}

	return tls;
}

struct esp_tls *connect_tls_server(void) {
	esp_tls_cfg_t tls_cfg = get_tls_cfg();
	struct esp_tls *tls =
		tls_connect(SERVER_IP, SERVER_IP_LENGTH, SERVER_PORT, &tls_cfg);
	if (!tls) {
		puts("tls=NULL");
		return NULL;
	}
	return tls;
}

void configure_tls_socket(struct esp_tls *tls, int *client_fd_tcp) {
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
