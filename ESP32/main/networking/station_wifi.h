#ifndef NETWORKING_STATION_WIFI_H_
#define NETWORKING_STATION_WIFI_H_
#include "str_utility.h"
void wifi_destroy();
extern int connected_to_wifi;
int wifi_init_sta(vstr *name, vstr *password);
#endif
