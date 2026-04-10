
#include <Arduino.h>
#include "servo.h"
#include "generalConfig.h"

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
	Log.warningln("G1 Not implemented yet");
}

void commandG4(char** params, int paramCount) {
	Log.warningln("G4 Not implemented yet");
}

void commandG28(char** params, int paramCount) {
	Log.warningln("G28 Not implemented yet");
}

void commandM114(char** params, int paramCount) {
	Log.warningln("M114 Not implemented yet");
}

void parseCommand(char* command) {
	if (strlen(command) == 0) return; // Ignore empty commands

	Log.traceln("Parsing command: '%s'", command);

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