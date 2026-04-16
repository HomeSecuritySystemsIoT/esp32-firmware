#ifndef _H_JPEG_
#define _H_JPEG_
#include "esp_camera.h"

#ifdef __cplusplus
#include"JPEGDEC/JPEGDEC.h"
extern "C" {
#endif
extern pthread_mutex_t snapshot_muted;

void jpeg_init();
void* th1_func(void* vfb);
void jpeg_to_bmp(camera_fb_t * snap);
extern int th1_command,analysis_init;
extern camera_fb_t * fb;
#ifdef __cplusplus
}
#endif


#endif