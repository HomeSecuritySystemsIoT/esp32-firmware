#ifndef _H_STATION_WIFI_
#define _H_STATION_WIFI_
void wifi_destroy();
#include"str.h"
int connected_to_wifi;
int wifi_init_sta(vstr * name,vstr*password);
#endif