#define CONFIG_BLINK_LED_STRIP_BACKEND_RMT 1
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <stdio.h>
// #include "led_strip_types.h"
#include "sdkconfig.h"
#define BLINK_GPIO 48
uint8_t *pixel_buff_led;
uint8_t r = 255, g = 0, b = 0;
static led_strip_handle_t led_strip;

void configure_led(void) {
	// ESP_LOGI(TAG, "Example configured to blink addressable LED!");
	led_strip_config_t strip_config = {
		.strip_gpio_num = BLINK_GPIO,
		.max_leds = 1, // at least one LED on board
	};
	led_strip_rmt_config_t rmt_config = {
		.resolution_hz = 10 * 1000 * 1000, // 10MHz
		.flags.with_dma = false,
	};
	if (led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip)) {
		puts("led_strip_new_rmt_device() error");
	}
	pixel_buff_led = get_pixel_buf_led();
	led_strip_clear(led_strip);
}

void blink_led(void) {
	// if (s_led_state) {
	// led_strip_rmt_set_pixel_handle_unoff(led_strip, 0, color&0xff,
	// (color>>8)&0xff, (color>>16));

	if ((r == 255) && (g != 255) && (b == 0)) {
		g++;
	}
	if ((r != 0) && (g == 255) && (b == 0)) {
		r--;
	}
	if ((r == 0) && (g == 255) && (b != 255)) {
		b++;
	}
	if ((r == 0) && (g != 0) && (b == 255)) {
		g--;
	}
	if ((r != 255) && (g == 0) && (b == 255)) {
		r++;
	}
	if ((r == 255) && (g == 0) && (b != 0)) {
		b--;
	}
	// ESP_LOGI(TAG, "%i %i %i\n",r,g,b);
	pixel_buff_led[0] = g; // GREEN
	pixel_buff_led[1] = r; // RED
	pixel_buff_led[2] = b; // BLUE

	led_strip_refresh(led_strip);
	// color=(color&0xff000000)?0:color+1;
	// led_strip_set_pixel(led_strip, 0, pixel_buf[0], pixel_buf[1],
	// pixel_buf[2]);

	// led_strip_rmt_refresh_handle(led_strip);
	//} else {
	//   led_strip_clear(led_strip);
	//}
}

void set_led(int red, int green, int blue) {
	// if (s_led_state) {
	// led_strip_rmt_set_pixel_handle_unoff(led_strip, 0, color&0xff,
	// (color>>8)&0xff, (color>>16));

	// ESP_LOGI(TAG, "%i %i %i\n",r,g,b);
	pixel_buff_led[0] = green; // GREEN
	pixel_buff_led[1] = red;   // RED
	pixel_buff_led[2] = blue;  // BLUE

	led_strip_refresh(led_strip);
	// color=(color&0xff000000)?0:color+1;
	// led_strip_set_pixel(led_strip, 0, pixel_buf[0], pixel_buf[1],
	// pixel_buf[2]);

	// led_strip_rmt_refresh_handle(led_strip);
	//} else {
	//   led_strip_clear(led_strip);
	//}
}
