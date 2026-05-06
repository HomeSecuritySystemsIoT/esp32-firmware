/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "mbedtls/base64.h"
#include "protocol_examples_common.h"
#include "sdkconfig.h"
#include <esp_event.h>
#include <esp_https_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

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
httpd_handle_t *SERVER;
vstr wifi_name, wifi_password;
vstr claim_token, backend_url;
int has_wifi = 0;

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

/* Decodes a URL-encoded field value from src into dst (max max_len bytes).
   Stops at '&', end of string, or max_len. Returns number of bytes written. */
static int url_decode_field(const char *src, char *dst, int max_len) {
	int n = 0;
	while (*src && *src != '&' && n < max_len) {
		if (*src == '+') {
			*dst++ = ' ';
			src++;
		} else if (*src == '%' && src[1] && src[2]) {
			*dst++ = (char)((cast_hex((unsigned char)src[1]) << 4) |
							cast_hex((unsigned char)src[2]));
			src += 3;
		} else {
			*dst++ = *src++;
		}
		n++;
	}
	*dst = '\0';
	return n;
}

/* Finds key= in url-encoded body anchored to field boundaries (&key= or start),
 * writes value into dst, returns bytes written or 0 */
static int extract_field(const char *body, const char *key, char *dst,
						 int max_len) {
	int klen = strlen(key);
	/* match at start of body */
	if (strncmp(body, key, klen) == 0 && body[klen] == '=')
		return url_decode_field(body + klen + 1, dst, max_len);
	/* match after & separator */
	char search[18];
	snprintf(search, sizeof(search), "&%s=", key);
	const char *p = strstr(body, search);
	if (!p) return 0;
	p += strlen(search);
	return url_decode_field(p, dst, max_len);
}

/* Extracts a JSON string value: finds "key":"<value>" and copies value into dst
 */
static int json_get_string(const char *json, const char *key, char *dst,
						   int max_len) {
	char search[32];
	snprintf(search, sizeof(search), "\"%s\":\"", key);
	const char *p = strstr(json, search);
	if (!p) return 0;
	p += strlen(search);
	int n = 0;
	while (*p && *p != '"' && n < max_len)
		dst[n++] = *p++;
	dst[n] = '\0';
	return n;
}

/* Root HTML page — no JavaScript, works on any browser/platform */
static const char root_html[] =
	"<!DOCTYPE html><html><head>"
	"<meta charset=\"utf-8\">"
	"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
	"<title>ESP32 Setup</title>"
	"<style>"
	"body{font-family:sans-serif;max-width:400px;margin:40px auto;padding:16px}"
	"label{display:block;margin-top:14px;font-size:.9em;font-weight:bold}"
	"input{width:100%;padding:8px;margin:4px "
	"0;box-sizing:border-box;border:1px solid #ccc;border-radius:4px}"
	"input[type=submit]{background:#28a745;color:#fff;border:none;padding:10px;"
	"cursor:pointer;margin-top:16px;font-size:1em}"
	".hint{font-size:.8em;color:#555;margin:12px 0;line-height:1.5}"
	"</style></head><body>"
	"<h2>ESP32 Camera Setup</h2>"
	"<p class=\"hint\">"
	"1. In the dashboard click <b>Add Camera</b> to show a QR code.<br>"
	"2. Scan it with your phone camera and copy the text it shows.<br>"
	"3. Paste that text below, fill in your WiFi details, then submit."
	"</p>"
	"<form method=\"POST\" action=\"/\">"
	"<label>Code from QR scan</label>"
	"<input type=\"text\" name=\"q\" required placeholder=\"Paste the copied "
	"text here\">"
	"<label>WiFi SSID</label>"
	"<input type=\"text\" name=\"n\" required placeholder=\"Your WiFi network "
	"name\">"
	"<label>WiFi Password</label>"
	"<input type=\"password\" name=\"p\" placeholder=\"Your WiFi password\">"
	"<input type=\"submit\" value=\"Connect &amp; Register\">"
	"</form>"
	"</body></html>";

/* GET / — serve the HTML setup page */
esp_err_t website_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send_chunk(req, root_html, sizeof(root_html) - 1);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

/* POST / — receive WiFi credentials + claim token + backend URL */
static esp_err_t root_post_handler(httpd_req_t *req) {
	int total_len = req->content_len;
	if (total_len <= 0 || total_len > 512) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad content length");
		return ESP_FAIL;
	}
	char *body = malloc(total_len + 1);
	if (!body) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
		return ESP_FAIL;
	}
	int received = 0;
	while (received < total_len) {
		int r = httpd_req_recv(req, body + received, total_len - received);
		if (r <= 0) {
			free(body);
			return ESP_FAIL;
		}
		received += r;
	}
	body[total_len] = '\0';

	wifi_name.size = extract_field(body, "n", wifi_name.data, 32);
	wifi_password.size = extract_field(body, "p", wifi_password.data, 64);

	/* Decode the base64 QR payload and extract claim token + backend URL */
	char q_b64[400] = {0};
	int q_len = extract_field(body, "q", q_b64, sizeof(q_b64) - 1);
	free(body);

	if (!wifi_name.size) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing WiFi SSID");
		return ESP_FAIL;
	}
	if (q_len > 0) {
		unsigned char decoded[300] = {0};
		size_t out_len = 0;
		int ret =
			mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &out_len,
								  (const unsigned char *)q_b64, (size_t)q_len);
		if (ret == 0 && out_len > 0) {
			decoded[out_len] = '\0';
			claim_token.size =
				json_get_string((char *)decoded, "t", claim_token.data, 64);
			backend_url.size =
				json_get_string((char *)decoded, "u", backend_url.data, 128);
		} else {
			printf("base64 decode failed: %d\n", ret);
		}
	}

	printf("Provisioned: ssid=%.*s token_len=%u url_len=%u\n",
		   (int)wifi_name.size, wifi_name.data, (unsigned)claim_token.size,
		   (unsigned)backend_url.size);

	httpd_resp_set_type(req, "text/html");
	httpd_resp_sendstr(
		req, "<html><body><h2>Saved! Rebooting...</h2></body></html>");
	has_wifi = 1;
	return ESP_OK;
}

static const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = website_handler,
};

static const httpd_uri_t root_post = {
	.uri = "/",
	.method = HTTP_POST,
	.handler = root_post_handler,
};

// special urls for captive portals
static const httpd_uri_t android_captive_1 = {
	.uri = "/gen_204",
	.method = HTTP_GET,
	.handler = website_handler,
};

static const httpd_uri_t android_captive_2 = {
	.uri = "/generate_204",
	.method = HTTP_GET,
	.handler = website_handler,
};

static const httpd_uri_t ios_captive_1 = {
	.uri = "/library/test/success.html",
	.method = HTTP_GET,
	.handler = website_handler,
};

static const httpd_uri_t ios_captive_2 = {
	.uri = "/hotspot-detect.html",
	.method = HTTP_GET,
	.handler = website_handler,
};

static const httpd_uri_t *uri_handlers[] = {
	&root,			&root_post,		&android_captive_1, &android_captive_2,
	&ios_captive_1, &ios_captive_2,
};

#define URI_HANDLERS_COUNT (sizeof(uri_handlers) / sizeof(uri_handlers[0]))

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

httpd_handle_t start_webserver(void) {
	httpd_handle_t server = NULL;

	// Start the httpd server
	ESP_LOGI("start_webserver", "Starting server");
	esp_err_t ret;
	httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

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

	if (ESP_OK != ret) {
		ESP_LOGI("httpd_ssl_start", "Error starting server!");
		return NULL;
	}

	// Set URI handlers
	ESP_LOGI("httpd_ssl_start", "Registering URI handlers");
	for (int i = 0; i < URI_HANDLERS_COUNT; i++) {
		httpd_register_uri_handler(server, uri_handlers[i]);
	}

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
}

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
					 void *event_data) {
	SERVER = (httpd_handle_t *)arg;
	if (*SERVER == NULL) {
		*SERVER = start_webserver();
	}
}
