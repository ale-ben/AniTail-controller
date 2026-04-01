#include <ESP32Servo.h>
#include "servo.h"
#include "generalConfig.h"

Servo servoA; 
Servo servoB;

void setupServo() {
	// Setup for servo A
	servoA.setPeriodHertz(SERVO_A_PERIOD_HZ);    // standard 50 hz servo
	servoA.attach(SERVO_A_PIN, SERVO_A_MIN_PULSE, SERVO_A_MAX_PULSE); 
	Log.traceln("Servo A initialized on pin: %d with home position at %d degrees", SERVO_A_PIN, SERVO_A_HOME_ANGLE);
	Log.verboseln("Servo A min angle: %d degrees, max angle: %d degrees", SERVO_A_MIN_ANGLE, SERVO_A_MAX_ANGLE);
	Log.verboseln("Servo A min pulse: %d us, max pulse: %d us, period: %d Hz", SERVO_A_MIN_PULSE, SERVO_A_MAX_PULSE, SERVO_A_PERIOD_HZ);


	// Setup for servo B
	servoB.setPeriodHertz(SERVO_B_PERIOD_HZ);    // standard 50 hz servo
	servoB.attach(SERVO_B_PIN, SERVO_B_MIN_PULSE, SERVO_B_MAX_PULSE); 
	Log.traceln("Servo B initialized on pin: %d with home position at %d degrees", SERVO_B_PIN, SERVO_B_HOME_ANGLE);
	Log.verboseln("Servo B min angle: %d degrees, max angle: %d degrees", SERVO_B_MIN_ANGLE, SERVO_B_MAX_ANGLE);
	Log.verboseln("Servo B min pulse: %d us, max pulse: %d us, period: %d Hz", SERVO_B_MIN_PULSE, SERVO_B_MAX_PULSE, SERVO_B_PERIOD_HZ);
}

void moveServoA(int angle) {
	if (angle < SERVO_A_MIN_ANGLE || angle > SERVO_A_MAX_ANGLE) {
		Log.errorln("Invalid angle for Servo A! Must be between %d and %d.", SERVO_A_MIN_ANGLE, SERVO_A_MAX_ANGLE);
		return;
	}
	Log.traceln("Moving Servo A to angle: %d", angle);
	servoA.write(angle);
}

void moveServoB(int angle) {
	if (angle < SERVO_B_MIN_ANGLE || angle > SERVO_B_MAX_ANGLE) {
		Log.errorln("Invalid angle for Servo B! Must be between %d and %d.", SERVO_B_MIN_ANGLE, SERVO_B_MAX_ANGLE);
		return;
	}
	Log.traceln("Moving Servo B to angle: %d", angle);
	servoB.write(angle);
}