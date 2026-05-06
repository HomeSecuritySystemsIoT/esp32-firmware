

#ifndef _H_CAMERA_
#define _H_CAMERA_
#include "esp_camera.h"

esp_err_t init_camera();

int take_snapshot();
#endif