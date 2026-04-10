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

void setup() {
	Serial.begin(115200);
	Log.begin(LOG_LEVEL, &Serial);
	Log.infoln("Starting AniTail...");
	setupServo();
	Log.infoln("Servo control ready!"); // Log message at notice level
#ifdef ENABLE_WIFI_CONTROL
	setupWiFiControl();
	Log.infoln("WiFi control ready!"); // Log message at notice level
#endif
#ifdef ENABLE_SERIAL_CONTROL
	Log.infoln("Serial control ready!"); // Log message at notice level
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

	delay(10); // Small delay to keep watchdog happy
}