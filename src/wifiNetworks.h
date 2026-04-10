#ifndef WIFI_NETWORKS_H
#define WIFI_NETWORKS_H
#include "generalConfig.h"

#ifdef ENABLE_WIFI_CONTROL
#ifndef ENABLE_WIFI_AP
#include <WiFiMulti.h>
void configureClientNetworks(WiFiMulti& wifiMulti);

#endif // ENABLE_WIFI_AP
#endif // ENABLE_WIFI_CONTROL

#endif // WIFI_NETWORKS_H