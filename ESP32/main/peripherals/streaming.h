#ifndef PERIPHERALS_STREAMING_H
#define PERIPHERALS_STREAMING_H

#include "esp_tls.h"

struct esp_tls *startup_phase(char *receive_buff, int *client_fd_tcp);
int capture_frame(struct esp_tls *tls);
int sync_and_handle_command(struct esp_tls *tls, char *receive_buff);
int send_frame_len(struct esp_tls *tls);
int send_frame_buf(struct esp_tls *tls);
void release_frame(void);

#endif
