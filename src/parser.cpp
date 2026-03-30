
#include <Arduino.h>

/**
 * @brief Splits a command string into tokens based on spaces. Modifies the input command string by replacing spaces with null terminators and returns an array of pointers to the tokens. The number of tokens is returned via the tokenCount reference parameter.
 * 
 * @param command The input command string to be split. This string will be modified in-place.
 * @param tokenCount Reference parameter that will be set to the number of tokens found in the command string.
 * @return char** An array of pointers to the tokens extracted from the command string. The first token (command type) will be at index 0, followed by any additional parameters. The caller is responsible for ensuring that the input command string remains valid while using the returned tokens.
 */
char** splitCommandIntoTokens(char* command, int& tokenCount) {
	static char** tokens = new char*[5]; // Max 5 tokens for simplicity

	// Reset token count and start with the first token as the command type
	tokenCount = 0;
	tokens[tokenCount++] = command; // First token is the command type

	// Iterate through the command string and split into tokens based on spaces
	for (size_t i = 0; i < strlen(command); i++) {
		if (command[i] == ' ') {
			command[i] = '\0'; // Null-terminate the token
			tokens[tokenCount++] = command + i + 1; // Store pointer to next token
			if (tokenCount >= 5) {
				Serial.println("Error: Too many tokens in command. Max is 5.");	
				break; // Prevent overflow
			}
		}
	}

	if (tokenCount < 4) {
		tokens[tokenCount] = nullptr; // Null-terminate the tokens array if less than max tokens found
	}

	return tokens;
}

void commandG0(char** params, int paramCount) {
	Serial.println("[WARN] G0 Not implemented yet");
}

void commandG1(char** params, int paramCount) {
	Serial.println("[WARN] G1 Not implemented yet");
}

void commandG4(char** params, int paramCount) {
	Serial.println("[WARN] G4 Not implemented yet");
}

void commandG28(char** params, int paramCount) {
	Serial.println("[WARN] G28 Not implemented yet");
}

void commandM114(char** params, int paramCount) {
	Serial.println("[WARN] M114 Not implemented yet");
}

void parseCommand(char* command) {
	if (strlen(command) == 0) return; // Ignore empty commands

	// Split the command into type and values using space as a delimiter
	int tokenCount = 0;
	char ** tokens = splitCommandIntoTokens(command, tokenCount);

	if (tokenCount == 0) {
		Serial.println("Error: No command type found.");
		return;
	} else {
		Serial.print("Command:");
		for (int i = 0; i < tokenCount; i++) {
			Serial.print(" '");
			Serial.print(tokens[i]);
			Serial.print("'");
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
		Serial.print("Error: Unknown command type '");
		Serial.print(tokens[0]);
		Serial.println("'.");
	}	
}	