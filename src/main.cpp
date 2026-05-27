#include <ESP32Servo.h>
#include "parser.h"
#include "servo.h"
#include "generalConfig.h"

#ifdef ENABLE_SERIAL_CONTROL
#include "serialControl.h"
#endif

#ifdef ENABLE_WIFI_CONTROL
#include "wifiControl.h"
#endif

#ifdef ENABLE_SDCARD_CONTROL
#include "sdCardControl.h"
int sdCardSetupResult = -1; // Global variable to track SD card setup status
#endif

void setup() {
	Serial.begin(115200);
	Log.begin(LOG_LEVEL, &Serial);
	Log.infoln("Starting AniTail...");
	setupServo();
	parseCommand("G28"); // Home all servos to home position on startup
	Log.infoln("Servo control ready!"); // Log message at notice level
#ifdef ENABLE_WIFI_CONTROL
	setupWiFiControl();
	Log.infoln("WiFi control ready!"); // Log message at notice level
#endif
#ifdef ENABLE_SERIAL_CONTROL
	Log.infoln("Serial control ready!"); // Log message at notice level
#endif
#ifdef ENABLE_SDCARD_CONTROL
	sdCardSetupResult = setupSDCard();
	if (sdCardSetupResult == 0) {
		Log.infoln("SD card control ready!"); // Log message at notice level
	} else {
		Log.errorln("SD card setup failed. SD card control will be unavailable.");
	}
#endif
}

char* getNextCommand() {
	char* command = nullptr;

#ifdef ENABLE_SERIAL_CONTROL
	command = readSerialInput();
#endif

#ifdef ENABLE_WIFI_CONTROL
	if (command == nullptr) command = readWiFiInput();
#endif

#ifdef ENABLE_SDCARD_CONTROL
	if (command == nullptr && sdCardSetupResult == 0) {
		command = readSDCardInput();
	}
#endif

	return command;
}

void loop() {
#ifdef ENABLE_WIFI_CONTROL
	checkWiFiStatus(); // Periodically log WiFi status
#endif

	// Get the next available command
	char* command = getNextCommand();
	if (command != nullptr) {
		Log.infoln("Received command: %s", command); // Log the received command at debug level

		// Parse and execute the command
		parseCommand(command);

		free(command); // Free the memory allocated for the command
	}

	#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_TRACE
	Log.traceln("Loop iteration complete. Waiting for next command...");
	delay(1000);
	#elif defined(LOG_LEVEL) && LOG_LEVEL == LOG_LEVEL_INFO
	delay(500); 
	#else
	delay(10); // Small delay to keep watchdog happy
	#endif
}