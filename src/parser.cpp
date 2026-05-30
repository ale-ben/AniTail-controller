
#include <Arduino.h>
#include "servo.h"
#include "generalConfig.h"

#ifdef ENABLE_SDCARD_CONTROL
#include "sdCardControl.h"
#endif

/**
 * @brief Linearly interpolates between two angles over a specified duration.
 *
 * @param startAngle The starting angle
 * @param endAngle The target angle
 * @param durationMs Total interpolation duration in milliseconds
 */
void interpolateServoSmooth(int startAngleA, int endAngleA, int startAngleB, int endAngleB, unsigned long durationMs) {
	Log.traceln("Interpolating servos smoothly over %lu ms", durationMs);
	unsigned long startTime = millis();
	unsigned long elapsed = 0;

	while (elapsed < durationMs) {
		elapsed = millis() - startTime;
		float progress = (float)elapsed / durationMs;
		if (progress > 1.0) progress = 1.0;

		// Linear interpolation for both servos
		int currentA = startAngleA + (endAngleA - startAngleA) * progress;
		int currentB = startAngleB + (endAngleB - startAngleB) * progress;

		moveServoA(currentA);
		moveServoB(currentB);

		delay(10); // 10ms update interval for smooth motion
	}

	// Ensure final position is exact
	moveServoA(endAngleA);
	moveServoB(endAngleB);
	Log.traceln("Interpolation complete. Final positions: A=%d, B=%d", endAngleA, endAngleB);
}

/**
 * @brief Splits a command string into tokens based on spaces. Modifies the input command string by replacing spaces with null terminators and populates the provided tokens array.
 *
 * @param command The input command string to be split. This string will be modified in-place.
 * @param tokens Array of char pointers to store the tokens (must have space for at least 5 elements).
 * @param tokenCount Reference parameter that will be set to the number of tokens found in the command string.
 */
void splitCommandIntoTokens(char* command, char** tokens, int& tokenCount) {
	Log.traceln("Splitting command into tokens: '%s'", command);

	// Reset token count and start with the first token as the command type
	int commandLength = strlen(command);
	tokenCount = 0;
	tokens[tokenCount++] = command; // First token is the command type

	// Iterate through the command string and split into tokens based on spaces
	Log.verboseln("Starting tokenization loop, command length: %d", commandLength);
	for (size_t i = 0; i < commandLength; i++) {
		if (command[i] == ' ') {
			command[i] = '\0'; // Null-terminate the token
			tokens[tokenCount++] = command + i + 1; // Store pointer to next token
			Log.verboseln("Found token: '%s'", tokens[tokenCount - 1]);
			if (tokenCount >= 5) {
				Log.errorln("Error: Too many tokens in command. Max is 5.");
				break; // Prevent overflow
			}
		}
	}

	if (tokenCount < 4) {
		tokens[tokenCount] = nullptr; // Null-terminate the tokens array if less than max tokens found
	}

	Log.traceln("Completed tokenization, token count: %d", tokenCount);
}

void commandG0(char** params, int paramCount) {
	bool hasA = false, hasB = false;
	int angleA = 0, angleB = 0;

	for (int i = 0; i < paramCount; i++) {
		if (params[i][0] == 'A') {
			angleA = atoi(params[i] + 1); // Convert the substring after 'A' to an integer
			hasA = true;
		} else if (params[i][0] == 'B') {
			angleB = atoi(params[i] + 1); // Convert the substring after 'B' to an integer
			hasB = true;
		} else {
			Log.errorln("Error: Unknown parameter '%s' in G0 command.", params[i]);
			return;
		}
	}

	if (hasA) moveServoA(angleA);
	if (hasB) moveServoB(angleB);
}

void commandG1(char** params, int paramCount) {
	bool hasA = false, hasB = false, hasT = false;
	int angleA = getCurrentAngleA(), angleB = getCurrentAngleB();
	unsigned long durationMs = 1000; // Default 1 second

	for (int i = 0; i < paramCount; i++) {
		if (params[i][0] == 'A') {
			angleA = atoi(params[i] + 1);
			hasA = true;
		} else if (params[i][0] == 'B') {
			angleB = atoi(params[i] + 1);
			hasB = true;
		} else if (params[i][0] == 'T') {
			durationMs = atoi(params[i] + 1);
			hasT = true;
		} else {
			Log.errorln("Error: Unknown parameter '%s' in G1 command.", params[i]);
			return;
		}
	}

	if (!hasA && !hasB) {
		Log.warningln("G1 command requires at least A or B parameter.");
		return;
	}

	int currentA = getCurrentAngleA();
	int currentB = getCurrentAngleB();
	Log.traceln("G1 command: moving from A=%d,B=%d to A=%d,B=%d over %lu ms",
	            currentA, currentB, angleA, angleB, durationMs);

	interpolateServoSmooth(currentA, angleA, currentB, angleB, durationMs);
}

void commandG4(char** params, int paramCount) {
	if (paramCount == 0) {
		Log.warningln("G4 requires a delay parameter (P/T in ms, or S in seconds).");
		return;
	}

	unsigned long delayMs = 0;
	bool hasDelay = false;

	for (int i = 0; i < paramCount; i++) {
		if (params[i][1] == '\0') {
			Log.errorln("Error: Missing value for G4 parameter '%s'.", params[i]);
			return;
		}

		if (params[i][0] == 'P' || params[i][0] == 'T') {
			delayMs = strtoul(params[i] + 1, nullptr, 10);
			hasDelay = true;
		} else if (params[i][0] == 'S') {
			delayMs = strtoul(params[i] + 1, nullptr, 10) * 1000UL;
			hasDelay = true;
		} else {
			Log.errorln("Error: Unknown parameter '%s' in G4 command.", params[i]);
			return;
		}
	}

	if (!hasDelay) {
		Log.warningln("G4 did not receive a valid delay value.");
		return;
	}

	Log.traceln("Executing G4 dwell for %lu ms", delayMs);
	delay(delayMs);
}

void commandG28(char** params, int paramCount) {
	Log.traceln("Executing G28 command to home all servos.");
	moveServoA(SERVO_A_HOME_ANGLE);
	moveServoB(SERVO_B_HOME_ANGLE);
}

void commandM114(char** params, int paramCount) {
	Log.traceln("Executing M114 command to get current angles.");
	int angleA = getCurrentAngleA();
	int angleB = getCurrentAngleB();
	Log.traceln("Current angles: A=%d, B=%d", angleA, angleB);
}

static bool commandTailIsOnlyWhitespace(const char* command, size_t startIndex) {
	for (size_t i = startIndex; command[i] != '\0'; i++) {
		if (command[i] != ' ') {
			return false;
		}
	}
	return true;
}

static void commandS0(char* command) {
	char* filename = command + 2;
	while (*filename == ' ') {
		filename++;
	}

	if (*filename == '\0') {
		Log.warningln("S0 command requires a filename.");
		return;
	}

	char* filenameEnd = filename + strlen(filename);
	while (filenameEnd > filename && filenameEnd[-1] == ' ') {
		filenameEnd--;
	}
	*filenameEnd = '\0';

	if (*filename == '\0') {
		Log.warningln("S0 command requires a filename.");
		return;
	}

#ifdef ENABLE_SDCARD_CONTROL
	setActiveSDCommandFile(filename);
#else
	Log.warningln("S0 command ignored: SD card control is disabled.");
#endif
}

static void commandS2() {
#ifdef ENABLE_SDCARD_CONTROL
	pauseSDCardPlayback();
#else
	Log.warningln("S2 command ignored: SD card control is disabled.");
#endif
}

static void commandS3() {
#ifdef ENABLE_SDCARD_CONTROL
	resumeSDCardPlayback();
#else
	Log.warningln("S3 command ignored: SD card control is disabled.");
#endif
}

void parseCommand(char* command) {
	if (strlen(command) == 0) return; // Ignore empty commands

	Log.traceln("Parsing command: '%s'", command);

	if (command[0] == 'S' && command[1] != '\0') {
		if (command[1] == '0') {
			commandS0(command);
			return;
		}

		if (command[1] == '1' && commandTailIsOnlyWhitespace(command, 2)) {
			Log.warningln("S1 command is only available inside SD TCODE files.");
			return;
		}

		if (command[1] == '2' && commandTailIsOnlyWhitespace(command, 2)) {
			commandS2();
			return;
		}

		if (command[1] == '3' && commandTailIsOnlyWhitespace(command, 2)) {
			commandS3();
			return;
		}
	}

	// Split the command into type and values using space as a delimiter
	char* tokens[5]; // Local array to hold token pointers
	int tokenCount = 0;
	splitCommandIntoTokens(command, tokens, tokenCount);

	if (tokenCount == 0) {
		Log.errorln("Error: No command type found.");
		return;
	} else {
		Log.verboseln("Parsed command with %d tokens", tokenCount);
		for (int i = 0; i < tokenCount; i++) {
			Log.verboseln("Token %d: '%s'", i, tokens[i]);
		}
	}

	if (strcmp(tokens[0], "G0") == 0) {
		commandG0(tokens + 1, tokenCount - 1);
	} else if (strcmp(tokens[0], "G1") == 0 ) {
		commandG1(tokens + 1, tokenCount - 1);
	} else if (strcmp(tokens[0], "G4") == 0) {
		commandG4(tokens + 1, tokenCount - 1);
	} else if (strcmp(tokens[0], "G28") == 0) {
		commandG28(tokens + 1, tokenCount - 1);
	} else if (strcmp(tokens[0], "M114") == 0) {
		commandM114(tokens + 1, tokenCount - 1);
	} else {
		Log.errorln("Error: Unknown command type '%s'.", tokens[0]);
	}
}