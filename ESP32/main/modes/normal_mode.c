#include "normal_mode.h"
#include "camera.h"
#include "led.h"
#include "station_wifi.h"
#include "http_website.h"
#include "str.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UTF8_MAX_SIZE 4

// TODO: update to the backend's address on the home network once station mode is wired up
#define BACKEND_IP  "192.168.4.2"
#define TCP_PORT    5005
#define UDP_PORT    5004

static const char *TAG = "normal_mode";

static int load_and_connect_wifi(void) {
    vstr wifi_name     = {NULL, 0};
    vstr wifi_password = {NULL, 0};

    wifi_name.data = malloc(98 * UTF8_MAX_SIZE);
    if (!wifi_name.data) return -1;
    wifi_password.data = wifi_name.data + 33 * UTF8_MAX_SIZE;

    FILE *f = fopen("/spiffs/wf", "rb");
    if (!f) { free(wifi_name.data); return -1; }

    fread(&wifi_name.size,     1, 1,              f);
    fread(&wifi_password.size, 1, 1,              f);
    fread(wifi_name.data,      wifi_name.size,     1, f);
    fread(wifi_password.data,  wifi_password.size, 1, f);
    fclose(f);
    wifi_name.data[wifi_name.size]         = 0;
    wifi_password.data[wifi_password.size] = 0;

    int ret = wifi_init_sta(&wifi_name, &wifi_password);
    free(wifi_name.data);
    return ret;
}

void run_normal_mode(void) {
    ESP_LOGI(TAG, "Entering normal mode");
    configure_led();

    if (load_and_connect_wifi() != 0) {
        ESP_LOGE(TAG, "Failed to connect to WiFi — clearing credentials and restarting");
        remove("/spiffs/wf");
        esp_restart();
        return;
    }
    ESP_LOGI(TAG, "Connected to WiFi");

    const char *host_ip = BACKEND_IP;
    struct sockaddr_in dest_addr_tcp, dest_addr_udp;
    uint16_t tcp_port = htons(TCP_PORT), udp_port = htons(UDP_PORT);

    printf("Connecting to backend at %s\n", host_ip);
    inet_pton(AF_INET, host_ip, &dest_addr_udp.sin_addr);
    dest_addr_udp.sin_family = AF_INET;
    dest_addr_udp.sin_port   = udp_port;

    inet_pton(AF_INET, host_ip, &dest_addr_tcp.sin_addr);
    dest_addr_tcp.sin_family = AF_INET;
    dest_addr_tcp.sin_port   = tcp_port;

    char receive_buff[1025];
    receive_buff[1024] = 0;
    int err, ret;

    int client_fd_udp = socket(AF_INET, SOCK_DGRAM,   IPPROTO_IP);
    int client_fd_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (client_fd_udp < 0 || client_fd_tcp < 0) {
        printf("Socket creation error %i %i\n", client_fd_udp, client_fd_tcp);
    }

    sleep(10); // Give the backend time to be reachable after WiFi connects

    err = connect(client_fd_udp, (struct sockaddr *)&dest_addr_udp, sizeof(dest_addr_udp));
    if (err) printf("connect() UDP error %i\n", err);

    err = send(client_fd_udp, "TEST", 4, 0);
    if (err < 0) printf("send() UDP error %i\n", err);

    err = recv(client_fd_udp, receive_buff, 2, 0);
    if (err < 0) printf("recv() UDP error %i\n", err);

    err = connect(client_fd_tcp, (struct sockaddr *)&dest_addr_tcp, sizeof(dest_addr_tcp));
    if (err) printf("connect() TCP error %i\n", err);

    struct timeval timeout = { .tv_sec = 10, .tv_usec = 0 };
    setsockopt(client_fd_udp, SOL_SOCKET,   SO_RCVTIMEO,  &timeout, sizeof(timeout));

    int val = 1;
    setsockopt(client_fd_tcp, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));

    size_t    packet_size, packet_i;
    int       r, g, b, tcp_len, packet_length;
    camera_fb_t *fb = NULL;

    while (1) {
        packet_length = recv(client_fd_udp, receive_buff, 1024, 0);
        printf("packet_length %i\n", packet_length);
        if (packet_length < 0) puts("UDP recv() error");
        if (packet_length == 0) continue;

        if (packet_length > 4) {
            if (memcmp(receive_buff, "/tcp/", 5) == 0) {
                // Receive a text message over TCP and print it (debug)
                err = recv(client_fd_tcp, &tcp_len, 4, 0);
                if (err < 0) { puts("recv() tcp error"); continue; }
                err = recv(client_fd_tcp, receive_buff, tcp_len, 0);
                if (err < 0) puts("recv() tcp error");
                else printf("%.*s\n", tcp_len, receive_buff);

            } else if (packet_length == 8 && memcmp(receive_buff, "/camera/", 8) == 0) {
                // Legacy simple mode: one frame every 10 seconds over UDP
                ret = init_camera();
                printf("Entering camera mode %i\n", ret);
                packet_size = sprintf(receive_buff, "init_camera() %i\n", ret);
                recv(client_fd_tcp, receive_buff, 1, 0);
                send(client_fd_tcp, (char *)&packet_size, 4, 0);
                send(client_fd_tcp, receive_buff, packet_size, 0);
                while (1) {
                    fb = esp_camera_fb_get();
                    if (!fb) {
                        puts("esp_camera_fb_get() failed");
                        send(client_fd_udp, "\xff\xff\xff\xff", 4, 0);
                    } else {
                        send(client_fd_udp, &fb->len, 4, 0);
                        packet_size = fb->len;
                        packet_i    = 0;
                        while (packet_size) {
                            packet_size = fb->len > 1400 ? 1400 : fb->len;
                            send(client_fd_udp, fb->buf + packet_i, packet_size, 0);
                            packet_i  += packet_size;
                            fb->len   -= packet_size;
                        }
                        esp_camera_fb_return(fb);
                    }
                    vTaskDelay(pdMS_TO_TICKS(10000));
                }

            } else if (packet_length == 8 && memcmp(receive_buff, "/camerb/", 8) == 0) {
                // Main streaming mode: backend drives frame rate via TCP commands
                ret = init_camera();
                vTaskDelay(pdMS_TO_TICKS(200));
                printf("Entering camera mode B %i\n", ret);
                packet_size = sprintf(receive_buff, "init_camera() %i\n", ret);
                recv(client_fd_tcp, receive_buff + 1023, 1, 0);
                puts(receive_buff[1023] == '2' ? "HandShake OK" : "HandShake failed");
                send(client_fd_tcp, "4", 1, 0);
                packet_i = 0;
                while (packet_i != 4) {
                    ret = send(client_fd_tcp, ((char *)&packet_size) + packet_i, 4 - packet_i, 0);
                    if (ret < 0) { puts("send() tcp error"); return; }
                    packet_i += ret;
                }
                packet_i = 0;
                while (packet_i != packet_size) {
                    ret = send(client_fd_tcp, receive_buff + packet_i, packet_size - packet_i, 0);
                    if (ret < 0) { puts("send() tcp error"); return; }
                    packet_i += ret;
                }

                while (1) {
                    fb = esp_camera_fb_get();
                    if (!fb) {
                        puts("esp_camera_fb_get() failed");
                        send(client_fd_tcp, "\xff\xff\xff\xff", 4, 0);
                        continue;
                    }

                    ret = recv(client_fd_tcp, receive_buff, 1, 0);
                    if (ret <= 0) {
                        puts("Sync error");
                        esp_camera_fb_return(fb);
                        return;
                    }

                    switch (*receive_buff) {
                    case 'G': // Send this frame
                        break;
                    case 'S': // Pause feed, wait for 'G' to resume
                        puts("Camera feed paused");
                        esp_camera_fb_return(fb);
                        fb = NULL;
                        while (1) {
                            ret = recv(client_fd_tcp, receive_buff, 1, 0);
                            if (ret <= 0) { exit(400); }
                            if (*receive_buff == 'G') { puts("Camera feed resumed"); break; }
                        }
                        continue;
                    default:
                        puts("Unknown command");
                        break;
                    }

                    packet_i = 0;
                    while (packet_i != 4) {
                        ret = send(client_fd_tcp, ((char *)&fb->len) + packet_i, 4 - packet_i, 0);
                        if (ret <= 0) break;
                        packet_i += ret;
                    }
                    packet_i = 0;
                    while (packet_i != fb->len) {
                        ret = send(client_fd_tcp, fb->buf + packet_i, fb->len - packet_i, 0);
                        if (ret <= 0) break;
                        packet_i += ret;
                    }

                    esp_camera_fb_return(fb);
                    vTaskDelay(pdMS_TO_TICKS(33)); // ~30fps cap, reduces sensor heat

                } // end /camerb/ loop

            } else if (memcmp(receive_buff, "/led/", 5) == 0) {
                if (packet_length == 5 && recv(client_fd_udp, receive_buff + 5, 1019, 0) < 6)
                    puts("led wrong format");
                r = (cast_hex(receive_buff[5]) << 4) | cast_hex(receive_buff[6]);
                g = (cast_hex(receive_buff[7]) << 4) | cast_hex(receive_buff[8]);
                b = (cast_hex(receive_buff[9]) << 4) | cast_hex(receive_buff[10]);
                set_led(r, g, b);

            } else {
                printf("%.*s\n", packet_length, receive_buff);
            }

        } else {
            if (receive_buff[0] == '\x08') {
                break; // Exit command
            } else {
                printf("%.*s\n", packet_length, receive_buff);
            }
        }
    }

    close(client_fd_tcp);
    close(client_fd_udp);
}
