#include <ESP32Servo.h>
#include "serial.h"
#include "parser.h"
#include "servo.h"
#include "generalConfig.h"

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

int pos = 0;    // variable to store the servo position

int servoPin = 21;

void setup() {
	Serial.begin(115200);
	Log.begin(LOG_LEVEL, &Serial);
	setupServo();
	Log.infoln("Servo control ready!"); // Log message at notice level
}

char* getNextCommand() {
	char* command = readSerialInput();
	if (command != nullptr) return command;

	//TODO: Implement other input methods (e.g. Bluetooth, WiFi) and check them here as well, returning the first available command from any source.

	return nullptr; // No command available yet
}

void loop() {
	// Get the next available command
	char* command = getNextCommand();
	if (command != nullptr) {
		Log.infoln("Received command: %s", command); // Log the received command at debug level

		// Parse and execute the command
		parseCommand(command);

		free(command); // Free the memory allocated for the command
	}

	delay(100); // Small delay to avoid overwhelming the loop
}