

#ifndef _H_CAMERA_
#define _H_CAMERA_
#include "esp_camera.h"

extern camera_fb_t *fb;

esp_err_t init_camera();

int take_snapshot();
#endif