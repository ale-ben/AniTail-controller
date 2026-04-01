#ifndef SERVO_H
#define SERVO_H

// A servo config
#define SERVO_A_PIN 21
#define SERVO_A_MIN_PULSE 500
#define SERVO_A_MAX_PULSE 2500
#define SERVO_A_PERIOD_HZ 50
#define SERVO_A_MIN_ANGLE 0
#define SERVO_A_MAX_ANGLE 180
#define SERVO_A_HOME_ANGLE 90

// B servo config
#define SERVO_B_PIN 22
#define SERVO_B_MIN_PULSE 500
#define SERVO_B_MAX_PULSE 2500
#define SERVO_B_PERIOD_HZ 50
#define SERVO_B_MIN_ANGLE 0
#define SERVO_B_MAX_ANGLE 180
#define SERVO_B_HOME_ANGLE 90

void setupServo();
void moveServoA(int angle);
void moveServoB(int angle);

#endif // SERVO_H