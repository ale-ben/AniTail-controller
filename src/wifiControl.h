#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include "generalConfig.h"

#ifdef ENABLE_WIFI_CONTROL
void setupWiFiControl();
char* readWiFiInput();
void checkWiFiStatus(); // Periodic status check

#endif // ENABLE_WIFI_CONTROL
#endif // WIFI_CONTROL_H