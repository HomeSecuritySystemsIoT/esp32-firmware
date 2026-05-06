#ifndef _H_STATION_WIFI_
#define _H_STATION_WIFI_
#include "str_utility.h"
void wifi_destroy();
extern int connected_to_wifi;
int wifi_init_sta(vstr *name, vstr *password);
#endif
