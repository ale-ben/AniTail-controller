#include <ESP32Servo.h>
#include "serial.h"
#include "parser.h"

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

int pos = 0;    // variable to store the servo position

int servoPin = 21;

void setup() {
	// // Allow allocation of all timers
	// ESP32PWM::allocateTimer(0);
	// myservo.setPeriodHertz(50);    // standard 50 hz servo
	// myservo.attach(servoPin, 500, 2500); 
	// // Extended pulse width range for better 0-180 degree sweep
	// // Adjust these values if servo tries to move past physical limits
	
	Serial.begin(115200);
	Serial.println("Servo control ready!");
	// Serial.println("Enter position (0-180):");
}

char* getNextCommand() {
	char* command = readSerialInput();
	if (command != nullptr) return command;

	//TODO: Implement other input methods (e.g. Bluetooth, WiFi) and check them here as well, returning the first available command from any source.

	return nullptr; // No command available yet
}

void loop() {
	// if (Serial.available() > 0) {
	// 	int newPos = Serial.parseInt();
		
	// 	// Validate position range
	// 	if (newPos >= 0 && newPos <= 180) {
	// 		pos = newPos;
	// 		myservo.write(pos);
	// 		Serial.print("Position set to: ");
	// 		Serial.println(pos);
	// 	} else {
	// 		Serial.println("Invalid position! Enter value between 0 and 180.");
	// 	}
		
	// 	// Clear any remaining input
	// 	while (Serial.available() > 0) {
	// 		Serial.read();
	// 	}
	// }

	// Get the next available command
	char* command = getNextCommand();
	if (command != nullptr) {
		Serial.print("Received command: ");
		Serial.println(command);

		// Parse and execute the command
		parseCommand(command);

		free(command); // Free the memory allocated for the command
	}

	delay(100); // Small delay to avoid overwhelming the loop
}