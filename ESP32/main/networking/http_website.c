/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "mbedtls/base64.h"
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

#include "http_website.h"
#include "str_utility.h"

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
	"<title>Camera Setup</title>"
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

// Redirect all captive portal probes to the setup page.
// iOS detects a captive portal when it does NOT get the expected "Success"
// response from captive.apple.com — returning a redirect triggers the popup.
static esp_err_t captive_redirect_handler(httpd_req_t *req) {
	ESP_LOGI("captive", "probe hit: %s", req->uri);
	httpd_resp_set_status(req, "302 Found");
	httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

// Catch-all: any unregistered path also redirects to the setup page so the
// CNA browser always lands correctly regardless of which probe URL iOS uses.
static esp_err_t http_404_redirect_handler(httpd_req_t *req,
										   httpd_err_code_t err) {
	ESP_LOGI("captive", "404 catch-all: %s", req->uri);
	httpd_resp_set_status(req, "302 Found");
	httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

// special urls for captive portals
static const httpd_uri_t android_captive_1 = {
	.uri = "/gen_204",
	.method = HTTP_GET,
	.handler = captive_redirect_handler,
};
static const httpd_uri_t android_captive_2 = {
	.uri = "/generate_204",
	.method = HTTP_GET,
	.handler = captive_redirect_handler,
};
static const httpd_uri_t ios_captive_1 = {
	.uri = "/library/test/success.html",
	.method = HTTP_GET,
	.handler = captive_redirect_handler,
};
static const httpd_uri_t ios_captive_2 = {
	.uri = "/hotspot-detect.html",
	.method = HTTP_GET,
	.handler = captive_redirect_handler,
};

static const httpd_uri_t *uri_handlers[] = {
	&root,			&root_post,		&android_captive_1, &android_captive_2,
	&ios_captive_1, &ios_captive_2,
};

#define URI_HANDLERS_COUNT (sizeof(uri_handlers) / sizeof(uri_handlers[0]))

httpd_handle_t start_webserver(void) {
	httpd_handle_t server = NULL;

	ESP_LOGI("start_webserver", "Starting server");

	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
	// Allow enough URI slots for all handlers
	conf.max_uri_handlers = URI_HANDLERS_COUNT;

	esp_err_t ret = httpd_start(&server, &conf);

	if (ESP_OK != ret) {
		ESP_LOGI("start_webserver", "Error starting server!");
		return NULL;
	}

	ESP_LOGI("start_webserver", "Registering URI handlers");
	for (int i = 0; i < URI_HANDLERS_COUNT; i++) {
		httpd_register_uri_handler(server, uri_handlers[i]);
	}

	httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
							   http_404_redirect_handler);

	return server;
}

esp_err_t stop_webserver(httpd_handle_t server) { return httpd_stop(server); }

void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
					 void *event_data) {
	SERVER = (httpd_handle_t *)arg;
	if (*SERVER == NULL) {
		*SERVER = start_webserver();
	}
}
