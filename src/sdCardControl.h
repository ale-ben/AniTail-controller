#ifndef SDCARD_CONTROL_H
#define SDCARD_CONTROL_H

#include "generalConfig.h"

#ifdef ENABLE_SDCARD_CONTROL
#define CHIP_SELECT 7
int setupSDCard();

bool setActiveSDCommandFile(const char* filename);
void pauseSDCardPlayback();
void resumeSDCardPlayback();
char* readSDCardInput();

#endif // ENABLE_SDCARD_CONTROL
#endif // SDCARD_CONTROL_H