#ifndef _H_STATION_WIFI_
#define _H_STATION_WIFI_

#include "str.h"

// Defined in station_wifi.c
extern int connected_to_wifi;

void wifi_destroy(void);
int  wifi_init_sta(vstr *name, vstr *password);

#endif
