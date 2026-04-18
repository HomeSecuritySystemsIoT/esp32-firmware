/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_eth.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>
// #include "esp_camera.h"
#include "esp_tls.h"
#include "sdkconfig.h"
#include <esp_https_server.h>
// #include<CamS3Library.h>
/* A simple example that demonstrates how to create GET and POST
 * handlers and start an HTTPS server.
 */

size_t cast_hex(size_t t) {
	if (t >= '0' && t <= '9') {
		return t - '0';
	}

	if (t >= 'a' && t <= 'f') {
		return t - 'a' + 10;
	}
	if (t >= 'A' && t <= 'F') {
		return t - 'A' + 10;
	}
	return 0;
}

#include "http_website.h"
#include "str.h"
// vstr wifi_name,wifi_password;
// int has_wifi=0;
/* Event handler for catching system events */
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
				   void *event_data) {
	puts("event_handler");
	if (event_base == ESP_HTTPS_SERVER_EVENT) {
		if (event_id == HTTPS_SERVER_EVENT_ERROR) {
			esp_https_server_last_error_t *last_error =
				(esp_tls_last_error_t *)event_data;
			ESP_LOGE("event_handler",
					 "Error event triggered: last_error = %s, last_tls_err = "
					 "%d, tls_flag = %d",
					 esp_err_to_name(last_error->last_error),
					 last_error->esp_tls_error_code, last_error->esp_tls_flags);
		}
	}
}

/* An HTTP GET handler */
esp_err_t root_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");

	const char *html =
		"<!DOCTYPE html><html><head></head><body><h2>ESP32 "
		"WIFI</h2><form><label for=\"n\">Wifi SSID:</label><br><input "
		"type=\"text\"id=\"n\" name=\"n\"><br><br><label for=\"p\">WIFI "
		"Password:</label><br><input type=\"text\" id=\"p\" "
		"name=\"p\"><br><br><input type=\"submit\" "
		"value=\"Submit\"></form></body></html>";
	httpd_resp_send(req, html, 285); //-1 for undetermined
	if (req->uri[1] == '\0')
		return ESP_OK;

	puts(req->uri);

	char *temp = req->uri + 4, *temp2 = wifi_name.data;
	// printf("HUJJJJ %c %p\n",*temp,temp);
	wifi_name.size = 0;
	while (*temp && (*temp != '&') && (wifi_name.size != 32)) {
		switch (*temp) {
		case '+':
			temp++;
			*temp2++ = ' ';
			break;
		case '%':
			*temp2++ = (cast_hex(temp[1]) << 4) | cast_hex(temp[2]);
			// printf("%hhx %hhx %hhx\n", *temp2++, ((temp[1] - '0') << 4),
			// (temp[2] - '0'));
			temp += 3;
			break;
		default:
			*temp2++ = *temp++;
			break;
		}
		wifi_name.size++;
	}
	if (!wifi_name.size)
		return ESP_LOG_ERROR;
	wifi_name.data[wifi_name.size] = 0;
	wifi_password.size = 0;
	temp += 3;
	temp2 = wifi_password.data;
	while (*temp && (*temp != '&') && (wifi_password.size != 64)) {
		switch (*temp) {
		case '+':
			temp++;
			*temp2++ = ' ';
			break;
		case '%':
			*temp2++ = (cast_hex(temp[1]) << 4) | cast_hex(temp[2]);
			temp += 3;
			break;
		default:
			*temp2++ = *temp++;
			break;
		}
		wifi_password.size++;
	}
	if (!wifi_password.size)
		return ESP_LOG_ERROR;
	wifi_password.data[wifi_password.size] = 0;

	printf("\nGOT SSID %u %.*s password %u %.*s\n", wifi_name.size,
		   wifi_name.size, wifi_name.data, wifi_password.size,
		   wifi_password.size, wifi_password.data);
	// httpd_ssl_stop(req->handle);
	has_wifi = 1;
	return ESP_OK;
}

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
static void print_peer_cert_info(const mbedtls_ssl_context *ssl) {
	const mbedtls_x509_crt *cert;
	const size_t buf_size = 1024;
	char *buf = calloc(buf_size, sizeof(char));
	if (buf == NULL) {
		ESP_LOGE("print_peer_cert_info",
				 "Out of memory - Callback execution failed!");
		return;
	}

	// Logging the peer certificate info
	cert = mbedtls_ssl_get_peer_cert(ssl);
	if (cert != NULL) {
		mbedtls_x509_crt_info((char *)buf, buf_size - 1, "    ", cert);
		ESP_LOGI("print_peer_cert_info", "Peer certificate info:\n%s", buf);
	} else {
		ESP_LOGW("print_peer_cert_info",
				 "Could not obtain the peer certificate!");
	}

	free(buf);
}
#endif
/**
 * Example callback function to get the certificate of connected clients,
 * whenever a new SSL connection is created and closed
 *
 * Can also be used to other information like Socket FD, Connection state, etc.
 *
 * NOTE: This callback will not be able to obtain the client certificate if the
 * following config `Set minimum Certificate Verification mode to Optional` is
 * not enabled (enabled by default in this example).
 *
 * The config option is found here - Component config → ESP-TLS
 *
 */
static void
https_server_user_callback(esp_https_server_user_cb_arg_t *user_cb) {
	ESP_LOGI("https_server_user_callback", "User callback invoked!");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
	mbedtls_ssl_context *ssl_ctx = NULL;
#endif
	switch (user_cb->user_cb_state) {
	case HTTPD_SSL_USER_CB_SESS_CREATE:
		ESP_LOGD("https_server_user_callback", "At session creation");

		// Logging the socket FD
		int sockfd = -1;
		esp_err_t esp_ret;
		esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
		if (esp_ret != ESP_OK) {
			ESP_LOGE("https_server_user_callback",
					 "Error in obtaining the sockfd from tls context");
			break;
		}
		ESP_LOGI(TAG, "Socket FD: %d", sockfd);
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
		ssl_ctx = (mbedtls_ssl_context *)esp_tls_get_ssl_context(user_cb->tls);
		if (ssl_ctx == NULL) {
			ESP_LOGE("https_server_user_callback",
					 "Error in obtaining ssl context");
			break;
		}
		// Logging the current ciphersuite
		ESP_LOGI("https_server_user_callback", "Current Ciphersuite: %s",
				 mbedtls_ssl_get_ciphersuite(ssl_ctx));
#endif
		break;

	case HTTPD_SSL_USER_CB_SESS_CLOSE:
		ESP_LOGD("https_server_user_callback", "At session close");
#ifdef CONFIG_ESP_TLS_USING_MBEDTLS
		// Logging the peer certificate
		ssl_ctx = (mbedtls_ssl_context *)esp_tls_get_ssl_context(user_cb->tls);
		if (ssl_ctx == NULL) {
			ESP_LOGE("https_server_user_callback",
					 "Error in obtaining ssl context");
			break;
		}
		print_peer_cert_info(ssl_ctx);
#endif
		break;
	default:
		ESP_LOGE("https_server_user_callback", "Illegal state!");
		return;
	}
}
#endif

const httpd_uri_t root = {
	.uri = "/", .method = HTTP_GET, .handler = root_get_handler};

httpd_handle_t start_webserver(void) {
	httpd_handle_t server = NULL;

	// Start the httpd server
	ESP_LOGI("start_webserver", "Starting server");
	esp_err_t ret;
	httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

	// printf("%u %u %u %u %u\n",conf.max_uri_len
	// ,conf.max_resp_headers,conf.max_open_sockets
	// ,conf.stack_size,conf.max_header_len);
	extern const unsigned char servercert_start[] asm(
		"_binary_servercert_pem_start");
	extern const unsigned char servercert_end[] asm(
		"_binary_servercert_pem_end");
	conf.servercert = servercert_start;
	conf.servercert_len = servercert_end - servercert_start;

	extern const unsigned char prvtkey_pem_start[] asm(
		"_binary_prvtkey_pem_start");
	extern const unsigned char prvtkey_pem_end[] asm("_binary_prvtkey_pem_end");
	conf.prvtkey_pem = prvtkey_pem_start;
	conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
	conf.user_cb = https_server_user_callback;
#endif

	ret = httpd_ssl_start(&server, &conf);
	// httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	// ret=httpd_start(&server, &config);

	if (ESP_OK != ret) {
		ESP_LOGI("httpd_ssl_start", "Error starting server!");
		return NULL;
	}

	// Set URI handlers
	ESP_LOGI("httpd_ssl_start", "Registering URI handlers");
	httpd_register_uri_handler(server, &root);
	return server;
}

esp_err_t stop_webserver(httpd_handle_t server) {
	// Stop the httpd server
	return httpd_ssl_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
						int32_t event_id, void *event_data) {
	if (*SERVER) {
		if (stop_webserver(*SERVER) == ESP_OK) {
			SERVER = NULL;
		} else {
			ESP_LOGE("disconnect_handler", "Failed to stop https server");
		}
	}
	// puts("disconnect_handler");
}

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
					 void *event_data) {
	SERVER = (httpd_handle_t *)arg;
	if (*SERVER == NULL) {
		*SERVER = start_webserver();
	}
	// puts("connect_handler");
}