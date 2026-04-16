#include <string.h>
#include"camera.h"
#include "sensor.h"
#define UTF8_MAX_SIZE 4
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include"esp_camera.h"
#define CAM_PIN_PWDN    -1 // Lub konkretny GPIO, np. 38
#define CAM_PIN_RESET   -1 // Lub konkretny GPIO, np. 39
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD     4
#define CAM_PIN_SIOC     5

#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      10
#define CAM_PIN_D2       8
#define CAM_PIN_D1       9
#define CAM_PIN_D0      11

#define CAM_PIN_VSYNC    6
#define CAM_PIN_HREF     7
#define CAM_PIN_PCLK     13



static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,    
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    
    .xclk_freq_hz = 10000000 ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG, 
    .frame_size = FRAMESIZE_HD,
    .jpeg_quality = 30,
    .fb_count = 2,           // Wymaga PSRAM dla więcej niż 1 bufora
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY ,//CAMERA_GRAB_LATEST
    .fb_location = CAMERA_FB_IN_PSRAM
};

esp_err_t init_camera() {
	vTaskDelay(pdMS_TO_TICKS(1000));
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        return err;
    }
    return ESP_OK;
}



int take_snapshot() {
    camera_fb_t * fb = esp_camera_fb_get();
    
    if (!fb) {
        return 1;
    }

    // fb->buf     
    // fb->len    
    // fb->width  
    // fb->height  
    
	

    esp_camera_fb_return(fb);
    return 0;
}



