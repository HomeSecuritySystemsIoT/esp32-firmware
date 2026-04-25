#define CONFIG_BLINK_LED_STRIP_BACKEND_RMT 1
#include "led.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#define BLINK_GPIO 48
#define TAG "led"

static led_strip_handle_t led_strip;

// State for the smooth rainbow cycle
static uint8_t r = 255, g = 0, b = 0;

void configure_led(void) {
    led_strip_config_t strip_config = {
        .strip_gpio_num  = BLINK_GPIO,
        .max_leds        = 1,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model       = LED_MODEL_WS2812,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .flags.with_dma = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device() failed: %s", esp_err_to_name(err));
        return;
    }

    led_strip_clear(led_strip);

    // Immediately light up white for 500 ms so we can confirm the driver works
    led_strip_set_pixel(led_strip, 0, 255, 255, 255);
    led_strip_refresh(led_strip);
    ESP_LOGI(TAG, "LED initialised on GPIO %d — white test pulse", BLINK_GPIO);
}

void blink_led(void) {
    if ((r == 255) && (g != 255) && (b == 0)) g++;
    if ((r != 0)   && (g == 255) && (b == 0)) r--;
    if ((r == 0)   && (g == 255) && (b != 255)) b++;
    if ((r == 0)   && (g != 0)   && (b == 255)) g--;
    if ((r != 255) && (g == 0)   && (b == 255)) r++;
    if ((r == 255) && (g == 0)   && (b != 0))   b--;

    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

void set_led(int red, int green, int blue) {
    esp_err_t err = led_strip_set_pixel(led_strip, 0, red, green, blue);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_pixel failed: %s", esp_err_to_name(err));
        return;
    }
    led_strip_refresh(led_strip);
}
